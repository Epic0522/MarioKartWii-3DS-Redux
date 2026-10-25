#pragma once
#include <algorithm>
#include <cstdint>

namespace mkw {
// Lightning screen flash. activateLightning (RMCU 0x807a91c0) restarts it on every local
// screen (0x8054a558). Its states (0x80548a20..0x80548b5c) run on the state manager at
// 0x8054894c: a requested state starts on the next update with its counter at 0, otherwise the
// counter counts up, then the state's calc runs. White fades in by 128 to 210, holds until the
// counter reaches 3, black at 150 holds until it reaches 51, then fades out by 5.
// Presentation only; one update per simulation step.
struct LightningFlash {
    enum : int8_t { FadeIn, White, Black, FadeOut, Idle };
    int8_t state=Idle, next=-1;
    uint16_t frames=0;
    float alpha=0;
    bool white=true;
    void start(){next=FadeIn;}
    void step(){
        if(next>=0){
            state=next;next=-1;frames=0;
            if(state==FadeIn){white=true;alpha=0;}       // 0x80548a30
            else if(state==Black){white=false;alpha=150;} // 0x80548ac4
        }else ++frames;
        switch(state){
        case FadeIn:alpha=std::min(210.f,alpha+128.f);if(alpha>=210.f)next=White;break;
        case White:if(frames>=3)next=Black;break;
        case Black:if(frames>=51)next=FadeOut;break;
        case FadeOut:alpha=std::max(0.f,alpha-5.f);if(alpha<=0.f)next=Idle;break;
        default:break;
        }
    }
    float opacity()const{return alpha/255.f;}
};
}
