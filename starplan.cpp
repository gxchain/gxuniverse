#include "starplan.hpp"

void starplan::init()
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////
    // 0 防止跨合约调用
    graphene_assert(checkSender(), CHECKSENDERMSG);

    // 1 校验调用者，只有调用者可以初始化底池，只许调用一次
    uint64_t sender_id = get_trx_origin();
    graphene_assert(sender_id == adminId, CHECKADMINMSG);

    // 2、校验充值的资产是否为2000000GXC
    uint64_t ast_id = get_action_asset_id();
    uint64_t amount = get_action_asset_amount();
    graphene_assert(ast_id == coreAsset && amount == initPool * precision, INITDEPOMSG);

    // 3、校验底池是否已经初始化
    auto glo_itor = tbglobals.find(0);
    graphene_assert(glo_itor == tbglobals.end(),INITGLOBALMSG);

    // 4、校验当前轮资金池是否已经初始化
    auto rou_itor = tbrounds.find(0);
    graphene_assert(rou_itor == tbrounds.end(),INITROUUNDMSG);

    //////////////////////////////////////// 校验通过后，初始化资金池 //////////////////////////////////////////
    // 1、初始化总资金池
    tbglobals.emplace(_self,[&](auto &obj) {
            obj.index           = 0;
            obj.pool_amount     = amount;
            obj.current_round   = 0;
            obj.is_upgrade      = 0;
        });
    // 2、初始化第一轮资金池，并启动第一轮
    tbrounds.emplace(_self,[&](auto &obj) {
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

    // 0 防止跨合约调用
    graphene_assert(checkSender(), CHECKSENDERMSG);

    uint64_t ast_id = get_action_asset_id();
    uint64_t amount = get_action_asset_amount();

    //1、判断是否充值0.1GXC
    std::string depomsg = DEPOMSG;
    depomsg = depomsg.replace(depomsg.find("%d"),1,std::to_string(0.1));
    graphene_assert(ast_id == coreAsset && amount >= precision / 10, depomsg.c_str());

    uint64_t sender_id = get_trx_origin();
    //2、验证账户
    if(inviter != ""){
        graphene_assert(isAccount(inviter), CHECKACCOUNTMSG);
        uint64_t inviter_id = get_account_id(inviter.c_str(), inviter.length());
        graphene_assert(isInviter(inviter), CHECKINVITERMSG);
        graphene_assert(inviter_id != sender_id, CHECKINVSENDMSG);

    }
    graphene_assert(isAccount(superstar), CHECKACCOUNTMSG);

    //3、验证合约是否初始化、合约是否在升级
    graphene_assert(isInit(), ISINITMSG);
    graphene_assert(!isUpgrade(), ISUPGRADEMSG);


    //4、验证当前轮是否结束
    graphene_assert(!bSmallRound(),CHECKROUENDMSG);

    //5、验证超级星账户
    auto super_id = get_account_id(superstar.c_str(), superstar.length());
    graphene_assert(isSuperStar(super_id), CHECKSUPERMSG);

    //////////////////////////////////////// 校验通过后，创建一个小行星 //////////////////////////////////////////

    //6、保存邀请关系(不允许重复邀请)
    invite(sender_id,inviter);

    //7、vote(允许重复投票)
    createVote(sender_id,superstar);

    //8、添加一个新的抵押金额
    addStake(sender_id,amount,super_id,STAKE_TYPE_VOTE);

    //9、存到smallPlanet表(不允许重复创建)
    if(canUpdateSmall(sender_id))
        addSmallPlanet(sender_id);

    //11、修改超级星的得票数
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(super_id);
    sup_idx.modify(sup_itor,_self,[&](auto &obj) {
            obj.vote_num  = obj.vote_num + amount;
        });
}

void starplan::uptobig()
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////

    // 0 防止跨合约调用
    graphene_assert(checkSender(), CHECKSENDERMSG);

    uint64_t ast_id = get_action_asset_id();
    uint64_t amount = get_action_asset_amount();

    //1、判断是否存入足够GXC
    uint64_t depositToBig = z1 + z2 + z3;
    std::string depomsg = DEPOMSG;
    depomsg = depomsg.replace(depomsg.find("%d"),1,std::to_string(depositToBig));
    graphene_assert(ast_id == coreAsset && amount == depositToBig * precision, depomsg.c_str());

    //2、验证合约是否初始化、合约是否在升级
    graphene_assert(isInit(), ISINITMSG);
    graphene_assert(!isUpgrade(), ISUPGRADEMSG);

    //3、判断是否是small planet，如果还不不是，则提示“You have to become a small planet first”
    uint64_t sender_id = get_trx_origin();
    graphene_assert(isSmallPlanet(sender_id), CHECKSMALLMSG);

    //5、验证当前轮是否结束
    graphene_assert(!bSmallRound(),CHECKROUENDMSG);

    //////////////////////////////////////// 校验通过后，创建一个大行星 //////////////////////////////////////////
    //6、存到bigPlanet表
    graphene_assert(addBigPlanet(sender_id), CHECKBIGMSG);

    //7、激活邀请关系
    actInvite(sender_id);

    //8、将3个GXC转移到奖池，将其中一个GXC发送给邀请人
    distriInvRewards(sender_id);

    //9、创建/更新活力星
    updateActivePlanetsByBig(sender_id);

    //10、当邀请关系满100人，开启新的一轮
    if(lastRound().current_round_invites >= roundSize ){
        endround();
    }
}

