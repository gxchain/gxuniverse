#include "starplan.hpp"

void starplan::init()
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////
    // 0、防止跨合约调用
    uint64_t sender_id = checkSender();

    // 1、校验调用者，只有调用者可以初始化底池，只许调用一次
    graphene_assert(sender_id == ADMIN_ID, MSG_CHECK_ADMIN);

    // 2、校验充值的资产是否为INIT_POOL的大小
    uint64_t amount = assetEqualCheck(INIT_POOL);

    // 3、校验底池是否已经初始化
    auto glo_itor = tbglobals.find(0);
    graphene_assert(glo_itor == tbglobals.end(), MSG_INIT_FAIL);

    // 4、校验当前轮资金池是否已经初始化
    auto rou_itor = tbrounds.find(0);
    graphene_assert(rou_itor == tbrounds.end(), MSG_INIT_ROUND_FAIL);

    //////////////////////////////////////// 校验通过后，初始化资金池 //////////////////////////////////////////
    // 5、初始化总资金池
    tbglobals.emplace(sender_id, [&](auto &obj) {
        obj.index           = 0;
        obj.pool_amount     = amount;
        obj.current_round   = 0;
        obj.upgrading       = 0;
    });
    // 6、初始化第一轮资金池，并启动第一轮
    tbrounds.emplace(sender_id, [&](auto &obj) {
        obj.round                   = tbrounds.available_primary_key();
        obj.current_round_invites   = 0;
        obj.base_pool               = 0;
        obj.random_rewards          = 0;
        obj.invite_rewards          = 0;
        obj.start_time              = get_head_block_time();;
        obj.end_time                = 0;
    });
    tbcurbigplans.emplace(sender_id, [&](auto &obj) {
        obj.index       = tbcurbigplans.available_primary_key();
        obj.bigplanets  = {};
        obj.rwdplanets  = {};
    });
}

void starplan::vote(const std::string &inviter, const std::string &superstar)
{
    //////////////////////////////// 校验 ////////////////////////////////
    baseCheck();
    roundFinishCheck();

    uint64_t amount = assetLargerCheck(MIN_VOTE_AMOUNT);
    graphene_assert(amount <= MAX_VOTE_AMOUNT, MSG_MAX_VOTE_AMOUNT);          // 为了安全，vote最多投票20000 GXC

    uint64_t sender_id = get_trx_origin();
    uint64_t inviter_id = inviterCheck(inviter, sender_id);

    uint64_t super_id = superStarCheck(superstar);

    //////////////////////////////// 执行 ////////////////////////////////
    invite(sender_id, inviter_id);

    uint64_t vote_id = createVote(sender_id, super_id, amount);
    createStaking(sender_id, amount, super_id, STAKING_TYPE_VOTE, vote_id);

    uint64_t totalVotes = updateAccountVote(sender_id, amount);
    if (totalVotes >= Y && !isSmallPlanet(sender_id))
        createSmallPlanet(sender_id);

    updateSuperstarVote(super_id, amount, sender_id);
}

void starplan::selfactivate(const std::string &superstar)
{
    //////////////////////////////// 校验 ////////////////////////////////
    baseCheck();
    roundFinishCheck();

    assetEqualCheck(Z + Z1 + Z2 + Z3);

    uint64_t sender_id = get_trx_origin();
    graphene_assert(isBigPlanet(sender_id) || isSuperstarEnabled(sender_id), MSG_SELF_ACTIVATE_AUTH_ERR);

    uint64_t super_id = superStarCheck(superstar);

    //////////////////////////////// 执行 ////////////////////////////////
    updateActivePlanet(sender_id, sender_id);

    uint64_t vote_id = createVote(sender_id, super_id, Z);

    createStaking(sender_id, Z, super_id, STAKING_TYPE_SELF_ACTIVATE, vote_id);

    updateAccountVote(sender_id, Z);

    updateSuperstarVote(super_id, Z, sender_id);

    distributeInviteRewards(sender_id, sender_id, RWD_TYPE_SELF_ACTIVATE);

    progress(sender_id);
}

void starplan::uptobig()
{
    //////////////////////////////// 校验 ////////////////////////////////
    baseCheck();
    roundFinishCheck();

    uint64_t amount = assetEqualCheck(Z1 + Z2 + Z3);

    uint64_t sender_id = get_trx_origin();
    graphene_assert(isSmallPlanet(sender_id), MSG_NOT_SMALL_PLANET);
    graphene_assert(!isBigPlanet(sender_id), MSG_ALREADY_BIG_PLANET);

    //////////////////////////////// 执行 ////////////////////////////////
    createBigPlanet(sender_id);

    activateInvite(sender_id);

    distributeInviteRewards(sender_id, getInviter(sender_id), RWD_TYPE_INVITE);

    auto invitee_idx = tbinvites.get_index<N(byinvitee)>();
    auto invitee_itor = invitee_idx.find(sender_id);
    if (invitee_itor != invitee_idx.end())
        updateActivePlanet(invitee_itor->inviter, invitee_itor->invitee);

    progress(sender_id);
}

