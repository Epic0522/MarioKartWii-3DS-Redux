// Audio::SoundTriggerMgr and RaceAmbienceMgr::PlayIntroAmbience, case by case
// from USA StaticR (docs/AUDIO_SOURCE.md "Course ambience"). Volumes are the
// floats at 8089d5f0.. (zones) and 8089cc58.. (intro). EchoMgr::SetEcho calls
// are left out: the port has no reverb.
#include <mkw/race_ambience.hpp>

namespace mkw {
namespace {
// Retail course ids (Kinoko::Course).
enum : unsigned {
 MarioCircuit = 0, MooMooMeadows = 1, MushroomGorge = 2, GrumbleVolcano = 3, ToadsFactory = 4, CoconutMall = 5,
 DKSummit = 6, WariosGoldMine = 7, LuigiCircuit = 8, DaisyCircuit = 9, MoonviewHighway = 0xa, MapleTreeway = 0xb,
 BowsersCastle = 0xc, RainbowRoad = 0xd, DryDryRuins = 0xe, KoopaCape = 0xf, GcnPeachBeach = 0x10,
 GcnMarioCircuit = 0x11, GcnWaluigiStadium = 0x12, GcnDKMountain = 0x13, DsYoshiFalls = 0x14,
 DsPeachGardens = 0x16, DsDelfinoSquare = 0x17, SnesGhostValley2 = 0x19, N64MarioRaceway = 0x1a,
 N64SherbetLand = 0x1b, N64BowsersCastle = 0x1c, N64DKsJungleParkway = 0x1d, GbaBowserCastle3 = 0x1e,
 GbaShyGuyBeach = 0x1f, BattleCourse23 = 0x23, LuigiCircuit2 = 0x3a,
};
}

void SoundTriggers::reset(unsigned course, bool mirror) {
 course_ = course; mirror_ = mirror; zone_ = -1; introCount_ = 0;
 ambience.clear(); music.clear();
}

void SoundTriggers::init() {
 // The first zone per course (Init's jump table 808c4738; competitions aside).
 int zone = 0;
 switch (course_) {
 case ToadsFactory: case BowsersCastle: zone = 7; break;
 case CoconutMall: zone = 5; break;
 case DKSummit: case LuigiCircuit: case LuigiCircuit2: zone = 2; break;
 case GcnDKMountain: case N64MarioRaceway: case N64DKsJungleParkway: zone = 1; break;
 case DsDelfinoSquare: zone = 3; break;
 default: break;
 }
 if (course_ == 0x2a || course_ >= 0x3b) return;
 // Applied with no kart, so the finished check passes.
 apply(zone, false, 3, 1);
}

void SoundTriggers::apply(int zone, bool finished, int raceState, unsigned maxLap) {
 if (finished) return;
 if (!((raceState > 2 && raceState < 7) || raceState == 1)) return;
 if (zone == zone_) return;
 const int previous = zone_;
 if (course_ < 0x3b) applyCourse(zone, previous, maxLap);
 zone_ = zone;
}

void SoundTriggers::applyCourse(int z, int prev, unsigned maxLap) {
 // The local racer leads the other local racers and this is no replay: always
 // so for one player.
 auto play = [&](std::uint16_t id) { ambience.push_back({AmbienceOp::Play, id, 0, 0, -1}); };
 auto vol = [&](std::uint16_t id, float v, std::uint16_t frames) { ambience.push_back({AmbienceOp::Volume, id, frames, v, -1}); };
 auto global = [&](std::uint16_t id) { ambience.push_back({AmbienceOp::Global, id, 0, 0, -1}); };
 auto oneShot = [&](std::uint16_t id, std::int16_t var = -1) { ambience.push_back({AmbienceOp::OneShot, id, 0, 0, var}); };
 auto stems = [&](std::uint8_t row, std::uint16_t frames) { music.push_back({MusicOp::Stems, row, frames, 0}); };
 auto bgmVolume = [&](float v, std::uint16_t frames) { music.push_back({MusicOp::Volume, 0, frames, v}); };
 // A loop brought in from silence: started, set to 0.01, then moved to `v`.
 auto rise = [&](std::uint16_t id) { play(id); vol(id, 0.01f, 0); };
 switch (course_) {
 case MushroomGorge:   // cave and wind, the music quieter inside
  if (z == 0) { bgmVolume(1.0f, 60); vol(0x298, 0, 120); play(0x299); }
  else if (z == 1) { bgmVolume(0.7f, 60); vol(0x299, 0, 120); play(0x298); if (prev == 0) vol(0x298, 0.01f, 0); vol(0x298, 0.6f, 60); }
  else if (z == 2) { bgmVolume(0.6f, 60); vol(0x299, 0, 120); play(0x298); if (prev == 0) vol(0x298, 0.01f, 0); vol(0x298, 0.8f, 60); }
  else if (z == 3) { bgmVolume(0.5f, 60); play(0x298); vol(0x298, 1.0f, 60); }
  break;
 case ToadsFactory:   // pistons, alarm and the crowd; the stream's stems by hall
  switch (z) {
  case 0: stems(0, 60); vol(0x2a3, 0, 120); vol(0x224, 0, 120); break;
  case 1: stems(0, 60); play(0x2a3); vol(0x2a3, 0.01f, 0); vol(0x2a3, 0.5f, 120); vol(0x224, 0, 120); break;
  case 2: stems(0, 60); vol(0x2a3, 0, 120); vol(0x2a4, 0, 120); break;
  case 3: stems(1, 180); play(0x2a4); if (prev == 2) vol(0x2a4, 0.01f, 0); vol(0x2a4, 0.2f, 120); break;
  case 4: stems(1, 60); vol(0x224, 0, 120); play(0x2a4); vol(0x2a4, 0.2f, 0); vol(0x2a4, 1.0f, 120); break;
  case 5:
   music.push_back({MusicOp::FactoryRestart, std::uint8_t(maxLap == 1), 0x16, 0});
   vol(0x2a4, 0, 120); rise(0x224); vol(0x224, 0.8f, 120);
   break;
  case 6: stems(0, 60); vol(0x224, 0, 120); break;
  case 7: stems(0, 60); rise(0x224); if (prev == -1) vol(0x224, 0.45f, 150); else vol(0x224, 0.6f, 120); break;
  default: break;
  }
  break;
 case CoconutMall:    // shoppers' chatter
  if (z == 0 || z == 3 || z == 5) vol(0x2a8, 0, 120);
  else if (z == 1) { if (prev == 0) rise(0x2a8); vol(0x2a8, 0.4f, 120); }
  else if (z == 2) { if (prev == 0) rise(0x2a8); vol(0x2a8, 0.7f, 120); }
  else if (z == 4) { if (prev == 3 || prev == 0) rise(0x2a8); vol(0x2a8, 0.8f, 120); }
  break;
 case DKSummit: case LuigiCircuit: case LuigiCircuit2:   // the audience
  if (z == 0) vol(0x224, 0, 120);
  else if (z == 1) { play(0x224); if (prev == 0) vol(0x224, 0.01f, 0); vol(0x224, 0.2f, 120); }
  else if (z == 2) { play(0x224); if (prev == -1 || prev == 0) vol(0x224, 0.01f, 0); vol(0x224, 0.4f, 120); }
  else if (z == 3) { play(0x224); if (prev == 0) vol(0x224, 0.01f, 0); vol(0x224, 0.8f, 120); }
  break;
 case WariosGoldMine:
  if (z == 0) stems(0, 60);
  else if (z == 1) stems(1, 120);
  break;
 case MoonviewHighway:   // the city
  switch (z) {
  case 0: stems(0, 60); vol(0x2dd, 0, 120); break;
  case 1: stems(1, 60); play(0x2dd); vol(0x2dd, 0.4f, 120); break;
  case 2: play(0x2dd); vol(0x2dd, 0.7f, 120); break;
  case 3: play(0x2dd); vol(0x2dd, 1.0f, 120); break;
  case 4: play(0x2dd); vol(0x2dd, 0.5f, 120); break;
  case 5: play(0x2dd); vol(0x2dd, 0.2f, 120); break;
  default: break;
  }
  break;
 case MapleTreeway:   // wind high in the tree
  if (z == 0) vol(0x2e6, 0, 120);
  else if (z == 4) play(0x2e6);
  break;
 case BowsersCastle:
  switch (z) {
  case 0: if (prev == 5) stems(0, 120); break;
  case 1: stems(0, 120); if (prev == 7) oneShot(0x2e9); break;   // Bowser laughs
  case 2: stems(2, 30); break;
  case 3: case 7: stems(0, 120); break;
  case 4: stems(1, 120); break;
  default: break;
  }
  break;
 case RainbowRoad:   // the ring sequence's step, and the tunnel
  if (z == 0) vol(0x2fc, 0, 60);
  else if (z >= 1 && z <= 5) oneShot(0x2f7, std::int16_t(2 * (z - 1)));
  else if (z == 6) { rise(0x2fc); vol(0x2fc, 1.0f, 30); }
  break;
 case DryDryRuins:   // falling sand
  if (z == 0) { stems(0, 120); vol(0x2ce, 0, 120); }
  else if (z == 1) { if (prev == 0) stems(1, 120); rise(0x2ce); vol(0x2ce, 1.0f, 120); }
  else if (z == 2) { if (prev == 0) stems(1, 120); vol(0x2ce, 0, 120); }
  break;
 case KoopaCape:   // the tube's water, the pipe, and the stems above and under water
  switch (z) {
  case 0: case 1: stems(0, 90); break;
  case 3: stems(1, 90); vol(0x2c5, 0, 90); if (prev == 6 || prev == 5 || prev == 0) oneShot(0x2c4); break;
  case 4: play(0x2c5); break;
  case 5: stems(0, 120); if (prev == 3) oneShot(0x2c4); break;
  case 6: stems(2, 90); break;
  default: break;
  }
  break;
 case GcnWaluigiStadium:
  if (z == 0 && prev == -1) global(0x368);
  break;
 case GcnDKMountain:   // jungle, river
  if (z == 0) { vol(0x1ee, 0, 120); vol(0x377, 0, 120); }
  else if (z == 1) { vol(0x377, 0, 120); rise(0x1ee); if (prev == -1) vol(0x1ee, 0.4f, 120); else vol(0x1ee, 0.5f, 60); }
  else if (z == 2) { vol(0x1ee, 0, 60); play(0x377); }
  break;
 case DsYoshiFalls: if (z == 0 && prev == -1) play(0x37a); break;
 case DsPeachGardens: if (z == 0 && prev == -1) play(0x38b); break;
 case DsDelfinoSquare:   // the crowd and the sea
  switch (z) {
  case 0: case 7: if (prev == 1) vol(0x1e5, 0.1f, 60); else vol(0x1e5, 0, 120); break;
  case 1:
   if (prev == 4 || prev == 5) { vol(0x382, 0, 120); vol(0x383, 0, 120); rise(0x1e5); }
   vol(0x1e5, 0.2f, 60);
   break;
  case 2: vol(0x1e5, 0.4f, 60); break;
  case 3: if (prev == -1) { rise(0x1e5); vol(0x1e5, 0.4f, 240); } else vol(0x1e5, 0.7f, 60); break;
  case 4: {
   vol(0x382, 0, 120); vol(0x383, 0, 120); vol(0x1e5, 0, 120);
   const std::uint16_t sea = prev == 5 ? 0x382 : 0x383;
   rise(sea); vol(sea, 1.0f, 60);
   break;
  }
  case 5: vol(0x382, 0, 120); vol(0x383, 0, 120); vol(0x1e5, 0, 120); break;
  default: break;
  }
  break;
 case N64MarioRaceway:   // the audience
  if (z == 0) vol(0x224, 0, 120);
  else if (z == 1) { play(0x224); if (prev == -1 || prev == 0) vol(0x224, 0.01f, 0); vol(0x224, 0.3f, 60); }
  else if (z == 2) vol(0x224, 0.4f, 120);
  break;
 case N64SherbetLand:   // the mountain's echo, the music quieter
  if (z == 0) { bgmVolume(1.0f, 60); vol(0x34f, 0, 120); }
  else if (z == 1) { bgmVolume(0.6f, 60); if (prev == 0) { play(0x34f); vol(0x34f, 0.6f, 0); } else vol(0x34f, 0.5f, 60); }
  else if (z == 2) { bgmVolume(0.3f, 60); vol(0x34f, 0.8f, 60); }
  break;
 case N64BowsersCastle:   // lava; Bowser laughs
  switch (z) {
  case 0: rise(0x35c); vol(0x35c, 1.0f, 120); break;
  case 1: case 3: vol(0x35c, 0, 120); break;
  case 2: oneShot(0x35b); break;
  case 5: if (prev != 6) rise(0x35c); vol(0x35c, 0.6f, 30); break;
  case 6: vol(0x35c, 1.0f, 30); break;
  default: break;
  }
  break;
 case N64DKsJungleParkway:   // the jungle
  if (z == 0 || z == 3) vol(0x1ee, 0, 120);
  else if (z == 1) { if (prev != 2) rise(0x1ee); vol(0x1ee, 0.3f, 60); }
  else if (z == 2) vol(0x1ee, 0.7f, 60);
  break;
 case GbaBowserCastle3: if (z == 0 && prev == -1) global(0x339); break;
 case GbaShyGuyBeach: if (z == 0 && prev == -1) play(0x327); break;
 case BattleCourse23: if (z == 0 && prev == -1) global(0x30c); break;
 default: break;   // Mario Circuit, Grumble Volcano, Daisy Circuit and the rest: echo only
 }
}

void SoundTriggers::intro() {
 const unsigned n = ++introCount_;
 if (n >= 4) return;   // EchoMgr::SetEcho(0) only
 auto stop = [&](std::uint16_t frames) { ambience.push_back({AmbienceOp::GlobalStop, 0, frames, 0, -1}); };
 auto global = [&](std::uint16_t id) { ambience.push_back({AmbienceOp::Global, id, 0, 0, -1}); };
 auto volume = [&](float v, std::uint16_t frames) { ambience.push_back({AmbienceOp::GlobalVolume, 0, frames, v, -1}); };
 auto fadeIn = [&](std::uint16_t frames) { ambience.push_back({AmbienceOp::GlobalFadeIn, 0, frames, 0, -1}); };
 const std::uint16_t mir = mirror_ ? 1 : 0;
 switch (course_) {
 case MooMooMeadows:
  if (n == 1) { stop(0); global(0x28b); volume(0.3f, 0); fadeIn(30); }
  else if (n == 2) volume(1.0f, 40);
  else volume(0.01f, 120);
  break;
 case MushroomGorge: stop(0); global(std::uint16_t(0x294 + n)); break;
 case GrumbleVolcano:
  if (n == 1) { stop(0); global(0x2d0); volume(0.4f, 0); fadeIn(60); }
  else if (n == 2) volume(0.7f, 30);
  else volume(0.3f, 120);
  break;
 case ToadsFactory:
  if (n == 1) { global(0x2a3); volume(0.6f, 0); fadeIn(30); }
  else if (n == 2) stop(45);
  else { global(0x224); volume(0.7f, 0); fadeIn(100); }
  break;
 case CoconutMall:
  if (n == 1) { global(0x2a8); volume(0.2f, 0); fadeIn(30); }
  else if (n == 2) volume(0.5f, 0);
  else stop(20);
  break;
 case DKSummit:
  if (n == 1) { global(0x224); volume(0.1f, 0); }
  else if (n == 2) volume(0.3f, 20);
  else volume(0.7f, 60);
  break;
 case LuigiCircuit: case LuigiCircuit2: stop(0); global(std::uint16_t(0x285 + n)); break;
 case MoonviewHighway:
  if (n == 1) { global(0x2db); volume(0.1f, 0); }
  else if (n == 2) volume(0.7f, 45);
  else volume(0.3f, 30);
  break;
 case MapleTreeway: {
  const std::uint16_t first = mirror_ ? 0x2e2 : 0x2df;
  if (n == 1) { stop(0); global(first); }
  else if (n == 2) global(std::uint16_t(first + 1));
  else global(0x2e1);
  break;
 }
 case RainbowRoad:
  if (n == 2) { global(std::uint16_t(0x2f5 + mir)); volume(0.4f, 0); }
  else if (n == 3) { stop(60); global(0x2fa); volume(0.3f, 0); }
  break;
 case KoopaCape:
  if (n == 2) { global(0x2c5); volume(0.7f, 0); }
  else if (n == 3) stop(30);
  break;
 case GcnPeachBeach:
  if (n == 1) { stop(0); global(mirror_ ? 0x361 : 0x35f); }
  else if (n == 2) { stop(30); global(0x360); }
  else stop(30);
  break;
 case GcnMarioCircuit: if (n == 1) { stop(0); global(std::uint16_t(0x364 + mir)); } break;
 case GcnWaluigiStadium:
  if (n == 1) { stop(0); global(0x367); volume(0.6f, 0); fadeIn(10); }
  else if (n == 2) volume(0.7f, 0);
  else volume(1.0f, 0);
  break;
 case GcnDKMountain:
  if (n == 1) { stop(30); global(0x36f); }
  else if (n == 2) { stop(30); global(0x370); volume(1.0f, 0); fadeIn(60); }
  else { volume(0.5f, 120); global(0x371); }
  break;
 case DsYoshiFalls:
  if (n == 1) { stop(0); global(0x379); volume(0.7f, 0); fadeIn(30); }
  else if (n == 2) volume(0.6f, 30);
  else volume(1.0f, 30);
  break;
 case DsPeachGardens:
  if (n == 1) { stop(0); global(mirror_ ? 0x38a : 0x387); }
  else { stop(30); global(std::uint16_t(0x386 + n)); }
  break;
 case DsDelfinoSquare:
  if (n == 1) { stop(0); global(0x37d); }
  else if (n == 2) { stop(30); global(mirror_ ? 0x380 : 0x37e); }
  else { stop(30); global(0x37f); }
  break;
 case SnesGhostValley2:
  if (n == 2) { stop(0); global(0x33d); }
  else if (n == 3) stop(30);
  break;
 case N64MarioRaceway: if (n == 3) { stop(0); global(0x352); } break;
 case N64SherbetLand:
  if (n == 2) { stop(0); global(0x341); }
  else if (n == 3) stop(30);
  break;
 case N64BowsersCastle:
  if (n == 1) { stop(0); global(0x358); }
  else { stop(30); global(std::uint16_t(0x357 + n)); }
  break;
 case N64DKsJungleParkway:
  if (n == 1) { stop(0); global(0x1ee); volume(0.8f, 0); fadeIn(30); }
  else if (n == 2) volume(0.1f, 30);
  else volume(1.0f, 30);
  break;
 case GbaBowserCastle3: if (n == 1) { stop(0); global(0x338); } break;
 case GbaShyGuyBeach: if (n == 1) { stop(0); global(0x326); } break;
 default: break;
 }
}
}
