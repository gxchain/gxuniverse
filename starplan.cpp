#include "starplan.hpp"

void starplan::init()
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////
    // 0、防止跨合约调用
    uint64_t sender_id = checkSender();

    // 1、校验调用者，只有调用者可以初始化底池，只许调用一次
    graphene_assert(sender_id == ADMIN_ID, MSG_CHECK_ADMIN);

    // 2、校验充值的资产是否为INIT_POOL的大小
    uint64_t amount = assetEqualCheck(INIT_POOL, MSG_INVALID_INIT_AMOUNT);

    // 3、校验底池是否已经初始化
    auto glo_itor = tbglobals.find(0);
    graphene_assert(glo_itor == tbglobals.end(),MSG_INIT_FAIL);

    // 4、校验当前轮资金池是否已经初始化
    auto rou_itor = tbrounds.find(0);
    graphene_assert(rou_itor == tbrounds.end(),MSG_INIT_ROUND_FAIL);

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
        obj.pool_amount             = 0;
        obj.random_pool_amount      = 0;
        obj.invite_pool_amount      = 0;
        obj.start_time              = get_head_block_time();;
        obj.end_time                = 0;
    });
}

void starplan::vote(std::string inviter,std::string superstar)
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////

    baseCheck();
    roundFinishCheck();

    uint64_t amount = assetLargerCheck(MIN_VOTE_AMOUNT, MSG_INVALID_VOTE_AMOUNT);

    // 3、验证inviter
    uint64_t sender_id = get_trx_origin();
    uint64_t inviter_id = inviterCheck(inviter, sender_id);

    // 5、验证超级星账户
    uint64_t super_id = superStarCheck(superstar);
    //////////////////////////////////////// 校验通过后，创建一个小行星 //////////////////////////////////////////

    // 6、保存邀请关系(不允许重复邀请)
    invite(sender_id, inviter_id);

    // 7、vote(允许重复投票)
    uint64_t vote_id = createVote(sender_id, superstar, amount);

    // 8、添加一个新的抵押金额
    addStaking(sender_id,amount,super_id,STAKING_TYPE_VOTE,vote_id);

    // 9、存到smallPlanet表(不允许重复创建)
    if(canUpdateSmall(sender_id))
        addSmallPlanet(sender_id);

    // 10、修改超级星的得票数
    addVote(super_id, amount, sender_id);
}

void starplan::selfactivate(std::string superstar)
{
    baseCheck();
    roundFinishCheck();

    uint64_t amount = assetEqualCheck(Z + Z1 + Z2 + Z3, MSG_INVALID_SELF_ACTIVATE_AMOUNT);

    uint64_t sender_id = get_trx_origin();
    graphene_assert(isBigPlanet(sender_id) || isSuperStar(sender_id), MSG_SELF_ACTIVATE_AUTH_ERR);

    auto super_id = superStarCheck(superstar);

    updateActivePlanet(sender_id,sender_id);

    uint64_t vote_id = createVote(sender_id, superstar, Z);

    addStaking(sender_id, Z, super_id, STAKING_TYPE_SELF_ACTIVATE, vote_id);

    addVote(super_id, Z, sender_id);

    distributeInviteRewards(sender_id, sender_id, RWD_TYPE_SELF_ACTIVATE);

    progress(sender_id);

    if (lastRound().current_round_invites >= ROUND_SIZE) {
        endround();
    }
}