void starplan::uptosuper(const std::string &inviter, const std::string &memo)
{
    //////////////////////////////// 校验 ////////////////////////////////
    baseCheck();
    roundFinishCheck();
    uint64_t amount = assetEqualCheck(X);

    uint64_t sender_id = get_trx_origin();
    uint64_t inviter_id = inviterCheck(inviter, sender_id);

    graphene_assert(memo.size() <= MAX_MEMO_LENGTH, MSG_MEMO_TOO_LONG);
    graphene_assert(!isSuperstarEnabled(sender_id), MSG_ALREADY_SUPER_STAR);

    //////////////////////////////// 执行 ////////////////////////////////
    if (isSuperstarExist(sender_id)) {
        enableSuperstar(sender_id, memo);
    } else {
        createSuperstar(sender_id, memo);
    }

    uint64_t vote_id = createVote(sender_id, 0, 0);    //创建1个空投票；//TODO createVote和createStaking绑定成1个函数

    createStaking(sender_id, amount, sender_id, STAKING_TYPE_TO_SUPER, vote_id);

    invite(sender_id, inviter_id);

    activateInvite(sender_id);

    updateActivePlanetForSuper(sender_id);

    progress(sender_id);
}

void starplan::claim(uint64_t stakingid)
{
    baseCheck();

    auto itor = tbstakes.find(stakingid);
    graphene_assert(itor != tbstakes.end(), MSG_MORTGAGE_NOT_FOUND);
    graphene_assert(get_head_block_time() > itor->end_time, MSG_MORTGAGE_LOCKED);
    graphene_assert(itor->claimed == false, MSG_MORTGAGE_CLAIMED);
    graphene_assert(itor->amount <= MAX_CLAIM_AMOUNT, MSG_MAX_CLAIM_AMOUNT);// 为了安全，claim最多提现20000 GXC

    inline_transfer(_self, itor->account, CORE_ASSET_ID, itor->amount, LOG_CLAIM, strlen(LOG_CLAIM));

    tbstakes.modify(itor, get_trx_sender(), [&](auto &obj) {
        obj.claimed = true;
        obj.claim_time = get_head_block_time();
    });

    if (itor->staking_type == STAKING_TYPE_VOTE || itor->staking_type == STAKING_TYPE_SELF_ACTIVATE) {
        cancelVote(itor->vote_index, itor->staking_to, itor->amount);
    } else if (itor->staking_type == STAKING_TYPE_TO_SUPER) {
        disableSuperStar(itor->staking_to);
    } else {
        graphene_assert(false, MSG_UNKNOWN_CLAIM_REASON);
    }
}

void starplan::upgrade(uint64_t flag)
{
    // 0、防止跨合约调用
    uint64_t sender_id = checkSender();

    // 1、验证合约是否已经初始化
    graphene_assert(isInit(), MSG_NOT_INITIALIZED);

    // 2、验证调用者账户是否为admin账户
    graphene_assert(sender_id == ADMIN_ID, MSG_CHECK_ADMIN);

    // 3、修改global表
    auto itor = tbglobals.find(0);
    tbglobals.modify(itor, sender_id, [&](auto &obj) {
        obj.upgrading = flag;
    });
}

void starplan::updatememo(const std::string &memo)
{
    graphene_assert(memo.size() <= MAX_MEMO_LENGTH, MSG_MEMO_TOO_LONG);
    baseCheck();

    // 1、判断账户是否为超级星
    uint64_t sender_id = get_trx_origin();
    graphene_assert(isSuperstarEnabled(sender_id), MSG_SUPER_STAR_NOT_EXIST);
    // 2、更新账户memo
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(sender_id);
    sup_idx.modify(sup_itor, sender_id, [&](auto &obj) {
        obj.memo = memo;
    });
}

void starplan::getbudget()
{
    baseCheck();
    endRoundCheck(lastRound().bstate.finished == false, MSG_GET_BUDGET);
    calcBudgets();
}

void starplan::calcrdmrwd()
{
    baseCheck();

    const struct tbround &curRound = lastRound();
    bool check = curRound.bstate.finished == true && curRound.rstate.randomPoolReady == false;
    endRoundCheck(check, MSG_PROGRESS_RANDOM_REWARDS);

    uint64_t sender_id = get_trx_sender();
    uint64_t actualRewardAmount = 0;

    const struct starplan::tbcurbigplan &curBigPlanet = curRoundBigPlanets();
    const vector<uint64_t> &bigPlanets = curBigPlanet.bigplanets;
    vector<uint64_t> bigPlanetsToReward;

    if(bigPlanets.size() > 0) {
        chooseBigPlanet(bigPlanets, bigPlanetsToReward);
        uint64_t rewardPerPlanet = curRound.bstate.randomBudget / bigPlanetsToReward.size();
        if (rewardPerPlanet > 0) {
            graphene_assert(rewardPerPlanet <= MAX_USER_REWARD, MSG_USER_REWARD_TOO_MUCH);
            for (auto bigPlanetId : bigPlanetsToReward) {
                actualRewardAmount += rewardPerPlanet;
                createReward(sender_id, curRound.round, sender_id, bigPlanetId, rewardPerPlanet, RWD_TYPE_RANDOM);
            }
        }
    }

    tbrounds.modify(curRound, sender_id, [actualRewardAmount, &curRound](auto &obj) {
        obj.rstate.randomPoolReady = true;
        obj.actual_rewards += actualRewardAmount;

        graphene_assert(obj.actual_rewards <= MAX_ROUND_REWARD + curRound.bstate.randomBudget, MSG_ROUND_REWARD_TOO_MUCH);
    });

    tbcurbigplans.modify(curBigPlanet, sender_id, [bigPlanetsToReward](auto &obj) {
        obj.rwdplanets = bigPlanetsToReward;
    });
}

