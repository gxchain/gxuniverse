#include <graphenelib/graphene.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/types.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/crypto.h>
#include <graphenelib/asset.h>

using namespace graphene;

const uint64_t      adminId              = 100;                      //admin账户id
const uint64_t      superStarLimit       = 50;                       //超级星最大数量（50）
const uint64_t      bigRoundSize         = 50;                       //一个大轮包含小轮数（50）                     
const uint64_t      roundAmount          = 2000;                     //每一小轮的底池资产数（2000GXC）                      
const uint64_t      roundSize            = 100;                      //每一轮的参与人数（100）
const uint64_t      x                    = 20000;                    //成为超级星需要抵押的资产数（20000GXC）
const uint64_t      y                    = 100;                      //成为小行星需要抵押的资产数（100GXC）
const uint64_t      z1                   = 1;                        //大行星投入每轮平均激励池资产数（1GXC）
const uint64_t      z2                   = 1;                        //大行星奖励给邀请人的资产数（1GXC）
const uint64_t      z3                   = 1;                        //大行星投入每轮随机奖励池资产数（1GXC）
const uint64_t      decayTime            = 4 * 3600;                 //衰减时间阈值，单位秒（4*3600s）
const uint64_t      decayDur             = 1 * 3600;                 //衰减时间间隔，单位秒（1*3600s）
const uint64_t      maxDecayCount        = 20;                       //最大衰减次数（20）
const float         payBackPercent       = 0.1;                      //返现比例（0.1）   
const float         activePercent        = 0.5;                      //活力星瓜分比例（0.5），剩余0.4为超级星瓜分比例      
const float         a                    = 1;                        //超级星奖励的影响因子（1）
const float         bDecay               = 0.85;                     //活力星奖励的影响因子（0.85）
const uint64_t      initPool             = 2000000;                  //初始化充值200万GXC
const uint64_t      coreAsset            = 1;                        //核心资产id
const uint64_t      precision            = 100000;                   //核心资产精度
const uint64_t      delayDay             = 90 * 24 * 3600;           //抵押90天后解锁
const uint64_t      depositToBig         = 3;                        //升级成大行星充值3GXC
const uint64_t      weight               = 1000;                     //权重，带三位精度
const uint64_t      delaytime            = 12 * 3600;                //最后一个大行星的延迟时间（12小时）

class starplan : public contract
{
  public:
    starplan(uint64_t id)
        : contract(id),tbglobals(_self,_self),tbrounds(_self,_self),tbvotes(_self,_self),tbstakes(_self,_self),tbsmallplans(_self,_self),tbbigplanets(_self,_self)\
            ,tbactiveplans(_self,_self),tbsuperstars(_self,_self),tbinvites(_self,_self){}
    //@abi action
    //@abi payable
    void     init();
    //@abi action
    //@abi payable
    void     uptosmall(std::string inviter,std::string superstar);
    //@abi action
    //@abi payable
    void     uptobig(std::string inviter);
    //@abi action
    //@abi payable
    void     uptosuper(std::string inviter);
    //@abi action
    void     endround();
    //@abi action
    void     unstake(std::string account);

  private:     

    void        invite(uint64_t original_sender,std::string inviter); 
    void        actinvite(uint64_t original_sender);                           //激活邀请关系
    void        vote(uint64_t original_sender,std::string superstar);
    bool        isSuperStar(uint64_t sender);
    bool        addSuperStar(uint64_t sender);
    bool        isSmallPlanet(uint64_t sender);
    bool        addSmallPlanet(uint64_t sender);
    bool        isBigPlanet(uint64_t sender);
    bool        addBigPlanet(uint64_t sender);
    uint32_t    currentRound(); 
    bool        bSmallRound();
    void        endSmallRound();

    bool        isInviter(std::string accname);
    bool        isAccount(std::string accname);
    bool        isInit();
    bool        hasInvited(uint64_t original_sender,std::string inviter);
    void        addStake(uint64_t sender,uint64_t amount);
    void        sendInviteReward(uint64_t sender);
    void        updateActivePlanetsbybig(uint64_t sender);
    void        updateActivePlanetsbysuper(uint64_t sender);
    void        calcCurrentRoundPoolAmount();
    void        updateActivePlanets();
    void        randomReward(); 
    void        rewardBigPlanet();
    void        rewardActivePlanet();
    void        rewardSuperStar();
    void        createnewround();

  private:
    //@abi table tbglobal i64
    struct tbglobal {                       // 全局状态表
        uint64_t index;
        uint64_t pool_amount;               // 总资金池剩余资产
        uint64_t current_round;	            // 当前轮数

