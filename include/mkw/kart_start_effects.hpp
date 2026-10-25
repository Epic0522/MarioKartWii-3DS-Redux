#pragma once
#include <algorithm>
#include <cstdint>
namespace mkw {
// Local KartEffect::UpdateStartUpEffects, RMCU 80696f0c. The original
// rear-wheel table [0x1a,0x1d) contains start0, start1 AND start2, not
// mutually exclusive charge tiers. Effect +0x15/16/18 holds a requested
// post-start duration (8069d380); the first request tick leaves handles alone.
class KartStartEffects {
public:
 static constexpr int Hold=-1;
 int step(unsigned raceTimer,bool preview,bool countdown,bool accelerator,
          bool leftGround,bool rightGround,bool bike,uint32_t serial,int duration){
  if(preview)return Hold;
  if(serial!=serial_){serial_=serial;requested_=true;frame_=0;duration_=duration;}
  if(raceTimer>2000)return Hold;
  const bool first=requested_&&frame_==0;
  int wheels=0;
  if(countdown&&accelerator)wheels=(leftGround?1:0)|(!bike&&rightGround?2:0);
  else if(first)wheels=Hold;
  else if(requested_&&frame_<duration_)wheels=(leftGround?1:0)|(!bike&&rightGround?2:0);
  frame_=std::min(frame_+1,1000);
  return wheels;
 }
private:
 uint32_t serial_=0;int frame_=0,duration_=0;bool requested_=false;
};
}