void starplan::calcbigrwd()
{
    baseCheck();

    const struct tbround &curRound = lastRound();
    bool check = curRound.bstate.finished == true && curRound.rstate.bigReady == false;
    endRoundCheck(check, MSG_PROGRESS_BIG_REWARDS);

    const struct starplan::tbcurbigplan &curBigPlanet = curRoundBigPlanets();
    const vector<uint64_t> &bigPlanets = curBigPlanet.bigplanets;

    uint64_t sender_id = get_trx_sender();
    if (bigPlanets.size() == 0) {
        tbrounds.modify(lastRound(), sender_id, [](auto &obj) {
            obj.rstate.bigReady = true;
        });
        return;
    }

    uint64_t lastBigPlanet = 0;
    uint64_t actualRewardAmount = 0;

    if (!isRoundFull() && isInviteTimeout(lastBigPlanet)) {             // 如果参与人数未满，并且超过12小时没有新的invitee则所有奖励归当轮最后一个大行星
        createReward(sender_id, curRound.round, _self, lastBigPlanet, curRound.bstate.bigPlanetBudget, RWD_TYPE_TIMEOUT);
        graphene_assert(curRound.bstate.bigPlanetBudget <= MAX_USER_REWARD, MSG_USER_REWARD_TOO_MUCH);
        actualRewardAmount = curRound.bstate.bigPlanetBudget;
    } else {
        uint64_t rewardPerPlanet = curRound.bstate.bigPlanetBudget / bigPlanets.size();
        if (rewardPerPlanet != 0) {
            graphene_assert(rewardPerPlanet <= MAX_USER_REWARD, MSG_USER_REWARD_TOO_MUCH);
            for (auto bigPlanetId : bigPlanets) {
                actualRewardAmount += rewardPerPlanet;
                createReward(sender_id, curRound.round, _self, bigPlanetId, rewardPerPlanet, RWD_TYPE_BIG);
            }
        }
    }

    tbrounds.modify(curRound, sender_id, [&](auto &obj) {
        obj.rstate.bigReady = true;
        obj.actual_rewards += actualRewardAmount;

        graphene_assert(obj.actual_rewards <= MAX_ROUND_REWARD + curRound.bstate.randomBudget, MSG_ROUND_REWARD_TOO_MUCH);
    });
}

void starplan::calcactrwd()
{
    baseCheck();

    const struct tbround &curRound = lastRound();
    auto g_itor = tbglobals.find(0);
    bool check = curRound.bstate.finished == true && curRound.rstate.activeReady == false && g_itor->total_weight != 0;
    endRoundCheck(check, MSG_PROGRESS_ACTIVE_REWARDS);

    uint64_t sender_id = get_trx_sender();

    // 2 计算每个活力星发的奖励
    auto act_idx = tbactiveplans.get_index<N(bytrave)>();
    auto id = curRound.rstate.traveIndex | 0x0100000000000000;
    auto itor = act_idx.lower_bound(id);
    uint64_t amount = 0;
    uint64_t totalAmount = 0;
    for (uint64_t count = 0; itor != act_idx.end() && itor->trave_index > 0x0100000000000000;) {
        if (count >= COUNT_OF_TRAVERSAL_PER) {
            tbrounds.modify(curRound, sender_id, [&](auto &obj) {
                obj.actual_rewards += totalAmount;
                obj.rstate.traveIndex = itor->trave_index;
            });
            return;
        } else {
            amount = curRound.bstate.activePlanetBudget * itor->weight / g_itor->total_weight;
            graphene_assert(amount <= MAX_USER_REWARD, MSG_USER_REWARD_TOO_MUCH);
            totalAmount += amount;
            createReward(sender_id, currentRound(), _self, itor->id, amount, RWD_TYPE_ACTIVE);
            auto pri_itor = tbactiveplans.find(itor->index);
            graphene_assert(pri_itor != tbactiveplans.end(), MSG_ACTIVE_PLANET_NOT_FOUND);
            itor++, count++;
            tbactiveplans.modify(pri_itor, sender_id, [&](auto &obj) {                           //修改活力星的权重
                uint64_t new_weight = obj.weight * B_DECAY_PERCENT / 100;
                obj.weight = new_weight;
                if(obj.weight == 0) obj.trave_index = obj.trave_index & 0xF0FFFFFFFFFFFFFF;
            });
        }
    }

    tbrounds.modify(curRound, sender_id, [&](auto &obj) {
        obj.rstate.activeReady = true;
        obj.actual_rewards += totalAmount;

        graphene_assert(obj.actual_rewards <= MAX_ROUND_REWARD + curRound.bstate.randomBudget, MSG_ROUND_REWARD_TOO_MUCH);
    });
}

void starplan::calcactrwd1()                           //全表遍历 假设10ms可以遍历200条，1秒钟可以遍历20000条，这个性能可以接受
{
    baseCheck();

    const struct tbround &curRound = lastRound();
    auto g_itor = tbglobals.find(0);
    bool check = curRound.bstate.finished == true && curRound.rstate.activeReady == false && g_itor->total_weight != 0;
    endRoundCheck(check, MSG_PROGRESS_ACTIVE_REWARDS);

    uint64_t sender_id = get_trx_sender();

    uint64_t count = 0;
    uint64_t amount = 0;
    uint64_t totalAmount = 0;
    auto itor = tbactiveplans.find(curRound.rstate.primaryIndex);
    if (itor == tbactiveplans.end()) return;

    do {
        if (itor->weight > 0) {
            amount = curRound.bstate.activePlanetBudget * itor->weight / g_itor->total_weight;
            graphene_assert(amount <= MAX_USER_REWARD, MSG_USER_REWARD_TOO_MUCH);
            totalAmount += amount;
            createReward(sender_id, curRound.round, _self, itor->id, amount, RWD_TYPE_ACTIVE);
            tbactiveplans.modify(itor, sender_id, [](auto &obj) {
                obj.weight = obj.weight * B_DECAY_PERCENT / 100;
            });
        }

        count++;
        if (count == COUNT_OF_TRAVERSAL_PER) break;

        itor++;
        if (itor == tbactiveplans.end()) {
            tbrounds.modify(curRound, sender_id, [&count, &totalAmount, &curRound](auto &obj) {
                obj.rstate.activeReady = true;
                obj.rstate.primaryIndex += count;
                obj.actual_rewards += totalAmount;
                graphene_assert(obj.actual_rewards <= MAX_ROUND_REWARD + curRound.bstate.randomBudget, MSG_ROUND_REWARD_TOO_MUCH);
            });

            return;
        }
    } while (true);

    tbrounds.modify(curRound, sender_id, [&totalAmount, &curRound](auto &obj) {
        obj.actual_rewards += totalAmount;
        obj.rstate.primaryIndex += COUNT_OF_TRAVERSAL_PER;
        graphene_assert(obj.actual_rewards <= MAX_ROUND_REWARD + curRound.bstate.randomBudget, MSG_ROUND_REWARD_TOO_MUCH);
    });
}

