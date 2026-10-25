#pragma once
#include "mkw/math.hpp"
#include <algorithm>
#include <array>
#include <cstdint>

namespace mkw {
// RMCU StaticR.rel 27dac8/27dc28 (single), 27e19c/27e52c (chain).
// Constants are verified by tools/verify_held_retail.py. All steps are 60 Hz.
struct TrailingItemPose {
    Vec3 anchor{},forward{0,0,1},up{0,1,0},scale{1,1,1};
    float rearDistance=130,height=28,speed=0,speedRatio=0,hop=0;
    bool movingForward=false,airborne=false;
};
struct TrailingItems {
    Vec3 smoothedForward{},backward{},lastBackward{},lastAnchor{};
    std::array<Vec3,3> bases{},positions{},corrections{};
    std::array<float,3> motionCarry{.93f,.93f,.93f};
    std::array<std::array<float,16>,2> hopHistory{};
    uint32_t history=0;
    float verticalResponse=.1f,horizontalResponse=.1f,hop=0;

    void init(const TrailingItemPose& p) {
        *this={};smoothedForward=p.forward;backward=lastBackward=-p.forward;
        lastAnchor=p.anchor;
        bases.fill(p.anchor+backward*(p.rearDistance*p.scale.z));
        positions.fill(bases[0]+p.up*p.height);
        // Retail's initial wall-response displacement, 27cb80.
        corrections[0]=p.up*(15*p.scale.z);
    }
    void step(const TrailingItemPose& p,unsigned count) {
        // 27d624: use anchor displacement, damped .002 then added to facing.
        const Vec3 displacement=p.anchor-lastAnchor;lastAnchor=p.anchor;
        Vec3 drift=displacement*.002f;drift.y=std::clamp(drift.y,-.3f,.3f);
        verticalResponse=p.airborne?.27f:std::max(.1f,verticalResponse-.01f);
        horizontalResponse=std::max(.1f,horizontalResponse-.01f);
        const Vec3 target=p.forward+drift;
        smoothedForward.x+=(target.x-smoothedForward.x)*horizontalResponse;
        smoothedForward.y+=(target.y-smoothedForward.y)*verticalResponse;
        smoothedForward.z+=(target.z-smoothedForward.z)*horizontalResponse;
        backward=-normalized(smoothedForward,p.forward);
        bases[0]=p.anchor+backward*(p.rearDistance*p.scale.z);
        hop=(hop+(count>1?.2f:.82f)*p.hop)*(count>1?.85f:.59f);
        positions[0]=bases[0]+p.up*(p.height+hop-p.hop);
        if(count<2)return;
        // Source turn inertia uses the change in backwards unit vector and
        // squared capped speed ratio, not an invented follow interpolation.
        const Vec3 turn=p.movingForward?(lastBackward-backward)*(110*p.speedRatio*p.speedRatio):Vec3{};
        lastBackward=backward;
        const unsigned cursor=history++&15,delayed=(cursor+16-6)&15;
        hopHistory[0][cursor]=hop;
        const float hopGain=p.movingForward?std::min(1.f,.125f*p.speed):0.f;
        float rearLimit=p.rearDistance*p.scale.z;
        for(unsigned slot=1;slot<count;++slot){
            bases[slot]+=turn*(15.f*slot);
            if(motionCarry[slot]>0)bases[slot]+=displacement*motionCarry[slot];
            if(slot>1)rearLimit-=20.f*(slot-1);
            const float behind=dot(bases[slot]-p.anchor,backward);
            if(behind<rearLimit)bases[slot]+=backward*(rearLimit-behind);
            const float down=.4f*(bases[slot-1].y-bases[slot].y);
            if(down<0)bases[slot].y+=down;
            const float delayedHop=hopHistory[slot-1][delayed];
            if(slot==1)hopHistory[1][cursor]=delayedHop;
            const Vec3 link=bases[slot]-bases[slot-1];
            const float distance=length(link),limit=35*p.scale.z;
            if(distance>limit){
                bases[slot]=bases[slot-1]+link*(limit/distance);
                if(motionCarry[slot]>0)motionCarry[slot]-=.15f;
            }
            positions[slot]=bases[slot]+p.up*(p.height+(delayedHop-p.hop)*hopGain);
        }
    }
    // Original held controller smooths wall correction separately from its
    // articulated bases (.2 on contact, .9 otherwise). Terrain query adapts
    // the existing native KCL interface; presentation adds shell spin separately.
    void correct(unsigned slot,Vec3 correction,bool contact) {
        corrections[slot]=contact?lerp(corrections[slot],correction,.2f):corrections[slot]*.9f;
        positions[slot]+=corrections[slot];
    }
};
}
