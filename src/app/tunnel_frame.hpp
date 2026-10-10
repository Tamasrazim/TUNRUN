#pragma once

#include "app/procedural_course.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tunrun {
struct FrameVector3 { float x=0.0F; float y=0.0F; float z=0.0F; };
struct TunnelFrame {
    FrameVector3 center, tangent, right, up;
    float radius=5.75F;
    float twist=0.0F;
};
[[nodiscard]] inline FrameVector3 frameAdd(FrameVector3 a, FrameVector3 b) noexcept {
    return {a.x+b.x,a.y+b.y,a.z+b.z};
}
[[nodiscard]] inline FrameVector3 frameScale(FrameVector3 a,float k) noexcept {
    return {a.x*k,a.y*k,a.z*k};
}
[[nodiscard]] inline float frameDot(FrameVector3 a,FrameVector3 b) noexcept {
    return a.x*b.x+a.y*b.y+a.z*b.z;
}
[[nodiscard]] inline FrameVector3 frameCross(FrameVector3 a,FrameVector3 b) noexcept {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
[[nodiscard]] inline float frameLength(FrameVector3 v) noexcept {
    return std::sqrt(std::max(0.0F,frameDot(v,v)));
}
[[nodiscard]] inline FrameVector3 frameNormalize(
    FrameVector3 v,FrameVector3 fallback) noexcept {
    const float n=frameLength(v);
    if(!std::isfinite(n)||n<1.0e-5F) return fallback;
    return frameScale(v,1.0F/n);
}
// Course distance increases toward -Z in renderer space. Derive the tangent
// from the same deterministic centreline sampler used by course validation.
[[nodiscard]] inline TunnelFrame sampleTunnelFrame(
    std::uint64_t seed,double playerDistance,double frameDistance) noexcept {
    if(!std::isfinite(playerDistance)) playerDistance=0.0;
    if(!std::isfinite(frameDistance)) frameDistance=playerDistance;
    playerDistance=std::clamp(playerDistance,-18000000.0,18000000.0);
    frameDistance=std::clamp(frameDistance,-18000000.0,18000000.0);
    constexpr double h=0.75;
    const auto player=sampleCourse(seed,playerDistance);
    const auto section=sampleCourse(seed,frameDistance);
    const auto before=sampleCourse(seed,frameDistance-h);
    const auto after=sampleCourse(seed,frameDistance+h);
    const FrameVector3 tangent=frameNormalize(
        {after.centerX-before.centerX,after.centerY-before.centerY,
         static_cast<float>(-2.0*h)},{0.0F,0.0F,-1.0F});
    const FrameVector3 worldUp{0.0F,1.0F,0.0F};
    const FrameVector3 right0=frameNormalize(frameCross(tangent,worldUp),{1.0F,0.0F,0.0F});
    const FrameVector3 up0=frameNormalize(frameCross(right0,tangent),worldUp);
    const float c=std::cos(section.twist),s=std::sin(section.twist);
    const FrameVector3 right=frameAdd(frameScale(right0,c),frameScale(up0,s));
    const FrameVector3 up=frameAdd(frameScale(up0,c),frameScale(right0,-s));
    const double longitudinal=std::clamp(frameDistance-playerDistance,-1000000.0,1000000.0);
    return TunnelFrame{
        {section.centerX-player.centerX,section.centerY-player.centerY,
         static_cast<float>(-longitudinal)},
        tangent,right,up,section.radius,section.twist};
}
[[nodiscard]] inline FrameVector3 tunnelFramePoint(
    const TunnelFrame& f,float rightOffset,float upOffset,float forwardOffset=0.0F) noexcept {
    return frameAdd(f.center,frameAdd(frameScale(f.right,rightOffset),
        frameAdd(frameScale(f.up,upOffset),frameScale(f.tangent,forwardOffset))));
}
} // namespace tunrun
