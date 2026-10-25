// NW4R EF animation curve executors (ResAnimCurve), ported from Mario Kart Wii RMCE
// main.dol: AnimCurveExecuteU8 0x80029d80, F32 0x8002b060, Rotate 0x8002c010, Texture
// 0x8002d0c0, Child 0x8002d800 (key creation 0x8002d690), baked U8 0x8002e5c0 and baked
// F32 0x8002e910. Curves stay big endian; see docs/VFX_RESEARCH.md for the layout.
#include "mkw/nw4r_ef_internal.hpp"
#include <cmath>

namespace mkw::ef {
namespace {
constexpr u8 ProcSync=0x04,ProcInfLoop=0x20,ProcTurn=0x40,ProcFit=0x80;
constexpr float Epsilon=1.1920928955078125e-07f;

u32 popCount(u8 mask){u32 n=0;for(;mask;mask&=mask-1)++n;return n;}
u32 topBit(u8 mask){u32 b=0;for(u32 bit=1;bit<=mask;bit<<=1)if(mask&bit)b=bit;return b;}
u32 lcg(u32 r){return r*0x343fdu+0x269ec3u;}
// __cvt_fp2unsigned
u32 toU32(float f){return f<=0.f?0u:f>=4294967296.f?0xffffffffu:u32(f);}

const u8* rangeTable(const u8* c){return c+0x20+be32(c+0xc);}
const u8* randomTable(const u8* c){return c+0x20+be32(c+0xc)+be32(c+0x10);}
const u8* nameTable(const u8* c){return randomTable(c)+be32(c+0x14);}

// Frame position of keyed U8/F32/Rotate curves: integer frame, exact position, loop count.
struct Position {u32 frame;float f;u32 loop;};
Position keyedPosition(const u8* c,u32 tick,u32 life){
    const u32 frames=be16(c+8);const u8 proc=c[4],loops=c[5];
    u32 frame=0,loop=0;float f=0;
    if(frames<2){loop=tick;}
    else if(!(proc&ProcInfLoop)&&loops<2){
        if(!(proc&ProcFit)){frame=std::min(tick,frames-1)&0xffff;f=float(frame);}
        else{
            const float last=float(int(frames-1));
            if(life==1)f=last;
            else{f=float(tick)*(last/float(life-1));if(last<f)f=last;}
            frame=u32(f);
        }
    }else if(!(proc&ProcFit)){
        const u32 n=frames-1;loop=tick/n;
        if(!(proc&ProcTurn)){
            if(!(proc&ProcInfLoop)&&loops<=loop){frame=n&0xffff;loop=(loops-1)&0xff;}
            else frame=(tick-loop*n)&0xffff;
        }else if(!(proc&ProcInfLoop)&&loops<=loop){frame=(loops&1)?n&0xffff:0;loop=(loops-1)&0xff;}
        else if(!(loop&1))frame=(tick-loop*n)&0xffff;
        else frame=(n*(loop+1)-tick)&0xffff;
        f=float(frame);
    }else if(tick<life-1){
        const u32 n=frames-1;
        const float d=float(loops)*(float(int(n))/float(life-1));
        const float scaled=float(tick)*d;
        loop=toU32(scaled/float(int(n)));
        f=scaled-float(loop*n);
        if((proc&ProcTurn)&&(loop&1))f=float(int(n))-f;
        frame=u32(f);
    }else{
        frame=(!(proc&ProcTurn)||(loops&1))?(frames-1)&0xffff:0;
        loop=(loops-1)&0xff;f=float(frame);
    }
    return {frame&0xffff,f,loop};
}

// Key search shared by the keyed executors: one key, or the pair to interpolate.
struct Found {int index;bool single;u32 lower,upper;};
Found findKey(const u8* c,u32 keySize,u32 frame,float f){
    const u8* keys=c+0x24;
    auto at=[&](int i){return u32(be16(keys+i*keySize));};
    int hi=int(be16(c+0x20))-1,mid=hi/2,lo=0;
    bool single=std::fabs(float(int(frame))-f)<Epsilon;
    u32 lower=at(0),upper=0;
    if(frame<lower){single=true;mid=0;}
    else if(frame==lower){mid=0;if(hi==0)single=true;else if(!single)upper=at(1);}
    else{
        upper=at(hi);
        if(frame<upper){
            u32 k=at(mid);
            while(lo<mid){
                if(frame==k){if(!single){upper=at(mid+1);lower=k;}return {mid,single,lower,upper};}
                u32 nextUpper=k;int nextHi=mid;
                if(k<frame){lo=mid;nextUpper=upper;lower=k;nextHi=hi;}
                mid=(lo+nextHi)/2;k=at(mid);upper=nextUpper;hi=nextHi;
            }
            single=false;mid=lo;
        }else{single=true;mid=hi;}
    }
    return {mid,single,lower,upper};
}

// Loop indices the two keys of a ping-pong pair hash with.
void turnLoops(const u8* c,const Found& k,u32 loop,u32& first,u32& second){
    first=second=loop;
    const u8 proc=c[4],loops=c[5];
    if((proc&ProcTurn)&&((proc&ProcInfLoop)||loops>1)){
        if(!(loop&1)&&int(be16(c+0x20))-1<=k.index+1&&((proc&ProcInfLoop)||loop<u32(loops)-1))second=loop+1;
        if((loop&1)&&k.index==0&&loop!=0)first=loop+1;
    }
}

// The range or random-table entry a random key draws from; advances r for the latter.
const u8* randomEntry(const u8* c,const u8* key,u32 entrySize,u32& r){
    if(!(key[2]&2))return rangeTable(c)+4+be16(key+0xc)*entrySize;
    const u8* t=randomTable(c);
    const u32 count=be16(t);
    const u8* e=t+4+entrySize*(count?(r>>16)%count:0);
    r=lcg(r);
    return e;
}

// Component values by bit position (index = bit number).
void randomU8(const u8* c,const u8* key,u16 seed,u32 loop,u8* out){
    const u8 mask=c[3],proc=c[4];const u32 top=topBit(mask);
    u32 r=curveHash(seed,be16(c+6),loop,be16(key+0xc));
    const u8* e=randomEntry(c,key,popCount(mask)*2,r);
    int bit=0;
    for(u32 m=1;m<=top;m<<=1,++bit){
        if(mask&m){int v=int(e[0])+int(e[1])*int(s16(r>>16))/32768;out[bit]=u8(v<0?0:v>255?255:v);e+=2;}
        if(!(proc&ProcSync))r=lcg(r);
    }
}
void randomF32(const u8* c,const u8* key,u16 seed,u32 loop,float* out){
    const u8 mask=c[3],proc=c[4];const u32 top=topBit(mask);
    u32 r=curveHash(seed,be16(c+6),loop,be16(key+0xc));
    const u8* e=randomEntry(c,key,popCount(mask)*8,r);
    int bit=0;
    for(u32 m=1;m<=top;m<<=1,++bit)if(mask&m){
        out[bit]=bef(e)+bef(e+4)*float(r>>16);e+=8;
        if(!(proc&ProcSync))r=lcg(r);
    }
}
// Rotation ranges end with a sign flag: the values use the next random and are
// negated as a whole when bit 16 of the key's random is clear.
void randomRotate(const u8* c,const u8* key,u16 seed,u32 loop,float* out){
    const u8 mask=c[3],proc=c[4];const u32 top=topBit(mask),n=popCount(mask);
    u32 r=curveHash(seed,be16(c+6),loop,be16(key+0xc));
    const u8* e=randomEntry(c,key,n*8+4,r);
    const bool sign=e[n*8]!=0;
    u32 v=sign?lcg(r):r;
    int bit=0;
    for(u32 m=1;m<=top;m<<=1,++bit){
        if(mask&m){float x=bef(e)+bef(e+4)*float(v>>16);if(sign&&!(r&0x10000))x=-x;out[bit]=x;e+=8;}
        if(!(proc&ProcSync))v=lcg(v);
    }
}

u8 interpolateU8(u8 a,u8 b,u8 type,u32 t16){
    switch(type&3){
    case 0:return u8(a+u8(u32(t16*u32(int(b)-int(a)))>>16));
    case 1:{
        const float t=float(t16)/65536.f,d=float(int(b)-int(a));
        if(type==1)return u8(int(float(a)+t*t*(3.f-2.f*t)*d));
        const float c5=(type>>2&1)?1.5f:0.f,c4=(type>>3&1)?1.5f:0.f;
        const float v=float(a)+t*d*(c4+t*t*((c4+c5)-2.f)+t*(3.f+(-2.f*c4-c5)));
        return v<0.f?0:v>255.f?255:u8(int(v));
    }
    case 2:return a;
    default:return 0;
    }
}
float interpolateF32(float a,float b,u8 type,float t){
    switch(type&3){
    case 0:return a+t*(b-a);
    case 1:{
        if(type==1)return a+t*t*(3.f-2.f*t)*(b-a);
        const float c4=(type>>3&1)?1.5f:0.f,c2=(type>>2&1)?1.5f:0.f;
        return a+t*(b-a)*(c4+t*t*((c4+c2)-2.f)+t*(3.f+(-2.f*c4-c2)));
    }
    case 2:return a;
    default:return 0.f;
    }
}

// Baked (0xAB) tables: one row of components per frame.
u32 bakedFrame(const u8* c,u32 tick,u32 life){
    const u32 frames=be16(c+8);
    if(frames<2)return 0;
    const u8 proc=c[4],loops=c[5];
    if(!(proc&ProcInfLoop)&&loops<2){
        if(!(proc&ProcFit))return tick<frames-1?tick&0xffff:(frames-1)&0xffff;
        if(life==1)return (frames-1)&0xffff;
        const float last=float(int(frames-1));
        float f=float(tick)*(last/float(life-1));if(last<f)f=last;
        return u32(f)&0xffff;
    }
    if(!(proc&ProcFit)){
        const u32 n=frames-1,loop=tick/n;
        if(!(proc&ProcTurn))return (!(proc&ProcInfLoop)&&loops<=loop)?n&0xffff:(tick-loop*n)&0xffff;
        if(!(proc&ProcInfLoop)&&loops<=loop)return (loops&1)?n&0xffff:0;
        return !(loop&1)?(tick-loop*n)&0xffff:(n*(loop+1)-tick)&0xffff;
    }
    if(tick<life-1){
        const u32 n=frames-1;
        const float d=float(loops)*(float(int(n))/float(life-1)),scaled=float(tick)*d;
        const u32 loop=toU32(scaled/float(int(n)));
        const float f=scaled-float(loop*n);
        if(!(proc&ProcTurn))return u32(f)&0xffff;
        return ((loop&1)?u32(float(int(n))-f):u32(f))&0xffff;
    }
    return (!(proc&ProcTurn)||(loops&1))?(frames-1)&0xffff:0;
}
}

u32 curveHash(u32 seed,u16 curveSeed,u32 loop,u32 index){
    const u32 h=seed*0x3f81f635u+u32(curveSeed)*0x30a74193u+loop*0x7b929u+index*0x371097e7u+0x4bf53u;
    u8 b0=u8(h>>24),b1=u8(h>>16),b2=u8(h>>8);const u8 b3=u8(h);
    b2^=b3;b1^=b2;b0^=b1;
    return u32(b0)<<24|u32(b1)<<16|u32(b2)<<8|b3;
}

void animU8(const u8* c,u8* target,u32 tick,u16 seed,u32 life){
    const u8 mask=c[3];const u32 n=popCount(mask),top=topBit(mask);
    if(c[0]==0xab){
        if(!n)return;
        const u8* v=c+0x20+n*bakedFrame(c,tick,life);
        for(u32 m=1;m<=top;m<<=1,++target)if(mask&m)*target=*v++;
        return;
    }
    if(!n)return;
    const Position pos=keyedPosition(c,tick,life);
    const u32 keySize=(n+13)&~1u;
    const Found k=findKey(c,keySize,pos.frame,pos.f);
    const u8* key=c+0x24+k.index*keySize;
    u8 a[8]{},b[8]{};
    if(k.single){
        if(!key[2]){const u8* v=key+0xc;for(u32 m=1;m<=top;m<<=1,++target)if(mask&m)*target=*v++;return;}
        randomU8(c,key,seed,pos.loop,a);
        int bit=0;for(u32 m=1;m<=top;m<<=1,++bit)if(mask&m)target[bit]=a[bit];
        return;
    }
    u32 loopA,loopB;turnLoops(c,k,pos.loop,loopA,loopB);
    const u32 t16=toU32(65536.f*(pos.f-float(k.lower)))/((k.upper&0xffff)-k.lower);
    const u8* next=key+keySize;
    const bool randomA=key[2]!=0,randomB=next[2]!=0;
    auto fixed=[&](const u8* q,u8* out){const u8* v=q+0xc;int bit=0;for(u32 m=1;m<=top;m<<=1,++bit)if(mask&m)out[bit]=*v++;};
    if(randomA)randomU8(c,key,seed,loopA,a);else fixed(key,a);
    if(randomB)randomU8(c,next,seed,loopB,b);else fixed(next,b);
    const u8* type=key+4;
    int bit=0;
    for(u32 m=1;m<=top;m<<=1,++bit)if(mask&m)
        target[bit]=(!randomA&&!randomB&&a[bit]==b[bit])?a[bit]:interpolateU8(a[bit],b[bit],type[bit],t16);
}

namespace {
template<bool Rotate> void keyedF32(const u8* c,float* target,u32 tick,u16 seed,u32 life){
    const u8 mask=c[3];const u32 n=popCount(mask),top=topBit(mask);
    const Position pos=keyedPosition(c,tick,life);
    if(!n)return;
    const u32 keySize=n*4+12;
    const Found k=findKey(c,keySize,pos.frame,pos.f);
    const u8* key=c+0x24+k.index*keySize;
    float a[8]{},b[8]{};
    auto random=[&](const u8* q,u32 loop,float* out){if(Rotate)randomRotate(c,q,seed,loop,out);else randomF32(c,q,seed,loop,out);};
    auto fixed=[&](const u8* q,float* out){const u8* v=q+0xc;int bit=0;for(u32 m=1;m<=top;m<<=1,++bit)if(mask&m){out[bit]=bef(v);v+=4;}};
    if(k.single){
        if(key[2])random(key,pos.loop,a);else fixed(key,a);
        int bit=0;for(u32 m=1;m<=top;m<<=1,++bit)if(mask&m)target[bit]=a[bit];
        return;
    }
    u32 loopA,loopB;turnLoops(c,k,pos.loop,loopA,loopB);
    const float t=(pos.f-float(k.lower))/float(int((k.upper&0xffff)-k.lower));
    const u8* next=key+keySize;
    const bool randomA=key[2]!=0,randomB=next[2]!=0;
    if(randomA)random(key,loopA,a);else fixed(key,a);
    if(randomB)random(next,loopB,b);else fixed(next,b);
    const u8* type=key+4;
    int bit=0;
    for(u32 m=1;m<=top;m<<=1,++bit)if(mask&m)
        target[bit]=(!randomA&&!randomB&&a[bit]==b[bit])?a[bit]:interpolateF32(a[bit],b[bit],type[bit],t);
}
}

void animF32(const u8* c,float* target,u32 tick,u16 seed,u32 life){
    if(c[0]==0xab){
        const u8 mask=c[3];const u32 n=popCount(mask),top=topBit(mask);
        if(!n)return;
        const u8* v=c+0x20+n*bakedFrame(c,tick,life)*4;
        for(u32 m=1;m<=top;m<<=1,++target)if(mask&m){*target=bef(v);v+=4;}
        return;
    }
    keyedF32<false>(c,target,tick,seed,life);
}
void animRotate(const u8* c,float* target,u32 tick,u16 seed,u32 life){keyedF32<true>(c,target,tick,seed,life);}

void animTexture(const u8* c,Particle& particle,u32 tick,u16 seed,u32 life){
    const u8 proc=c[4],loops=c[5];const u32 frames=be16(c+8);
    u32 frame=0,loop=0;
    if(!(proc&ProcInfLoop)&&loops<2){
        if(!(proc&ProcFit))frame=tick<frames?tick&0xffff:frames;
        else frame=(frames*tick)/life&0xffff;
    }else if(!(proc&ProcFit)){
        frame=frames;
        if(frames<2||!(proc&ProcTurn)){
            loop=tick/frames;
            if(!(proc&ProcInfLoop)&&loops<=loop)loop=(loops-1)&0xff;
            else frame=(tick-loop*frames)&0xffff;
        }else{
            const u32 n=frames-1;loop=tick/n;
            if(!(proc&ProcInfLoop)&&loops<=loop){frame=(loops&1)?n&0xffff:0;loop=(loops-1)&0xff;}
            else if(!(loop&1))frame=(tick-loop*n)&0xffff;
            else frame=(n*(loop+1)-tick)&0xffff;
        }
    }else if(tick<life){
        frame=frames;
        if(frames<2||!(proc&ProcTurn)){
            const float d=float(loops)*(float(frames)/float(life)),scaled=float(tick)*d;
            loop=toU32(scaled/float(frames));
            frame=u32(scaled-float(loop*frames));
        }else{
            const u32 n=frames-1;
            const u32 t=toU32(float(tick)*((1.f+float(int(n))*float(loops))/float(life)));
            loop=t/n;
            if(loop<loops)frame=(loop&1)?(n*(loop+1)-t)&0xffff:(t-loop*n)&0xffff;
            else{frame=(loops&1)?n&0xffff:0;loop=(loops-1)&0xff;}
        }
    }else{
        frame=(!(proc&ProcTurn)||(loops&1))?frames:0;
        loop=(loops-1)&0xff;
    }
    int layer;
    switch(c[1]){case 0x68:layer=0;break;case 0x6c:layer=1;break;case 0x70:layer=2;break;default:return;}
    frame&=0xffff;
    const u8* keys=c+0x24;
    auto at=[&](int i){return u32(be16(keys+i*16));};
    const int last=int(be16(c+0x20))-1;
    int index=0;
    if(frame>=at(0)){
        index=last;
        if(frame<at(last)){
            int lo=0,hi=last,mid=last/2;u32 k=at(mid);
            while(lo<mid){if(frame>=k)lo=mid;else hi=mid;mid=(lo+hi)/2;k=at(mid);}
            index=lo;
        }
    }
    const u8* key=keys+index*16;
    u32 wrap,reverse,name;
    if(!key[2]){wrap=key[0xc];reverse=key[0xd];name=be16(key+0xe);}
    else{
        const u32 r=curveHash(seed,be16(c+6),loop,be16(key+0xc));
        const u8 low=u8(r>>16);
        if(!(key[2]&2)){
            const u8* e=rangeTable(c)+4+be16(key+0xc)*8;
            wrap=e[0];reverse=e[1];name=be16(e+2);
            switch(e[4]){case 1:reverse=(reverse&2)|(low&1);break;case 2:reverse=(reverse&1)|(low&2);break;case 3:reverse=low&3;break;}
        }else{
            const u8* t=randomTable(c);const u32 count=be16(t);
            const u32 i=count?(r>>16)%count:0;
            wrap=t[4+i*4];reverse=t[5+i*4];name=be16(t+6+i*4);
        }
    }
    particle.p.texture[layer]=be32(nameTable(c)+4+name*4);
    particle.p.textureWrap=u16((particle.p.textureWrap&~(0xf<<(layer*4)))|((wrap&0xf)<<(layer*4)));
    particle.p.textureReverse=u8((particle.p.textureReverse&~(3<<(layer*2)))|((reverse&3)<<(layer*2)));
}

namespace {
// Child curve frame at a tick; atEnd marks a ping-pong curve returning to frame 0 on its
// last loop exactly at this tick, which still fires the keys there.
void childFrame(const u8* c,u32 tick,u32 life,bool first,u32& frame,u32& loop,bool& atEnd){
    const u8 proc=c[4],loops=c[5];const u32 frames=be16(c+8);
    loop=0;atEnd=false;
    if(!(proc&ProcInfLoop)&&loops<2){
        if(!(proc&ProcFit))frame=tick<frames?tick&0xffff:frames;
        else frame=(frames*tick)/life&0xffff;
    }else if(!(proc&ProcFit)){
        frame=frames;
        if(frames<2||!(proc&ProcTurn)){
            loop=tick/frames;
            if(!(proc&ProcInfLoop)&&loops<=loop)loop=(loops-1)&0xff;
            else frame=(tick-loop*frames)&0xffff;
        }else{
            const u32 n=frames-1;loop=tick/n;
            if(!(proc&ProcInfLoop)&&loops<=loop){
                if(!(loops&1)){if(first&&loop==loops&&tick==loop*n)atEnd=true;frame=0;}
                else frame=n&0xffff;
                loop=(loops-1)&0xff;
            }else if(!(loop&1))frame=(tick-loop*n)&0xffff;
            else frame=(n*(loop+1)-tick)&0xffff;
        }
    }else if(tick<life){
        frame=frames;
        if(frames<2||!(proc&ProcTurn)){
            const float d=float(loops)*(float(frames)/float(life)),scaled=float(tick)*d;
            loop=toU32(scaled/float(frames));
            frame=u32(scaled-float(loop*frames));
        }else{
            const u32 n=frames-1;
            const u32 t=toU32(float(tick)*((1.f+float(int(n))*float(loops))/float(life)));
            loop=t/n;
            if(loop<loops)frame=(loop&1)?(n*(loop+1)-t)&0xffff:(t-loop*n)&0xffff;
            else{
                const bool odd=loops&1;
                frame=odd?n&0xffff:0;
                if(first)atEnd=!odd&&tick==life-1;
                loop=(loops-1)&0xff;
            }
        }
    }else{
        frame=(!(proc&ProcTurn)||(loops&1))?frames:0;
        loop=(loops-1)&0xff;
    }
}

// First key with frame >= f (forward search of 0x8002d800).
int lowerKey(const u8* keys,int count,u32 f){
    auto at=[&](int i){return u32(be16(keys+i*0x18));};
    int hi=count-1,mid=hi/2;
    if(f<=at(0))return 0;
    if(f>at(hi))return hi;
    int lo=0;u32 k=at(mid);
    while(lo<mid){
        int nlo=lo,nhi=mid;
        if(f!=k&&f>k){nlo=mid;nhi=hi;}
        mid=(nlo+nhi)/2;k=at(mid);lo=nlo;hi=nhi;
    }
    return lo;
}
// Last key with frame <= f (backward search).
int upperKey(const u8* keys,int count,u32 f){
    auto at=[&](int i){return u32(be16(keys+i*0x18));};
    int hi=count-1,mid=hi/2;
    if(f<at(0))return 0;
    if(!(f<at(hi)))return hi;
    int lo=0;u32 k=at(mid);
    while(lo<mid){
        int nlo=mid;
        if(f!=k&&f<=k){nlo=lo;hi=mid;}
        mid=(nlo+hi)/2;k=at(mid);lo=nlo;
    }
    return lo;
}
}

void animChild(const u8* c,Particle& particle,u32 tick,u16 seed,u32 life){
    const u8 proc=c[4];const u32 frames=be16(c+8);
    const u8* keys=c+0x24;const int count=be16(c+0x20);
    auto at=[&](int i){return u32(be16(keys+i*0x18));};
    u32 frameA,loopA,frameB,loopB;bool atEnd,unused;
    childFrame(c,tick,life,true,frameA,loopA,atEnd);
    childFrame(c,tick+1,life,false,frameB,loopB,unused);
    auto fire=[&](int i){childKey(c,keys+i*0x18,seed,particle,loopA);};
    if(loopA==loopB){
        if((frameA&0xffff)==(frameB&0xffff)){if(!atEnd)return;frameB=1;}
        const u32 a=frameA&0xffff,b=frameB&0xffff;
        if(a<b){
            for(int i=lowerKey(keys,count,a);i<count;++i)if(a<=at(i)){if(b<=at(i))break;fire(i);}
        }else{
            for(int i=upperKey(keys,count,a);i>=0;--i)if(at(i)<=a){if(at(i)<=b)break;fire(i);}
        }
        return;
    }
    const u32 a=frameA&0xffff,b=frameB&0xffff;
    if(!(proc&ProcTurn)){
        for(int i=lowerKey(keys,count,a);i<count;++i)if(a<=at(i))fire(i);
        for(u32 l=loopA+1;l<loopB;++l)for(int i=0;i<count;++i)fire(i);
        for(int i=0;i<count&&at(i)<b;++i)fire(i);
        return;
    }
    if(!(loopA&1)){
        for(int i=lowerKey(keys,count,a);i<count;++i)if(a<=at(i)){if(at(i)==frames-1)break;fire(i);}
    }else{
        for(int i=upperKey(keys,count,a);i>=0;--i)if(at(i)<=a){if(at(i)==0)break;fire(i);}
    }
    for(u32 l=loopA+1;l<loopB;++l){
        if(!(loopA&1)){for(int i=0;i<count&&at(i)!=frames-1;++i)fire(i);}
        else{for(int i=count-1;i>=0&&at(i)!=0;--i)fire(i);}
    }
    if(!(loopB&1)){for(int i=0;i<count&&at(i)<b;++i)fire(i);}
    else{for(int i=count-1;i>=0&&b<at(i);--i)fire(i);}
}
}
