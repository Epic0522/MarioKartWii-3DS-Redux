#pragma once
#include <algorithm>
#include <cmath>

namespace mkw {
// The scene fader every section change uses: original DOL EGG::ColorFader::calc,
// USA 80215234. Opacity is an 8-bit integer ramp over 20 frames; fully opaque
// and hidden state changes have distinct tails (the section changes on frame 22
// of a fade out; a fade in ends on frame 21).
inline constexpr float ColorFadeFrames=20.f,ColorFadeOutLastFrame=22.f,ColorFadeInLastFrame=21.f;
// Black opacity `frame` frames into a fade (frame <= 0: the fade's first frame).
inline float colorFaderOpacity(float frame,bool entering){
 const float value=std::floor(std::clamp(frame,0.f,ColorFadeFrames)*255.f/ColorFadeFrames)/255.f;
 return entering?1.f-value:value;
}
}
