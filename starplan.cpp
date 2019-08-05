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
            obj.pool_amount             = roundAmount;
            obj.big_stars               = {};
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
    graphene_assert(ast_id == coreAsset && amount == vote_amount * precision, "100 GXC required");

    //2、验证邀请账户
    graphene_assert(isInviter(inviter) == true, "StarPlan Contract Error: Inviters must be big planets and super star"); 
    uint64_t sender_id = get_trx_origin();
    uint64_t inviter_id = get_account_id(inviter.c_str(), inviter.length());
    graphene_assert(inviter_id != sender_id, "StarPlan Contract Error: Can't invite yourself ");

    //3、验证超级星账户
    graphene_assert(isSuperStar(superStar) == true, "StarPlan Contract Error: No found super planet account in superplanets table");

    //4、检查global表和round表的状态
    check_stat();

    //////////////////////////////////////// 校验通过后，创建一个小行星 //////////////////////////////////////////
    // 1、存到smallPlanet表(不允许重复创建)
    auto round_itor = tbrounds.cend();
    graphene_assert(round_itor != tbrounds.cbegin(), "StarPlan Contract Error: Found round table wrong! ");
    if(!isSmallPlanet(sender_id)){                                      //创建小行星
        tbsmallplans.emplace(_self,[&](auto &obj){
            obj.index                   = tbsmallplans.available_primary_key();
            obj.id                      = sender_id;
            obj.create_time             = get_head_block_time();
            obj.create_round            = round_itor->round;
        });
    }
    // 2、保存邀请关系(不允许重复邀请)
    auto sender_itor = tbinvites.find(sender_id);
    if(!hasInvited(sender_id,inviter)){                                 //不存在邀请关系则创建，
        tbinvites.emplace(_self,[&](auto &obj) {
            obj.index                   = tbinvites.available_primary_key();
            obj.invitee                 = sender_id;
            obj.inviter                 = inviter_id;
            obj.enabled                 = false;       
            obj.create_round            = round_itor->round;            //升级为小行星或者超级星时，重新设置
            obj.create_time             = get_head_block_time();
        });
    }
    
    // 3、vote(允许重复投票)
    uint64_t super_id = get_account_id(superStar.c_str(), superStar.length()); 
    tbvotes.emplace(_self,[&](auto &obj) {
            obj.index                   = tbvotes.available_primary_key();
            obj.round                   = round_itor->round;
            obj.stake_amount            = amount;
            obj.from                    = sender_id; 
            obj.to                      = super_id;           
            obj.vote_time               = get_head_block_time();
        });
    // 4、添加一个新的抵押金额
    tbstakes.emplace(_self,[&](auto &obj) {
            obj.index                   = tbstakes.available_primary_key();
            obj.account                 = sender_id;
            obj.amount                  = amount;
            obj.end_time                = get_head_block_time() + delayDay; 
        });
    // 5、修改超级星的得票数
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>
    auto sup_itor = sup_idx.find(super_id);
    tbsuperstars.modify(sup_itor,_self,[&](auto &obj) {
            obj.vote_num                  = obj.vote_num + amount;
        });
}

void starplan::uptobig(uint64_t inviter)
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////
    uint64_t ast_id = get_action_asset_id();
    uint64_t amount = get_action_asset_amount();

    //1、判断是否存入足够GXC
    graphene_assert(ast_id == coreAsset && amount == depositToBig * precision, "3 GXC required");

    //2. 判断是否是small planet，如果还不不是，则提示“You have to become a small planet first”
    uint64_t sender_id = get_trx_origin();
    graphene_assert(isSmallPlanet(sender_id), "You have to become a small planet first");

    //4、验证当前时间是否超过当前小轮的时长，例如：当前小轮超过12小时，不支持大行星创建
    // TODO
    //5、验证邀请账户是否存在
    if(inviter != 0){
        // 4.1 验证邀请账户是否存在
        check_account(inviter);
        // 4.2 验证邀请账户不能为自己
        graphene_assert(inviter != sender_id, "StarPlan Contract Error: Can't invite yourself ");
    }
    //6、验证当前轮是否启动
    check_stat();

    //////////////////////////////////////// 校验通过后，创建一个大行星 //////////////////////////////////////////
    accounts.modify(sender_itor,_self,[&](auto &obj) {                  //修改账户属性，增加大行星属性
            obj.type                    = obj.type | startype::big;       
        });
    auto round_itor = roundstats.find(0);
    bigplanets.emplace(_self,[&](auto &obj){                            //创建一个大行星
            obj.id                      = sender_id;
            obj.create_time             = get_head_block_time();
            obj.create_round_num        = round_itor->current_round;
        });
    
}

