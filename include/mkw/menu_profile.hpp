#pragma once
#include <mkw/renderer.hpp>
#include <mkw/render_metrics.hpp>
#include <array>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace mkw {
// QA-only per-frame menu timing. Rows stay in memory and are written once at
// exit, so SD writes never appear inside the measured intervals.
class MenuProfile {
public:
 enum Stage {Input,Compose,Resident,Textures,Movies,Audio,Models,Begin,Background,Scene,Foreground,Submit,StageCount};
 static constexpr const char* StageNames[StageCount]={"input","compose","resident","textures","movies","audio","models","begin","background","scene","foreground","submit"};
 void enable(){enabled_=true;rows_.reserve(24000);}
 // Release builds (MKW_RELEASE) never profile; the compiler drops the rest.
#ifdef MKW_RELEASE
 bool enabled()const{return false;}
#else
 bool enabled()const{return enabled_;}
#endif
 void hold(){row_.hold=true;}
 void begin(unsigned page,bool transition){
  if(!enabled())return;
  const auto now=svcGetSystemTick();
  row_={};row_.page=page;row_.transition=transition;row_.interval=previous_?ms(now-previous_):0;
  previous_=stamp_=now;renderMetrics={};
 }
 void mark(Stage stage){if(!enabled())return;const auto now=svcGetSystemTick();row_.ms[stage]+=ms(now-stamp_);stamp_=now;}
 void end(const Renderer& renderer,unsigned quads){
  if(!enabled())return;
  row_.draws=renderer.drawCalls;row_.quads=quads;row_.flushes=renderMetrics.flushCalls;row_.flushBytes=renderMetrics.flushBytes;row_.skinned=renderMetrics.skinnedVertices;row_.poses=renderMetrics.poseEvaluations;row_.skinMs=renderMetrics.skinMs;row_.environmentMs=renderMetrics.environmentMs;row_.drawMs=renderMetrics.drawMs;row_.linearFree=linearSpaceFree();
  if(rows_.size()<rows_.capacity())rows_.push_back(row_);
 }
 // Race frames reuse the same file with page=Race and only interval/submit.
 void raceFrame(float interval){if(!enabled())return;Row r{};r.page=9;r.interval=interval;if(rows_.size()<rows_.capacity())rows_.push_back(r);previous_=0;}
 void resetInterval(){previous_=0;}
 void event(const char* text){if(enabled())events_.emplace_back(unsigned(rows_.size()),text);}
 void write(const char* path)const{
  if(!enabled())return;
  FILE* f=std::fopen(path,"w");if(!f)return;
  std::fputs("row,page,transition,hold,interval_ms",f);for(auto* n:StageNames)std::fprintf(f,",%s_ms",n);
  std::fputs(",draws,quads,flushes,flush_kb,skinned,poses,skin_ms,env_ms,model_draw_ms,linear_free\n",f);
  for(size_t i=0;i<rows_.size();++i){const auto& r=rows_[i];
   std::fprintf(f,"%u,%u,%u,%u,%.3f",unsigned(i),r.page,unsigned(r.transition),unsigned(r.hold),r.interval);
   for(float v:r.ms)std::fprintf(f,",%.3f",v);
   std::fprintf(f,",%u,%u,%u,%u,%u,%u,%.3f,%.3f,%.3f,%u\n",r.draws,r.quads,r.flushes,r.flushBytes/1024,r.skinned,r.poses,r.skinMs,r.environmentMs,r.drawMs,r.linearFree);
  }
  for(const auto& e:events_)std::fprintf(f,"#event,%u,%s\n",e.first,e.second.c_str());
  std::fclose(f);
 }
private:
 static float ms(uint64_t ticks){return float(double(ticks)*1000.0/SYSCLOCK_ARM11);}
 struct Row {unsigned page=0;bool transition=false,hold=false;float interval=0;std::array<float,StageCount> ms{};unsigned draws=0,quads=0,flushes=0,flushBytes=0,skinned=0,poses=0;float skinMs=0,environmentMs=0,drawMs=0;unsigned linearFree=0;};
 std::vector<Row> rows_;std::vector<std::pair<unsigned,std::string>> events_;Row row_{};
 uint64_t previous_=0,stamp_=0;bool enabled_=false;
};
}