void starplan::calcsuprwd()
{
    baseCheck();

    const struct tbround &curRound = lastRound();
    bool check = curRound.bstate.finished == true && curRound.rstate.superReady == false;
    endRoundCheck(check, MSG_PROGRESS_SUPER_REWARDS);

    uint64_t sender_id = get_trx_sender();

    vector<SuperStar> superStars;
    uint64_t totalVote = getCurrentRoundSuperStars(superStars);
    if (totalVote == 0) {
        tbrounds.modify(curRound, sender_id, [](auto &obj) {
            obj.rstate.superReady = true;
        });

        return;
    }

    uint64_t amount = 0;
    uint64_t totalAmount = 0;
    for (const auto &superStar : superStars) {
        if (superStar.vote <= 0) continue;
        amount = curRound.bstate.superStarBudget * superStar.vote / totalVote;
        if (amount == 0) continue;
        graphene_assert(amount <= MAX_USER_REWARD, MSG_USER_REWARD_TOO_MUCH);
        totalAmount += amount;
        createReward(sender_id, curRound.round, _self, superStar.id, amount, RWD_TYPE_SUPER);
    }

    tbrounds.modify(curRound, sender_id, [&](auto &obj) {
        obj.rstate.superReady = true;
        obj.actual_rewards += totalAmount;

        graphene_assert(obj.actual_rewards <= MAX_ROUND_REWARD + curRound.bstate.randomBudget, MSG_ROUND_REWARD_TOO_MUCH);
    });
}

void starplan::dorwd(uint64_t limit)
{
    baseCheck();
    graphene_assert(get_trx_sender() == ADMIN_ID, MSG_CHECK_ADMIN);

    auto rwd_idx = tbrewards.get_index<N(byflag)>();
    auto itor = rwd_idx.find(0);
    graphene_assert(itor != rwd_idx.end(), MSG_REWARDS_NOT_FOUND);

    uint64_t now = get_head_block_time();
    for (auto i = 0; i < limit; i++) {
        if (itor->amount == 0) continue;
        graphene_assert(now >= itor->create_time,MSG_CREATE_TIME_ERR);
        if (now - itor->create_time < REWARD_DELAY_TIME) continue;
        if (itor == rwd_idx.end() || itor->rewarded == true) break;

        if (itor->type == RWD_TYPE_INVITE || itor->type == RWD_TYPE_SELF_ACTIVATE) {
            std::string rewardReason;
            buildRewardReason(itor->from, itor->to, itor->type, rewardReason);
            inline_transfer(_self, itor->to, CORE_ASSET_ID, itor->amount, rewardReason.c_str(), rewardReason.length());
        } else {
            inline_transfer(_self, itor->to, CORE_ASSET_ID, itor->amount, reward_reasons[itor->type], strlen(reward_reasons[itor->type]));
        }

        auto pri_itor = tbrewards.find(itor->index);
        itor++;
        tbrewards.modify(pri_itor, get_trx_sender(), [&](auto &obj) {
            obj.rewarded = 1;
            obj.reward_time = now;
        });
    }
}

void starplan::newround()
{
    baseCheck();

    const starplan::tbround &curRound = lastRound();

    bool check = curRound.bstate.finished == true &&
                 curRound.rstate.bigReady == true &&
                 curRound.rstate.randomPoolReady == true &&
                 curRound.rstate.activeReady == true &&
                 curRound.rstate.superReady == true;
    endRoundCheck(check, MSG_CALC_REWARDS);

    uint64_t sender_id = get_trx_sender();

    // 1、修改总的资金池
    auto g_itor = tbglobals.find(0);
    tbglobals.modify(g_itor, sender_id, [&](auto &obj) {
        graphene_assert(obj.pool_amount > curRound.actual_rewards, MSG_INVALID_POOL_AMOUNT_MODIFY);
        obj.pool_amount = obj.pool_amount + curRound.random_rewards + curRound.invite_rewards - curRound.actual_rewards;
        obj.total_weight = obj.total_weight * B_DECAY_PERCENT / 100;
    });

    createNewRound();
}

bool starplan::isInit()
{
    auto global_itor = tbglobals.find(0);
    auto round_itor = tbrounds.find(0);
    return global_itor != tbglobals.end() && round_itor != tbrounds.end();
}

bool starplan::isInviter(uint64_t inviter_id)
{
    return isSuperstarEnabled(inviter_id) || isBigPlanet(inviter_id);
}

