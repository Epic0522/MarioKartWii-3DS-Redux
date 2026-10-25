#pragma once
#include "mkw/star_colors.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace mkw {
inline constexpr uint16_t StarDuration=450;
// Retail StarMaterialAnimation RMCU .text+5aa8c samples the shared CLR0 each
// tick, ramps RGB strength by .1, pulses during its final 100 ticks and fades
// through the last ten. Returns additive RRGGBB00, never replacement alpha.
inline uint32_t starMaterialColor(uint16_t remaining){
    if(!remaining)return 0;
    const unsigned age=StarDuration-std::min(remaining,StarDuration)+1;
    const unsigned frame=(age-1)%StarColorPeriod+1;
    float strength=std::min(1.f,float(age)*.1f);
    if(age>=StarDuration-10)strength=std::max(0.f,float(int(StarDuration)-int(age)-1)*.1f);
    if(age>=StarDuration-100){
        const uint16_t phase=uint16_t((age-(StarDuration-100)+1)*2000u);
        strength*=.2f+.4f*(1.f-std::sin(float(phase)*(6.2831853071795864769f/65536.f)));
    }
    const uint32_t color=StarColors[frame];uint32_t result=0;
    for(unsigned shift:{24u,16u,8u})result|=uint32_t(float((color>>shift)&255)*strength)<<shift;
    return result;
}
}