        uint64_t primary_key() const { return index; }

        GRAPHENE_SERIALIZE(tbglobal, (index)(pool_amount)(current_round))
    };
    typedef multi_index<N(tbglobal), tbglobal> tbglobal_index;
    tbglobal_index tbglobals;

    //@abi table tbround i64
    struct tbround {                      
        uint64_t round;                     // 索引
        uint64_t current_round_invites;     // 当前轮完成邀请数
        uint64_t pool_amount;               // 当前轮奖池资产数
        uint64_t random_pool_amount;        // 当前随机池资产数
        uint64_t invite_pool_amount;        // 当前邀请奖励池资产数
        uint64_t start_time;               	// 当前轮的启动时间
        uint64_t end_time;               	// 当前轮的结束时间

        uint64_t primary_key() const { return round; }

        GRAPHENE_SERIALIZE(tbround, (round)(current_round_invites)(pool_amount)(random_pool_amount)(invite_pool_amount)(start_time)(end_time))
    };
    typedef multi_index<N(tbround), tbround> tbround_index;
    tbround_index tbrounds;

    //@abi table tbvote i64
    struct tbvote {
        uint64_t index;					    // 自增索引
        uint64_t round;					    // 当前轮数
        uint64_t stake_amount;	            // 抵押GXC数量
        uint64_t from;					    // 投票者id
        uint64_t to;						// 被投票者id
        uint64_t vote_time;			        // 投票时间

        uint64_t primary_key() const { return index; }
        uint64_t by_vote_from() const { return from; }
        uint64_t by_vote_to() const { return to; }
        uint64_t by_round() const { return round;}

        GRAPHENE_SERIALIZE(tbvote, (index)(round)(stake_amount)(from)(to)(vote_time))
    };
    typedef multi_index<N(tbvote), tbvote,
                        indexed_by<N(byfrom), const_mem_fun<tbvote, uint64_t, &tbvote::by_vote_from>>,
                        indexed_by<N(byto), const_mem_fun<tbvote, uint64_t, &tbvote::by_vote_to>>,
                        indexed_by<N(byround), const_mem_fun<tbvote, uint64_t, &tbvote::by_round>>> tbvote_index;
    tbvote_index tbvotes;

    //@abi table tbstake i64
    struct tbstake {
        uint64_t index;                     // 索引
        uint64_t account;                   // account id
        uint64_t amount;                    // 抵押数量
        uint64_t end_time;                  // 抵押时间

        uint64_t primary_key() const { return index; }
        uint64_t by_acc_id() const { return account; }

        GRAPHENE_SERIALIZE(tbstake, (index)(account)(amount)(end_time))
    };
    typedef multi_index<N(tbstake), tbstake,
                        indexed_by<N(byaccid), const_mem_fun<tbstake, uint64_t, &tbstake::by_acc_id>>> tbstake_index;
    tbstake_index tbstakes;

    //@abi table tbsmallplan i64
    struct tbsmallplan {
        uint64_t index;                     // 索引
        uint64_t id;                        // 账户id
        uint64_t create_time;               // 创建时间
        uint64_t create_round;              // 晋升轮数（第几轮晋升）

        uint64_t primary_key() const { return index; }
        uint64_t by_acc_id() const { return id; }
        uint64_t by_create_round() const { return create_round; }

        GRAPHENE_SERIALIZE(tbsmallplan, (index)(id)(create_time)(create_round))
    };
    typedef multi_index<N(tbsmallplan), tbsmallplan,
                        indexed_by<N(byaccid), const_mem_fun<tbsmallplan, uint64_t, &tbsmallplan::by_acc_id>>,
                        indexed_by<N(byround), const_mem_fun<tbsmallplan, uint64_t, &tbsmallplan::by_create_round>>> tbsmallplan_index;
    tbsmallplan_index tbsmallplans;

    //@abi table tbbigplanet i64
    struct tbbigplanet {
        uint64_t index;                     // 索引
        uint64_t id;                        // 账户id
        uint64_t create_time;               // 创建时间
        uint64_t create_round;              // 晋升轮数（第几轮晋升）

        uint64_t primary_key() const { return index; }
        uint64_t by_acc_id() const { return id; }
        uint64_t by_create_round() const { return create_round; }

