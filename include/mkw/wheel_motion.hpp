#pragma once
#include <algorithm>
#include <cmath>

namespace mkw {
// Retail visual wheel speed (Move+0x220/0x224), independent of chassis motion.
// All inputs are snapshots; neither this state nor its output enters physics.
struct WheelMotion {
    struct Inputs {
        unsigned stage=0;
        float speed=0,startCharge=0,minSpeed=-20,maxSpeed=100;
        bool accelerate=false,brake=false,ground=false,wall=false,action=false;
        bool ssmt=false,burnout=false,chargeStart=false;
    };
    float speed=0,target=0;
    float step(const Inputs& in){
        float response=.3f;
        if(in.stage>=2){
            // calcSsmtStart and KartBurnout::calc write 10 to Move+0x224;
            // calcWheelSpeed retains that target while either state is active.
            if(in.ssmt||in.burnout)target=10.f;
            else if(std::fabs(in.speed)<5.f&&in.ground&&(in.accelerate||in.brake)&&!in.action&&in.wall)
                target=in.accelerate?5.f:-5.f;
            else target=in.speed;
        }else{
            // KartState's original charge curve supplies 10*charge during the
            // countdown. After release, use the original faster wheel decay.
            if(in.stage>=1)target=10.f*in.startCharge;
            if(!in.chargeStart){target=0;response=.7f;}
        }
        speed+=response*(target-speed);
        // Body copies Move+0x220, then clamps to authored display +8/+4.
        return std::clamp(speed,in.minSpeed,in.maxSpeed);
    }
};
}
