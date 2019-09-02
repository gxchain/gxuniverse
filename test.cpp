#include "starplan.hpp"

void starplan::small(uint64_t count)
{
    /* 
   for(auto i = 0; i < 100; i++){
        tbbigplanets.emplace(get_trx_sender(), [&](auto &obj) {
            obj.index           = tbbigplanets.available_primary_key();
            obj.id              = ((roundnum << 8)) | i + 11;                                       // account id > 10
            obj.create_time     = get_head_block_time();
            obj.create_round    = roundnum;
        });
    } */
}
void starplan::bigcount(uint64_t count)
{
    for(auto i = 0; i < count; i++){
        auto sender_id = ((currentRound() << 8)) | i + 11 ;
        createBigPlanet(sender_id);
        distributeInviteRewards(sender_id, sender_id + 100, RWD_TYPE_INVITE);
        updateActivePlanet(sender_id + 100, sender_id);
        progress(sender_id);
        if(lastRound().current_round_invites >= 100) break;
    } 
}
void starplan::bigfull()
{
    for(auto i = 0; i < 100; i++){
        auto sender_id = ((currentRound() << 8)) | i + 11 ;
        createBigPlanet(sender_id);

        distributeInviteRewards(sender_id, sender_id + 100, RWD_TYPE_INVITE);
        updateActivePlanet(sender_id + 100, sender_id);
        progress(sender_id);
        if(lastRound().current_round_invites >= 100) break;
    } 
}
void starplan::createbigs(uint64_t roundnum)
{
    for(auto i = 0; i < 100; i++){
        tbbigplanets.emplace(get_trx_sender(), [&](auto &obj) {
            obj.index           = tbbigplanets.available_primary_key();
            obj.id              = ((roundnum << 8)) | i + 11;                                       // account id > 10
            obj.create_time     = get_head_block_time();
            obj.create_round    = roundnum;
        });
    }
}

void starplan::createacts1(uint64_t idindex)
{
    idindex++;
    for(auto i = 0; i < 100; i++){
        uint64_t activePlanetAccountId = (idindex << 8) | i;
        tbactiveplans.emplace(get_trx_sender(), [&](auto &obj) {                                      //创建活力星
            obj.index = tbactiveplans.available_primary_key();
            obj.id = activePlanetAccountId;
            obj.create_time = get_head_block_time();
            obj.create_round = 0;
            obj.weight = 0;
            obj.trave_index = activePlanetAccountId | 0x0000000000000000;
        });
    }
}

void starplan::createacts2(uint64_t idindex)
{
    uint64_t total_weight = 0;
    idindex++;
    for(auto i = 0; i < 100; i++){
        uint64_t activePlanetAccountId = (idindex << 8) | i;
        tbactiveplans.emplace(get_trx_sender(), [&](auto &obj) {                                      //创建活力星
            obj.index = tbactiveplans.available_primary_key();
            obj.id = activePlanetAccountId;
            obj.create_time = get_head_block_time();
            obj.create_round = 0;
            obj.weight = i * 100 + 100;
            total_weight += i*100;
            obj.trave_index = activePlanetAccountId | 0x0100000000000000;
        });
    }
    auto g_itor = tbglobals.find(0);
    tbglobals.modify(g_itor, get_trx_sender(), [&](auto &obj) {
        obj.total_weight = obj.total_weight + total_weight;
    });
}

void starplan::createsups(uint64_t count)
{
    for(auto i = 0; i < count; i++){
        auto idx = tbsuperstars.available_primary_key();
        tbsuperstars.emplace(get_trx_sender(), [&](auto &obj) {
            obj.index                   = idx;
            obj.id                      = idx + 11;       // account id > 10
            obj.create_time             = get_head_block_time();
            obj.create_round            = 0;
            obj.vote_num                = idx * 10 * PRECISION;
            obj.disabled                = false;
            obj.memo                    = "test";
        });
    }
}
void starplan::withdraw(uint64_t amount)
{
    inline_transfer(_self, get_trx_sender(), CORE_ASSET_ID, amount * PRECISION, LOG_CLAIM, strlen(LOG_CLAIM));
}

void starplan::testuptobig(uint64_t count)
{
    for(auto i=0;i<count;i++){
        auto idx = tbbigplanets.available_primary_key();
        uint64_t sender = get_trx_sender();
        tbbigplanets.emplace(sender, [&](auto &obj) {
            obj.index           = idx;
            obj.id              = idx + 10;
            obj.create_time     = get_head_block_time();
            obj.create_round    = currentRound();
        });
        auto itor = tbcurbigplans.find(currentRound());
        tbcurbigplans.modify(itor,sender,[&](auto &obj){
            obj.bigplanets.push_back(idx + 10);
        });
    }
}