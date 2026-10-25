#pragma once
#include <mkw/race_voice_tables.hpp>
#include <mkw/sound_ids.hpp>
#include <vector>
namespace mkw {
// Damage is an immediate one-shot, unlike sustained engine/ambience sounds.
// Keep its original waves outside the contended cache when optional groups
// are demoted after the pause UI and course graphics take their LINEAR.
inline void appendRaceDamageEffects(std::vector<std::uint32_t>& sounds) {
 using namespace snd::id;
 const std::uint32_t effects[]={SE_ITM_KAME_HIT,SE_ITM_HANE_EXPLODE,
  SE_ITM_BOMB_EXPLODE,SE_ITM_BOX_HIT,SE_ITM_BOX_HIT_FX,SE_ITM_BOX_SMOKE,
  SE_ITM_STAR_HIT,SE_VCL_DMG_PRESS,SE_VCL_DMG_PRESS_REV,SE_VCL_SPIN,
  SE_VCL_COL_CAR,SE_VCL_COL_CONCRETE,SE_VCL_COL_WOOD,SE_VCL_COL_SHIGEMI,
  SE_VCL_COL_GAKE,SE_VCL_COL_SNOWROCK,SE_VCL_COL_JELLY,SE_VCL_COL_METAL};
 sounds.insert(sounds.end(),effects,effects+sizeof(effects)/sizeof(effects[0]));
}
inline void appendRaceDamageVoices(std::vector<std::uint32_t>& sounds,unsigned character) {
 if(character>=24)return;
 // CharacterActor::PlayCharacterCollisionSound -> damageVoice() types.
 // Range uses [local=0]; Remap uses [local=1], as in voicePlay().
 const unsigned types[]={3,0x11,0x12,0x13,0x14,0x15};
 for(unsigned type:types){
  const int picker=voice::Remap[character][1][type];
  if(picker<0||picker>=29)continue;
  const auto* range=voice::Range[character][0][picker];
  if(range[0]==snd::id::VO_DUMMY)continue;
  for(std::uint32_t id=range[0];id<=range[1];++id)sounds.push_back(id);
 }
}
}
