#include <mkw/race_flow_pose.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace mkw {
float flowSample(const FlowKey* keys,unsigned count,float frame,bool step){
 if(!count)return 0;
 if(frame<keys[0].frame)return keys[0].value;
 if(step){
  float value=keys[0].value;
  for(unsigned i=0;i<count&&keys[i].frame<=frame;++i)value=keys[i].value;
  return value;
 }
 for(unsigned i=1;i<count;++i){
  if(frame<keys[i].frame){
   const auto& a=keys[i-1];const auto& b=keys[i];
   const float span=b.frame-a.frame,t=(frame-a.frame)/span,t2=t*t,t3=t2*t;
   return (2*t3-3*t2+1)*a.value+(t3-2*t2+t)*a.slope*span+(-2*t3+3*t2)*b.value+(t3-t2)*b.slope*span;
  }
 }
 return keys[count-1].value;
}

FlowPlay flowPlay(const FlowLayoutDef& def,const char* group,const char* clip){
 for(unsigned i=0;i<def.clipCount;++i)
  if(!std::strcmp(def.clips[i].group,group)&&!std::strcmp(def.clips[i].name,clip))return {int16_t(i),0};
 return {};
}

void flowAdvance(const FlowLayoutDef& def,FlowPlay& play,float frames){
 if(play.clip<0)return;
 play.frame+=frames*def.clips[play.clip].rate;
 for(unsigned guard=0;guard<4;++guard){
  const auto& c=def.clips[play.clip];
  if(play.frame<c.frames)return;
  if(c.loop){play.frame=c.frames>0?std::fmod(play.frame,c.frames):0;return;}
  if(c.next<0){play.frame=c.frames;return;}
  play.frame-=c.frames;play.clip=c.next;
 }
}

bool flowFinished(const FlowLayoutDef& def,const FlowPlay& play){
 if(play.clip<0)return true;
 const auto& c=def.clips[play.clip];
 return !c.loop&&c.next<0&&play.frame>=c.frames;
}

int flowPaneIndex(const FlowLayoutDef& def,const char* name){
 for(unsigned i=0;i<def.paneCount;++i)if(!std::strcmp(def.panes[i].name,name))return int(i);
 return -1;
}
int flowMaterialIndex(const FlowLayoutDef& def,const char* name){
 for(unsigned i=0;i<def.materialCount;++i)if(!std::strcmp(def.materials[i].name,name))return int(i);
 return -1;
}

namespace {
uint32_t setChannel(uint32_t rgba,unsigned channel,float value){
 const unsigned shift=24-8*channel;
 const uint32_t v=uint32_t(std::clamp(std::lround(value),0l,255l));
 return (rgba&~(0xffu<<shift))|(v<<shift);
}
}

void FlowPose::evaluate(const FlowLayoutDef& def,const FlowPlay* plays,unsigned playCount){
 const unsigned paneCount=std::min<unsigned>(def.paneCount,MaxPanes),materialCount=std::min<unsigned>(def.materialCount,MaxMaterials);
 auto& local=local_;
 for(unsigned i=0;i<paneCount;++i){
  const auto& p=def.panes[i];
  local[i]={{p.x,p.y,p.z,0,0,p.rotation,p.sx,p.sy,p.w,p.h},float(p.alpha),(p.flags&1)!=0};
  panes[i].vertex=p.vertex;
 }
 for(unsigned i=0;i<materialCount;++i){
  const auto& m=def.materials[i];auto& out=materials[i];
  for(unsigned c=0;c<4;++c){out.c0[c]=m.c0[c];out.c1[c]=m.c1[c];}
  out.srt=m.srt;
 }
 // Groups apply in BRCTR order; each binds only its own panes' tracks.
 for(unsigned g=0;g<playCount;++g){
  const auto& play=plays[g];if(play.clip<0||unsigned(play.clip)>=def.clipCount)continue;
  const auto& clip=def.clips[play.clip];
  for(unsigned t=clip.firstTrack;t<clip.firstTrack+clip.trackCount;++t){
   const auto& track=def.tracks[t];
   const float value=flowSample(def.keys+track.first,track.count,play.frame,track.step);
   switch(track.kind){
   case 0:if(track.target<paneCount&&track.index<10)local[track.target].v[track.index]=value;break;
   case 1:
    if(track.target>=paneCount)break;
    if(track.index==16)local[track.target].alpha=value;
    else if(track.index<16){auto& c=panes[track.target].vertex[track.index/4];c=setChannel(c,track.index%4,value);}
    break;
   case 2:
    if(track.target>=materialCount)break;
    if(track.index>=4&&track.index<8)materials[track.target].c0[track.index-4]=value;
    else if(track.index>=8&&track.index<12)materials[track.target].c1[track.index-8]=value;
    break;
   case 3:if(track.target<materialCount&&track.index<5)materials[track.target].srt[track.index]=value;break;
   case 4:if(track.target<paneCount)local[track.target].visible=value>=.5f;break;
   default:break;
   }
  }
 }
 // NW4R Pane::CalculateMtx: parent x T(x,y) x R(z) x S(sx,sy). A pane's own
 // alpha always applies to itself; it reaches children only when the pane
 // has the influenced-alpha flag (bit 1).
 auto& passed=passed_;
 for(unsigned i=0;i<paneCount;++i){
  const auto& p=def.panes[i];const auto& l=local[i];auto& out=panes[i];
  const float r=l.v[5]*3.14159265358979323846f/180.f,c=std::cos(r),s=std::sin(r);
  const float m[6]={c*l.v[6],-s*l.v[7],l.v[0],s*l.v[6],c*l.v[7],l.v[1]};
  const float own=std::clamp(l.alpha/255.f,0.f,1.f);
  out.w=l.v[8];out.h=l.v[9];out.z=l.v[2];
  if(p.parent>=0&&unsigned(p.parent)<i){
   const auto& q=panes[p.parent];
   out.m[0]=q.m[0]*m[0]+q.m[1]*m[3];out.m[1]=q.m[0]*m[1]+q.m[1]*m[4];out.m[2]=q.m[0]*m[2]+q.m[1]*m[5]+q.m[2];
   out.m[3]=q.m[3]*m[0]+q.m[4]*m[3];out.m[4]=q.m[3]*m[1]+q.m[4]*m[4];out.m[5]=q.m[3]*m[2]+q.m[4]*m[5]+q.m[5];
   out.alpha=own*passed[p.parent];out.visible=l.visible&&q.visible;out.z+=q.z;
   passed[i]=(p.flags&2)?out.alpha:passed[p.parent];
  }else{
   std::copy(m,m+6,out.m);out.alpha=own;out.visible=l.visible;passed[i]=(p.flags&2)?own:1.f;
  }
 }
}
}