bool starplan::isSuperstarEnabled(uint64_t superId)
{
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(superId);
    return sup_itor != sup_idx.end() && sup_itor->disabled == false;
}

bool starplan::isSuperstarExist(uint64_t superId)
{
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(superId);
    return sup_itor != sup_idx.end();
}

uint64_t starplan::accountCheck(const std::string &account)
{
    int64_t acc_id = get_account_id(account.c_str(), account.length());
    graphene_assert(-1 != acc_id, MSG_CHECK_ACCOUNT_EXIST);
    return acc_id;
}

void starplan::createSuperstar(uint64_t accountId, const std::string &memo)
{
    tbsuperstars.emplace(accountId, [&](auto &obj) {
        obj.index           = tbsuperstars.available_primary_key();
        obj.id              = accountId;
        obj.create_time     = get_head_block_time();
        obj.create_round    = currentRound();
        obj.vote_num        = 0;
        obj.disabled        = false;
        obj.memo            = memo;
    });
}

void starplan::enableSuperstar(uint64_t superId, const std::string &memo)
{
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(superId);
    sup_idx.modify(sup_itor, superId, [&](auto &obj) {
        obj.disabled = false;
        obj.memo = memo;
        obj.create_time = get_head_block_time();
        obj.create_round = currentRound();
    });
}

bool starplan::isSmallPlanet(uint64_t sender)
{
    auto small_idx = tbsmallplans.get_index<N(byaccid)>();
    auto small_itor = small_idx.find(sender);
    return small_itor != small_idx.end();
}

void starplan::createSmallPlanet(uint64_t sender)
{
    tbsmallplans.emplace(sender, [&](auto &obj) {
        obj.index           = tbsmallplans.available_primary_key();
        obj.id              = sender;
        obj.create_time     = get_head_block_time();
        obj.create_round    = currentRound();
    });
}

bool starplan::isBigPlanet(uint64_t sender)
{
    auto big_idx = tbbigplanets.get_index<N(byaccid)>();
    auto big_itor = big_idx.find(sender);
    return big_itor != big_idx.end();
}

void starplan::createBigPlanet(uint64_t sender)
{
    tbbigplanets.emplace(sender, [&](auto &obj) {
        obj.index           = tbbigplanets.available_primary_key();
        obj.id              = sender;
        obj.create_time     = get_head_block_time();
        obj.create_round    = currentRound();
    });
    auto itor = tbcurbigplans.find(currentRound());
    tbcurbigplans.modify(itor, sender, [&sender](auto &obj) {
        obj.bigplanets.push_back(sender);
    });
}

bool starplan::hasInvited(uint64_t invitee)
{
    auto invitee_idx = tbinvites.get_index<N(byinvitee)>();
    auto invite_itor = invitee_idx.find(invitee);
    return invite_itor != invitee_idx.end();
}

bool starplan::isInviteTimeout(uint64_t &lastBigPlanet)
{
    auto big_itor = tbbigplanets.end();
    if (big_itor == tbbigplanets.begin()) {
        return false;
    }
    big_itor--;

    if (big_itor->create_round != currentRound()) {
        return false;
    }

    graphene_assert(get_head_block_time() > big_itor->create_time, MSG_ILLIGAL_BIG_PLANET_CREATE_TIME);
    if ((get_head_block_time() - big_itor->create_time) > DELAY_TIME) {
        lastBigPlanet = big_itor->id;
        return true;
    }

    return false;
}

bool starplan::isRoundFull()
{
    auto round_itor = tbrounds.end();
    graphene_assert(round_itor != tbrounds.begin(), MSG_ROUND_NOT_FOUND);
    round_itor--;
    return round_itor->current_round_invites >= ROUND_SIZE;
}

bool starplan::isRoundFinish()
{
    uint64_t lastBigPlanet = 0;
    return isInviteTimeout(lastBigPlanet) || isRoundFull();
}

uint64_t starplan::currentRound()
{
    auto itor = tbglobals.find(0);
    return itor->current_round;
}

void starplan::invite(uint64_t invitee, uint64_t inviter)
{
    if (!hasInvited(invitee)) {
        tbinvites.emplace(invitee, [&](auto &obj) {
            obj.index           = tbinvites.available_primary_key();
            obj.invitee         = invitee;
            obj.inviter         = inviter;
            obj.enabled         = false;
            obj.create_round    = currentRound();
            obj.create_time     = get_head_block_time();
        });
    }
}

void starplan::activateInvite(uint64_t sender)
{
    auto invite_idx = tbinvites.get_index<N(byinvitee)>();
    auto invite_itor = invite_idx.find(sender);
    if (invite_itor != invite_idx.end()) {
        invite_idx.modify(invite_itor, sender, [](auto &obj) {
            obj.enabled = true;
        });
    }
}

void starplan::progress(uint64_t ramPayer)
{
    tbrounds.modify(lastRound(), ramPayer, [&](auto &obj) {
        obj.current_round_invites = obj.current_round_invites + 1;
    });
}

uint64_t starplan::createVote(uint64_t sender, uint64_t super_id, uint64_t voteCount)
{
    uint64_t vote_id;
    tbvotes.emplace(sender, [&](auto &obj) {
        obj.index           = tbvotes.available_primary_key();
        obj.round           = currentRound();
        obj.staking_amount  = voteCount;
        obj.from            = sender;
        obj.to              = super_id;
        obj.vote_time       = get_head_block_time();
        obj.disabled        = false;
        vote_id             = obj.index;
    });

    return vote_id;
}

