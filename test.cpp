#include "starplan.hpp"

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
            obj.weight = i * 100;
            total_weight += i*100;
            obj.trave_index = activePlanetAccountId | 0x0100000000000000;
        });
    }
    auto g_itor = tbglobals.find(0);
    tbglobals.modify(g_itor, get_trx_sender(), [&](auto &obj) {
        obj.total_weight = obj.total_weight + total_weight;
    });
}

void starplan::createsups()
{
    for(auto i = 0; i < 50; i++){
        tbsuperstars.emplace(get_trx_sender(), [&](auto &obj) {
            obj.index                   = tbsuperstars.available_primary_key();
            obj.id                      = i + 11;       // account id > 10
            obj.create_time             = get_head_block_time();
            obj.create_round            = 0;
            obj.vote_num                = i * 10 * PRECISION;
            obj.disabled                = false;
            obj.memo                    = "test";
        });
    }
}
void starplan::withdraw(uint64_t amount)
{
    inline_transfer(_self, get_trx_sender(), CORE_ASSET_ID, amount * PRECISION, LOG_CLAIM, strlen(LOG_CLAIM));
}