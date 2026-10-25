#pragma once
#include <cstdint>
namespace mkw {
// USA StaticR ItemRoulette: init29f51c, update29f8e8; constants4+162c0.
struct ItemRoulette {
    enum class Step { None, NextIcon, PrizeIcon, Finished };
    uint16_t age=0;
    float speed=1.100000023841858f,phase=0;
    bool settling=false;
    Step advance(bool stopPressed,bool showingPrize){
        ++age;
        speed=speed*.9674999713897705f+.0020000000949949026f;
        phase+=speed;
        if(!settling){
            const bool next=phase>=1.f;
            if(next)phase-=1.f;
            if(age>184||(stopPressed&&age>=60))settling=true;
            return next?Step::NextIcon:Step::None;
        }
        if(phase<1.f)return Step::None;
        if(showingPrize){phase=0;return Step::Finished;}
        phase-=1.f;return Step::PrizeIcon;
    }
};
}
