# Star Program

系统角色：

- 超级星（抵押20,000GXC）
- 小行星（抵押100GXC）
- 活力星（小行星邀请超过5人）
- 大行星（小行星消耗3GXC）

## 静态变量（Static Parameters）

静态变量在合约初始化时定义，会在合约中参与计算

```c++
const uint64_t      CORE_ASSET_ID           = 1;                        //核心资产id
const uint64_t      PRECISION               = 100000;                   //核心资产精度

const uint64_t      Z1                      = 1 * PRECISION;            //大行星投入每轮平均激励池资产数（1GXC）
const uint64_t      Z2                      = 1 * PRECISION;            //大行星奖励给邀请人的资产数（1GXC）
const uint64_t      Z3                      = 1 * PRECISION;            //大行星投入每轮随机奖励池资产数（1GXC）

const uint64_t      MAX_DECAY_COUNT         = 20;                       //最大衰减次数（20）
const uint64_t      PAYBACK_PERCENT         = 10;                       //返现比例（10%）
const uint64_t      ACTIVE_PERCENT          = 50;                       //活力星瓜分比例（50%），剩余40%为超级星瓜分比例
const uint64_t      WEIGHT                  = 1000;                     //权重，带三位精度
const uint64_t      B_DECAY_PERCENT         = 85;                       //活力星奖励的影响因子（85%）
const uint64_t      MIN_VOTE_AMOUNT         = 10 * PRECISION;
const uint64_t      MAX_MEMO_LENGTH         = 15;
const uint64_t      ACTIVE_PROMOT_INVITES   = 5;
const uint64_t      RANDOM_COUNT            = 10;
const uint64_t      COUNT_OF_TRAVERSAL_PER  = 100;

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

const uint64_t      MAX_ROUND_REWARD        = ROUND_AMOUNT + ROUND_SIZE * Z1;   //当前轮最大奖励发放金额
const uint64_t      MAX_USER_REWARD         = MAX_ROUND_REWARD * ACTIVE_PERCENT / 100 * 1; //每个用户最多发放奖励（只有一个活力星的时候）
```



## 数据结构（Structs）

### Global

全局状态表，**只有1条记录**，初始化的时候存入

```c++
struct tbglobal {
  uint64_t index;                     // 自增索引
  uint64_t pool_amount;               // 总资金池剩余资产
  uint64_t current_round;             // 当前轮数
  uint8_t upgrading;                  // 合约升级标志
  uint64_t total_weight;              // 总权重
  uint64_t reserve1;                  // 保留字段1 
  std::string reserve2;               // 保留字段2
};
```

### Round

每一轮都会产生一条记录，记录当前轮次的信息

```c++
struct tbround{                      
  uint64_t round;                     // 当前轮数&索引
  uint64_t current_round_invites;     // 当前轮完成邀请数
  uint64_t base_pool;                 // 当前轮底池资产数
  uint64_t random_rewards;            // 当前轮随机池资产数
  uint64_t invite_rewards;            // 当前邀请奖励池资产数
  uint64_t actual_rewards;            // 当前轮endround实际发放奖励统计
  uint64_t start_time;                // 当前轮的启动时间
  uint64_t end_time;                  // 当前轮的结束时间
  budgetstate bstate;                 // 当前轮endround获取应发奖励状态
  rewardstate rstate;                 // 当前轮endround奖励计算进度状态
  uint64_t reserve1;                  // 保留字段1
  std::string reserve2;               // 保留字段2
};
```
### Account

账户投票表

```c++
struct tbaccount {
  uint64_t account_id;                // 账户id&索引                 
  uint64_t vote_count;                // 总投票数
  uint64_t reserve1;                  // 保留字段1
  std::string reserve2;               // 保留字段2
};
```

### Vote

投票操作表

```c++
struct tbvote {
  uint64_t index;                     // 自增索引
  uint64_t round;                     // 当前轮数
  uint64_t staking_amount;            // 抵押GXC数量
  uint64_t from;                      // 投票者id
  uint64_t to;                        // 被投票者id
  uint64_t vote_time;                 // 投票时间
  uint64_t disabled;                  // 是否撤销投票
  uint64_t reserve1;                  // 保留字段1
  std::string reserve2;               // 保留字段2
}
```

### Staking

锁仓表，每一次升级小行星（投票）和升级超级星，都会产生一条staking记录

