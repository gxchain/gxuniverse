# Star Program

系统角色：

- 超级星（抵押20,000GXC）
- 小行星（抵押100GXC）
- 活力星（小行星邀请超过5人）
- 大行星（小行星消耗3GXC）

## 静态变量（Static Parameters）

静态变量在合约初始化时定义，会在合约中参与计算

```c++
const uint64_t superStarLimit = 50;	// 超级星最大数量（50）
const uint64_t bigRoundSize = 50;		// 一个大轮包含小轮数（50）
const uint64_t roundAmount = 2000;	// 每一小轮的底池资产数（2000GXC）
const uint64_t roundSize = 100; 		// 每一轮的参与人数（100）
const uint64_t x = 20000; 					// 成为超级星需要抵押的资产数（20000GXC）
const uint64_t y = 100;							// 成为小行星需要抵押的资产数（100GXC）
const uint64_t z1 = 1; 							// 大行星投入每轮平均激励池资产数（1GXC）
const uint64_t z2 = 1; 							// 大行星奖励给邀请人的资产数（1GXC）
const uint64_t z3 = 1; 							// 大行星投入每轮随机奖励池资产数（1GXC）
const uint64_t decayTime = 4*3600; 	// 衰减时间阈值，单位秒（4*3600s）
const uint64_t decayDur = 1*3600;  	// 衰减时间间隔，单位秒（1*3600s）
const uint64_t maxDecayCount = 20; 	// 最大衰减次数（20）
const float payBackPercent = 0.1; 	// 返现比例（0.1）
const float activePercent = 0.5; 		// 活力星瓜分比例（0.5），剩余0.4为超级星瓜分比例
const float a=1;										// 超级星奖励的影响因子
const float bDecay=0.85; 						// 活力星奖励的影响因子衰减系数（0.85）
const uint64_t initPool = 2000000;  // 初始化充值200万GXC
const uint64_t coreAsset = 1;       // 核心资产id
const uint64_t precision = 100000;  // 核心资产精度
const uint64_t delayDay = 90 * 24 * 3600; // 抵押90天后解锁
const uint64_t depositToBig = 3;    // 升级成大行星充值3GXC
const uint64_t weight = 1000;       // 权重，带三位精度
const uint64_t delaytime = 12 * 3600; // 最后一个大行星的延迟时间（12小时）
const uint64_t defaultinviter = 0;  // 默认邀请账户id  

const char* vote_reason  = "vote to super star";  // 给超级星投票
const char* stake_reason = "super star stake";    // 超级星晋升
```



## 数据结构（Structs）

### Global

全局状态表，**只有1条记录**，初始化的时候存入

```C++
struct tbglobal {
  uint64_t index;					// 自增索引
  uint64_t pool_amount;		// 总资金池剩余资产
  uint64_t current_round;	// 当前轮数
};
```

### Round

每一轮都会产生一条记录，记录当前轮次的信息

```C++
struct tbround{                      
  uint64_t round;                     // 当前轮数&索引
  uint64_t current_round_invites;     // 当前轮完成邀请数
  uint64_t pool_amount;								// 当前轮奖池资产数，在当前轮结束后计算
  uint64_t random_pool_amount;				// 当前随机池资产数
  uint64_t invite_pool_amount;				// 当前邀请奖励池资产数
  uint64_t start_time;               	// 当前轮的启动时间
  uint64_t end_time;               		// 当前轮的结束时间
};
```

### Vote

投票操作表

```C++
struct tbvote {
  uint64_t index;						// 自增索引
  uint64_t round;						// 当前轮数
  uint64_t staking_amount;	// 抵押GXC数量
  uint64_t from;						// 投票者id
  uint64_t to;							// 被投票者id
  uint64_t vote_time;				// 投票时间
}
```

### Staking

锁仓表，每一次升级小行星（投票）和升级超级星，都会产生一条staking记录

```C++
struct tbstake {
  uint64_t index;				// 自增索引
  uint64_t account;			// 账号id
  uint64_t amount;			// 锁仓金额
  uint64_t end_time;		// 锁仓结束时间
  uint64_t staketo;     // 为哪个账户抵押（小行星投票给超级星 / 超级星升级）
  std::string reason;   // 抵押原因 
}
```

### SmallPlanet

小行星表

```C++
struct tbsmallplanet{
  uint64_t index;							// 自增索引 
  uint64_t id;                // 账户id
  uint64_t create_time;       // 创建时间
  uint64_t create_round;  		// 晋升轮数（第几轮晋升）
}
```