void starplan::uptobig()
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////

    baseCheck();
    roundFinishCheck();

    // 2、判断是否存入足够GXC
    uint64_t amount = assetEqualCheck(Z1 + Z2 + Z3, MSG_INVALID_BIG_AMOUNT);

    // 3、判断是否是small planet，如果还不不是，则提示“You have to become a small planet first”
    uint64_t sender_id = get_trx_origin();
    graphene_assert(isSmallPlanet(sender_id), MSG_NOT_SMALL_PLANET);

    //////////////////////////////////////// 校验通过后，创建一个大行星 //////////////////////////////////////////
    // 5、存到bigPlanet表
    graphene_assert(addBigPlanet(sender_id), MSG_ALREADY_BIG_PLANET);

    // 6、激活邀请关系
    activateInvite(sender_id);

    // 7、当前轮进度+1
    progress(sender_id);

    // 7、将2个GXC转移到奖池，1个GXC发送给邀请人
    distributeInviteRewards(sender_id, getInviter(sender_id), RWD_TYPE_INVITE);

    // 8、创建/更新活力星
    auto invitee_idx = tbinvites.get_index<N(byinvitee)>();
    auto invitee_itor = invitee_idx.find(sender_id);
    if(invitee_itor != invitee_idx.end())//TODO check
        updateActivePlanet(invitee_itor->inviter,invitee_itor->invitee);

    // 9、当邀请关系满100人，开启新的一轮
    if(lastRound().current_round_invites >= ROUND_SIZE){
        endround();
    }
}

// inviter为0，表示没有邀请账户
void starplan::uptosuper(std::string inviter,std::string memo)
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////

    baseCheck();
    roundFinishCheck();

    // 2、判断是否存入足够GXC
    uint64_t amount = assetEqualCheck(X, MSG_INVALID_SUPER_AMOUNT);

    uint64_t sender_id = get_trx_origin();

    // 3、验证账户是否存在
    uint64_t inviter_id = inviterCheck(inviter, sender_id);

    //////////////////////////////////////// 校验通过后，创建一个超级星 //////////////////////////////////////////

    // 5、创建超级星
    graphene_assert(addSuperStar(sender_id,memo), MSG_ALREADY_SUPER_STAR);

    // 6、创建抵押项
    addStaking(sender_id, amount, sender_id, STAKING_TYPE_TO_SUPER);

    // 7、保存邀请关系，激活邀请关系
    invite(sender_id, inviter_id);
    activateInvite(sender_id);

    // 8、当前轮进度+1
    progress(sender_id);

    // 8、插入更新一条活力星记录，权重为1
    updateActivePlanetForSuper(sender_id);

    if(lastRound().current_round_invites >= ROUND_SIZE){
        endround();
    }
}

void starplan::endround()
{
    baseCheck();
    graphene_assert(isRoundFinish(), MSG_ROUND_END);

    uint64_t sender_id = get_trx_origin();
    if (lastRound().current_round_invites < ROUND_SIZE) {
        graphene_assert(sender_id == ADMIN_ID, MSG_CHECK_ADMIN);
    }

    calcCurrentRoundPoolAmount();

    uint64_t randomBudget = 0;
    uint64_t bigPlanetBudget = 0;
    uint64_t activePlanetBudget = 0;
    uint64_t superStarBudget = 0;
    getBudgets(randomBudget, bigPlanetBudget, activePlanetBudget, superStarBudget);

    uint64_t actualReward = 0;
    vector<reward> rewardList;
    actualReward += calcRandomReward(rewardList, randomBudget);
    actualReward += calcBigPlanetReward(rewardList, bigPlanetBudget);
    actualReward += calcActivePlanetReward(rewardList, activePlanetBudget);
    actualReward += calcSuperStarReward(rewardList, superStarBudget);

    if (baseSecureCheck(rewardList, randomBudget)) {
        doReward(rewardList);
    }

    // 6、修改总的资金池
    auto g_itor = tbglobals.find(0);
    tbglobals.modify(g_itor, sender_id, [&](auto &obj) {
        obj.pool_amount -= actualReward;
    });

    tbrounds.modify(lastRound(),sender_id, [&](auto &obj) {
        obj.actual_rewards = actualReward;
    });

    // 5、更新活力星权重
    decayActivePlanetWeight();

    // 6、开启新的一轮
    createNewRound();
}

