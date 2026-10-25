#pragma once
#include <algorithm>
#include <array>
#include <cstdint>

namespace mkw {
// Retail USA StaticR.rel: ItemHolderPlayer_useGolden (text 293288),
// KartMove::activateMega (6e27c), and thunder-cloud strike (6dd54).
inline constexpr uint16_t GoldenDuration=450;
inline constexpr uint16_t MegaDuration=450, CloudShrinkDuration=612;
inline constexpr uint16_t CloudDuration=600;
// RMCU ItemObjKumo (0x807d6c88 spawn, 0x807d71c0 update, 0x807d85a0 transfer).
inline constexpr uint16_t CloudSpawnCooldown=5, CloudTransferCooldown=45, CloudSlotFrames=60,
    CloudImmunityAge=30, CloudLastTransfer=598, CloudTransferTimerCap=480;
// RMCU PowMgr: timer 159 at deploy (0x807d9808), stomps at 143/95/45 (0x807d9d4c) lifting
// targets by 6 and 10 (.rodata 0x808a3010/0x808a3014); the third opens a 16-update hit window.
inline constexpr uint16_t PowDuration=159;
inline constexpr int16_t PowStompTimers[3]={143,95,45};
inline constexpr float PowStompLift[2]={6,10};

// RMCU item behaviour entries (object type, objects per use; e.g. 0x808a0ed0 Triple
// Mushroom) and object limits (ItemObjProperties +0x4, e.g. Blue 0x808a105c). DecideItem
// (0x807ac9cc) offers an item only while its type is under the limit, counting live objects
// and the copies held in inventories; the capacity (limit - 1 + the largest per-use count,
// 0x80787fac) must also hold the whole use (0x80790bdc), or the roulette gives a Mushroom.
struct ItemObjectUse { uint8_t type, count; };
inline constexpr std::array<ItemObjectUse,19> ItemObjects{{
    {0,1},{1,1},{2,1},{7,1},{3,1},{3,3},{9,1},{5,1},{6,1},{4,1},
    {12,1},{8,1},{10,1},{11,1},{14,1},{13,1},{0,3},{1,3},{2,3}}};
inline constexpr std::array<uint8_t,15> ItemObjectLimits{12,8,16,12,3,1,1,6,2,3,1,1,2,1,1};
inline constexpr unsigned itemObjectCapacity(unsigned type){
    unsigned most=1;for(const auto& use:ItemObjects)if(use.type==type&&use.count>most)most=use.count;
    return ItemObjectLimits[type]-1+most;
}
inline constexpr uint8_t BulletObjectType=13;

inline uint16_t lightningShrinkDuration(unsigned rank,unsigned racers){
    // PlayerSub10_applyLightning (6dba0): the small-race field is aligned to
    // the back of the original twelve-position table, then adds 72 frames.
    constexpr std::array<uint16_t,12> recovery={690,645,600,555,510,450,390,330,270,210,150,90};
    racers=std::clamp(racers,1u,12u);rank=std::clamp(rank,1u,racers);
    return recovery[rank+11-racers]+72;
}
}
