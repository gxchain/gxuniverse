#include <graphenelib/graphene.hpp>
#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/types.h>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/crypto.h>
#include <graphenelib/asset.h>
#include "message.hpp"
#include "config.hpp"

using namespace graphene;

struct reward {
    uint64_t to;
    uint64_t amount;
    uint64_t type;
};

struct ActivePlanet {
    uint64_t id;
    uint64_t weight;
};

struct SuperStar {
    uint64_t id;
    uint64_t vote;
    bool end = false;
};

class starplan : public contract
{
  public:
    starplan(uint64_t id) :
            contract(id),
            tbglobals(_self, _self),
            tbrounds(_self, _self),
            tbvotes(_self, _self),
            tbstakes(_self,_self),
            tbsmallplans(_self, _self),
            tbbigplanets(_self, _self),
            tbactiveplans(_self, _self),
            tbsuperstars(_self, _self),
            tbinvites(_self, _self),
            tbrewards(_self,_self)
    {
    }

    PAYABLE             init();
    PAYABLE             vote(std::string inviter,std::string superstar);
    PAYABLE             selfactivate(std::string superstar);
    PAYABLE             uptobig();
    PAYABLE             uptosuper(std::string inviter,std::string memo);
    ACTION              endround();
    ACTION              claim(std::string account);
    ACTION              upgrade(uint64_t flag);
    ACTION              updatememo(std::string memo);

  private:

    void                invite(uint64_t sender, uint64_t inviter);
    inline void         activateInvite(uint64_t sender);                           //激活邀请关系
    uint64_t            createVote(uint64_t sender,std::string superstar, uint64_t voteCount);
    bool                isSuperStar(uint64_t sender);
    bool                addSuperStar(uint64_t sender,std::string memo);
    bool                isSmallPlanet(uint64_t sender);
    bool                addSmallPlanet(uint64_t sender);
    bool                isBigPlanet(uint64_t sender);
    bool                addBigPlanet(uint64_t sender);
    uint64_t            currentRound();
    inline bool         isInviteTimeout(uint64_t &lastBigPlanet);//>12 hours
    inline bool         isRoundFull();//>=100 inviatees
    inline bool         isRoundFinish();

    bool                isInviter(std::string accname);
    bool                isInit();
    bool                hasInvited(uint64_t sender);
    void                addStaking(uint64_t sender,uint64_t amount,uint64_t to,uint64_t reason,uint64_t index=0);
    inline uint64_t     getInviter(uint64_t invitee);
    void                distributeInviteRewards(uint64_t invitee, uint64_t rewardAccountId, uint64_t rewardType);
    void                updateActivePlanet(uint64_t activePlanetAccountId,uint64_t subAccountId);
    void                updateActivePlanetForSuper(uint64_t activePlanetAccountId);
    void                calcCurrentRoundPoolAmount();
    void                decayActivePlanetWeight();

    void                getCurrentRoundBigPlanets(vector<uint64_t> &bigPlanets);
    uint64_t            getCurrentRoundActivePlanets(vector<ActivePlanet> &activePlanets);
    uint64_t            getCurrentRoundSuperStars(vector<SuperStar> &superStars);
    void                chooseBigPlanet(const vector<uint64_t> &bigPlanets, vector<uint64_t> &choosed);

    void                getBudgets(uint64_t &randomRewardBudget, uint64_t &bigPlanetRewardBudget,
                                   uint64_t &activePlanetRewardBudget, uint64_t &superStarRewardBudget);
    inline uint64_t     calcRandomReward(vector<reward> &rewardList, uint64_t rewardBudget);
    inline uint64_t     calcBigPlanetReward(vector<reward> &rewardList, uint64_t rewardBudget);
    inline uint64_t     calcActivePlanetReward(vector<reward> &rewardList, uint64_t rewardBudget);
    inline uint64_t     calcSuperStarReward(vector<reward> &rewardList, uint64_t rewardBudget);

    bool                baseSecureCheck(vector<reward> &rewardList);
    void                doReward(vector<reward> &rewardList);

    void                createNewRound();
    bool                canUpdateSmall(uint64_t sender);

