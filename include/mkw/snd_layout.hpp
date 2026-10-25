#pragma once
// Where a file's resident waves and stream heads go in its LINEAR block
// (SoundSystem::loadWaves). Resident waves that sit next to each other in
// the RWAR form runs read in one piece, keeping their offsets' 32-byte phase
// (DSP-ADPCM data must stay where the RWAV header says); a streamed wave
// keeps a head per channel. Everything is placed above a single high-water
// mark, so nothing overlaps. Platform free (tests/snd_layout_test.cpp).
#include <cstdint>
#include <vector>

namespace mkw::snd {
struct LayoutWave {
 std::uint32_t offset = 0, size = 0;  // RWAV in the RWAR
 bool resident = false;               // copied whole
 std::uint32_t headBytes = 0;         // > 0: streamed, this many bytes per channel
 int channels = 1;
};
struct LayoutRun { std::uint32_t src = 0, end = 0, at = 0; };   // RWAR [src, end) at block offset `at`
struct Layout {
 std::vector<LayoutRun> runs;
 std::vector<std::uint32_t> waveAt;   // block offset per wave (resident), else ~0u
 std::vector<std::uint32_t> headAt;   // block offset of each streamed wave's heads, else ~0u
 std::uint32_t total = 0;             // block size, a multiple of 32
};
inline Layout layoutWaves(const std::vector<LayoutWave>& waves) {
 Layout l;
 l.waveAt.assign(waves.size(), ~0u); l.headAt.assign(waves.size(), ~0u);
 std::uint32_t total = 0;
 bool open = false;   // the last run may still grow
 for (std::size_t i = 0; i < waves.size(); ++i) {
  const LayoutWave& w = waves[i];
  if (w.headBytes) {
   const std::uint32_t at = (total + 31) & ~31u;
   l.headAt[i] = at; total = at + w.headBytes * std::uint32_t(w.channels);
   open = false;
   continue;
  }
  if (!w.resident) { open = false; continue; }
  if (open && w.offset >= l.runs.back().end && w.offset - l.runs.back().end < 64) {
   auto& r = l.runs.back();
   l.waveAt[i] = r.at + (w.offset - r.src);
   r.end = w.offset + w.size;
   total = r.at + (r.end - r.src);
   continue;
  }
  const std::uint32_t at = ((total + 31) & ~31u) + (w.offset & 31);
  l.runs.push_back({w.offset, w.offset + w.size, at});
  l.waveAt[i] = at;
  total = at + w.size;
  open = true;
 }
 l.total = (total + 31) & ~31u;
 return l;
}
}
