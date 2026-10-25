#pragma once
#include "mkw/race_items.hpp"
#include <algorithm>

namespace mkw {
// Retail RMCU StaticR: ObjectItembox constructor .text+308a68, static
// initialization +309abc, update +308d18; ObjectDirector +30adac.
// BRRES has no skeletal clip and is centered at zero: hover/rotation are code.
inline constexpr float ItemBoxHeight=180.f, ItemBoxScale=.9f;
// ObjFlow.bin object 0x65 uses a sphere of radius 215, scaled by its object.
inline constexpr float ItemBoxRadius=215.f;
inline constexpr uint16_t ItemBoxRespawn=150;
inline constexpr unsigned ItemBoxCollisionDelay=10, ItemBoxAppearanceFrames=18;
inline ItemBox courseItemBox(Vec3 anchor,int16_t heightSetting,unsigned objectIndex){
    ItemBox box;box.position=anchor+Vec3{0,ItemBoxHeight+heightSetting,0};
    box.phase=uint16_t((objectIndex%4)*45);return box;
}
struct ItemBoxPose { Vec3 position,scale;float angle; };
inline ItemBoxPose itemBoxPose(const ItemBox& box){
    float scale=1, rise=0;
    const unsigned age=box.appearance;
    if(age<=ItemBoxAppearanceFrames){
        // Original 12-frame growth / six-frame settling curves. Only the
        // appearance jumps; mature boxes hover at a fixed height and tumble.
        if(age<=12){
            const float t=float(age)-12;
            scale=.01f+1.2f-1.2f*t*t/(11*11);
            rise=std::max(0.f,45.f-45.f*t*t/(7*7));
        }else{
            const float t=float(age)-12;
            scale=.01f+1.2f-.2f*t*t/(6*6);
            rise=std::max(0.f,45.f-45.f*t*t/(6*6));
        }
    }
    return {box.position+Vec3{0,rise,0},box.scale*std::max(0.f,scale),
            float(box.phase%181)*(6.2831853071795864769f/180.f)};
}
inline float itemBoxPickupRadius(const ItemBox& box){return box.radius*std::abs(box.scale.x);}
}
