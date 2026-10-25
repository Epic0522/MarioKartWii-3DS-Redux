#pragma once
#include "mkw/math.hpp"
#include <algorithm>
#include <cmath>

namespace mkw {
// Race camera shake (RMCU, the camera's +0x338 object). Two channels move the eye along the
// camera's right and up axes (0x80599098 tail); each trigger adds amplitude up to its
// entry's cap and every camera update decays it (0x8059ea08). Presentation only.
struct CameraShakeEntry { float amplitude, speed, decay; };
// .rodata 0x8088dae0, 0x18 bytes per entry: {side channel, lift channel}.
inline constexpr CameraShakeEntry CameraShakeEntries[7][2]={
    {{3,.3f,.85f},{40,.6f,.85f}},
    {{50,.7f,.9f},{45,.65f,.9f}},    // Blue Shell and Bob-omb blasts (OnKill)
    {{20,.5f,.92f},{40,.7f,.92f}},
    {{3.5f,.7f,.85f},{25,.8f,.88f}}, // POW impact 1
    {{4,.7f,.9f},{30,.8f,.9f}},      // POW impact 2
    {{10,.7f,.9f},{35,.85f,.96f}},   // POW impact 3
    {{1,.55f,.55f},{9,.65f,.85f}},
};
enum : unsigned { CameraShakeBlast=1, CameraShakePow1=3 };
// Distance rows (.rodata 0x8088db88): full strength within `near`, fading to nothing at
// `far`, and none more than `height` above or below.
struct CameraShakeRange { float near, far, height; };
inline constexpr CameraShakeRange CameraShakeRanges[3]={{2000,10000,1000},{2000,25000,1000},{3000,3000,1000}};

class CameraShake {
public:
    struct Channel {
        const CameraShakeEntry* entry=nullptr;
        float phase=0, amplitude=0, direction=1, output=0;
        bool active=false;
        void add(const CameraShakeEntry& e,float strength){
            entry=&e;amplitude=std::min(e.amplitude,amplitude+e.amplitude*strength);active=true;
        }
        // 0x8059ea08: a triangle wave between -1 and 1 scaled by a decaying amplitude.
        void step(){
            if(!active||!entry)return;
            phase+=entry->speed*direction;
            if(phase>1){direction=-direction;phase=2-phase;}
            else if(phase<-1){direction=-direction;phase=-2-phase;}
            amplitude=amplitude*entry->decay-.001f;
            output=phase*amplitude;
            if(amplitude<=0){output=0;active=false;}
        }
    };
    /// 0x8059e034: the strength of a shake at `distance` from the camera's racer and `height`
    /// apart vertically, or a negative value when out of range.
    static float strength(unsigned range,float distance,float height){
        const auto& r=CameraShakeRanges[range<3?range:0];
        if(distance>r.far||std::fabs(height)>r.height)return -1;
        return distance>r.near?1-(distance-r.near)/(r.far-r.near):1;
    }
    void trigger(unsigned entry,float strength){
        if(entry>=7||strength<0)return;
        side.add(CameraShakeEntries[entry][0],strength);lift.add(CameraShakeEntries[entry][1],strength);
    }
    /// A blast at `at` seen by the camera following the racer at `racer`.
    void blast(Vec3 at,Vec3 racer){
        const Vec3 d=at-racer;trigger(CameraShakeBlast,strength(0,length(d),d.y));
    }
    void step(){side.step();lift.step();}
    /// Eye displacement for a camera looking from `eye` at `target` (right = up x back).
    Vec3 offset(Vec3 eye,Vec3 target)const{
        if(!side.output&&!lift.output)return {};
        const Vec3 back=normalized(eye-target,{0,0,1});
        const Vec3 right=normalized(cross({0,1,0},back),{1,0,0}),up=normalized(cross(back,right),{0,1,0});
        return right*side.output+up*lift.output;
    }
    void reset(){*this={};}
    Channel side, lift;
};
}
