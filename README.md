# Star Program

系统角色：

- 超级星（抵押20,000GXC）
- 小行星（抵押100GXC）
- 活力星（小行星邀请超过5人）
- 大行星（小行星消耗3GXC）

## 静态变量（Static Parameters）

静态变量在合约初始化时定义，会在合约中参与计算

```c++
const uint64_t      CORE_ASSET_ID        = 1;                        //核心资产id
const uint64_t      PRECISION            = 100000;                   //核心资产精度

const uint64_t      Z1                   = 1 * PRECISION;            //大行星投入每轮平均激励池资产数（1GXC）
const uint64_t      Z2                   = 1 * PRECISION;            //大行星奖励给邀请人的资产数（1GXC）
const uint64_t      Z3                   = 1 * PRECISION;            //大行星投入每轮随机奖励池资产数（1GXC）

const uint64_t      MAX_DECAY_COUNT      = 20;                       //最大衰减次数（20）
const uint64_t      PAYBACK_PERCENT      = 10;                       //返现比例（10%）
const uint64_t      ACTIVE_PERCENT       = 50;                       //活力星瓜分比例（50%），剩余40%为超级星瓜分比例
const uint64_t      WEIGHT               = 1000;                     //权重，带三位精度
const uint64_t      B_DECAY_PERCENT      = 85;                       //活力星奖励的影响因子（85%）
const uint64_t      ADMIN_ID             = 426;                      //admin账户id
const uint64_t      DEFAULT_INVITER      = 0;                        //默认邀请账户id//TODO blockcity account id
const uint64_t      SUPER_STAR_LIMIT     = 50;                       //超级星最大数量（50）
const uint64_t      BIG_ROUND_SIZE       = 50;                       //一个大轮包含小轮数（50）
const uint64_t      ROUND_AMOUNT         = 2000 * PRECISION;         //每一小轮的底池资产数（20GXC）
const uint64_t      ROUND_SIZE           = 100;                      //每一轮的参与人数（10）
const uint64_t      X                    = 200 * PRECISION;          //成为超级星需要抵押的资产数（20GXC）
const uint64_t      Y                    = 100 * PRECISION;          //成为小行星需要抵押的资产数（10GXC）
const uint64_t      Z                    = 110 * PRECISION;          //自激活需要抵押的资产数（11GXC）
const uint64_t      DECAY_TIME           = 4 * 3600;                 //衰减时间阈值，单位秒（4*3600s）
const uint64_t      DECAY_DURATION       = 1 * 3600;                 //衰减时间间隔，单位秒（1*3600s）
const uint64_t      INIT_POOL            = 2000000 * PRECISION;      //初始化充值200万GXC
const uint64_t      STAKING_DELAY_TIME   = 90 * 24 * 3600;           //抵押90天后解锁
const uint64_t      DELAY_TIME           = 12 * 3600;                //最后一个大行星的延迟时间（12小时）

```



## 数据结构（Structs）

### Global

全局状态表，**只有1条记录**，初始化的时候存入

```c++
struct tbglobal {
  uint64_t index;           // 自增索引
  uint64_t pool_amount;     // 总资金池剩余资产
  uint64_t current_round;   // 当前轮数
  bool upgrading;           // 升级维护中
};
```

### Round

每一轮都会产生一条记录，记录当前轮次的信息

```c++
struct tbround{                      
  uint64_t round;                   // 当前轮数&索引
  uint64_t current_round_invites;   // 当前轮完成邀请数
  uint64_t pool_amount;             // 当前轮奖池资产数，在当前轮结束后计算
  uint64_t random_pool_amount;      // 当前随机池资产数
  uint64_t invite_pool_amount;      // 当前邀请奖励池资产数
  uint64_t start_time;              // 当前轮的启动时间
  uint64_t end_time;                // 当前轮的结束时间
};
```

### Vote

投票操作表

```c++
struct tbvote {
  uint64_t index;           // 自增索引
  uint64_t round;           // 当前轮数
  uint64_t staking_amount;  // 抵押GXC数量
  uint64_t from;            // 投票者id
  uint64_t to;              // 被投票者id
  uint64_t vote_time;       // 投票时间
}
```

