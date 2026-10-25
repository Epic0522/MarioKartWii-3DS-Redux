#pragma once
#include <algorithm>
#include <cstdlib>
namespace mkw {
// Toad's Factory's switching conveyor (retail ObjBeltCurveA, USA StaticR
// 0x807EFD4C ctor, 0x807F05F4 update, 0x807F07CC animation update). KMP
// setting 1 == 1 starts it forward; settings 2 and 3 are its two direction
// changes in seconds of the race timer (Kinoko RaceManager::timer).
// Presentation only: it reads the timer and the placement.
struct BeltCurveTiming {
    // SRT0 playback rate away from a change (0x808CF200).
    static constexpr float Rate=1.15f;
    // The fade and slowdown span either side of a change (0x808CF1FC).
    static constexpr int Ramp=60;
    bool startForward=false;
    int change1=0,change2=0;
    static BeltCurveTiming fromSettings(int setting1,int setting2,int setting3){return {setting1==1,setting2*60,setting3*60};}
    // Forward until the first change, back until the second, then forward.
    // Retail shows BeltCurveB while forward and BeltCurveC otherwise.
    bool forward(unsigned timer) const {return int(timer)>change1&&int(timer)<=change2?!startForward:startForward;}
    // Frames to the nearer direction change.
    int distance(unsigned timer) const {return std::min(std::abs(int(timer)-change1),std::abs(int(timer)-change2));}
    // B_on_off/C_on_off frame: 0 (opaque) away from a change, 60 - distance
    // within 60 frames of one (fully transparent at the change).
    float fadeFrame(unsigned timer) const {const int d=distance(timer);return d<=Ramp?float(Ramp-d):0.f;}
    // SRT0 rate this frame: Rate, scaled by distance/60 near a change.
    float rate(unsigned timer) const {const int d=distance(timer);return d<=Ramp?Rate*float(d)/float(Ramp):Rate;}
    // SRT0 frame after `ticks` scene frames, the last `timer` of which advanced
    // the race timer (1..timer); earlier frames run at rate(0).
    float scroll(unsigned ticks,unsigned timer) const {
        timer=std::min(timer,ticks);
        // Sum of rate(1..timer) = Rate * (timer - deficit/60), where the deficit
        // is only nonzero within Ramp frames of a change.
        int deficit=0;
        auto window=[&](int change,int skipFirst,int skipLast){
            for(int k=std::max(1,change-Ramp);k<=std::min(int(timer),change+Ramp);++k)
                if(k<skipFirst||k>skipLast)deficit+=std::max(0,Ramp-distance(unsigned(k)));
        };
        window(change1,1,0);
        window(change2,change1-Ramp,change1+Ramp);
        return rate(0)*float(ticks-timer)+Rate*(float(timer)-float(deficit)/float(Ramp));
    }
};
}
