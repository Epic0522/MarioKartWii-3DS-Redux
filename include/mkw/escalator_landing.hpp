#pragma once
#include <algorithm>
namespace mkw {
// Coconut Mall escalator treads. Kinoko moves each escalator by its step
// motion (0,10,-30)*phase (ObjectEscalator STEP_DIMS, two-step period 20); the
// draw transform carries that motion, so these adjust the source mesh in object
// space before it. In the moved frame:
// - treads past the fixed upper landing flatten beneath it (Y=1750);
// - treads past the lower landing (Y<0) slide back up their own slope to the
//   landing edge. Submerged steps shrink into the metal comb plate (local
//   Y=59.738 over Z=0..600) and degenerate there. They must not keep
//   descending: the plaza floor past the fountain is see-through, so the
//   mesh's 1,200-unit lower tail showed as tread strips on the floor. They
//   must not be raised flat either, which spread a textured sheet over it.
struct EscalatorPoint {float y,z;};
inline EscalatorPoint escalatorLanding(float sourceY,float sourceZ,float phase){
    const float dy=10.f*phase,dz=-30.f*phase;
    float y=sourceY+dy,z=sourceZ+dz;
    if(y>1750.f)y=1750.f;
    else if(y<0.f){z+=3.f*y;y=0.f;} // along the motion (0,10,-30): dz/dy = -3
    return {y-dy,z-dz};
}
}
