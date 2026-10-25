#pragma once
#include <algorithm>

namespace mkw {
// RMCU ItemObjProperties (27babc/27be3c). IDs here are inventory IDs;
// retail object IDs differ (FakeBox=7, Bobomb=9, Blue=5).
struct ItemProperties {
    float terrain,heldContact,releasedContact,heldScale,releasedScale,growth,dropGrowth;
};
inline constexpr ItemProperties itemProperties(unsigned kind){
    switch(kind){
    case 0:case 1:case 16:case 17:return {31,70,34,1,2.1f,.03f,.03f};
    case 2:case 18:return {31,60,29,1.15f,2,.1f,.1f};
    case 3:return {72,72,58,1,2.5f,.04f,.07f};
    case 6:return {41,60,29,1.3f,2.2f,.1f,.1f};
    case 4:case 5:return {27,60,33,1,2.2f,.1f,.1f}; // Mushroom/Star property records
    case 9:return {27,60,33,1,2.5f,.1f,.1f}; // Star: section5+1f648
    case 7:return {25,50,29,1,1,.014f,.014f};
    default:return {40,40,40,1,1,0,0};
    }
}
// 2895b4 initializes scale to .0001. Singles wait >12 ticks and grow .2;
// chain head/ring grow .1, followers .1 + .05*index (27ec98).
inline float heldGrowth(float scale,float target,unsigned age,bool single,unsigned slot){
    if(single&&age<=12)return scale;
    return std::min(target,scale+(single?.2f:.1f+.05f*slot));
}
inline constexpr float HeldInitialScale=.0001f,HeldContactMinimumScale=.8f;
}
