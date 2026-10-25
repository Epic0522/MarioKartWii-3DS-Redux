#include <mkw/menu_index.hpp>
#include <array>
#include <cstdint>
#include <cstring>

namespace mkw {
namespace {
constexpr uint32_t Prime=16777619u,Basis=2166136261u;
uint32_t mix(uint32_t h,const char* s){for(;*s;++s){h^=uint8_t(*s);h*=Prime;}return h;}
uint32_t hashKey(const char* a,const char* b,const char* c){
 uint32_t h=mix(Basis,a);
 if(b){h^=0x1fu;h*=Prime;h=mix(h,b);}
 if(c){h^=0x1eu;h*=Prime;h=mix(h,c);}
 return h;
}
bool same(const char* a,const char* b){return !b||!std::strcmp(a,b);}
// Open addressing over row numbers (+1, zero is empty). Rows are inserted in
// table order and later duplicates are skipped, preserving first-match lookup.
template<unsigned Bits,class Key>
class Index {
public:
 explicit Index(unsigned rows,Key key):key_(key){
  for(unsigned row=0;row<rows;++row){
   const auto k=key_(row);
   if(find(k[0],k[1],k[2])!=Missing)continue;
   for(uint32_t slot=hashKey(k[0],k[1],k[2])&Mask;;slot=(slot+1)&Mask)if(!slots_[slot]){slots_[slot]=uint16_t(row+1);break;}
  }
 }
 static constexpr unsigned Missing=~0u;
 unsigned find(const char* a,const char* b=nullptr,const char* c=nullptr)const{
  for(uint32_t slot=hashKey(a,b,c)&Mask;slots_[slot];slot=(slot+1)&Mask){
   const unsigned row=slots_[slot]-1u;const auto k=key_(row);
   if(!std::strcmp(k[0],a)&&same(k[1],b)&&same(k[2],c))return row;
  }
  return Missing;
 }
private:
 static constexpr uint32_t Mask=(1u<<Bits)-1u;
 std::array<uint16_t,1u<<Bits> slots_{};
 Key key_;
};
template<unsigned Bits,class Key> Index<Bits,Key> build(unsigned rows,Key key){return Index<Bits,Key>(rows,key);}
using Keys=std::array<const char*,3>;
template<class T,std::size_t N> constexpr unsigned rows(const T (&)[N]){static_assert(N<65535);return unsigned(N);}
}

const UiControlPose* uiFindControlPose(const char* control,const char* variant){
 static const auto index=build<11>(rows(UiControlPoses),[](unsigned i){return Keys{UiControlPoses[i].control,UiControlPoses[i].variant,nullptr};});
 const unsigned row=index.find(control,variant);return row==index.Missing?nullptr:&UiControlPoses[row];
}
const UiControlMotion* uiFindControlMotion(const char* control,const char* variant){
 static const auto index=build<11>(rows(UiControlMotions),[](unsigned i){return Keys{UiControlMotions[i].control,UiControlMotions[i].variant,nullptr};});
 const unsigned row=index.find(control,variant);return row==index.Missing?nullptr:&UiControlMotions[row];
}
const UiAnimationClip* uiFindAnimationClip(const char* control,const char* group,const char* name){
 static const auto index=build<12>(rows(UiAnimationClips),[](unsigned i){return Keys{UiAnimationClips[i].control,UiAnimationClips[i].group,UiAnimationClips[i].name};});
 const unsigned row=index.find(control,group,name);return row==index.Missing?nullptr:&UiAnimationClips[row];
}
const UiPanelInfo* uiFindPanel(const char* key){
 static const auto index=build<9>(rows(UiPanels),[](unsigned i){return Keys{UiPanels[i].key,nullptr,nullptr};});
 const unsigned row=index.find(key);return row==index.Missing?nullptr:&UiPanels[row];
}
const UiBorderLoop* uiFindBorderLoop(const char* key){
 static const auto index=build<7>(rows(UiBorderLoops),[](unsigned i){return Keys{UiBorderLoops[i].key,nullptr,nullptr};});
 const unsigned row=index.find(key);return row==index.Missing?nullptr:&UiBorderLoops[row];
}
const UiPaneInfo* uiFindPane(const char* panel,const char* name){
 static const auto index=build<13>(rows(UiPanes),[](unsigned i){return Keys{UiPanes[i].panel,UiPanes[i].name,nullptr};});
 const unsigned row=index.find(panel,name);return row==index.Missing?nullptr:&UiPanes[row];
}
const UiMovieInfo* uiFindMovie(const char* key){
 static const auto index=build<8>(rows(UiMovies),[](unsigned i){return Keys{UiMovies[i].key,nullptr,nullptr};});
 const unsigned row=index.find(key);return row==index.Missing?nullptr:&UiMovies[row];
}
const UiPaneNode* uiFindPaneNode(const char* panel,const char* name){
 static const auto index=build<11>(rows(UiPaneNodes),[](unsigned i){return Keys{UiPaneNodes[i].panel,UiPaneNodes[i].name,nullptr};});
 const unsigned row=index.find(panel,name);return row==index.Missing?nullptr:&UiPaneNodes[row];
}
unsigned uiFindMovieRegion(const char* key){
 static const auto index=build<8>(rows(UiMovieRegions),[](unsigned i){return Keys{UiMovieRegions[i].key,nullptr,nullptr};});
 const unsigned row=index.find(key);return row==index.Missing?unsigned(std::size(UiMovieRegions)):row;
}
unsigned uiFindSpriteKey(const char* key){
 static const auto index=build<13>(rows(UiSprites),[](unsigned i){return Keys{UiSprites[i].key,nullptr,nullptr};});
 const unsigned row=index.find(key);return row==index.Missing?unsigned(std::size(UiSprites)):row;
}
}