void starplan::updateSuperstarVote(uint64_t account, uint64_t voteCount, uint64_t feePayer)
{
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(account);
    if (sup_itor != sup_idx.end()) {
        sup_idx.modify(sup_itor, feePayer, [&voteCount](auto &obj) {
            obj.vote_num += voteCount;
        });
    }
}

void starplan::createStaking(uint64_t sender, uint64_t amount, uint64_t to, uint64_t stakingType, uint64_t index)
{
    tbstakes.emplace(sender, [&](auto &obj) {
        obj.index           = tbstakes.available_primary_key();
        obj.account         = sender;
        obj.amount          = amount;
        obj.end_time        = get_head_block_time() + STAKING_DURATION_TIME;
        obj.staking_to      = to;
        obj.staking_type    = stakingType;
        obj.claimed         = false;
        obj.claim_time      = 0;
        obj.vote_index      = index;
    });
}

uint64_t starplan::getInviter(uint64_t invitee)
{
    auto invite_idx = tbinvites.get_index<N(byinvitee)>();
    auto invite_itor = invite_idx.find(invitee);
    graphene_assert(invite_itor != invite_idx.end(), MSG_INVITER_NOT_EXIST);
    return invite_itor->inviter;
}

void starplan::buildRewardReason(uint64_t invitee, uint64_t inviter, uint64_t rewardType, std::string &rewardReason)
{
    char inviteeName[64] = { 0 };
    char inviterName[64] = { 0 };

    if (RWD_TYPE_SELF_ACTIVATE == rewardType) {
        graphene_assert(0 == get_account_name_by_id(inviteeName, 63, invitee), MSG_GET_INVITEE_NAME_FAIL);
        rewardReason = std::string(inviterName) + " get reward for self activate";
    } else if (RWD_TYPE_INVITE == rewardType) {
        graphene_assert(0 == get_account_name_by_id(inviteeName, 63, invitee), MSG_GET_INVITEE_NAME_FAIL);
        graphene_assert(0 == get_account_name_by_id(inviterName, 63, inviter), MSG_GET_INVITER_NAME_FAIL);
        rewardReason = std::string(inviterName) + " get reward for inviting " + std::string(inviteeName);
    } else {
        graphene_assert(false, MSG_INVALIDE_REWARD_TYPE);
    }
}

void starplan::buildDepositMsg(uint64_t amount, bool equalCheck, std::string &msg)
{
    if (equalCheck)
        msg = "Error: " + std::to_string(amount / PRECISION) + " GXC required";
    else
        msg = "Error: Minimum " + std::to_string(amount / PRECISION) + " GXC required";
}

void starplan::distributeInviteRewards(uint64_t invitee, uint64_t rewardAccountId, uint64_t rewardType)
{
    tbrounds.modify(lastRound(), invitee, [](auto &obj) {
        obj.random_rewards = obj.random_rewards + Z3;
        obj.invite_rewards = obj.invite_rewards + Z1;
    });

    createReward(invitee, currentRound(), invitee, rewardAccountId, Z2, rewardType);
}

void starplan::updateActivePlanet(uint64_t activePlanetAccountId, uint64_t inviteeId)
{
    bool is_add_weight = false;

    auto act_idx = tbactiveplans.get_index<N(byaccid)>();
    auto act_itor = act_idx.find(activePlanetAccountId);
    if (act_itor != act_idx.end()) {
        act_idx.modify(act_itor, inviteeId, [&](auto &obj) {
            obj.invitees.push_back(inviteeId);
            if(obj.invitees.size() == ACTIVE_PROMOT_INVITES) {
                if (DEFAULT_INVITER != activePlanetAccountId) { //默认账户权重总是0，不可以主动参与游戏，不参与活力星奖励的瓜分，但是可以接受邀请奖励；20190823和pm确认
                    obj.weight += WEIGHT;
                    is_add_weight = true;
                    obj.trave_index = obj.trave_index | 0x0100000000000000;
                }
                obj.invitees.clear();
            }
        });
    } else {
        tbactiveplans.emplace(inviteeId, [&](auto &obj) {                                      //创建活力星
            obj.index           = tbactiveplans.available_primary_key();
            obj.id              = activePlanetAccountId;
            obj.invitees.push_back(inviteeId);
            obj.create_time     = get_head_block_time();
            obj.create_round    = currentRound();
            obj.weight          = 0;
            obj.trave_index     = activePlanetAccountId | 0x0000000000000000;
        });
    }

    if (is_add_weight == true) {
        auto g_itor = tbglobals.find(0);
        tbglobals.modify(g_itor, inviteeId, [](auto &obj) {
            obj.total_weight = obj.total_weight + WEIGHT;
        });
    }
}

void starplan::updateActivePlanetForSuper(uint64_t activePlanetAccountId)
{
    auto act_idx = tbactiveplans.get_index<N(byaccid)>();
    auto act_itor = act_idx.find(activePlanetAccountId);
    if (act_itor != act_idx.end()) {
        act_idx.modify(act_itor, activePlanetAccountId, [&](auto &obj) {
            obj.weight += WEIGHT;
            obj.trave_index = obj.trave_index | 0x0100000000000000;
        });
    } else {
        tbactiveplans.emplace(activePlanetAccountId, [&](auto &obj) {                                      //创建活力星
            obj.index           = tbactiveplans.available_primary_key();
            obj.id              = activePlanetAccountId;
            obj.invitees        = {};
            obj.create_time     = get_head_block_time();
            obj.create_round    = currentRound();
            obj.weight          = WEIGHT;
            obj.trave_index     = activePlanetAccountId | 0x0100000000000000;
        });
    }

    auto g_itor = tbglobals.find(0);
    tbglobals.modify(g_itor, activePlanetAccountId, [](auto &obj) {
        obj.total_weight += WEIGHT;
    });
}