void starplan::claim(std::string account)
{
    baseCheck();

    uint64_t acc_id = get_account_id(account.c_str(), account.length());
    auto sta_idx = tbstakes.get_index<N(byaccid)>();
    auto itor = sta_idx.find(acc_id);
    for(; itor != sta_idx.end() && itor->account == acc_id;){
        if(get_head_block_time() > itor->end_time && itor->claimed == false){
            // 1.1、解除抵押提现
            inline_transfer(_self , acc_id , CORE_ASSET_ID , itor->amount, LOG_CLAIM,strlen(LOG_CLAIM));
            // 1.2、修改该项抵押失效 
            sta_idx.modify(itor,get_trx_sender(),[&](auto &obj){
                obj.claimed          =   true;
                obj.claim_time       =   get_head_block_time();
            });
            // 1.3、获取抵押类型，禁用某投票项，修改超级星得票数等等
            if(itor->staking_type == STAKING_TYPE_VOTE){
                cancelVote(itor->vote_index,itor->staking_to,itor->amount);
            }else if(itor->staking_type == STAKING_TYPE_TO_SUPER){
                disableSuperStar(itor->staking_to);
            }else{
                graphene_assert(false,MSG_UNKNOWN_CLAIM_REASON);
            }
            itor++;
        }else{
            itor++;
        }
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
    graphene_assert(isSuperStar(sender_id), MSG_CHECK_SUPER_STAR_EXIST);
    // 2、更新账户memo
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(sender_id);
    sup_idx.modify(sup_itor, sender_id, [&](auto &obj) {
        obj.memo = memo;
    });
}

bool starplan::isInit()
{
    bool retValue = false;
    // 1 校验底池是否已经初始化
    auto itor = tbglobals.find(0);
    // 2 校验当前轮资金池是否已经初始化
    auto itor2 = tbrounds.find(0);
    if (itor != tbglobals.end() && itor2 != tbrounds.end()) {
        retValue = true;
    }
    return retValue;
}

bool starplan::isInviter(std::string accname)
{
    // 验证邀请账户是否为大行星或者超级星
    bool retValue = false;
    uint64_t inviter_id = get_account_id(accname.c_str(), accname.length());
    if(isSuperStar(inviter_id) || isBigPlanet(inviter_id))
        retValue = true;
    return retValue;
}

bool starplan::isSuperStar(uint64_t sender)
{
    bool retValue = false;
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(sender);
    if(sup_itor != sup_idx.end() && sup_itor->disabled == false) { retValue = true; }
    return retValue;
}

bool starplan::addSuperStar(uint64_t sender, const std::string &memo)//TODO memo should be lawful
{
    graphene_assert(memo.size() <= MAX_MEMO_LENGTH, MSG_MEMO_TOO_LONG);
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(sender);
    if(sup_itor == sup_idx.end()){
        tbsuperstars.emplace(sender,[&](auto &obj) {
            obj.index                   = tbsuperstars.available_primary_key();
            obj.id                      = sender;
            obj.create_time             = get_head_block_time();
            obj.create_round            = currentRound();
            obj.vote_num                = 0;
            obj.disabled                = false;
            obj.memo                    = memo;
        });
        return true;
    }else{
        if(sup_itor->disabled == true) { 
            sup_idx.modify(sup_itor,sender,[&](auto &obj) {
                obj.disabled                = false;
                obj.memo                    = memo;
            });
            return true;
        }else{
            return false;
        }
    }
    return false;
}

bool starplan::isSmallPlanet(uint64_t sender)
{
    bool retValue = false;
    auto small_idx = tbsmallplans.get_index<N(byaccid)>();
    auto small_itor = small_idx.find(sender);
    if(small_itor != small_idx.end()) { retValue = true; }
    return retValue;
}

bool starplan::addSmallPlanet(uint64_t sender)
{
    if(!isSmallPlanet(sender)){
        tbsmallplans.emplace(sender,[&](auto &obj){
            obj.index                   = tbsmallplans.available_primary_key();
            obj.id                      = sender;
            obj.create_time             = get_head_block_time();
            obj.create_round            = currentRound();
        });
        return true;
    }
    return false;
}

bool starplan::isBigPlanet(uint64_t sender)
{
    bool retValue = false;
    auto big_idx = tbbigplanets.get_index<N(byaccid)>();
    auto big_itor = big_idx.find(sender);
    if (big_itor != big_idx.end()) {
        retValue = true;
    }
    return retValue;
}

bool starplan::addBigPlanet(uint64_t sender)
{
    if (!isBigPlanet(sender)) {
        tbbigplanets.emplace(sender, [&](auto &obj) {
            obj.index           = tbbigplanets.available_primary_key();
            obj.id              = sender;
            obj.create_time     = get_head_block_time();
            obj.create_round    = currentRound();
        });
        return true;
    }
    return false;
}

bool starplan::hasInvited(uint64_t sender)
{
    bool retValue = false;
    auto invite_idx = tbinvites.get_index<N(byinvitee)>();
    auto invite_itor = invite_idx.find(sender);
    if (invite_itor != invite_idx.end()) {
        retValue = true;
    }
    return retValue;
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

void starplan::invite(uint64_t sender, uint64_t inviter)
{
    if (!hasInvited(sender)) {
        tbinvites.emplace(sender, [&](auto &obj) {
            obj.index           = tbinvites.available_primary_key();
            obj.invitee         = sender;
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
    invite_idx.modify(invite_itor, sender, [&](auto &obj) {
        obj.enabled = true;
    });
}

void starplan::progress(uint64_t ramPayer)
{
    tbrounds.modify(lastRound(), ramPayer, [](auto &obj) {
        obj.current_round_invites = obj.current_round_invites + 1;
    });
}

uint64_t starplan::createVote(uint64_t sender, std::string superstar, uint64_t voteCount)
{
    uint64_t vote_id;
    uint64_t super_id = get_account_id(superstar.c_str(), superstar.length());
    tbvotes.emplace(sender,[&](auto &obj) {
        obj.index                   = tbvotes.available_primary_key();
        obj.round                   = currentRound();
        obj.staking_amount          = voteCount;
        obj.from                    = sender;
        obj.to                      = super_id;
        obj.vote_time               = get_head_block_time();
        obj.disabled                = false;
        vote_id                     = obj.index;
    });

    return vote_id;
}

void starplan::addVote(uint64_t account, uint64_t voteCount, uint64_t feePayer)
{
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(account);
    sup_idx.modify(sup_itor, feePayer, [&](auto &obj) {
        obj.vote_num += voteCount;
    });
}

void starplan::addStaking(uint64_t sender,uint64_t amount,uint64_t to,uint64_t stakingType, uint64_t index)
{
    tbstakes.emplace(sender,[&](auto &obj) {
        obj.index                   = tbstakes.available_primary_key();
        obj.account                 = sender;
        obj.amount                  = amount;
        obj.end_time                = get_head_block_time() + STAKING_DURATION_TIME;
        obj.staking_to              = to;
        obj.staking_type            = stakingType;
        obj.claimed                 = false;
        obj.claim_time              = 0;
        obj.vote_index              = index;
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

    if(RWD_TYPE_SELF_ACTIVATE == rewardType) {
        graphene_assert(0 == get_account_name_by_id(inviteeName, 63, invitee), MSG_GET_INVITEE_NAME_FAIL);
        rewardReason = std::string(inviterName) + "get reward for self activate";
    } else if (RWD_TYPE_INVITE == rewardType) {
        graphene_assert(0 == get_account_name_by_id(inviteeName, 63, invitee), MSG_GET_INVITEE_NAME_FAIL);
        graphene_assert(0 == get_account_name_by_id(inviterName, 63, inviter), MSG_GET_INVITER_NAME_FAIL);
        rewardReason = std::string(inviterName) + "get reward for invitee " + std::string(inviteeName);
    } else {
    }
}

void starplan::distributeInviteRewards(uint64_t invitee, uint64_t rewardAccountId, uint64_t rewardType)
{
    std::string rewardReason;
    buildRewardReason(invitee, rewardAccountId, rewardType, rewardReason);
    inline_transfer(_self, rewardAccountId, CORE_ASSET_ID, Z2, rewardReason.c_str(), rewardReason.length());

    tbrounds.modify(lastRound(), invitee, [&](auto &obj)
    {
        obj.random_pool_amount = obj.random_pool_amount + Z3;
        obj.invite_pool_amount = obj.invite_pool_amount + Z1;
    });

    tbrewards.emplace(invitee, [&](auto &obj)
    {
        obj.index = tbrewards.available_primary_key();
        obj.round = currentRound();
        obj.from = invitee;
        obj.to = rewardAccountId;
        obj.amount = Z2;
        obj.type = rewardType;
    });
}

void starplan::updateActivePlanet(uint64_t activePlanetAccountId,uint64_t subAccountId)
{
    auto act_idx = tbactiveplans.get_index<N(byaccid)>();
    auto act_itor = act_idx.find(activePlanetAccountId);
    if (act_itor != act_idx.end()) {
        act_idx.modify(act_itor, subAccountId, [&](auto &obj) {
            obj.invite_list.push_back(subAccountId);
            if(obj.invite_list.size() == ACTIVE_PROMOT_INVITES) {
                obj.weight += WEIGHT;
                obj.invite_list.clear();
            }
        });
    } else {
        tbactiveplans.emplace(subAccountId, [&](auto &obj) {                                      //创建活力星
            obj.index = tbactiveplans.available_primary_key();
            obj.id = activePlanetAccountId;
            obj.invite_list.push_back(subAccountId);
            obj.create_time = get_head_block_time();
            obj.create_round = currentRound();
            obj.weight = 0;
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
        });
    } else {
        tbactiveplans.emplace(activePlanetAccountId, [&](auto &obj) {                                      //创建活力星
            obj.index = tbactiveplans.available_primary_key();
            obj.id = activePlanetAccountId;
            obj.invite_list = {};
            obj.create_time = get_head_block_time();
            obj.create_round = currentRound();
            obj.weight = WEIGHT;
        });
    }
}

void starplan::calcCurrentRoundPoolAmount()
{
    // 1、获取平均奖励池
    auto &round = lastRound();

    // 2、默认底池
    auto pool_amount = ROUND_AMOUNT + round.invite_pool_amount;
    // 3、超过四小时，每小时减少底池金额
    auto x = currentRound()%BIG_ROUND_SIZE + 1;
    // 4、计算当前小轮的运行时间
    if(get_head_block_time() - round.start_time > DECAY_TIME){
        uint64_t mod = (get_head_block_time() - round.start_time - DECAY_TIME) % DECAY_DURATION;
        uint64_t takeInteger = mod > 0 ? 1 : 0;
        uint64_t dursize = ((get_head_block_time() - round.start_time - DECAY_TIME) / DECAY_DURATION) + takeInteger;
        dursize = dursize > MAX_DECAY_COUNT ? MAX_DECAY_COUNT:dursize;
        graphene_assert(pool_amount > (dursize * x), MSG_INSUFFICIENT_POOL_AMOUNT);
        pool_amount = pool_amount - dursize * x;
    }
    // 5、修改当前轮底池 pool_amount
    auto sender = get_trx_sender();
    tbrounds.modify(round, sender, [&](auto &obj){                               //修改奖池金额pool_amount
        obj.pool_amount = pool_amount;
    });
}
void starplan::decayActivePlanetWeight()
{
    // 更新活力星的权重
    auto act_itor = tbactiveplans.begin();
    for(; act_itor != tbactiveplans.end(); act_itor++){
        tbactiveplans.modify(act_itor, get_trx_sender(), [](auto &obj){                           //修改活力星的权重
            uint64_t new_weight  = obj.weight * B_DECAY_PERCENT / 100;
            obj.weight = new_weight;
        });
    }
}

void starplan::getCurrentRoundBigPlanets(vector<uint64_t> &bigPlanets)
{
    auto big_idx = tbbigplanets.get_index<N(byround)>();
    auto round = currentRound();
    auto itor = big_idx.find(round);
    while (itor != big_idx.end() && itor->create_round == round)
    {
        bigPlanets.push_back(itor->id);
        itor++;
    }
}

uint64_t starplan::getCurrentRoundActivePlanets(vector<ActivePlanet> &activePlanets)
{
    auto act_idx = tbactiveplans.get_index<N(byweight)>();
    uint64_t total_weights = 0;
    auto itor = act_idx.upper_bound(0);
    while (itor != act_idx.end() && itor->weight > 0)
    {
        if(itor->id == DEFAULT_INVITER) continue;
        total_weights += itor->weight;
        activePlanets.push_back(ActivePlanet { itor->id, itor->weight });
        itor++;
    }

    return total_weights;
}

uint64_t starplan::getCurrentRoundSuperStars(vector<SuperStar> &superStars)
{
    uint64_t total_votes = 0;

    for (auto itor = tbsuperstars.begin(); itor != tbsuperstars.end(); itor++)
    {
        if(itor->disabled == false){
            total_votes += itor->vote_num;
            superStars.push_back(SuperStar{itor->id, itor->vote_num, itor->vote_num != 0});
        }
    }

    return total_votes;
}

void starplan::chooseBigPlanet(const vector<uint64_t> &bigPlanets, vector<uint64_t> &choosed)
{
    auto bigplanet_size = bigPlanets.size() > 10 ? 10 : bigPlanets.size();
    int64_t block_num = get_head_block_num();
    uint64_t block_time = get_head_block_time();
    std::string random_str = std::to_string(block_num) + std::to_string(block_time);
    checksum160 sum160;
    ripemd160(const_cast<char *>(random_str.c_str()), random_str.length(), &sum160);
    for (uint64_t i = 0; i < bigplanet_size; i++)
    {
        auto j = i;
        while (true)
        {
            uint8_t share = (uint8_t) (sum160.hash[j % 20] * (j + 1));
            uint8_t number = share % bigplanet_size;
            auto it = std::find(choosed.begin(), choosed.end(), number);
            if (it != choosed.end())
            {
                j++;
                continue;
            }
            else
            {
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

void starplan::getBudgets(uint64_t &randomRewardBudget, uint64_t &bigPlanetRewardBudget,
                       uint64_t &activePlanetRewardBudget, uint64_t &superStarRewardBudget)
{
    const tbround& round = lastRound();

    randomRewardBudget = round.random_pool_amount;

    bigPlanetRewardBudget = round.pool_amount * PAYBACK_PERCENT / 100;
    activePlanetRewardBudget = round.pool_amount * ACTIVE_PERCENT / 100;
    superStarRewardBudget = round.pool_amount - bigPlanetRewardBudget - activePlanetRewardBudget;
}

uint64_t starplan::calcRandomReward(vector<reward> &rewardList, uint64_t rewardBudget)
{
    vector<uint64_t> bigPlanets;
    vector<uint64_t> bigPlanetsToReward;

    getCurrentRoundBigPlanets(bigPlanets);
    chooseBigPlanet(bigPlanets, bigPlanetsToReward);

    if(bigPlanetsToReward.size() == 0) return 0;

    uint64_t actualRewardAmount = 0;
    uint64_t rewardPerPlanet = rewardBudget / bigPlanetsToReward.size();

    for(auto bigPlanetId : bigPlanetsToReward) {
        actualRewardAmount += rewardPerPlanet;
        rewardList.push_back(reward{bigPlanetId, rewardPerPlanet, RWD_TYPE_RANDOM});
    }

    return actualRewardAmount;
}

uint64_t starplan::calcBigPlanetReward(vector<reward> &rewardList, uint64_t rewardBudget)
{
    vector<uint64_t> bigPlanets;
    getCurrentRoundBigPlanets(bigPlanets);

    if(bigPlanets.size() == 0) return 0;

    uint64_t lastBigPlanet = 0;
    if(isInviteTimeout(lastBigPlanet)) {// 如果超过12小时没有新的invitee则所有奖励归当轮最后一个大行星
        rewardList.push_back(reward{lastBigPlanet, rewardBudget, RWD_TYPE_TIMEOUT});
        return rewardBudget;
    }

    uint64_t actualRewardAmount = 0;
    uint64_t rewardPerPlanet = rewardBudget / bigPlanets.size();

    for(auto bigPlanetId : bigPlanets) {
        actualRewardAmount += rewardPerPlanet;
        rewardList.push_back(reward{bigPlanetId, rewardPerPlanet, RWD_TYPE_POOL});
    }

    return actualRewardAmount;
}

uint64_t starplan::calcActivePlanetReward(vector<reward> &rewardList, uint64_t rewardBudget)
{
    vector<ActivePlanet> activePlanets;
    uint64_t totalWeight = getCurrentRoundActivePlanets(activePlanets);
    if(totalWeight == 0) return 0;

    uint64_t amount = 0;
    uint64_t totalAmount = 0;
    for(const auto &activePlanet : activePlanets) {
        amount = rewardBudget * activePlanet.weight / totalWeight;
        totalAmount += amount;
        rewardList.push_back(reward{activePlanet.id, amount, RWD_TYPE_ACTIVE});
    }

    return totalAmount;
}

uint64_t starplan::calcSuperStarReward(vector<reward> &rewardList, uint64_t rewardBudget)
{
    vector<SuperStar> superStars;

    uint64_t totalVote = getCurrentRoundSuperStars(superStars);
    if(totalVote == 0) return 0;

    uint64_t amount = 0;
    uint64_t totalAmount = 0;
    for(const auto &superStar : superStars) {
        if(superStar.vote <= 0) continue;
        amount = rewardBudget * superStar.vote / totalVote;
        totalAmount += amount;
        rewardList.push_back(reward{superStar.id, amount, RWD_TYPE_SUPER});
    }

    return totalAmount;
}

bool starplan::baseSecureCheck(vector<reward> &rewardList, uint64_t randomBudget)
{
    if (rewardList.size() == 0) {
        return false;
    }

    uint64_t totalReward = 0;
    for(const auto &reward : rewardList) {
        totalReward += reward.amount;
        graphene_assert(reward.amount <= MAX_USER_REWARD, MSG_USER_REWARD_TOO_MUCH);
    }

    graphene_assert(totalReward <= MAX_ROUND_REWARD + randomBudget, MSG_ROUND_REWARD_TOO_MUCH);

    //TODO add other secure check
    return true;
}

void starplan::doReward(vector<reward> &rewardList)
{
    for (const auto &reward : rewardList)
    {
        if(reward.amount == 0) continue;

        inline_transfer(
                _self,
                reward.to,
                CORE_ASSET_ID,
                reward.amount,
                reward_reasons[reward.type],
                strlen(reward_reasons[reward.type])
        );
        tbrewards.emplace(get_trx_sender(), [&](auto &obj){
            obj.index           = tbrewards.available_primary_key();
            obj.round           = currentRound();
            obj.from            = _self;
            obj.to              = reward.to;
            obj.amount          = reward.amount;
            obj.type            = reward.type;
        });
    }
}

void starplan::createNewRound()
{
    // 1 结束当前轮，修改round表和global表
    auto sender = get_trx_sender();
    tbrounds.modify(lastRound(), sender, [&](auto &obj){
        obj.end_time            = get_head_block_time();
    });
    // 2 创建新的一轮
    auto g_itor = tbglobals.find(0);
    tbglobals.modify(g_itor,sender,[&](auto &obj){
        obj.current_round      = obj.current_round + 1;
    });
    tbrounds.emplace(sender,[&](auto &obj) {
        obj.round                   = tbrounds.available_primary_key();
        obj.current_round_invites   = 0;
        obj.pool_amount             = 0;
        obj.random_pool_amount      = 0;
        obj.invite_pool_amount      = 0;
        obj.start_time              = get_head_block_time();
        obj.end_time                = 0;
    });
}
bool starplan::canUpdateSmall(uint64_t sender)
{
    bool retValue = false;
    auto stk_idx = tbstakes.get_index<N(byaccid)>();
    uint64_t total_vote = 0;
    auto itor = stk_idx.find(sender);
    for(;itor != stk_idx.end();itor++){
        if(itor->account == sender && itor->claimed == false && itor->staking_type == STAKING_TYPE_VOTE){
            total_vote += itor->amount;
            if(total_vote>=Y){
                retValue = true;
                break;
            }
        }else{
            break;
        }
    }
    return retValue;
}

uint64_t starplan::checkSender()
{
    auto sender_id = get_trx_sender();
    auto origin_id = get_trx_origin();
    graphene_assert(sender_id == origin_id, MSG_CHECK_SENDER);
    return sender_id;
}

bool starplan::isUpgrading()
{
    bool retValue   = false;
    graphene_assert(isInit(), MSG_NOT_INITIALIZED);
    auto itor = tbglobals.find(0);
    if(itor->upgrading > 0){retValue = true;}
    return retValue;
}
void starplan::cancelVote(uint64_t voteIndex,uint64_t superAccId,uint64_t amount)
{
    // 修改投票表
    auto vote_itor = tbvotes.find(voteIndex);
    graphene_assert(vote_itor!=tbvotes.end(),MSG_INVALID_ITOR);
    tbvotes.modify(vote_itor,get_trx_sender(),[&](auto &obj){
        obj.disabled            =   true;
    });
    // 修改超级星得票数
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(superAccId);
    graphene_assert(sup_itor!=sup_idx.end(),MSG_INVALID_ITOR);
    sup_idx.modify(sup_itor,get_trx_sender(),[&](auto &obj){
        obj.vote_num            =   obj.vote_num - amount;
    });
}
void starplan::disableSuperStar(uint64_t superAccId)
{
    // 修改超级星表失效
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(superAccId);
    graphene_assert(sup_itor!=sup_idx.end(),MSG_INVALID_ITOR);
    sup_idx.modify(sup_itor,get_trx_sender(),[&](auto &obj){
        obj.disabled            =   true;
    });
}

void starplan::baseCheck()
{
    // 0、防止跨合约调用
    checkSender();
    // 1、验证合约是否初始化
    graphene_assert(isInit(), MSG_NOT_INITIALIZED);
    // 1、验证合约是否在升级
    graphene_assert(!isUpgrading(), MSG_UPGRADING);
}

void starplan::roundFinishCheck()
{
    graphene_assert(!isRoundFinish(), MSG_ROUND_END);
}

uint64_t starplan::assetEqualCheck(uint64_t expectedAmount, const char* errMsg)
{
    graphene_assert(get_action_asset_id() == CORE_ASSET_ID, MSG_CORE_ASSET_REQUIRED);
    graphene_assert(get_action_asset_amount() == expectedAmount, errMsg);

    return expectedAmount;
}

uint64_t starplan::assetLargerCheck(uint64_t expectedAmount, const char* errMsg)
{
    uint64_t actualAmount = get_action_asset_amount();
    graphene_assert(get_action_asset_id() == CORE_ASSET_ID, MSG_CORE_ASSET_REQUIRED);
    graphene_assert(actualAmount >= expectedAmount, errMsg);

    return actualAmount;
}

uint64_t starplan::inviterCheck(const std::string &inviter, uint64_t inviteeId)
{
    if ("" != inviter) {
        uint64_t inviter_id = get_account_id(inviter.c_str(), inviter.length());
        graphene_assert(isInviter(inviter), MSG_CHECK_INVITER_VALID);
        graphene_assert(inviter_id != inviteeId, MSG_CHECK_INVITE_SELF);
        return inviter_id;
    }

    return DEFAULT_INVITER;
}

uint64_t starplan::superStarCheck(const std::string &superStarAccount)
{
    int64_t super_id = get_account_id(superStarAccount.c_str(), superStarAccount.length());
    graphene_assert(isSuperStar(super_id), MSG_CHECK_SUPER_STAR_EXIST);//TODO FIXME check super_id type
    return super_id;
}