### BigPlanet

大行星表

```C++
struct tbbigplanet{
  uint64_t index;							// 自增索引 
  uint64_t id;                // 账户id
  uint64_t create_time;       // 创建时间
  uint64_t create_round;  		// 晋升轮数（第几轮晋升）
}
```

### ActivePlanet

活跃星表

```C++
struct tbactiveplan {
  uint64_t index;								// 自增索引
  uint64_t id;                  // 账户id
  uint64_t invite_count;        // 邀请人数，每达到5个大行星，置为0，记录create_round=当前轮，weight=1
  uint64_t create_time;         // 创建时间
  uint64_t create_round;        // 晋升轮数（权重=1时的轮数）
  uint64_t weight;              // 权重b = floor(weight * 1.0 * pow(0.85,n)
}
```

### SuperStar

超级星表

```C++
struct tbsuperstar {
  uint64_t index;											// 自增索引
  uint64_t id;                        // 账户id
  uint64_t create_time;               // 创建时间
  uint64_t create_round;          		// 晋升轮数（第几轮晋升）
  uint64_t vote_num;                 	// 得票数
}
```

### Invite

邀请关系表

```C++
struct tbinvite {
  uint64_t index;											// 自增索引
  uint64_t invitee;										// 被邀请账户
  uint64_t inviter;										// 邀请账户
  bool enabled;												// 邀请关系是否生效（invitee是否升级为大行星）
  uint64_t create_round;							// 当前轮数
  uint64_t create_time;								// 邀请时间
}
```



## 接口（Actions）

### 1. 初始化存入底池

``` C++
PAYABLE init(){
  // globals[0].index = 0;
  // globals[0].poll_amount = 2000000;
  // globals[0].current_round=0;
}
```

### 2. 升级成为超级星

``` C++
PAYABLE uptosuper(std:string inviter){
    uint64_t orig_sender = get_trx_origin();
    uint64_t ast_id = get_action_asset_id();
    int64_t amount = get_action_asset_amount();
    // 1. 判断是否存入足够GXC
    graphene_assert(ast_id==1&&amount==x*100000,x+"GXC required");
    // 2. 判断是否已经是superstar，如果已经是，则提示"You are already a super star"
    graphene_assert(isSuperStar(orig_sender),“You are already a super star”);
    // 3. 存到superstart表
    addSuperStar(orig_sender);
    // 4. 保存邀请关系
    invite(orig_sender,inviter);
  	// 5. 插入/更新一条新的活力星记录，权重为1
  	addActiveStar(orig_sender);
}
```

### 3. 升级成为小行星
``` C++
PAYABLE uptosmall(std:string inviter,std:string superStar){
    uint64_t orig_sender = get_trx_origin();
    uint64_t ast_id = get_action_asset_id();
    int64_t amount = get_action_asset_amount();
    // 1. 判断是否存入足够GXC
    graphene_assert(ast_id==1&&amount==y*100000,y+"GXC required");
    // 2. 存到smallPlanet表（不允许重复创建）
    if(!isSmallPlanet(orig_sender)){
    	addSmallPlanet(orig_sender);
    }
    // 4. 保存邀请关系（不允许重复邀请）
  	if(!hasInvited(orig_sender,inviter)){
      invite(orig_sender,inviter);
    }
    // 5. vote(允许重复投票)
    vote(orig_sender, superStar);
}
```
### 4. 升级成为大行星

``` C++
PAYABLE uptobig(){
    uint64_t orig_sender = get_trx_origin();
    uint64_t ast_id = get_action_asset_id();
    int64_t amount = get_action_asset_amount();
    // 1. 判断是否存入足够GXC
    graphene_assert(ast_id==1&&amount==(z1+z2+z3)*100000,(z1+z2+z3)+"GXC required");
    // 2. 判断是否是small planet，如果还不是，则提示“You have to become a small planet first”
    graphene_assert(!isSmallPlanet(orig_sender),“You have to become a small planet first”);
    // 3. 判断是否已经是bigPlanet，如果已经是，则提示"You are already a big planet"
    graphene_assert(isBigPlanet(orig_sender),“You are already a big planet”);
    // 4. 存到bigPlanet表
    addBigPlanet(orig_sender);
    // 5. 保存邀请关系
    invite(orig_sender,inviter);
    // 6. 发放邀请奖励
    sendInviteReward(inviter, z1);
    // 7. 判断是否完成一小轮
    endSmallRound();
}
```

