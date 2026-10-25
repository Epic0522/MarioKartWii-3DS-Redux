#pragma once
#include <algorithm>
#include <cstdint>

namespace mkw {
// RMCU 290b88/290d08, 28fa84, 28fd84, 28fe4c, 28f6f4.
inline constexpr int BombFuse=300,BombDropFuse=90;
// Blast state (Bob-omb 0x8079b728, Blue Shell 0x807a36a4): the collision scale grows by
// 3.12 per update to 36 (.data globals 80511e78/e7c, 80511e90/e94) times the item's contact
// radius 29. At update 30 the blast flag (+0x1d8 / +0x334) turns contacts from the
// explosion (action 7) into a spin (action 0); update 42 unregisters the collision and the
// object despawns after update 85. `age` counts the blast updates done; the collision taken
// in the next kart pass uses the state after that update.
inline constexpr float BombBlastScale=36,BombBlastGrowth=3.12f,BombContactRadius=29;
inline constexpr unsigned BombSpinAge=31,BombCollisionAge=42,BombDespawnAge=87;
inline int bombProximityFuse(float distanceSquared,unsigned engineClass=2){
    const float blastSquared=(BombBlastScale*BombContactRadius)*(BombBlastScale*BombContactRadius);
    const int interval=engineClass==0?180:engineClass==1?150:120;
    if(distanceSquared>64*blastSquared)return BombFuse;
    if(distanceSquared>16*blastSquared)return interval;
    if(distanceSquared>4*blastSquared)return interval/2;
    if(distanceSquared>blastSquared)return interval/4;
    return 4;
}
inline float bombBlastRadius(unsigned age,float initialScale){
    return age>BombCollisionAge?0:std::min(BombBlastScale,initialScale+BombBlastGrowth*float(age))*BombContactRadius;
}
/// The kart action a blast contact requests (PlayerSub18_handleBombCollision /
/// handleBlueShellCollision).
inline constexpr int8_t bombBlastAction(unsigned age){return age>=BombSpinAge?0:7;}
}
