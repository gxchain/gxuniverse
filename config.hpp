/*
const uint64_t      adminId              = 426;                      //admin账户id
const uint64_t      superStarLimit       = 50;                       //超级星最大数量（50）
const uint64_t      bigRoundSize         = 50;                       //一个大轮包含小轮数（50）
const uint64_t      roundAmount          = 2000;                     //每一小轮的底池资产数（2000GXC）
const uint64_t      roundSize            = 100;                      //每一轮的参与人数（100）
const uint64_t      x                    = 20000;                    //成为超级星需要抵押的资产数（20000GXC）
const uint64_t      y                    = 100;                      //成为小行星需要抵押的资产数（100GXC）
const uint64_t      z                    = 110;                      //自激活需要抵押的资产数（110GXC）
const uint64_t      z1                   = 1;                        //大行星投入每轮平均激励池资产数（1GXC）
const uint64_t      z2                   = 1;                        //大行星奖励给邀请人的资产数（1GXC）
const uint64_t      z3                   = 1;                        //大行星投入每轮随机奖励池资产数（1GXC）
const uint64_t      decayTime            = 4 * 3600;                 //衰减时间阈值，单位秒（4*3600s）
const uint64_t      decayDur             = 1 * 3600;                 //衰减时间间隔，单位秒（1*3600s）
const uint64_t      maxDecayCount        = 20;                       //最大衰减次数（20）
const uint64_t      payBackPercent       = 10;                       //返现比例（10%）
const uint64_t      activePercent        = 50;                       //活力星瓜分比例（50%），剩余40%为超级星瓜分比例
const float         a                    = 1;                        //超级星奖励的影响因子（1）
const float         bDecay               = 0.85;                     //活力星奖励的影响因子（0.85）

const uint64_t      initPool             = 2000000;                  //初始化充值200万GXC
const uint64_t      coreAsset            = 1;                        //核心资产id
const uint64_t      precision            = 100000;                   //核心资产精度
const uint64_t      stakingDelayTime     = 90 * 24 * 3600;           //抵押90天后解锁
const uint64_t      weight               = 1000;                     //权重，带三位精度
const uint64_t      delaytime            = 12 * 3600;                //最后一个大行星的延迟时间（12小时）
const uint64_t      defaultinviter       = 0;                        //默认邀请账户id
*/
const uint64_t      adminId              = 426;                      //admin账户id
const uint64_t      superStarLimit       = 5;                        //超级星最大数量（50）
const uint64_t      bigRoundSize         = 5;                        //一个大轮包含小轮数（50）
const uint64_t      roundAmount          = 20;                       //每一小轮的底池资产数（2000GXC）
const uint64_t      roundSize            = 10;                       //每一轮的参与人数（10）
const uint64_t      x                    = 20;                       //成为超级星需要抵押的资产数（20GXC）
const uint64_t      y                    = 10;                       //成为小行星需要抵押的资产数（10GXC）
const uint64_t      z                    = 11;                       //自激活需要抵押的资产数（11GXC）
const uint64_t      z1                   = 1;                        //大行星投入每轮平均激励池资产数（1GXC）
const uint64_t      z2                   = 1;                        //大行星奖励给邀请人的资产数（1GXC）
const uint64_t      z3                   = 1;                        //大行星投入每轮随机奖励池资产数（1GXC）
const uint64_t      decayTime            = 4 * 60;                   //衰减时间阈值，单位秒（4*3600s）
const uint64_t      decayDur             = 1 * 60;                   //衰减时间间隔，单位秒（1*3600s）
const uint64_t      maxDecayCount        = 20;                       //最大衰减次数（20）
const uint64_t      payBackPercent       = 10;                       //返现比例（10%）
const uint64_t      activePercent        = 50;                       //活力星瓜分比例（50%），剩余40%为超级星瓜分比例
const float         a                    = 1;                        //超级星奖励的影响因子（1）
const float         bDecay               = 0.85;                     //活力星奖励的影响因子（0.85）

const uint64_t      initPool             = 200;                      //初始化充值200万GXC
const uint64_t      coreAsset            = 1;                        //核心资产id
const uint64_t      precision            = 100000;                   //核心资产精度
const uint64_t      stakingDelayTime     = 1800;                     //抵押90天后解锁
const uint64_t      weight               = 1000;                     //权重，带三位精度
const uint64_t      delaytime            = 2 * 3600;                 //最后一个大行星的延迟时间（12小时）
const uint64_t      defaultinviter       = 0;                        //默认邀请账户id

const uint64_t      MAX_ROUND_REWARD    = (roundAmount + roundSize * z1) * precision;
const uint64_t      MAX_USER_REWARD     = MAX_ROUND_REWARD * activePercent / 100 * 1;//只有一个活力星的时候

#define STAKE_TYPE_TOSUPER  0
#define STAKE_TYPE_VOTE     1

#define RWD_TYPE_RANDOM     0
#define RWD_TYPE_POOL       1
#define RWD_TYPE_ACTIVE     2
#define RWD_TYPE_SUPER      3
#define RWD_TYPE_TIMEOUT    4
const char* const reward_reasons[5] = {
        "RWD_TYPE_RANDOM",
        "RWD_TYPE_POOL",
        "RWD_TYPE_ACTIVE",
        "RWD_TYPE_SUPER",
        "RWD_TYPE_TIMEOUT"
};