    inline uint64_t     checkSender();                                                  //验证调用者和原始调用者是否相同
    bool                isUpgrading();                                                    //验证合约状态升级
    void                cancelVote(uint64_t voteIndex,uint64_t superAccId,uint64_t amount);
    void                cancelSuperStake(uint64_t superAccId);

    inline void         baseCheck();
    inline void         roundFinishCheck();
    inline uint64_t     assetEqualCheck(uint64_t expectedAmount, const char* errMsg);
    inline uint64_t     assetLargerCheck(uint64_t expectedAmount, const char* errMsg);
    inline uint64_t     inviterCheck(const std::string &inviter, uint64_t inviteeId);
    inline uint64_t     superStarCheck(const std::string &superStarAccount);
    inline void         progress(uint64_t ramPayer);

  private:
    //@abi table tbglobal i64
    struct tbglobal {                       // 全局状态表
        uint64_t index;
        uint64_t pool_amount;               // 总资金池剩余资产
        uint64_t current_round;             // 当前轮数
        uint64_t is_upgrade;                // 合约升级

        uint64_t primary_key() const { return index; }

        GRAPHENE_SERIALIZE(tbglobal, (index)(pool_amount)(current_round)(is_upgrade))
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
        uint64_t start_time;                // 当前轮的启动时间
        uint64_t end_time;                  // 当前轮的结束时间

        uint64_t primary_key() const { return round; }

        GRAPHENE_SERIALIZE(tbround, (round)(current_round_invites)(pool_amount)(random_pool_amount)(invite_pool_amount)(start_time)(end_time))
    };
    typedef multi_index<N(tbround), tbround> tbround_index;
    tbround_index tbrounds;

    //@abi table tbvote i64
    struct tbvote {
        uint64_t index;                     // 自增索引
        uint64_t round;                     // 当前轮数
        uint64_t staking_amount;            // 抵押GXC数量
        uint64_t from;                      // 投票者id
        uint64_t to;                        // 被投票者id
        uint64_t vote_time;                 // 投票时间
        uint64_t disabled;                  // 是否撤销投票

        uint64_t primary_key() const { return index; }
        uint64_t by_vote_from() const { return from; }
        uint64_t by_vote_to() const { return to; }
        uint64_t by_round() const { return round;}

        GRAPHENE_SERIALIZE(tbvote, (index)(round)(staking_amount)(from)(to)(vote_time)(disabled))

    };
    typedef multi_index<N(tbvote), tbvote,
                        indexed_by<N(byfrom), const_mem_fun<tbvote, uint64_t, &tbvote::by_vote_from>>,
                        indexed_by<N(byto), const_mem_fun<tbvote, uint64_t, &tbvote::by_vote_to>>,
                        indexed_by<N(byround), const_mem_fun<tbvote, uint64_t, &tbvote::by_round>>> tbvote_index;
    tbvote_index tbvotes;

    //@abi table tbstaking i64
    struct tbstaking {
        uint64_t index;                     // 索引
        uint64_t account;                   // account id
        uint64_t amount;                    // 抵押数量
        uint64_t end_time;                  // 抵押时间
        uint64_t staking_to;                // 为哪个账户抵押（小行星投票给超级星 / 超级星升级）
        uint64_t stake_type;                // 抵押类型

        uint64_t claimed;                   // 是否解除抵押
        uint64_t claim_time;                // 解除抵押的时间
        uint64_t vote_index;                // 记录对应投票表项id

        uint64_t primary_key() const { return index; }
        uint64_t by_acc_id() const { return account; }

        GRAPHENE_SERIALIZE(tbstaking, (index)(account)(amount)(end_time)(staking_to)(stake_type)(claimed)(claim_time)(vote_index))
    };
    typedef multi_index<N(tbstaking), tbstaking,
                        indexed_by<N(byaccid), const_mem_fun<tbstaking, uint64_t, &tbstaking::by_acc_id>>> tbstaking_index;
    tbstaking_index tbstakes;

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
        std::vector<uint64_t> invite_list;  // 邀请人数，每达到5个大行星，置为0，记录create_round=当前轮，weight=1
        uint64_t create_time;               // 创建时间
        uint64_t create_round;              // 晋升轮数（第几轮晋升）
        uint64_t weight;                    // 权重，每小轮0.85的幅度衰减，衰减为0，重新计算