## 内部方法（Private methods）

### 1. 建立邀请关系

``` C++
void invite(uint64_t original_sender,std:string inviter){
    if(inviter!=null){
        int64_t acc_id = get_account_id(inviter.c_str(), inviter.length());
        graphene_assert(acc_id!=null,"inviter account: "+inviter+" does not exist");
      	graphene_assert(!isBigPlanet(acc_id),inviter+' is not a big planet');
        // 1. 查找inviteTable，如果未被邀请，则插入新邀请记录（original_sender, acc_id）
        addInvites(original_sender, acc_id);
    }
}
```
### 2. 小行星给超级星投票
``` C++
void vote(uint64_t orig_sender,std:string superstar){
  uint64_t star_acc_id = get_account_id(superstar);
  graphene_assert(!isSuperStar(star_acc_id),“The account you voted is not a superstar”);
  graphene_assert(!isSmallPlanet(orig_sender),“You have to become a small planet first”);
  if(vote.find(orig_sender)!=null){
    vote.emplace({orig_sender,star_acc_id,time,round});
  }
}
```
### 3. 是否已经是超级星

``` C++
bool isSuperStar(uint64_t sender);
```

### 4. 添加超级星

``` C++
bool addSuperStar(uint64_t sender);
```

### 5. 是否已经是小行星

``` C++
bool isSmallPlanet(uint64_t sender);
```

### 6. 添加小行星

``` C++
bool addSmallPlanet(uint64_t sender);
```

### 7. 是否已经是大行星

``` C++
bool isBigPlanet(uint64_t sender);
```

### 8. 添加大行星

``` C++
bool addBigPlanet(uint64_t sender);
```

### 9. 获取当前轮数

``` C++
uint32_t currentRound();
```

### 10. 判断是否开启新一小轮

- case1: 当前区块头时间 - 上一个大行星加入时间 > 12小时
- case2: totalInvites() - currentRound() * roundSize > roundSize

``` C++
bool bSmallRound(){
    if(case1||case2){
        return true;
    }
    return false;
}
```

### 11. 结束当前一小轮

``` C++
void endSmallRound(){
    if(bSmallRound()){
      calcCurrentRoundPoolAmount();	// 计算当前轮奖励数额，保存到round表中
      updateActivePlanets();				// 统计当前轮活力星数据，保存到round表中
      randomReward();								// 发放随机奖励池
      rewardBigPlanet(); 						// 发放当前轮晋升的大行星奖励 = 当前轮总奖池的10%
      rewardActivePlanet(); 				// 发放活力星奖励 = 当前轮总奖池的50%
      rewardSuperStar(); 						// 发放超级星奖励 = 当前轮总奖池的40%
      startNewRound();								// 开始新的一轮
    }
}
```

### 12. 发送邀请人奖励

```C++
void sendInviteReward(uint64_t inviter, uint64_t amount);
```

### 13. 计算当前轮奖励数额

在当前轮结束时，需要计算当前轮奖励数额：

- 每一轮默认底池为default_amount = 2000GXC（不包含邀请增加的部分）
- 超过4小时没有结束，则底池会减少，没隔1小时底池减少x-1次，其中x=(current_round % bigRoundSize)+1
- 每一轮总底池pool_amount = default_amount + rounds.upper_bound().invite_pool_amount;

```C++
void calcCurrentRoundPoolAmount();
```

### 14. 统计当前轮活力星数据

- 大行星邀请满5人后，可以升级为活力星
- 活力星的权重初始为1，即晋升的那一小轮，按照100%的权重分发，之后每一小轮权重降为原来的0.85，直至权重为0，权重取3位小数
- 重新邀请满5人后（这5人可以跨多轮邀请到），活力星的权重可重回1

```C++
void updateActivePlanets();
```


### 15. 发放随机奖励池

```C++
void randomReward();
```

### 16. 发放当前轮晋升的大行星奖励

```C++
void rewardBigPlanet();
```

### 17. 发放活力星奖励

```C++
void rewardActivePlanet();
```

### 18. 发放超级星奖励

```C++
void rewardSuperStar();
```

### 19. 开始新的一轮

```c++
void startNewRound();	
```

### 20. 激活邀请关系

```c++
void actinvite(uint64_t original_sender);
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
void updateActivePlanetsbybig(uint64_t sender);
```

### 27. 超级星升级时，更新活力星

```c++
void updateActivePlanetsbysuper(uint64_t sender);
```

### 28. 开启新的一轮

```c++
void createnewround();
```