void starplan::calcBudgets()
{
    auto &round = lastRound();

    auto pool_amount = ROUND_AMOUNT + round.invite_rewards;         //计算底池 pool_amount

    uint64_t now = (uint64_t) get_head_block_time();
    graphene_assert(now > round.start_time, MSG_BLOCK_TIME_ERR);
    uint64_t curRoundElapseTime = now - round.start_time;

    if(curRoundElapseTime > DECAY_TIME){                            //开奖时超过DECAY_TIME时间，奖励进行衰减，
        uint64_t decayAmountUnit = (currentRound() % BIG_ROUND_SIZE) * PRECISION;           //每次衰减奖励数量 fix: 衰减单位从1-50，修改为0-49
        uint64_t decayCount = (curRoundElapseTime - DECAY_TIME) / DECAY_DURATION;           //衰减次数
        decayCount += (curRoundElapseTime - DECAY_TIME) % DECAY_DURATION > 0 ? 1 : 0;       //存在余数，衰减次数+1
        decayCount = decayCount > MAX_DECAY_COUNT ? MAX_DECAY_COUNT : decayCount;           //最多衰减 MAX_DECAY_COUNT次
        uint64_t decayAmount = decayAmountUnit * decayCount;                                //每次衰减数量 * 衰减次数 = 总衰减金额
        graphene_assert(pool_amount > decayAmount, MSG_POOL_AMOUNT_DECAY_ERR);
        pool_amount = pool_amount - decayAmount;                    //计算衰减之后的底池 pool_amount
    }

    uint64_t randomBudget = round.random_rewards;
    uint64_t bigPlanetBudget = pool_amount * PAYBACK_PERCENT / 100;
    uint64_t activePlanetBudget = pool_amount * ACTIVE_PERCENT / 100;
    graphene_assert(pool_amount >= bigPlanetBudget + activePlanetBudget, MSG_CACL_POOL_BUDGET_ERR);
    uint64_t superStarBudget = pool_amount - bigPlanetBudget - activePlanetBudget;

    auto sender = get_trx_sender();
    tbrounds.modify(round, sender, [&](auto &obj) {
        graphene_assert(pool_amount >= round.invite_rewards, MSG_CACL_POOL_BUDGET_ERR);
        obj.base_pool = pool_amount - round.invite_rewards;
        obj.bstate.randomBudget = randomBudget;
        obj.bstate.bigPlanetBudget = bigPlanetBudget;
        obj.bstate.activePlanetBudget = activePlanetBudget;
        obj.bstate.superStarBudget = superStarBudget;
        obj.bstate.finished = true;
    });
}

uint64_t starplan::getCurrentRoundSuperStars(vector<SuperStar> &superStars)
{
    uint64_t total_votes = 0;

    for (auto itor = tbsuperstars.begin(); itor != tbsuperstars.end(); itor++) {
        if (itor->disabled == false) {
            total_votes += itor->vote_num;
            superStars.push_back(SuperStar { itor->id, itor->vote_num, itor->vote_num != 0 });
        }
    }

    return total_votes;
}

void starplan::chooseBigPlanet(const vector<uint64_t> &bigPlanets, vector<uint64_t> &choosed)
{
    if (bigPlanets.size() <= RANDOM_COUNT) {
        choosed = bigPlanets;
        return;
    }

    int64_t block_num = get_head_block_num();
    uint64_t block_time = get_head_block_time();
    std::string random_str = std::to_string(block_num) + std::to_string(block_time);
    checksum160 sum160;
    ripemd160(const_cast<char *>(random_str.c_str()), random_str.length(), &sum160);
    checksum160 block_hash;
    get_head_block_id(&block_hash);
    for (uint64_t i = 0; i < RANDOM_COUNT; i++) {
        auto j = i;
        while (true) {
            uint8_t share = (uint8_t) (sum160.hash[j % 20] * (j + 1));
            uint8_t number = (share + block_hash.hash[j % 20]) % bigPlanets.size();
            auto it = std::find(choosed.begin(), choosed.end(), bigPlanets[number]);
            if (it != choosed.end()) {
                j++;
                continue;
            } else {
                choosed.push_back(bigPlanets[number]);
                break;
            }
        }
    }
}

const struct starplan::tbround& starplan::lastRound()
{
    auto round_itor = tbrounds.end();
    graphene_assert(round_itor != tbrounds.begin(), MSG_ROUND_NOT_FOUND);
    round_itor--;
    return *round_itor;
}

const struct starplan::tbcurbigplan& starplan::curRoundBigPlanets()
{
    auto itor = tbcurbigplans.end();
    graphene_assert(itor != tbcurbigplans.begin(), MSG_CUR_BIG_PLANETS_NOT_FOUND);
    itor--;
    return *itor;
}

void starplan::createNewRound()
{
    // 1 结束当前轮，修改round表和global表
    uint64_t sender = get_trx_sender();
    tbrounds.modify(lastRound(), sender, [&](auto &obj) {
        obj.end_time = get_head_block_time();
    });

    // 2 创建新的一轮
    auto g_itor = tbglobals.find(0);
    tbglobals.modify(g_itor, sender, [](auto &obj) {
        obj.current_round += 1;
    });

    tbrounds.emplace(sender, [&](auto &obj) {
        obj.round                   = tbrounds.available_primary_key();
        obj.current_round_invites   = 0;
        obj.base_pool               = 0;
        obj.random_rewards          = 0;
        obj.invite_rewards          = 0;
        obj.start_time              = get_head_block_time();
        obj.end_time                = 0;
    });

    tbcurbigplans.emplace(sender, [&](auto &obj) {
        obj.index = tbcurbigplans.available_primary_key();
        obj.bigplanets = {};
        obj.rwdplanets = {};
    });
}

