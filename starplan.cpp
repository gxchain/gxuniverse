#include "starplan.hpp"

void starplan::init()
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////
    // 1 校验调用者，只有调用者可以初始化底池，只许调用一次
    uint64_t sender_id = get_trx_origin();
    graphene_assert(sender_id == adminId, "StarPlan Contract Error: Only admin account can init! ");

    // 2、校验充值的资产是否为2000000GXC
    uint64_t ast_id = get_action_asset_id();
    uint64_t amount = get_action_asset_amount();
    graphene_assert(ast_id == coreAsset && amount == initPool * precision, "StarPlan Contract Error: Init amount must is 2000000 GXC and Only can deposit GXC asset!");

    // 3、校验底池是否已经初始化
    auto glo_itor = tbglobals.find(0);
    graphene_assert(glo_itor == tbglobals.end(),"StarPlan Contract Error:  The bonus pool has been initialized");
    
    // 4、校验当前轮资金池是否已经初始化
    auto rou_itor = tbrounds.find(0);
    graphene_assert(rou_itor == tbrounds.end(),"StarPlan Contract Error:  The Current bonus pool has been initialized");

    //////////////////////////////////////// 校验通过后，初始化资金池 //////////////////////////////////////////
    // 1、初始化总资金池
    tbglobals.emplace(_self,[&](auto &obj) {
            obj.index           = 0;
            obj.pool_amount     = amount - roundAmount;
            obj.current_round   = 0;
        });
    // 2、初始化第一轮资金池，并启动第一轮
    tbrounds.emplace(_self,[&](auto &obj) {
            obj.round                   = tbrounds.available_primary_key();
            obj.current_round_invites   = 0;       
            obj.pool_amount             = 0;
            obj.random_pool_amount      = 0;
            obj.invite_pool_amount      = roundAmount;
            obj.start_time              = get_head_block_time();;
            obj.end_time                = 0;
        });
}
void starplan::uptosmall(std:string inviter,std:string superStar)
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////
    uint64_t ast_id = get_action_asset_id();
    uint64_t amount = get_action_asset_amount();

    //1、判断是否存入足够GXC
    graphene_assert(ast_id == coreAsset && amount == y * precision, "100 GXC required");

    //2、验证账户
    if(inviter != "")
        graphene_assert(isAccount(inviter), "inviter account is invalid");
    graphene_assert(isAccount(superStar), "superStar account is invalid");

    //3、验证邀请账户
    graphene_assert(isInviter(inviter), "StarPlan Contract Error: Inviters must be big planets and super star"); 
    uint64_t sender_id = get_trx_origin();
    uint64_t inviter_id = get_account_id(inviter.c_str(), inviter.length());
    graphene_assert(inviter_id != sender_id, "StarPlan Contract Error: Can't invite yourself ");

    //4、验证当前轮是否结束
    graphene_assert(!bSmallRound(),"Current round is end, can't create small planet");

    //5、验证超级星账户
    graphene_assert(isSuperStar(superStar), "StarPlan Contract Error: No found super planet account in superplanets table");

    //6、检查global表和round表的状态
    graphene_assert(isInit(), "Please initialize the game first");

    //////////////////////////////////////// 校验通过后，创建一个小行星 //////////////////////////////////////////
    //7、存到smallPlanet表(不允许重复创建)
    addSmallPlanet(sender_id);

    //8、保存邀请关系(不允许重复邀请)
    invite(sender_id,inviter);
    
    //9、vote(允许重复投票)
    vote(sender_id,superStar);

    //10、添加一个新的抵押金额
    addStake(sender_id,amount);

    //11、修改超级星的得票数
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>
    auto sup_itor = sup_idx.find(super_id);
    sup_idx.modify(sup_itor,_self,[&](auto &obj) {
            obj.vote_num  = obj.vote_num + amount;
        });
}

void starplan::uptobig(std:string inviter)
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////
    uint64_t ast_id = get_action_asset_id();
    uint64_t amount = get_action_asset_amount();

    //1、判断是否存入足够GXC
    graphene_assert(ast_id == coreAsset && amount == depositToBig * precision, "3 GXC required");

    //2、判断是否是small planet，如果还不不是，则提示“You have to become a small planet first”
    uint64_t sender_id = get_trx_origin();
    graphene_assert(isSmallPlanet(sender_id), "You have to become a small planet first");

    //3、判断是否已经是bigPlanet，如果已经是，则提示"You are already a big planet"
    graphene_assert(!isBigPlanet(sender_id), "You are already a big planet");

    //4、验证当前轮是否结束
    graphene_assert(!bSmallRound(),"Current round is end, can't create big planet");

    //5、验证当前轮状态和global状态
    graphene_assert(isInit(), "Please initialize the game first");

    //////////////////////////////////////// 校验通过后，创建一个大行星 //////////////////////////////////////////
    //6、存到bigPlanet表
    addBigPlanet(sender_id);

    //7、激活邀请关系
    actinvite(sender_id);

    //8、将3个GXC转移到奖池，将其中一个GXC发送给邀请人
    sendInviteReward(sender_id);

    //9、创建/更新活力星
    addActivebybig(sender_id);
}

