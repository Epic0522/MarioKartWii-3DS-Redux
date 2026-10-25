#pragma once
#include "mkw/math.hpp"
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace mkw {
// Scratch for the unique transformed positions, one per skinning thread:
// 0 the main thread, 1 the job worker, 2 the render thread (deferred writes).
// Those threads set positionScratchIndex.
constexpr unsigned PositionScratchCount=3;
inline std::vector<Vec3>& positionScratch(unsigned thread){static std::vector<Vec3> scratch[PositionScratchCount];return scratch[thread];}
inline thread_local unsigned positionScratchIndex=0;
inline std::vector<Vec3>& sharedPositionScratch(){return positionScratch(positionScratchIndex);}
// Cooked skinned meshes repeat each (position, matrix) pair at every corner
// that uses it (~5x). Skinning a position depends only on that pair, so each
// unique pair is transformed once and copied to its corners: the same
// arithmetic on the same inputs, bit-identical vertices.
struct SharedPositions {
    std::vector<uint16_t> first;   // unique pair -> first vertex using it
    std::vector<uint16_t> of;      // vertex -> unique pair
    // Source has float p[3] and uint32_t matrix. Leaves the map empty when it
    // would not save work (or the mesh is too large for 16-bit indices).
    template<class Source> void build(const Source* source,unsigned count){
        first.clear();of.clear();
        if(!source||!count||count>65535)return;
        struct Key{uint32_t w[4];bool operator==(const Key& k)const{return !std::memcmp(w,k.w,sizeof w);}};
        struct Hash{size_t operator()(const Key& k)const{uint32_t h=2166136261u;for(uint32_t w:k.w)h=(h^w)*16777619u;return h;}};
        std::unordered_map<Key,uint16_t,Hash> index;index.reserve(count);
        of.resize(count);
        for(unsigned i=0;i<count;++i){
            Key k;std::memcpy(k.w,source[i].p,12);k.w[3]=source[i].matrix;
            auto [it,fresh]=index.emplace(k,uint16_t(first.size()));
            if(fresh)first.push_back(uint16_t(i));
            of[i]=it->second;
        }
        if(first.size()*4>size_t(count)*3){first.clear();of.clear();return;}
        first.shrink_to_fit();
        // Sized here, at load: skinning never allocates while racing.
        for(unsigned thread=0;thread<PositionScratchCount;++thread){auto& scratch=positionScratch(thread);if(scratch.size()<first.size())scratch.resize(first.size());}
    }
    bool ready() const {return !first.empty();}
};
}