        uint64_t primary_key() const { return index; }
        uint64_t by_acc_id() const { return id; }
        uint64_t by_create_round() const { return create_round; }
        uint64_t by_weight() const { return weight; }

        GRAPHENE_SERIALIZE(tbactiveplan, (index)(id)(invite_list)(create_time)(create_round)(weight))
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
        uint64_t disabled;                  // 是否已经撤销抵押
        std::string memo;                   // 超级星memo

        uint64_t primary_key() const { return index; }
        uint64_t by_acc_id() const { return id; }
        uint64_t by_create_round() const { return create_round; }
        uint64_t by_vote_num() const { return vote_num; }

        GRAPHENE_SERIALIZE(tbsuperstar, (index)(id)(create_time)(create_round)(vote_num)(disabled)(memo))
    };
    typedef multi_index<N(tbsuperstar), tbsuperstar,
                        indexed_by<N(byaccid), const_mem_fun<tbsuperstar, uint64_t, &tbsuperstar::by_acc_id>>,
                        indexed_by<N(byround), const_mem_fun<tbsuperstar, uint64_t, &tbsuperstar::by_create_round>>,
                        indexed_by<N(byvote), const_mem_fun<tbsuperstar, uint64_t, &tbsuperstar::by_vote_num>>> tbsuperstar_index;
    tbsuperstar_index tbsuperstars;

    //@abi table tbinvite i64
    struct tbinvite {
        uint64_t index;                     // 自增索引
        uint64_t invitee;                   // 被邀请账户
        uint64_t inviter;                   // 邀请账户
        bool     enabled;                   // 邀请关系是否⽣生效(invitee是否升级为⼤行星)
        uint64_t create_round;              // 当前轮数
        uint64_t create_time;               // 邀请时间

        uint64_t primary_key() const { return index; }
        uint64_t by_invitee() const { return invitee; }
        uint64_t by_inviter() const { return inviter; }
        uint64_t by_round() const { return create_round; }
        uint64_t by_enable() const { return enabled; }

        GRAPHENE_SERIALIZE(tbinvite, (index)(invitee)(inviter)(enabled)(create_round)(create_time))
    };
    typedef multi_index<N(tbinvite), tbinvite,
                        indexed_by<N(byinvitee), const_mem_fun<tbinvite, uint64_t, &tbinvite::by_invitee>>,
                        indexed_by<N(byinviter), const_mem_fun<tbinvite, uint64_t, &tbinvite::by_inviter>>,
                        indexed_by<N(byenable), const_mem_fun<tbinvite, uint64_t, &tbinvite::by_enable>>,
                        indexed_by<N(byround), const_mem_fun<tbinvite, uint64_t, &tbinvite::by_round>>> tbinvite_index;
    tbinvite_index tbinvites;

    //@abi table tbreward i64
    struct tbreward {
        uint64_t index;                     // 自增索引
        uint64_t round;                     // 小轮数
        uint64_t from;                      // 奖励来源账户
        uint64_t to;                        // 奖励去向账户
        uint64_t amount;                    // 奖励金额
        uint64_t type;                      // 奖励类型

        uint64_t primary_key() const { return index; }
        uint64_t by_round() const { return round; }
        uint64_t by_acc_id() const { return to; }

        GRAPHENE_SERIALIZE(tbreward, (index)(round)(from)(to)(amount)(type))
    };
    typedef multi_index<N(tbreward), tbreward,
                        indexed_by<N(byaccid), const_mem_fun<tbreward, uint64_t, &tbreward::by_round>>,
                        indexed_by<N(byinviteid), const_mem_fun<tbreward, uint64_t, &tbreward::by_acc_id>>> tbreward_index;
    tbreward_index tbrewards;

    inline const struct starplan::tbround& lastRound();
};
GRAPHENE_ABI(starplan, (init)(vote)(selfactivate)(uptobig)(uptosuper)(endround)(claim)(upgrade)(updatememo))