// inviter为0，表示没有邀请账户
void starplan::uptosuper(std:string inviter)
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////
    uint64_t ast_id = get_action_asset_id();
    uint64_t amount = get_action_asset_amount();

    //1、判断是否存入足够GXC
    graphene_assert(ast_id == coreAsset && amount == x * precision, "20000 GXC required");

    //2、判断是否已经是superstar，如果已经是，则提示"You are already a super star"
    uint64_t sender_id = get_trx_origin();
    graphene_assert(!isSuperStar(sender_id), "You are already a super star");

    //3、验证账户是否存在
    if(inviter != "")
        graphene_assert(isAccount(inviter), "inviter account is invalid");

    //4、验证邀请账户
    graphene_assert(isInviter(inviter), "StarPlan Contract Error: Inviters must be big planets and super star"); 
    uint64_t sender_id = get_trx_origin();
    uint64_t inviter_id = get_account_id(inviter.c_str(), inviter.length());
    graphene_assert(inviter_id != sender_id, "StarPlan Contract Error: Can't invite yourself ");

    //5、验证当前轮是否结束
    graphene_assert(!bSmallRound(),"Current round is end, can't create super star");

    //6、验证是否已经初始化
    graphene_assert(isInit(), "Please initialize the game first");

    //////////////////////////////////////// 校验通过后，创建一个超级星 //////////////////////////////////////////
    //7、创建超级星
    addSuperStar(sender_id);

    //8、创建抵押项
    addStake(sender_id,amount);

    //9、保存邀请关系，激活邀请关系
    invite(sender_id,inviter);
    actinvite(sender_id);

    //10、插入更新一条活力星记录，权重为1
    addActivebysuper(sender_id);
}
void starplan::endround()
{
    // 1 验证调用者账户是否为admin账户
    uint64_t sender_id = get_trx_origin();
    graphene_assert(sender_id == adminId, "StarPlan Contract Error: Only support admin account! ");
    // 2 验证当前轮是否可以结束
    graphene_assert(bSmallRound(),"Current round is not end");
    // 3 计算奖励 / 发放奖励
    // todo
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
    if(big_itor != tbbigplanets.end() || sup_itor != tbsuperstars.end())
        retValue = true;
    return retValue;
}
bool starplan::isSuperStar(uint64_t sender)
{
    bool retValue = false;
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>();
    auto sup_itor = sup_idx.find(sender);
    if(sup_itor != tbsuperstars.end()) { retValue = true; }
    return retValue;
}
bool starplan::addSuperStar(uint64_t sender)
{
    tbsuperstars.emplace(_self,[&](auto &obj) {                 //创建超级星
        obj.index                   = tbsuperstars.available_primary_key();
        obj.id                      = sender_id;
        obj.create_time             = get_head_block_time();      
        obj.create_round            = currentRound();
        obj.voted_num               = 0; 
    });
}
bool starplan::isSmallPlanet(uint64_t sender)
{
    bool retValue = false;
    auto small_idx = tbsmallplans.get_index<N(byaccid)>();
    auto small_itor = small_idx.find(sender);
    if(small_itor != tbsmallplans.end()) { retValue = true; }
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
    if(big_itor != tbbigplanets.end()) { retValue = true; }
    return retValue;
}
bool starplan::addBigPlanet(uint64_t sender)
{
    tbbigplanets.emplace(_self,[&](auto &obj){                            //创建一个大行星
            obj.index                   = tbbigplanets.available_primary_key();
            obj.id                      = sender;
            obj.create_time             = get_head_block_time();
            obj.create_round_num        = currentRound();
        });
    return true;
}
bool starplan::hasInvited(uint64_t original_sender,std:string inviter)
{
    bool retValue = false;
    auto invite_idx = tbinvites.get_index<N(byaccid)>();
    auto invite_itor = invite_idx.find(original_sender);
    if(invite_itor != invite_idx.end()) { retValue = true; }
    return retValue;
}
uint32_t starplan::totalInvites()
{
    auto inv_idx = tbinvites.get_index<N(byenable)>();
    return inv_idx.upper_bound(1) - inv_idx.lower_bound(1);
}
bool starplan::bSmallRound()
{
    bool retValue = false;
    // 获取最后一个大行星
    auto big_itor = tbbigplanets.rend();
    graphene_assert(get_head_block_time() > (get_big_itor->create_time), "StarPlan Contract Error: big planet create time is error ");
    bool isDelay = (get_head_block_time() - (get_big_itor->create_time)) > delaytime;
    bool isWrong = totalInvites() - currentRound()*roundSize >= roundSize;
    if( isDelay || isWrong) { retValue = true; }
    return retValue;
}
uint32_t starplan::currentRound()
{
    auto round_itor = tbrounds.rend();
    graphene_assert(round_itor != tbrounds.rbegin(), "StarPlan Contract Error: Found round table wrong! ");
    return round_itor->round;
}
void starplan::invite(uint64_t original_sender,std:string inviter)
{
    uint64_t inviter_id = get_account_id(inviter.c_str(), inviter.length());
    if(!hasInvited(original_sender,inviter)){                                 //不存在邀请关系则创建，
        tbinvites.emplace(_self,[&](auto &obj) {
            obj.index                   = tbinvites.available_primary_key();
            obj.invitee                 = original_sender;
            obj.inviter                 = inviter_id;
            obj.enabled                 = false;       
            obj.create_round            = currentRound();                      //升级为大行星或者超级星时，重新设置
            obj.create_time             = get_head_block_time();
        });
    }
}
void starplan::actinvite(uint64_t original_sender)
{
    auto invite_idx = tbinvites.get_index<N(byaccid)>();
    auto invite_itor = invite_idx.find(original_sender);
    invite_idx.modify(invite_itor,_self,[&](auto &obj){                        
            obj.enabled                 = true;
            obj.create_round            = currentRound();
            obj.create_time             = get_head_block_time();
        });
}