### Staking

锁仓表，每一次升级小行星（投票）和升级超级星，都会产生一条staking记录

```c++
struct tbstaking {
  uint64_t index;       // 自增索引
  uint64_t account;     // 账号id
  uint64_t amount;      // 锁仓金额
  uint64_t end_time;    // 锁仓结束时间
  uint64_t staking_to;  // 为哪个账户抵押（小行星投票给超级星 / 超级星升级）
  uint64_t reason;      // 抵押原因 
  bool claimed;         // 是否取回
  uint64_t claim_time;  // 取回时间
}
```

### SmallPlanet

小行星表

```c++
struct tbsmallplan{
  uint64_t index;           // 自增索引 
  uint64_t id;              // 账户id
  uint64_t create_time;     // 创建时间
  uint64_t create_round;    // 晋升轮数（第几轮晋升）
}
```

### BigPlanet

大行星表

```c++
struct tbbigplanet{
  uint64_t index;           // 自增索引 
  uint64_t id;              // 账户id
  uint64_t create_time;     // 创建时间
  uint64_t create_round;    // 晋升轮数（第几轮晋升）
}
```

### ActivePlanet

活跃星表

```c++
struct tbactiveplan {
  uint64_t index;               // 自增索引
  uint64_t id;                  // 账户id
  uint64_t invite_count;        // 邀请人数，每达到5个大行星，置为0，记录create_round=当前轮，weight=1
  uint64_t create_time;         // 创建时间
  uint64_t create_round;        // 晋升轮数（权重=1时的轮数）
  uint64_t weight;              // 权重b = floor(weight * 1.0 * pow(0.85,n)
}
```

### SuperStar

超级星表

```c++
struct tbsuperstar {
  uint64_t index;           // 自增索引
  uint64_t id;              // 账户id
  uint64_t create_time;     // 创建时间
  uint64_t create_round;    // 晋升轮数（第几轮晋升）
  uint64_t vote_num;        // 得票数
}
```

### Invite

邀请关系表

```c++
struct tbinvite {
  uint64_t index;           // 自增索引
  uint64_t invitee;         // 被邀请账户
  uint64_t inviter;         // 邀请账户
  bool enabled;             // 邀请关系是否生效（invitee是否升级为大行星）
  uint64_t create_round;    // 当前轮数
  uint64_t create_time;     // 邀请时间
}
```



## 接口（Actions）

### 1. 初始化存入底池