```c++
struct tbstaking {
  uint64_t index;                     // 索引
  uint64_t account;                   // 账号id
  uint64_t amount;                    // 抵押数量
  uint64_t end_time;                  // 抵押时间
  uint64_t staking_to;                // 为哪个账户抵押（小行星投票给超级星 / 超级星升级）
  uint64_t staking_type;              // 抵押类型
  uint64_t claimed;                   // 是否解除抵押
  uint64_t claim_time;                // 解除抵押的时间
  uint64_t vote_index;                // 记录对应投票表项id
  uint64_t reserve1;                  // 保留字段1
  std::string reserve2;               // 保留字段2
}
```

### SmallPlanet

小行星表

```c++
struct tbsmallplan{
  uint64_t index;                     // 索引
  uint64_t id;                        // 账户id
  uint64_t create_time;               // 创建时间
  uint64_t create_round;              // 晋升轮数（第几轮晋升）
  uint64_t reserve1;                  // 保留字段1
  std::string reserve2;               // 保留字段2
}
```

### BigPlanet

大行星表

```c++
struct tbbigplanet{
  uint64_t index;                     // 索引
  uint64_t id;                        // 账户id
  uint64_t create_time;               // 创建时间
  uint64_t create_round;              // 晋升轮数（第几轮晋升）
  uint64_t reserve1;                  // 保留字段1
  std::string reserve2;               // 保留字段2
}
```

### ActivePlanet

活跃星表

```c++
struct tbactiveplan {
  uint64_t index;                     // 索引
  uint64_t id;                        // 账户id
  std::vector<uint64_t> invitees;     // 邀请人数，每达到5个大行星，置为0，记录create_round=当前轮，weight=1
  uint64_t create_time;               // 创建时间
  uint64_t create_round;              // 晋升轮数（第几轮晋升）
  uint64_t weight;                    // 权重b = floor(weight * 1.0 * pow(0.85,n)
  uint64_t trave_index;               // 用于性能优化，遍历索引，高位字节为bool值，保存是否为活力星（权重是否大于0），低7位字节表示账户id。0x01FFFFFFFFFFFFFF
  uint64_t reserve1;                  // 保留字段1
  std::string reserve2;               // 保留字段2
}
```

### SuperStar

超级星表

```c++
struct tbsuperstar {
  uint64_t index;                     // 索引
  uint64_t id;                        // 账户id
  uint64_t create_time;               // 创建时间
  uint64_t create_round;              // 晋升轮数（第几轮晋升）
  uint64_t vote_num;                  // 得票数
  uint64_t disabled;                  // 是否已经撤销抵押
  uint64_t reserve1;                  // 保留字段1
  std::string memo;                   // 超级星memo
  std::string reserve2;               // 保留字段2
}
```

### Invite

邀请关系表

```c++
struct tbinvite {
  uint64_t index;                     // 自增索引
  uint64_t invitee;                   // 被邀请账户
  uint64_t inviter;                   // 邀请账户
  bool     enabled;                   // 邀请关系是否⽣生效(invitee是否升级为⼤行星)
  uint64_t create_round;              // 当前轮数
  uint64_t create_time;               // 邀请时间
  uint64_t reserve1;                  // 保留字段1
  std::string reserve2;               // 保留字段2
}
```

### Reward

邀请关系表

```c++
struct tbreward {
  uint64_t index;                     // 自增索引
  uint64_t round;                     // 小轮数
  uint64_t from;                      // 奖励来源账户
  uint64_t to;                        // 奖励去向账户
  uint64_t amount;                    // 奖励金额
  uint64_t type;                      // 奖励类型 //TODO check type
  uint64_t create_time;               // 创建时间
  uint64_t reward_time;               // 发奖时间
  uint8_t rewarded;                   // 是否已经发放
  uint64_t reserve1;                  // 保留字段1
  std::string reserve2;               // 保留字段2
}
```

## 接口（Actions）

### 1. 初始化存入底池

``` c++
PAYABLE init(){

  // globals[0].index = 0;
  // globals[0].poll_amount = 2000000;
  // globals[0].current_round=0;
  // globals[0].upgrading=false;

}
```

### 2. 升级成为超级星

``` c++
PAYABLE uptosuper(std:string inviter){

    // 1. 判断是否存入足够GXC
    // 2. 判断是否已经是superstar，如果已经是，则提示"You are already a super star"
    // 3. 存到superstart表
    // 4. 保存邀请关系
    // 5. 插入/更新一条新的活力星记录，权重为1

}
```

### 3. 给超级星投票
``` c++
PAYABLE vote(std:string inviter,std:string superStar){

    // 1. 判断是否满足最小投票数额
    // 2. 保存邀请关系（不允许重复邀请）
    // 3. 判断是否足够升级成小行星，如果满足要求，则存到smallPlanet表（不允许重复创建）
    // 4. vote(允许重复投票)
    // 5. 修改超级星得票数

}
```
### 4. 升级成为大行星