void starplan::vote(uint64_t original_sender,std:string superstar)
{
    uint64_t super_id = get_account_id(superstar.c_str(), superstar.length()); 
    tbvotes.emplace(_self,[&](auto &obj) {
        obj.index                   = tbvotes.available_primary_key();
        obj.round                   = currentRound();
        obj.stake_amount            = y * precision;
        obj.from                    = original_sender; 
        obj.to                      = super_id;           
        obj.vote_time               = get_head_block_time();
    });
}
void starplan::addStake(uint64_t sender,uint64_t amount)
{
    tbstakes.emplace(_self,[&](auto &obj) {
        obj.index                   = tbstakes.available_primary_key();
        obj.account                 = sender;
        obj.amount                  = amount;
        obj.end_time                = get_head_block_time() + delayDay; 
    });
}

void starplan::sendInviteReward(uint64_t sender)
{
    auto round_itor = tbrounds.rend();
    graphene_assert(round_itor != tbrounds.rbegin(), "StarPlan Contract Error: Found round table wrong! ");
    auto invite_idx = tbinvites.get_index<N(byaccid)>();
    auto invite_itor = invite_idx.find(sender);
    tbrounds.modify(*round_itor, _self, [&](auto &obj){                               //修改奖池金额
            obj.random_pool_amount      = obj.random_pool_amount + z3 * precision;
            if(invite_itor->inviter == 0)
                obj.invite_pool_amount  = obj.invite_pool_amount + (z1 + z2) * precision;
            else
                obj.invite_pool_amount  = obj.invite_pool_amount + z1 * precision;
    });
    if(invite_itor->inviter != 0)
        inline_transfer(_self , invite_itor->inviter , coreAsset , z2 * precision,inviter_withdraw.c_str(),inviter_withdraw,length());
    
}
void starplan::updateActivePlanetsbybig(uint64_t sender)
{
    auto invite_idx = tbinvites.get_index<N(byaccid)>();
    auto invite_itor = invite_idx.find(sender);
    auto act_idx = tbactiveplans.get_index<N(byaccid)>();
    auto act_itor = act_idx.find(invite_itor->inviter);
    if(act_itor != act_idx.end()){
        act_idx.modify(act_itor,_self,[&](auto &obj){                                   //修改活力星
            if(obj.invite_count == 4){
                obj.invite_count = 0;
                obj.create_round = currentRound();
                obj.weight       = weight;
            }else{
                obj.invite_count = obj.invite_count + 1;
            }
        });
    }else{
        tbactiveplans.emplace(_self,[&](auto &obj){                                      //创建活力星
            obj.index           = act_idx.available_primary_key();
            obj.id              = invite_itor->inviter;
            obj.invite_count    = 0;
            obj.create_time     = get_head_block_time();
            obj.create_round    = 0;
            obj.weight          = 0;
        });
    }
}
void starplan::updateActivePlanetsbysuper(uint64_t sender)
{
    auto invite_idx = tbinvites.get_index<N(byaccid)>();
    auto invite_itor = invite_idx.find(sender);
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
            obj.index           = act_idx.available_primary_key();
            obj.id              = invite_itor->inviter;
            obj.invite_count    = 0;
            obj.create_time     = get_head_block_time();
            obj.create_round    = currentRound();
            obj.weight          = weight;
        });
    }
}
