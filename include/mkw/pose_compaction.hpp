#pragma once
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
namespace mkw {
// Lossless vertex welding across an entire immutable pose bank. A vertex is
// shared only if every byte (position, normal, both UVs, color) matches at EVERY
// age. Source order and each triangle's corner order are unchanged.
template<class Vertex> std::vector<uint16_t> compactPoses(Vertex* frames,unsigned vertices,unsigned count){
 std::map<uint64_t,std::vector<unsigned>> buckets;
 std::vector<uint16_t> remap(vertices);std::vector<unsigned> representatives;representatives.reserve(vertices);
 for(unsigned v=0;v<vertices;++v){
  uint64_t hash=1469598103934665603ull;
  for(unsigned f=0;f<count;++f){auto p=reinterpret_cast<const uint8_t*>(frames+f*vertices+v);for(unsigned b=0;b<sizeof(Vertex);++b){hash^=p[b];hash*=1099511628211ull;}}
  auto& candidates=buckets[hash];bool found=false;
  for(auto candidate:candidates){bool same=true;unsigned original=representatives[candidate];
   for(unsigned f=0;f<count&&same;++f)same=std::memcmp(frames+f*vertices+v,frames+f*vertices+original,sizeof(Vertex))==0;
   if(same){remap[v]=candidate;found=true;break;}
  }
  if(!found){remap[v]=representatives.size();candidates.push_back(representatives.size());representatives.push_back(v);}
 }
 // Representatives are increasing source indices. Every destination precedes
 // its source, so forward compaction cannot overwrite an unread vertex.
 for(unsigned f=0;f<count;++f)for(unsigned v=0;v<representatives.size();++v)
  frames[f*representatives.size()+v]=frames[f*vertices+representatives[v]];
 return remap;
}
}
