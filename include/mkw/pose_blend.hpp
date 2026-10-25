#pragma once
#include <cmath>
#include <algorithm>

namespace mkw {
// Blend rigid bone rotations, not already-skinned head/limb vertices. Linear
// vertex crossfades shrink rotating parts midway through a pose change.
inline void blendPosePair(float* out,const float* a,const float* b,float t){
    struct Pose {float q[4],s[3],r[9];};
    auto decompose=[](const float* m,Pose& p){
        for(unsigned c=0;c<3;++c){
            p.s[c]=std::sqrt(m[c]*m[c]+m[4+c]*m[4+c]+m[8+c]*m[8+c]);
            if(p.s[c]<1e-6f)return false;
            for(unsigned r=0;r<3;++r)p.r[r*3+c]=m[r*4+c]/p.s[c];
        }
        auto* r=p.r;
        for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j)
            if(std::fabs(r[i]*r[j]+r[3+i]*r[3+j]+r[6+i]*r[6+j])>0.001f)return false;
        const float det=r[0]*(r[4]*r[8]-r[5]*r[7])-r[1]*(r[3]*r[8]-r[5]*r[6])+r[2]*(r[3]*r[7]-r[4]*r[6]);
        if(det<0){p.s[0]=-p.s[0];for(unsigned i=0;i<3;++i)r[i*3]=-r[i*3];}
        const float trace=r[0]+r[4]+r[8];
        if(trace>0){const float s=2*std::sqrt(trace+1);p.q[3]=s/4;p.q[0]=(r[7]-r[5])/s;p.q[1]=(r[2]-r[6])/s;p.q[2]=(r[3]-r[1])/s;}
        else{
            unsigned i=r[0]>r[4]?(r[0]>r[8]?0:2):(r[4]>r[8]?1:2),j=(i+1)%3,k=(i+2)%3;
            const float s=2*std::sqrt(std::max(0.f,1+r[i*3+i]-r[j*3+j]-r[k*3+k]));
            if(s<1e-6f)return false;
            p.q[i]=s/4;p.q[j]=(r[j*3+i]+r[i*3+j])/s;p.q[k]=(r[k*3+i]+r[i*3+k])/s;p.q[3]=(r[k*3+j]-r[j*3+k])/s;
        }
        return true;
    };
    Pose pa{},pb{};
    if(!decompose(a,pa)||!decompose(b,pb)){
        // Preserve authored shear in weighted envelope matrices.
        for(unsigned i=0;i<21;++i)out[i]=a[i]+(b[i]-a[i])*t;
        return;
    }
    float dot=0;for(unsigned i=0;i<4;++i)dot+=pa.q[i]*pb.q[i];
    float q[4],norm=0;
    for(unsigned i=0;i<4;++i){q[i]=pa.q[i]*(1-t)+pb.q[i]*(dot<0?-t:t);norm+=q[i]*q[i];}
    norm=std::sqrt(norm);for(float& v:q)v/=norm;
    const float x=q[0],y=q[1],z=q[2],w=q[3];
    const float r[]={1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w),2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w),2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)};
    for(unsigned row=0;row<3;++row){
        out[row*4+3]=a[row*4+3]+(b[row*4+3]-a[row*4+3])*t;
        for(unsigned col=0;col<3;++col){
            const float scale=pa.s[col]+(pb.s[col]-pa.s[col])*t;
            out[row*4+col]=r[row*3+col]*scale;
            out[12+row*3+col]=std::fabs(scale)>1e-6f?r[row*3+col]/scale:0;
        }
    }
}
}