uint64_t starplan::checkSender()
{
    auto sender_id = get_trx_sender();
    auto origin_id = get_trx_origin();
    graphene_assert(sender_id == origin_id, MSG_CHECK_SENDER);
    graphene_assert(sender_id != DEFAULT_INVITER, MSG_CHECK_SENDER_NOT_DEFAULT);
    return sender_id;
}

bool starplan::isUpgrading()
{
    auto itor = tbglobals.find(0);
    return itor->upgrading > 0;
}

void starplan::cancelVote(uint64_t voteIndex, uint64_t superAccId, uint64_t amount)
{
    // 修改投票表
    auto vote_itor = tbvotes.find(voteIndex);
    graphene_assert(vote_itor != tbvotes.end(), MSG_INVALID_ITOR);
    tbvotes.modify(vote_itor, get_trx_sender(), [&](auto &obj) {
        obj.disabled = true;//TODO check 修改了也没用了，只是用来记录和展示
    });

    // 修改超级星得票数
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(superAccId);
    graphene_assert(sup_itor != sup_idx.end(), MSG_INVALID_ITOR);
    sup_idx.modify(sup_itor, get_trx_sender(), [&amount](auto &obj) {
        graphene_assert(obj.vote_num >= amount, MSG_INVALID_VOTE_MODIFY);
        obj.vote_num -= amount;
    });
}

void starplan::disableSuperStar(uint64_t superId)
{
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(superId);
    graphene_assert(sup_itor != sup_idx.end(), MSG_SUPER_STAR_NOT_EXIST);
    sup_idx.modify(sup_itor, get_trx_sender(), [](auto &obj) {
        obj.disabled = true;
    });
}

void starplan::baseCheck()
{
    checkSender();
    graphene_assert(isInit(), MSG_NOT_INITIALIZED);
    graphene_assert(!isUpgrading(), MSG_UPGRADING);
}

void starplan::roundFinishCheck()
{
    graphene_assert(!isRoundFinish(), MSG_ROUND_END);
}

uint64_t starplan::assetEqualCheck(uint64_t expectedAmount)
{
    std::string errMsg;
    buildDepositMsg(expectedAmount, true, errMsg);
    graphene_assert(get_action_asset_id() == CORE_ASSET_ID, MSG_CORE_ASSET_REQUIRED);
    graphene_assert(get_action_asset_amount() == expectedAmount, errMsg.c_str());

    return expectedAmount;
}

uint64_t starplan::assetLargerCheck(uint64_t expectedAmount)
{
    std::string errMsg;
    buildDepositMsg(expectedAmount, false, errMsg);
    uint64_t actualAmount = get_action_asset_amount();
    graphene_assert(get_action_asset_id() == CORE_ASSET_ID, MSG_CORE_ASSET_REQUIRED);
    graphene_assert(actualAmount >= expectedAmount, errMsg.c_str());

    return actualAmount;
}

uint64_t starplan::inviterCheck(const std::string &inviter, uint64_t inviteeId)
{
    if ("" != inviter) {
        uint64_t inviter_id = accountCheck(inviter);
        graphene_assert(isInviter(inviter_id), MSG_CHECK_INVITER_VALID);
        graphene_assert(inviter_id != inviteeId, MSG_CHECK_INVITE_SELF);
        return inviter_id;
    }

    return DEFAULT_INVITER;
}

uint64_t starplan::superStarCheck(const std::string &superStarAccount)
{
    uint64_t super_id = accountCheck(superStarAccount);
    graphene_assert(isSuperstarEnabled(super_id), MSG_SUPER_STAR_NOT_EXIST);
    return super_id;
}

uint64_t starplan::updateAccountVote(uint64_t sender, uint64_t voteCount)
{
    uint64_t totalVotes = 0;

    auto itor = tbaccounts.find(sender);
    if (itor == tbaccounts.end()) {
        tbaccounts.emplace(sender, [&sender, &voteCount, &totalVotes](auto &obj) {
            obj.account_id      = sender;
            obj.vote_count      = voteCount;
            totalVotes          = obj.vote_count;
        });
    } else {
        tbaccounts.modify(itor, sender, [&voteCount, &totalVotes](auto &obj) {
            obj.vote_count += voteCount;
            totalVotes = obj.vote_count;
        });
    }

    return totalVotes;
}

void starplan::endRoundCheck(bool check, const std::string &msg)
{
    graphene_assert(isRoundFinish(), MSG_ROUND_NOT_END);
    uint64_t sender_id = get_trx_origin();
    graphene_assert(sender_id == ADMIN_ID, MSG_CHECK_ADMIN);
    graphene_assert(check, msg.c_str());
}

void starplan::createReward(uint64_t feePayer, uint64_t round, uint64_t from, uint64_t to, uint64_t amount, uint8_t type)
{
    if (amount == 0) return; //对于amount为0的奖励不插入reward表，优化性能，节省内存
    tbrewards.emplace(feePayer, [&](auto &obj) {
        obj.index           = tbrewards.available_primary_key();
        obj.round           = round;
        obj.from            = from;
        obj.to              = to;
        obj.amount          = amount;
        obj.type            = type;
        obj.create_time     = get_head_block_time();
        obj.rewarded        = 0;
    });
}
