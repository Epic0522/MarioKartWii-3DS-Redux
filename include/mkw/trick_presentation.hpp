#pragma once
#include <cstdint>

namespace mkw {
// The driver model's trick (retail PlayerModel, USA StaticR). A trick start
// requests the model's trick animation: PlayerTrick_start 0x8056F684 (karts),
// PlayerTrickBike_start 0x8056FEF4 and the half-pipe trick fn_1_64AAC
// 0x805AF9E8 (variant 2) call setTrick (fn_1_2C1538 0x807C2E8C), which stores
// a threshold by vehicle and jump variant (Kinoko SurfaceVariant): karts
// {0, 25, 50} (0x808A1FC8), bikes {5, 25, 35} (0x808A1FBC). The model update
// (fn_1_2BF15C 0x807C0AB0 karts, fn_1_2BF63C 0x807C0F90 bikes) counts frames
// off the ground (TouchingGround, +0x2C8; fn_1_2BADE4 copies the previous
// count to +0x2CC first) and starts the trick animation once that count
// exceeds the threshold while the kart still tricks (status 38 InATrick or
// 42 OverZipper; otherwise the request never fires). The same frame calls
// fn_1_18BE08 0x80697D34 for the local kart: KartEffect restarts
// rk_jumpSeikou and KartActor::StartSoundLimited plays SE_VCL_JUMP_ACTION
// (0x198). Presentation only; it reads Kinoko and never writes it.
class TrickPresentation {
public:
    static constexpr int Never=0x34bc0;   // the model's "not tricking" threshold
    static int threshold(bool bike,unsigned variant){
        constexpr int kart[3]={0,25,50},bikes[3]={5,25,35};
        return variant<3?(bike?bikes[variant]:kart[variant]):Never;
    }
    void reset(){*this=TrickPresentation{};}
    // One model update after the kart's step. jumpVariant is KartJump's surface
    // variant (read when InATrick rises); a half-pipe trick (ZipperTrick rising)
    // uses variant 2. Returns true on the update the trick animation starts.
    bool step(bool touchingGround,bool inATrick,bool overZipper,bool zipperTrick,unsigned jumpVariant,bool bike){
        airborneSeen_=airborne_;
        airborne_=touchingGround?0:airborne_+1;
        const bool jumpStart=inATrick&&!wasInATrick_,zipperStart=zipperTrick&&!wasZipperTrick_;
        wasInATrick_=inATrick;wasZipperTrick_=zipperTrick;
        if(jumpStart||zipperStart){pending_=true;threshold_=threshold(bike,zipperStart?2u:jumpVariant);}
        if(!pending_)return false;
        if(!inATrick&&!overZipper)threshold_=Never;
        if(airborneSeen_<=threshold_)return false;
        pending_=false;return true;
    }
    int airborne()const{return airborne_;}
private:
    int airborne_=0,airborneSeen_=0,threshold_=Never;
    bool pending_=false,wasInATrick_=false,wasZipperTrick_=false;
};
}
