#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace dingosdk::game::orientation_target {
using Matrix = std::array<float,16>;
using Quaternion = std::array<double,4>;

// Rotation rows use the same convention as LinearTransform. Translation and
// SIMD fourth lanes are not part of the angular constraint.
inline bool basis_valid(const Matrix& m) noexcept {
    for(unsigned a=0;a<3;++a)for(unsigned b=0;b<=a;++b) {
        double dot{};
        for(unsigned k=0;k<3;++k)dot+=double(m[4*a+k])*m[4*b+k];
        if(!std::isfinite(dot) || std::abs(dot-(a==b?1.:0.))>.001)return false;
    }
    const double determinant=double(m[0])*(double(m[5])*m[10]-double(m[6])*m[9])-
        double(m[1])*(double(m[4])*m[10]-double(m[6])*m[8])+
        double(m[2])*(double(m[4])*m[9]-double(m[5])*m[8]);
    return std::isfinite(determinant) && determinant>0;
}
inline Matrix compose_basis(const Matrix& inner,const Matrix& outer) noexcept {
    Matrix result=inner;
    for(unsigned row=0;row<3;++row)for(unsigned axis=0;axis<3;++axis) {
        result[4*row+axis]=0;
        for(unsigned k=0;k<3;++k)result[4*row+axis]+=inner[4*row+k]*outer[4*k+axis];
    }
    return result;
}
inline Matrix inverse_basis(Matrix m) noexcept {
    for(unsigned row=0;row<3;++row)for(unsigned axis=0;axis<row;++axis)
        std::swap(m[4*row+axis],m[4*axis+row]);
    return m;
}
// Matches the native predicate: the largest angle between corresponding axes.
inline float error(const Matrix& actual,const Matrix& target) noexcept {
    float minimum=1;
    for(unsigned row=0;row<3;++row) {
        float dot{};
        for(unsigned axis=0;axis<3;++axis)dot+=actual[4*row+axis]*target[4*row+axis];
        minimum=std::min(minimum,dot);
    }
    return std::acos(std::clamp(minimum,-1.f,1.f));
}
inline Quaternion quaternion(const Matrix& m) noexcept {
    // Transpose the rotation rows to the conventional column matrix formula.
    const auto at=[&](unsigned r,unsigned c){return double(m[4*c+r]);};
    Quaternion q{};
    const double trace=at(0,0)+at(1,1)+at(2,2);
    if(trace>0) {
        const double s=2*std::sqrt(trace+1);
        q={(at(2,1)-at(1,2))/s,(at(0,2)-at(2,0))/s,(at(1,0)-at(0,1))/s,s/4};
    } else {
        unsigned i=0;
        if(at(1,1)>at(i,i))i=1;
        if(at(2,2)>at(i,i))i=2;
        const unsigned j=(i+1)%3,k=(i+2)%3;
        const double s=2*std::sqrt(1+at(i,i)-at(j,j)-at(k,k));
        q[i]=s/4;q[j]=(at(j,i)+at(i,j))/s;q[k]=(at(k,i)+at(i,k))/s;
        q[3]=(at(k,j)-at(j,k))/s;
    }
    double length{};for(double x:q)length+=x*x;
    length=std::sqrt(length)*(q[3]<0?-1:1);
    for(double& x:q)x/=length;
    return q;
}
inline Matrix rotation(Quaternion q) noexcept {
    const auto [x,y,z,w]=q;
    Matrix m{};
    m[0]=float(1-2*(y*y+z*z));m[1]=float(2*(x*y+z*w));m[2]=float(2*(x*z-y*w));
    m[4]=float(2*(x*y-z*w));m[5]=float(1-2*(x*x+z*z));m[6]=float(2*(y*z+x*w));
    m[8]=float(2*(x*z+y*w));m[9]=float(2*(y*z-x*w));m[10]=float(1-2*(x*x+y*y));
    return m;
}
struct Result {float requested_error{},applied_error{},fraction{};};
// Closest point on the shortest rotation arc satisfying the supplied native
// per-axis constraint. This neither moves a body nor changes the constraint.
inline bool bound(const Matrix& actual,const Matrix& requested,float limit,
        Matrix& out,Result& report) noexcept {
    if(!basis_valid(actual) || !basis_valid(requested) || !std::isfinite(limit) ||
       !(limit>0) || limit>3.14159265358979323846f)return false;
    Result result;result.requested_error=error(actual,requested);
    if(result.requested_error<limit) {
        result.applied_error=result.requested_error;result.fraction=1;
        out=requested;report=result;return true;
    }
    const auto q=quaternion(compose_basis(inverse_basis(actual),requested));
    const double half=std::acos(std::clamp(q[3],0.,1.));
    const double sine=std::sin(half);
    if(!(sine>0) || !std::isfinite(sine))return false;
    const auto sample=[&](double fraction) {
        const double scale=std::sin(half*fraction)/sine;
        const auto turned=compose_basis(actual,rotation({q[0]*scale,q[1]*scale,q[2]*scale,std::cos(half*fraction)}));
        Matrix candidate=requested;
        for(unsigned row=0;row<3;++row)for(unsigned axis=0;axis<3;++axis)
            candidate[4*row+axis]=turned[4*row+axis];
        return candidate;
    };
    double low=0,high=1;
    Matrix candidate=sample(0);
    if(!(error(actual,candidate)<limit))return false;
    // Float output precision, not a gameplay rate or duration.
    for(unsigned bit=0;bit<24;++bit) {
        const double middle=(low+high)/2;
        const auto trial=sample(middle);
        if(error(actual,trial)<limit){low=middle;candidate=trial;}else high=middle;
    }
    if(!basis_valid(candidate))return false;
    result.applied_error=error(actual,candidate);result.fraction=float(low);
    out=candidate;report=result;return true;
}
}
