#pragma once
#include <mkw/math.hpp>
#include <egg/math/Vector.hh>

namespace Kinoko::Kart { class KartObject; }
namespace mkw {
// Presentation copy of the forward race camera state (retail RaceCamera +0x19c,
// Kinoko Render::KartCamera). The arithmetic is Kinoko's, statement for
// statement, so while nothing re-seats it the eye equals Kinoko's camera
// exactly. Kinoko keeps its own camera for the simulation's collision side
// effects and never re-seats it on respawn; retail does (RaceCamera::OnRespawn,
// PAL 0x805A49BC), so this copy is the one shown. No KCL is queried here.
class RaceChaseCamera {
public:
 void init(const Kinoko::Kart::KartObject& kart);  // KartCamera::initPos (0x805A49BC)
 void calc(const Kinoko::Kart::KartObject& kart);  // KartCamera::calc (0x805A21D0), forward only
 Vec3 eye()const{return {pos_.x,pos_.y,pos_.z};}
 Vec3 pivot()const{return {targetPos_.x,targetPos_.y,targetPos_.z};}
private:
 void calcForward(float t,const Kinoko::Kart::KartObject& kart);
 void calcDriftOffset(const Kinoko::Kart::KartObject& kart);
 void calcCamera(float horiz,float vert,float fastVert,const Kinoko::Kart::KartObject& kart,const Kinoko::EGG::Vector3f& targetPos);
 void calcAirtimeHeight(const Kinoko::Kart::KartObject& kart);
 float driftYaw_=0,hopPosY_=0;
 Kinoko::EGG::Vector3f forward_,right_;
 // KartCameraState (forward).
 Kinoko::EGG::Vector3f pos_,targetPos_;
 float bigAirHeight_=0,pitchRate_=0,dist_=0,bigAirFallPitch_=0;
 float camPosY_=0;
};
}