``` c++
PAYABLE init(){

  // globals[0].index = 0;
  // globals[0].poll_amount = 2000000;
  // globals[0].current_round=0;
  // globals[0].current_round=0;

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

### 5. 结束当前轮
admin账户发起心跳接口，判断是否需要手动结束当前轮

```c++
ACTION endround(){

    // 1. 判断sender是否合法
    // 2. 判断当前轮是否可以结束
    // 3. 计算奖池，发放奖励
    // 4. 重新计算活力星权重
    // 5. 开始新的一轮
}
```


### 6. 开启维护模式
admin账户发起维护模式，更新global表中的upgrading值

```c++
ACTION upgrade(bool flag){
    // 1. 判断sender是否合法
    // 2. globals[0].upgrading = flag;
}
```

### 7. 到期取回抵押资产
任何账户可以主动发起取回操作

```c++
ACTION claim(){
    // 遍历staking表，对到期的staking记录进行处理，转给对应的资产owner
}
```


## 内部方法（Private methods）

### 1. 建立邀请关系

``` c++
void invite(uint64_t original_sender,std:string inviter){
    if(inviter!=null){
      // 1. 判断inviter是否是一个合法账号
      // 2. 判断inviter是否是一个大行星
      // 3. 查找invite表，如果未被邀请，则插入新邀请记录（original_sender, acc_id）
    }
}
```
### 2. 小行星给超级星投票
``` c++
void vote(uint64_t orig_sender,std:string superstar)；
```
### 3. 是否已经是超级星

``` c++
bool isSuperStar(uint64_t sender);
```

### 4. 添加超级星

``` c++
bool addSuperStar(uint64_t sender);
```

### 5. 是否已经是小行星

``` c++
bool isSmallPlanet(uint64_t sender);
```

### 6. 添加小行星

``` c++
bool addSmallPlanet(uint64_t sender);
```

### 7. 是否已经是大行星

``` c++
bool isBigPlanet(uint64_t sender);
```

### 8. 添加大行星

``` c++
bool addBigPlanet(uint64_t sender);
```

### 9. 获取当前轮数

``` c++
uint32_t currentRound();
```

### 10. 判断是否开启新一小轮

- case1: 当前区块头时间 - 上一个大行星加入时间 > 12小时
- case2: totalInvites() - currentRound() * roundSize > roundSize

``` c++
bool bSmallRound(){
    if(case1||case2){
        return true;
    }
    return false;
}
```

### 11. 结束当前一小轮

``` c++
void endSmallRound(){
    if(bSmallRound()){
      calcCurrentRoundPoolAmount(); // 计算当前轮奖励数额，保存到round表中
      updateActivePlanets();                // 统计当前轮活力星数据，保存到round表中
      randomReward();                               // 发放随机奖励池
      rewardBigPlanet();                        // 发放当前轮晋升的大行星奖励 = 当前轮总奖池的10%
      rewardActivePlanet();                 // 发放活力星奖励 = 当前轮总奖池的50%
      rewardSuperStar();                        // 发放超级星奖励 = 当前轮总奖池的40%
      startNewRound();                              // 开始新的一轮
    }
}
```

### 12. 发送邀请人奖励

```c++
void sendInviteReward(uint64_t inviter, uint64_t amount);
```

### 13. 计算当前轮奖励数额

在当前轮结束时，需要计算当前轮奖励数额：

- 每一轮默认底池为default_amount = 2000GXC（不包含邀请增加的部分）
- 超过4小时没有结束，则底池会减少，没隔1小时底池减少x-1次，其中x=(current_round % bigRoundSize)+1
- 每一轮总底池pool_amount = default_amount + rounds.upper_bound().invite_pool_amount;

```c++
void calcCurrentRoundPoolAmount();
```

### 14. 统计当前轮活力星数据

- 大行星邀请满5人后，可以升级为活力星
- 活力星的权重初始为1，即晋升的那一小轮，按照100%的权重分发，之后每一小轮权重降为原来的0.85，直至权重为0，权重取3位小数
- 重新邀请满5人后（这5人可以跨多轮邀请到），活力星的权重可重回1

```c++
void updateActivePlanets();
```


### 15. 发放随机奖励池

```c++
void randomReward();
```

### 16. 发放当前轮晋升的大行星奖励

```c++
void rewardBigPlanet();
```

### 17. 发放活力星奖励

```c++
void rewardActivePlanet();
```

### 18. 发放超级星奖励

```c++
void rewardSuperStar();
```

### 19. 开始新的一轮

```c++
void startNewRound();   
```

### 20. 激活邀请关系

```c++
void actInvite(uint64_t original_sender);
```

### 21. 是否满足邀请条件

```c++
bool isInviter(std::string accname);

```

### 22. 是否为账户

```c++
bool isAccount(std::string accname);
```

### 23. 是否初始化

```c++
bool isInit();
```

### 24. 是否存在邀请关系

```c++
bool hasInvited(uint64_t original_sender,std::string inviter);
```

### 25. 添加一个抵押项

```c++
void addStake(uint64_t sender,uint64_t amount,uint64_t to,std::string reason);
```

### 26. 大行星升级时，更新活力星

```c++
void updateActivePlanetsByBig(uint64_t sender);
```

### 27. 超级星升级时，更新活力星

```c++
void updateActivePlanetsBySuper(uint64_t sender);
```

### 28. 开启新的一轮

```c++
void createNewRound();
```

### 29. 是否升级小行星

投票满100票时，可以升级小行星

```c++
bool canUpdateSmall(uint64_t sender);
```

### 30. 解除抵押时，删除投票信息

```c++
void deleteVote();
```

### 31. 检查提现金额

```c++
void checkWithdraw();
```