``` c++
PAYABLE uptobig(){

    // 1. 判断是否存入足够GXC
    // 2. 判断是否是small planet，如果还不是，则提示“You have to become a small planet first”
    // 3. 判断是否已经是bigPlanet，如果已经是，则提示"You are already a big planet"
    // 4. 存到bigPlanet表
    // 5. 保存邀请关系
    // 6. 发放邀请奖励
    // 7. 判断是否完成一小轮

}
```

### 5. 通过自邀请成为活力星

```c++
PAYABLE selfactivate(const std::string &superstar){
    // 活力星可以通过自邀请来增加权重，当邀请满5人时，权重加1
}
```

### 6. 结束当前轮
admin账户发起心跳接口，判断是否需要手动结束当前轮

#### 6.1 获取应发奖励budget

```c++
ACTION getbudget(){
  // 计算当轮应给大行星、活力星、超级星发放的奖励
}
```

#### 6.2 计算随机奖励池

```c++
ACTION calcrdmrwd(){
  // 计算随机奖励池，应发放的奖励信息，放入reward表
}
```
#### 6.3 计算大行星奖励池

```c++
ACTION calcbigrwd(){
  // 计算固定奖励池中的大行星奖励，放入reward表
}
```

#### 6.4 计算活力星奖励池

```c++
ACTION calcactrwd(){
  // 由于性能限制（执行时间限制），需要分多次调用来遍历，使用trave_index索引来遍历
  // 计算固定奖池中的活力星奖励，放入reward表
}
```
#### 6.5 计算超级星奖励池

```c++
ACTION calcsuprwd(){
  // 计算固定奖池中的超级星奖励，放入reward表
}
```

#### 6.6 分发奖励

```c++
ACTION dorwd(uint64_t limit){
  // 异步发放奖励，遍历reward表，指定发放limit条。（本轮结束可以延期发奖）
}
```

#### 6.7 开启新的一轮

```c++
ACTION newround(){
  // 开始新的一轮
}
```


### 7. 开启维护模式
admin账户发起维护模式，更新global表中的upgrading值

```c++
ACTION upgrade(bool flag){
    // 1. 判断sender是否合法
    // 2. globals[0].upgrading = flag;
}
```

### 8. 到期取回抵押资产
任何账户可以主动发起取回操作

```c++
ACTION claim(uint64_t stakingid){
    // 根据stakingid，认领到期的抵押项
}
```

### 9. 超级星更新备注

```c++
ACTION starplan::updatememo(std::string memo){
  // 判断sender是否为超级星
  // 更新sender对应超级星的memo
}
```


## 内部方法（Private methods）

### 1. 建立邀请关系

``` c++
void invite(uint64_t original_sender,std:string inviter){
    if(inviter!=null){
      // 1. 判断inviter是否是一个合法账号
      // 2. 判断inviter是否是一个大行星
      // 3. 查找invite表，如果未被邀请，则插入新邀请记录（original_sender, acc_id）,邀请关系未激活，当被邀请者成为大行星时激活
    }
}
```

### 2. 激活邀请关系

```c++
inline void activateInvite(uint64_t sender){
    // 激活邀请关系
}
```

### 3. 给超级星投票

``` c++
uint64_t createVote(uint64_t sender, uint64_t super_id, uint64_t voteCount);
```

### 4. 更新超级星得票数

```c++
inline void updateSuperstarVote(uint64_t account, uint64_t voteCount, uint64_t feePayer);
```

### 5. 超级星是否可用

``` c++
inline bool superstarEnabled(uint64_t superId);
```

### 6. 超级星是否存在

``` c++
inline bool superstarExist(uint64_t superId);
```

### 7. 添加超级星

``` c++
inline void createSuperstar(uint64_t sender, const std::string &memo);
```

### 8. 启用超级星

``` c++
//适用于超级星解除抵押之后，再激活的状态
void enableSuperstar(uint64_t superId, const std::string &memo);
```

### 9. 禁用超级星

``` c++
void disableSuperStar(uint64_t superId);
```

### 10. 是否已经是小行星

``` c++
bool isSmallPlanet(uint64_t sender);
```

### 11. 添加小行星

``` c++
bool createSmallPlanet(uint64_t sender);
```

### 12. 是否已经是大行星

``` c++
bool isBigPlanet(uint64_t sender);
```

### 13. 添加大行星