// inviter为0，表示没有邀请账户
void starplan::uptosuper(std::string inviter)
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////

    // 0 防止跨合约调用
    graphene_assert(checkSender(), CHECKSENDERMSG);

    uint64_t ast_id = get_action_asset_id();
    uint64_t amount = get_action_asset_amount();

    //1、判断是否存入足够GXC
    std::string depomsg = DEPOMSG;
    depomsg = depomsg.replace(depomsg.find("%d"),1,std::to_string(x));
    graphene_assert(ast_id == coreAsset && amount == x * precision, depomsg.c_str());

    uint64_t sender_id = get_trx_origin();

    //3、验证账户是否存在
    if(inviter != ""){
        graphene_assert(isAccount(inviter), CHECKACCOUNTMSG);
        graphene_assert(isInviter(inviter), CHECKINVITERMSG);
        uint64_t inviter_id = get_account_id(inviter.c_str(), inviter.length());
        graphene_assert(inviter_id != sender_id, CHECKINVSENDMSG);
    }

    //4、验证合约是否初始化、合约是否在升级
    graphene_assert(isInit(), ISINITMSG);
    graphene_assert(!isUpgrade(), ISUPGRADEMSG);

    //5、验证当前轮是否结束
    graphene_assert(!bSmallRound(),CHECKROUENDMSG);

    //////////////////////////////////////// 校验通过后，创建一个超级星 //////////////////////////////////////////

    //7、创建超级星
    graphene_assert(addSuperStar(sender_id), ISSUPERMSG);

    //8、创建抵押项
    addStake(sender_id, amount, sender_id, STAKE_TYPE_TOSUPER);

    //9、保存邀请关系，激活邀请关系
    invite(sender_id,inviter);
    actInvite(sender_id);

    //10、插入更新一条活力星记录，权重为1
    updateActivePlanetsBySuper(sender_id);
}
void starplan::endround()
{
    // 0 防止跨合约调用
    graphene_assert(checkSender(), CHECKSENDERMSG);

    // 1 验证调用者账户是否为admin账户
    if(lastRound().current_round_invites < roundSize ){
        uint64_t sender_id = get_trx_origin();
        graphene_assert(sender_id == adminId, CHECKADMINMSG);
    }

    graphene_assert(isInit(), ISINITMSG);
    graphene_assert(!isUpgrade(), ISUPGRADEMSG);

    // 2 验证当前轮是否可以结束
    graphene_assert(bSmallRound(),ISENDROUNDMSG);
    // 3 计算奖池
    calcCurrentRoundPoolAmount();

    uint64_t randomBudget = getRandomRewardBudget();
    uint64_t bigPlanetBudget = getBigPlanetRewardBudget();
    uint64_t activePlanetBudget = getActivePlanetRewardBudget();
    uint64_t superStarBudget = getSuperStarRewardBudget();

    uint64_t actualReward = 0;
    vector<reward> rewardList;

    actualReward += calcRandomReward(rewardList, randomBudget);
    actualReward += calcBigPlanetReward(rewardList, bigPlanetBudget);
    actualReward += calcActivePlanetReward(rewardList, activePlanetBudget);
    actualReward += calcSuperStarReward(rewardList, superStarBudget);

    if (baseSecureCheck(rewardList))
    {
        doReward(rewardList);
    }


    // 更新活力星权重
    updateActivePlanets();

    // 9 开启新的一轮
    createNewRound();
}
void starplan::unstake(std::string account)
{
    // 0 防止跨合约调用
    graphene_assert(checkSender(), CHECKSENDERMSG);
    graphene_assert(isInit(), ISINITMSG);
    graphene_assert(!isUpgrade(), ISUPGRADEMSG);

    const std::string unstake_withdraw = UNSTAKELOG;                        //抵押提现
    uint64_t acc_id = get_account_id(account.c_str(), account.length());
    auto sta_idx = tbstakes.get_index<N(byaccid)>();
    auto itor = sta_idx.find(acc_id);
    for(; itor != sta_idx.end() && itor->account == acc_id;){
        if(get_head_block_time() > itor->end_time){
            // 解除抵押提现
            inline_transfer(_self , acc_id , coreAsset , itor->amount, unstake_withdraw.c_str(),unstake_withdraw.length());
            sta_idx.modify(itor,_self,[&](auto &obj){
                obj.is_unstake          =   true;
                obj.unstake_time        =   get_head_block_time();
            });
            itor++;
        }else{
            itor++;
        }
    }
}
void starplan::upgrade(uint64_t flag)
{
    // 0 防止跨合约调用
    graphene_assert(checkSender(), CHECKSENDERMSG);

    // 1 验证调用者账户是否为admin账户
    uint64_t sender_id = get_trx_origin();
    graphene_assert(sender_id == adminId, CHECKADMINMSG);
    // 2 验证合约是否已经初始化
    graphene_assert(isInit(), ISINITMSG);
    // 3 修改global表
    auto itor = tbglobals.find(0);
    tbglobals.modify(itor,_self,[&](auto &obj) {
        obj.is_upgrade          =   flag;
    });
}
bool starplan::isAccount(std::string accname)
{
    bool retValue = false;
    int64_t acc_id = get_account_id(accname.c_str(), accname.length());
    if(acc_id != -1){ retValue = true; }
    return retValue;
}
bool starplan::isInit()
{
    bool retValue = false;
    // 1 校验底池是否已经初始化
    auto itor = tbglobals.find(0);
    // 2 校验当前轮资金池是否已经初始化
    auto itor2 = tbrounds.find(0);
    if(itor != tbglobals.end() && itor2 != tbrounds.end()){ retValue = true;}
    return retValue;
}
bool starplan::isInviter(std::string accname)
{
    // 验证邀请账户是否为大行星或者超级星
    bool retValue = false;
    uint64_t inviter_id = get_account_id(accname.c_str(), accname.length());
    auto big_idx = tbbigplanets.get_index<N(byaccid)>();
    auto big_itor = big_idx.find(inviter_id);
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(inviter_id);
    if(big_itor != big_idx.end() || sup_itor != sup_idx.end())
        retValue = true;
    return retValue;
}
bool starplan::isSuperStar(uint64_t sender)
{
    bool retValue = false;
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(sender);
    if(sup_itor != sup_idx.end()) { retValue = true; }
    return retValue;
}
bool starplan::addSuperStar(uint64_t sender)
{
    if(!isBigPlanet(sender)){
        tbsuperstars.emplace(_self,[&](auto &obj) {                 //创建超级星
            obj.index                   = tbsuperstars.available_primary_key();
            obj.id                      = sender;
            obj.create_time             = get_head_block_time();
            obj.create_round            = currentRound();
            obj.vote_num                = 0;
        });
        return true;
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
    if(!isSmallPlanet(sender)){                                                  //创建小行星
        tbsmallplans.emplace(_self,[&](auto &obj){
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
    if(big_itor != big_idx.end()) { retValue = true; }
    return retValue;
}
bool starplan::addBigPlanet(uint64_t sender)
{
    if(!isBigPlanet(sender)){
        tbbigplanets.emplace(_self,[&](auto &obj){                            //创建一个大行星
                obj.index                   = tbbigplanets.available_primary_key();
                obj.id                      = sender;
                obj.create_time             = get_head_block_time();
                obj.create_round            = currentRound();
            });
        return true;
    }
    return false;
}
bool starplan::hasInvited(uint64_t sender)
{
    bool retValue = false;
    auto invite_idx = tbinvites.get_index<N(byaccid)>();
    auto invite_itor = invite_idx.find(sender);
    if(invite_itor != invite_idx.end()) { retValue = true; }
    return retValue;
}

bool starplan::bSmallRound()
{
    bool retValue = false;
    // 获取最后一个大行星
    auto big_itor = tbbigplanets.end();
    if(big_itor == tbbigplanets.begin()){
        return retValue;
    }
    big_itor--;
    if(big_itor->create_round !=currentRound()){ return false;}
    graphene_assert(get_head_block_time() > (big_itor->create_time), CHECKBIGTIMEMSG);
    bool isDelay = (get_head_block_time() - (big_itor->create_time)) > delaytime;
    auto round_itor = tbrounds.end();
    graphene_assert(round_itor != tbrounds.begin(), FINDROUNDMSG);
    round_itor--;
    bool isFull = round_itor->current_round_invites >= roundSize;
    if( isDelay || isFull) { retValue = true; }
    return retValue;
}

bool starplan::isInviteTimeout(uint64_t &lastBigPlanet)
{
    auto big_itor = tbbigplanets.end();
    if(big_itor == tbbigplanets.begin()){
        return false;
    }
    big_itor--;

    graphene_assert(get_head_block_time() > big_itor->create_time, CHECKBIGTIMEMSG);
    if((get_head_block_time() - big_itor->create_time) > delaytime) {
        lastBigPlanet = big_itor->id;
        return true;
    }

    return false;
}

bool starplan::isRoundFull()
{
    auto round_itor = tbrounds.end();
    graphene_assert(round_itor != tbrounds.begin(), FINDROUNDMSG);
    round_itor--;
    return round_itor->current_round_invites >= roundSize;
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
void starplan::invite(uint64_t sender,std::string inviter)
{
    uint64_t inviter_id = get_account_id(inviter.c_str(), inviter.length());
    if(inviter_id == -1)
        inviter_id = defaultinviter;
    if(!hasInvited(sender,inviter)){                                 //不存在邀请关系则创建，
        tbinvites.emplace(_self,[&](auto &obj) {
            obj.index                   = tbinvites.available_primary_key();
            obj.invitee                 = sender;
            obj.inviter                 = inviter_id;
            obj.enabled                 = false;
            obj.create_round            = currentRound();                      //升级为大行星或者超级星时，重新设置
            obj.create_time             = get_head_block_time();
        });
    }
}
void starplan::actInvite(uint64_t sender)
{
    auto invite_idx = tbinvites.get_index<N(byaccid)>();
    auto invite_itor = invite_idx.find(sender);
    invite_idx.modify(invite_itor,_self,[&](auto &obj){
        obj.enabled                 = true;
    });
    // 当前轮邀请数自增1
    tbrounds.modify(lastRound(),_self,[&](auto &obj){
        obj.current_round_invites   = obj.current_round_invites + 1;
    });
}

void starplan::createVote(uint64_t sender,std::string superstar)
{
    uint64_t amount = get_action_asset_amount();
    uint64_t super_id = get_account_id(superstar.c_str(), superstar.length());
    tbvotes.emplace(_self,[&](auto &obj) {
        obj.index                   = tbvotes.available_primary_key();
        obj.round                   = currentRound();
        obj.stake_amount            = amount;
        obj.from                    = sender;
        obj.to                      = super_id;
        obj.vote_time               = get_head_block_time();
    });
}
void starplan::addStake(uint64_t sender,uint64_t amount,uint64_t to,uint64_t reason)
{
    tbstakes.emplace(_self,[&](auto &obj) {
        obj.index                   = tbstakes.available_primary_key();
        obj.account                 = sender;
        obj.amount                  = amount;
        obj.end_time                = get_head_block_time() + stakingDelayTime;
        obj.staketo                 = to;
        obj.reason                  = reason;
        obj.is_unstake              = 0;
        obj.unstake_time            = 0;
    });
}

void starplan::distriInvRewards(uint64_t sender)
{
    std::string   inviter_withdraw     = INVITERLOG;  //提现一个1GXC到邀请人账户
    auto invite_idx = tbinvites.get_index<N(byaccid)>();
    auto invite_itor = invite_idx.find(sender);
    tbrounds.modify(lastRound(), _self, [&](auto &obj){                               //修改奖池金额
            obj.random_pool_amount      = obj.random_pool_amount + z3 * precision;
            obj.invite_pool_amount      = obj.invite_pool_amount + z1 * precision;
    });
    inline_transfer(_self , invite_itor->inviter , coreAsset , z2 * precision,inviter_withdraw.c_str(),inviter_withdraw.length());
}
void starplan::updateActivePlanetsByBig(uint64_t sender)
{
    auto invite_idx = tbinvites.get_index<N(byaccid)>();
    auto invite_itor = invite_idx.find(sender);
    auto act_idx = tbactiveplans.get_index<N(byaccid)>();
    auto act_itor = act_idx.find(invite_itor->inviter);
    if(act_itor != act_idx.end()){
        act_idx.modify(act_itor,_self,[&](auto &obj){                                   //修改活力星
            if(obj.invite_count == 5){
                obj.invite_count = 1;
                obj.create_round = currentRound();
                obj.weight       = weight;
            }else{
                obj.invite_count = obj.invite_count + 1;
            }
        });
    }else{
        tbactiveplans.emplace(_self,[&](auto &obj){                                      //创建活力星
            obj.index           = tbactiveplans.available_primary_key();
            obj.id              = invite_itor->inviter;
            obj.invite_count    = 1;
            obj.create_time     = get_head_block_time();
            obj.create_round    = 0;
            obj.weight          = 0;
        });
    }
}
void starplan::updateActivePlanetsBySuper(uint64_t sender)
{
    auto act_idx = tbactiveplans.get_index<N(byaccid)>();
    auto act_itor = act_idx.find(sender);
    if(act_itor != act_idx.end()){
        act_idx.modify(act_itor,_self,[&](auto &obj){                                   //修改活力星
            obj.invite_count    = 0;
            obj.create_round    = currentRound();
            obj.weight          = weight;
        });
    }else{
        tbactiveplans.emplace(_self,[&](auto &obj){                                      //创建活力星
            obj.index           = tbactiveplans.available_primary_key();
            obj.id              = sender;
            obj.invite_count    = 0;
            obj.create_time     = get_head_block_time();
            obj.create_round    = currentRound();
            obj.weight          = weight;
        });
    }
}

void starplan::calcCurrentRoundPoolAmount()
{
    // 1、获取平均奖励池
    auto &round = lastRound();

    // 2、默认底池
    auto pool_amount = roundAmount * precision + round.invite_pool_amount;
    // 3、超过四小时，每小时减少底池金额
    auto x = currentRound()%bigRoundSize + 1;
    // 4、计算当前小轮的运行时间
    if(get_head_block_time() - round.start_time > decayTime){
        auto dursize = ((get_head_block_time() - round.start_time - decayTime) / decayDur) + 1;
        dursize = dursize > maxDecayCount ? maxDecayCount:dursize;
        graphene_assert(pool_amount > (dursize * x * precision), CHECKATTENMSG);
        pool_amount = pool_amount - dursize * x * precision;
    }
    // 5、修改当前轮底池 pool_amount
    tbrounds.modify(round, _self, [&](auto &obj){                               //修改奖池金额pool_amount
        obj.pool_amount      = pool_amount;
    });
    // 6、修改总的资金池
    auto g_itor = tbglobals.find(0);
    tbglobals.modify(g_itor,_self,[&](auto &obj){
        obj.pool_amount      = obj.pool_amount - pool_amount;
    });
}
void starplan::updateActivePlanets()
{
    // 更新活力星的权重
    auto act_itor = tbactiveplans.begin();
    for( ; act_itor != tbactiveplans.end(); act_itor++){
        tbactiveplans.modify(act_itor,_self,[&](auto &obj){                           //修改活力星的权重
            uint64_t bDecay_prec = bDecay * 1000;
            uint64_t new_weight  = obj.weight * bDecay_prec / 1000;
            obj.weight      = new_weight;
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
    uint64_t total_weight = 0;
    auto itor = act_idx.upper_bound(0);
    while (itor != act_idx.end() && itor->weight > 0)
    {
        total_weight += itor->weight;
        activePlanets.push_back(ActivePlanet { itor->id, itor->weight });
        itor++;
    }

    return total_weight;
}

uint64_t starplan::getCurrentRoundSuperStars(vector<SuperStar> &superStars)
{
    uint64_t total_vote = 0;

    for (auto itor = tbsuperstars.begin(); itor != tbsuperstars.end(); itor++)
    {
        total_vote += itor->vote_num;
        superStars.push_back(SuperStar{itor->id, itor->vote_num, itor->vote_num != 0});
    }

    return total_vote;
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
                choosed.push_back(number);
                break;
            }
        }
    }
}

const struct tbround& starplan::lastRound()
{
    auto round_itor = tbrounds.end();
    graphene_assert(round_itor != tbrounds.begin(), findRoundMsg);
    round_itor--;
    return *round_itor;
}

uint64_t starplan::getRandomRewardBudget()
{
    return lastRound().random_pool_amount;
}

uint64_t starplan::getBigPlanetRewardBudget()
{
    return lastRound().pool_amount * payBackPercent / 100 ;
}

uint64_t starplan::getActivePlanetRewardBudget()
{
    return lastRound().pool_amount * activePercent / 100 ;
}

uint64_t starplan::getSuperStarRewardBudget()
{
    return lastRound().pool_amount * (100 - payBackPercent - activePercent) / 100;
}

uint64_t starplan::calcRandomReward(vector<reward> &rewardList, uint64_t rewardBudget)
{
    vector<uint64_t> &bigPlanets;
    vector<uint64_t> &bigPlanetsToReward;

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
    vector<uint64_t> &bigPlanets;
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

bool starplan::baseSecureCheck(vector<reward> &rewardList)
{
    uint64_t totalReward = 0;
    for(const auto &reward : rewardList) {
        graphene_assert(reward.to > 0, "");
        totalReward + reward.amount;
        graphene_assert(reward.amount > 0 && reward.amount < MAX_USER_REWARD, "");
    }

    graphene_assert(totalReward > 0 && totalReward < MAX_ROUND_REWARD, "");

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
                coreAsset,
                reward.amount,
                reward_reasons[reward.type],
                strlen(reward_reasons[reward.type])
        );
    }
}

void starplan::createNewRound()
{
    // 1 结束当前轮，修改round表和global表
    tbrounds.modify(lastRound(), _self, [&](auto &obj){
        obj.end_time            = get_head_block_time();
    });
    // 2 创建新的一轮
    auto g_itor = tbglobals.find(0);
    tbglobals.modify(g_itor,_self,[&](auto &obj){
        obj.current_round      = obj.current_round + 1;
    });
    tbrounds.emplace(_self,[&](auto &obj) {
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
        if(itor->account == sender && itor->is_unstake == 0){
            total_vote += itor->amount;
            if(total_vote>=y*precision){
                retValue = true;
                break;
            }
        }else{
            break;
        }
    }
    return retValue;
}
void starplan::deleteVote(uint64_t sender,uint64_t time)
{
    auto vot_idx = tbvotes.get_index<N(byfrom)>();
    auto itor = vot_idx.find(sender);
    for(;itor != vot_idx.end();){
        if(itor->from == sender){
            if(itor->vote_time == time + stakingDelayTime){
                itor = vot_idx.erase(itor);
                break;
            }else{ itor++ ;}
        }else{
            break;
        }
    }
}
void starplan::checkWithdraw(uint64_t pool,uint64_t amount)
{
    if(pool<amount){
        graphene_assert(false, CHECKAMOUNTMSG);
    }
}
bool starplan::checkSender()
{
    bool retValue = false;
    auto sender_id = get_trx_sender();
    auto origin_id = get_trx_origin();
    if(sender_id == origin_id){ retValue = true; }//TODO here to assert
    return retValue;
}
bool starplan::isUpgrade()
{
    bool retValue   = false;
    graphene_assert(isInit(), ISINITMSG);
    auto itor = tbglobals.find(0);
    if(itor->is_upgrade > 0){retValue = true;}
    return retValue;
}