        GRAPHENE_SERIALIZE(tbbigplanet, (index)(id)(create_time)(create_round))
    };
    typedef multi_index<N(tbbigplanet), tbbigplanet,
                        indexed_by<N(byaccid), const_mem_fun<tbbigplanet, uint64_t, &tbbigplanet::by_acc_id>>,
                        indexed_by<N(byround), const_mem_fun<tbbigplanet, uint64_t, &tbbigplanet::by_create_round>>> tbbigplanet_index;
    tbbigplanet_index tbbigplanets;

    //@abi table tbactiveplan i64
    struct tbactiveplan {
        uint64_t index;                     // 索引
        uint64_t id;                        // 账户id
        uint64_t invite_count;              // 邀请人数，每达到5个大行星，置为0，记录create_round=当前轮，weight=1
        uint64_t create_time;               // 创建时间 
        uint64_t create_round;              // 晋升轮数（第几轮晋升）
        uint64_t weight;                    // 权重，每小轮0.85的幅度衰减，衰减为0，重新计算

        uint64_t primary_key() const { return index; }
        uint64_t by_acc_id() const { return id; }
        uint64_t by_create_round() const { return create_round; }
        uint64_t by_weight() const { return weight; }

        GRAPHENE_SERIALIZE(tbactiveplan, (index)(id)(invite_count)(create_time)(create_round)(weight))
    };
    typedef multi_index<N(tbactiveplan), tbactiveplan,
                        indexed_by<N(byaccid), const_mem_fun<tbactiveplan, uint64_t, &tbactiveplan::by_acc_id>>,
                        indexed_by<N(byround), const_mem_fun<tbactiveplan, uint64_t, &tbactiveplan::by_create_round>>,
                        indexed_by<N(byweight), const_mem_fun<tbactiveplan, uint64_t, &tbactiveplan::by_weight>>> tbactiveplan_index;
    tbactiveplan_index tbactiveplans;

    //@abi table tbsuperstar i64
    struct tbsuperstar {
        uint64_t index;                     // 索引
        uint64_t id;                        // 账户id
        uint64_t create_time;               // 创建时间
        uint64_t create_round;              // 晋升轮数（第几轮晋升）
        uint64_t vote_num;                  // 得票数

        uint64_t primary_key() const { return index; }
        uint64_t by_acc_id() const { return id; }
        uint64_t by_create_round() const { return create_round; }
        uint64_t by_vote_num() const { return vote_num; }

        GRAPHENE_SERIALIZE(tbsuperstar, (index)(id)(create_time)(create_round)(vote_num))
    };
    typedef multi_index<N(tbsuperstar), tbsuperstar,
                        indexed_by<N(byaccid), const_mem_fun<tbsuperstar, uint64_t, &tbsuperstar::by_acc_id>>,
                        indexed_by<N(byround), const_mem_fun<tbsuperstar, uint64_t, &tbsuperstar::by_create_round>>,
                        indexed_by<N(byvote), const_mem_fun<tbsuperstar, uint64_t, &tbsuperstar::by_vote_num>>> tbsuperstar_index;
    tbsuperstar_index tbsuperstars;

    //@abi table tbinvite i64
    struct tbinvite {
        uint64_t index;						// 自增索引
        uint64_t invitee;				    // 被邀请账户
        uint64_t inviter;					// 邀请账户
        bool     enabled;                   // 邀请关系是否⽣生效(invitee是否升级为⼤行星)
        uint64_t create_round;			    // 当前轮数
        uint64_t create_time;				// 邀请时间

        uint64_t primary_key() const { return index; }
        uint64_t by_acc_id() const { return invitee; }
        uint64_t by_invite_id() const { return inviter; }
        uint64_t by_round() const { return create_round; }
        uint64_t by_enable() const { return enabled; }

        GRAPHENE_SERIALIZE(tbinvite, (index)(invitee)(inviter)(enabled)(create_round)(create_time))
    };
    typedef multi_index<N(tbinvite), tbinvite,
                        indexed_by<N(byaccid), const_mem_fun<tbinvite, uint64_t, &tbinvite::by_acc_id>>,
                        indexed_by<N(byinviteid), const_mem_fun<tbinvite, uint64_t, &tbinvite::by_invite_id>>,
                        indexed_by<N(byenable), const_mem_fun<tbinvite, uint64_t, &tbinvite::by_enable>>,
                        indexed_by<N(byround), const_mem_fun<tbinvite, uint64_t, &tbinvite::by_round>>> tbinvite_index;
    tbinvite_index tbinvites;
};
GRAPHENE_ABI(starplan, (init)(uptosmall)(uptobig)(uptosuper)(endround)(unstake))