``` c++
bool createBigPlanet(uint64_t sender);
```

### 14. 获取当前轮数

``` c++
uint32_t currentRound();
```

### 15. 是否已经超过最后大行星过期时间

``` c++
inline bool isInviteTimeout(uint64_t sender);
```

### 16. 是否满足最大邀请数

``` c++
inline bool isRoundFull(uint64_t sender);
```

### 17. 获当前轮是否结束

``` c++
inline bool isRoundFinish();
```

### 18. 更新账户投票数

``` c++
inline uint64_t updateAccountVote(uint64_t sender,uint64_t voteCount);
```

### 19. 是否满足邀请条件

``` c++
bool isInviter(std::string accname);
```

### 20. 是否已经初始化

``` c++
inline bool isInit();
```

### 21. 是否存在邀请关系

``` c++
bool hasInvited(uint64_t invitee);
```

### 21. 创建抵押项

``` c++
void createStaking(uint64_t sender,uint64_t amount,uint64_t to,uint64_t reason,uint64_t index=0);
```

### 22. 获取邀请者

``` c++
inline uint64_t getInviter(uint64_t invitee);
```

### 23. 构建发奖原因字符串

``` c++
inline void buildRewardReason(uint64_t invitee, uint64_t inviter, uint64_t rewardType, std::string &rewardReason);
```

### 24. 构建充值日志字符串

``` c++
inline void buildDepositMsg(uint64_t amount, bool equalCheck, std::string &msg);
```
### 25. 分发大行星和自邀请消耗的3个GXC

``` c++
void distributeInviteRewards(uint64_t invitee, uint64_t rewardAccountId, uint64_t rewardType);
```

### 26. 升级大行星或者自邀请时，更新活力星信息

- 大行星邀请满5人后，可以升级为活力星
- 活力星的权重初始为1，即晋升的那一小轮，按照100%的权重分发，之后每一小轮权重降为原来的0.85，直至权重为0，权重取3位小数
- 重新邀请满5人后（这5人可以跨多轮邀请到），活力星的权重增加1

``` c++
void updateActivePlanet(uint64_t activePlanetAccountId,uint64_t subAccountId);
```

### 27. 升级超级星时，更新活力星信息

``` c++
void updateActivePlanetForSuper(uint64_t activePlanetAccountId);
```

### 28. 计算应发奖励

在当前轮结束时，需要计算当前轮奖励数额：

- 每一轮默认底池为default_amount = 2000GXC（不包含邀请增加的部分）
- 超过4小时没有结束，则底池会减少，没隔1小时底池减少x-1次，其中x=(current_round % bigRoundSize)+1
- 每一轮总底池pool_amount = default_amount + rounds.upper_bound().invite_pool_amount;

``` c++
void calcBudgets();
```

### 29. 获取当前轮大行星

``` c++
void getCurrentRoundBigPlanets(vector<uint64_t> &bigPlanets);
```

### 30. 获取超级星

``` c++
uint64_t getCurrentRoundSuperStars(vector<SuperStar> &superStars);
```

### 31.随机选择最多十个大行星

``` c++
void chooseBigPlanet(const vector<uint64_t> &bigPlanets, vector<uint64_t> &choosed);
```

### 32. 创建新的一轮

``` c++
void createNewRound();
```

### 33. 检查调用者

``` c++
inline uint64_t checkSender();;
```

### 34. 检查合约是否升级

``` c++
bool isUpgrading();
```

### 35. 解除抵押时，取消投票项

```c++
void cancelVote(uint64_t voteIndex,uint64_t superAccId,uint64_t amount);;
```

### 36. 调用基本检查

```c++
void baseCheck();
```

### 37. 检查当前轮是否完成

```c++
inline void         roundFinishCheck();
```
### 38. 发放随机奖励池

```c++
void randomReward();
```

### 39. 验证充值金额是否等于某个值

```c++
inline uint64_t assetEqualCheck(uint64_t expectedAmount);
```

### 40. 验证充值金额是否不小于某个值

```c++
inline uint64_t assetLargerCheck(uint64_t expectedAmount);
```

### 41. 邀请者检查

```c++
inline uint64_t inviterCheck(const std::string &inviter, uint64_t inviteeId);
```

### 42. 超级星检查

```c++
inline uint64_t superStarCheck(const std::string &superStarAccount);
```

### 43. 有效邀请数自增1，满一百结束当前轮

```c++
inline void progress(uint64_t ramPayer);
```

### 44. 结束当前轮，计算奖励时的检查

```c++
inline void endRoundCheck(bool check,const std::string &msg);
```