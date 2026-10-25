#pragma once
#include <array>
#include <cstdint>

namespace mkw {
// Race-flow transitions for presentation subscribers (audio: countdown beeps,
// start, fall, finish jingle, results music, menu sounds). Emitted once per
// transition from the main loop; a listener must not change race state.
enum class RaceEvent : uint8_t {
 PreviewEnd,     // course preview finished or skipped; the start pan begins
 Countdown,      // RaceManager stage Intro -> Countdown
 Start,          // Countdown -> Race (GO)
 Fall,           // player out of bounds; value = retail camera request (0x8/0x18)
 Respawn,        // Lakitu picks the player up (InRespawn)
 Finish,         // player crossed the line on the final lap; value = rank
 ResultsShown,   // the race result board begins to slide in
 TotalsShown,    // the total-points board begins to slide in
 MenuShown,      // the end-of-race options appear
 OptionChosen,   // value = RaceEndOption
};
enum class RaceEndOption : int {Next=0,Quit=1};
struct RaceEventInfo {RaceEvent event;unsigned tick;int value;};
using RaceEventListener=void(*)(const RaceEventInfo&,void* context);

class RaceEvents {
public:
 static RaceEvents& instance(){static RaceEvents events;return events;}
 bool subscribe(RaceEventListener listener,void* context){
  for(auto& s:slots_)if(!s.listener){s={listener,context};return true;}
  return false;
 }
 void unsubscribe(RaceEventListener listener,void* context){
  for(auto& s:slots_)if(s.listener==listener&&s.context==context)s={};
 }
 void emit(RaceEvent event,unsigned tick,int value=0){
  const RaceEventInfo info{event,tick,value};
  for(const auto& s:slots_)if(s.listener)s.listener(info,s.context);
 }
private:
 struct Slot {RaceEventListener listener=nullptr;void* context=nullptr;};
 std::array<Slot,8> slots_{};
};
}