// inviter为0，表示没有邀请账户
void starplan::uptosuper(uint64_t inviter)
{
    //////////////////////////////////////// 对调用进行校验 /////////////////////////////////////////////////
    //1、校验充值资产是否为GXC
    check_asset_id();
    //2、校验充值资产是否满足抵押20000 GXC
    uint64_t amount = get_action_asset_amount();
    graphene_assert(amount == super_stake_amount, "StarPlan Contract Error: Super planet stake amount must is 20000 GXC ");
    //3、校验该账户是否不是超级星账户，超级星账户只能成为一次
    uint64_t sender_id = get_trx_sender();
    auto itor = accounts.find(sender_id);
    if(itor != accounts.end() ){
        graphene_assert(!(itor->type & startype::super), "StarPlan Contract Error:  The Account already is super planet");
    }
    //4、验证邀请账户是否存在
    if(inviter != 0){
        // 4.1 验证邀请账户是否存在
        check_account(inviter);
        // 4.2 验证邀请账户不能为自己
        graphene_assert(inviter != sender_id, "StarPlan Contract Error: Can't invite yourself ");
    }
    //5、验证当前轮是否启动
    check_pool_stat();

    //////////////////////////////////////// 校验通过后，创建一个超级星 //////////////////////////////////////////
    // 1、创建或更新账户表
    if(itor == accounts.end()){                     //创建账户
        accounts.emplace(_self,[&](auto &obj) {
            obj.id                      = sender_id;
            obj.type                    = startype::super;       
            obj.stake_amount            = super_stake_amount;
            obj.invite_id               = inviter; 
        });
    }else{                                          //更新账户
        accounts.modify(itor,_self,[&](auto &obj) {
            obj.type                    = obj.type | startype::super;       
            obj.stake_amount            = obj.stake_amount + super_stake_amount;
        });
    }
    // 2、创建超级星表
    superplanets.emplace(_self,[&](auto &obj) {     //创建超级星
            obj.id                      = sender_id;
            obj.create_time             = startype::super;       
            obj.create_round_num        = super_stake_amount;
            obj.voted_num               = inviter; 
        });

}
void starplan::check_account(std::string acc_name)
{
    int64_t acc_id = get_account_id(data.c_str(), data.length());
    graphene_assert(acc_id != -1, "StarPlan Contract Error: Account does not exist! ");
}
void starplan::check_stat()
{
    // 1 校验底池是否已经初始化
    auto itor = tbglobals.find(0);
    graphene_assert(itor != tbglobals.end(), "StarPlan Contract Error:  The bonus pool has not been initialized! ");
    
    // 2 校验当前轮资金池是否已经初始化
    auto itor2 = tbrounds.find(0);
    graphene_assert(itor2 != tbrounds.end(), "StarPlan Contract Error:  The Current bonus pool has not been initialized! ");
}
bool starplan::isInviter(std::string accname)
{
    bool retValue = false;
    if(accname != ""){
        // 1 验证邀请账户是否存在
        check_account(accname);
        uint64_t inviter_id = get_account_id(inviter.c_str(), inviter.length());
        // 2 验证邀请账户是否为大行星或者超级星
        auto big_idx = tbbigplanets.get_index<N(byaccid)>
        auto big_itor = big_idx.find(inviter_id);
        auto sup_idx = tbsuperstars.get_index<N(byaccid)>
        auto sup_itor = sup_idx.find(inviter_id);
        if(big_itor != tbbigplanets.end() || sup_itor != tbsuperstars.end())
            retValue = true;
    }else
    {
        retValue = true;
    }
    return retValue;
}
bool starplan::isSuperStar(uint64_t sender)
{
    bool retValue = false;
    check_account(sender);
    uint64_t sender_id = get_account_id(sender.c_str(), sender.length());
    auto sup_idx = tbsuperstars.get_index<N(byaccid)>
    auto sup_itor = sup_idx.find(inviter_id);
    if(sup_itor != tbsuperstars.end()) { retValue = true; }
    return retValue;
}
bool starplan::isSmallPlanet(uint64_t sender)
{
    bool retValue = false;
    auto small_idx = tbsmallplans.get_index<N(byaccid)>
    auto small_itor = small_idx.find(sender_id);
    if(small_itor != tbsmallplans.end()) { retValue = true; }
    return retValue;
}
bool hasInvited(uint64_t original_sender,std:string inviter)
{
    bool retValue = false;
    auto invite_idx = tbinvites.get_index<N(byaccid)>
    auto invite_itor = invite_idx.find(original_sender);
    uint64_t inviter_id = get_account_id(inviter.c_str(), inviter.length());
    if(invite_itor != invite_idx.end()) { retValue = true; }
    return retValue;
}