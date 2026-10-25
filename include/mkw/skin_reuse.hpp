#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>

namespace mkw {
using SkinRepresentatives=std::unique_ptr<uint16_t[]>;
struct AllocateSkinRepresentatives {
    SkinRepresentatives operator()(unsigned count) const {
        return SkinRepresentatives(new(std::nothrow) uint16_t[count]);
    }
};

// DOBJ vertices contain exactly position[3], normal[3], matrix. Matching their
// bytes preserves split normals, palette slots and even signed zero. Each map
// entry names the earliest matching vertex, so skinning can reuse its already
// written destination without a scratch buffer. Called only while loading.
// The allocator parameter lets host tests exercise both allocation failures.
template<class Source,class Allocate=AllocateSkinRepresentatives>
SkinRepresentatives buildSkinRepresentatives(const Source* source,unsigned count,Allocate allocate={}){
    static_assert(sizeof(Source)==28,"DOBJ position/normal/matrix key must have no padding");
    if(!source||!count||count>65535)return {};
    auto representative=allocate(count);
    if(!representative)return {};
    auto order=allocate(count);
    if(!order)return {};
    for(unsigned i=0;i<count;++i)order[i]=uint16_t(i);
    std::sort(order.get(),order.get()+count,[&](uint16_t a,uint16_t b){
        const int comparison=std::memcmp(source+a,source+b,sizeof(Source));
        return comparison?comparison<0:a<b;
    });
    unsigned unique=0;
    for(unsigned at=0;at<count;){
        const uint16_t first=order[at++];
        representative[first]=first;++unique;
        while(at<count&&!std::memcmp(source+first,source+order[at],sizeof(Source)))
            representative[order[at++]]=first;
    }
    // As for SharedPositions, retain the map only when at least one quarter
    // of transforms disappear. Both arrays free on failure or rejection.
    if(unique*4>count*3)return {};
    return representative;
}
}
