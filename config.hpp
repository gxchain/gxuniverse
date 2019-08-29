//////////////////////// common ////////////////////////
const uint64_t      CORE_ASSET_ID           = 1;                        //核心资产id
const uint64_t      PRECISION               = 100000;                   //核心资产精度

const uint64_t      Z1                      = 1 * PRECISION;            //大行星投入每轮平均激励池资产数（1GXC）
const uint64_t      Z2                      = 1 * PRECISION;            //大行星奖励给邀请人的资产数（1GXC）
const uint64_t      Z3                      = 1 * PRECISION;            //大行星投入每轮随机奖励池资产数（1GXC）

const uint64_t      PAYBACK_PERCENT         = 10;                       //返现比例（10%）
const uint64_t      ACTIVE_PERCENT          = 50;                       //活力星瓜分比例（50%），剩余40%为超级星瓜分比例
const uint64_t      WEIGHT                  = 1000;                     //权重，带三位精度
const uint64_t      B_DECAY_PERCENT         = 85;                       //活力星奖励的影响因子（85%）
const uint64_t      MIN_VOTE_AMOUNT         = 10 * PRECISION;
const uint64_t      MAX_MEMO_LENGTH         = 15;
const uint64_t      ACTIVE_PROMOT_INVITES   = 5;
const uint64_t      RANDOM_COUNT            = 10;
const uint64_t      COUNT_OF_TRAVERSAL_PER  = 30;

//////////////////////// for test ////////////////////////
const uint64_t      ADMIN_ID                = 426;                      //admin账户id
const uint64_t      DEFAULT_INVITER         = 0;                        //默认邀请账户id//TODO blockcity account id
const uint64_t      SUPER_STAR_LIMIT        = 5;                        //超级星最大数量（50）
const uint64_t      BIG_ROUND_SIZE          = 5;                        //一个大轮包含小轮数（50）
const uint64_t      ROUND_AMOUNT            = 40 * PRECISION;           //每一小轮的底池资产数（20GXC）
const uint64_t      ROUND_SIZE              = 5;                       //每一轮的参与人数（10）
const uint64_t      X                       = 20 * PRECISION;           //成为超级星需要抵押的资产数（20GXC）
const uint64_t      Y                       = 10 * PRECISION;           //成为小行星需要抵押的资产数（10GXC）
const uint64_t      Z                       = 11 * PRECISION;           //自激活需要抵押的资产数（11GXC）
const uint64_t      DECAY_TIME              = 4 * 60;                   //衰减时间阈值，单位秒（4*3600s）
const uint64_t      DECAY_DURATION          = 1 * 60;                   //衰减时间间隔，单位秒（1*3600s）
const uint64_t      INIT_POOL               = 400 * PRECISION;          //初始化充值200GXC
const uint64_t      STAKING_DURATION_TIME   = 1800;                     //抵押30分钟后解锁
const uint64_t      DELAY_TIME              = 2 * 3600;                 //最后一个大行星的延迟时间（2小时）
const uint64_t      MAX_DECAY_COUNT         = 5;                        //最大衰减次数（5）
const uint64_t      REWARD_DELAY_TIME       = 60;                       //延迟发奖时间1min

//////////////////////// for release ////////////////////////
/*
const uint64_t      ADMIN_ID                = 426;                      //admin账户id
const uint64_t      DEFAULT_INVITER         = 0;                        //默认邀请账户id//TODO blockcity account id
const uint64_t      SUPER_STAR_LIMIT        = 50;                       //超级星最大数量（50）
const uint64_t      BIG_ROUND_SIZE          = 50;                       //一个大轮包含小轮数（50）
const uint64_t      ROUND_AMOUNT            = 2000 * PRECISION;         //每一小轮的底池资产数（20GXC）
const uint64_t      ROUND_SIZE              = 100;                      //每一轮的参与人数（10）
const uint64_t      X                       = 200 * PRECISION;          //成为超级星需要抵押的资产数（20GXC）
const uint64_t      Y                       = 100 * PRECISION;          //成为小行星需要抵押的资产数（10GXC）
const uint64_t      Z                       = 110 * PRECISION;          //自激活需要抵押的资产数（11GXC）
const uint64_t      DECAY_TIME              = 4 * 3600;                 //衰减时间阈值，单位秒（4*3600s）
const uint64_t      DECAY_DURATION          = 1 * 3600;                 //衰减时间间隔，单位秒（1*3600s）
const uint64_t      INIT_POOL               = 2000000 * PRECISION;      //初始化充值200万GXC
const uint64_t      STAKING_DELAY_TIME      = 90 * 24 * 3600;           //抵押90天后解锁
const uint64_t      DELAY_TIME              = 12 * 3600;                //最后一个大行星的延迟时间（12小时）
const uint64_t      MAX_DECAY_COUNT         = 20;                       //最大衰减次数（20）
const uint64_t      REWARD_DELAY_TIME       = 60 * 20;                  //延迟发奖时间20min
*/

//////////////////////// common ////////////////////////
const uint64_t      MAX_ROUND_REWARD        = ROUND_AMOUNT + ROUND_SIZE * Z1;
const uint64_t      MAX_USER_REWARD         = MAX_ROUND_REWARD * ACTIVE_PERCENT / 100 * 1;//只有一个活力星的时候
const uint64_t      MAX_VOTE_CLAIM_COUNT    = 20000 * PRECISION;

#define STAKING_TYPE_TO_SUPER       0
#define STAKING_TYPE_VOTE           1
#define STAKING_TYPE_SELF_ACTIVATE  2

#define RWD_TYPE_RANDOM             0
#define RWD_TYPE_BIG                1
#define RWD_TYPE_ACTIVE             2
#define RWD_TYPE_SUPER              3
#define RWD_TYPE_TIMEOUT            4
#define RWD_TYPE_INVITE             5
#define RWD_TYPE_SELF_ACTIVATE      6
const char* const reward_reasons[7] = {
        "reward current round random pool to big planet",
        "reward current round base pool to big planet",
        "reward current round base pool to active planet",
        "reward current round base pool to super star",
        "reward current round base pool to last big planet for invite timeout",
        "",
        ""
};

