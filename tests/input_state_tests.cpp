#include "app/input_state.hpp"
#include "app/flight_physics.hpp"
#include "app/raw_mouse.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    using tunrun::ButtonEdge;
    using tunrun::Screen;
    using tunrun::ScreenStack;

    ButtonEdge edge;
    assert(!edge.update(false));
    assert(edge.update(true));
    assert(!edge.update(true));
    assert(!edge.update(true));
    assert(!edge.update(false));
    assert(edge.update(true));
    edge.reset();
    assert(!edge.update(false));
    assert(edge.update(true));

    ScreenStack stack;
    assert(stack.current() == Screen::MainMenu);
    assert(stack.size() == 1U);
    assert(!stack.pop());
    stack.push(Screen::Preview);
    stack.push(Screen::Pause);
    stack.push(Screen::Settings);
    assert(stack.current() == Screen::Settings);
    assert(stack.pop() && stack.current() == Screen::Pause);
    assert(stack.pop() && stack.current() == Screen::Preview);
    assert(stack.pop() && stack.current() == Screen::MainMenu);
    stack.push(Screen::Settings);
    stack.replace(Screen::Credits);
    assert(stack.current() == Screen::Credits);
    assert(stack.pop() && stack.current() == Screen::MainMenu);
    stack.reset();
    assert(stack.current() == Screen::MainMenu && stack.size() == 1U);

    float mouseX = 0.0F;
    float mouseY = 0.0F;
    applyRelativeMouseSteering(mouseX, mouseY, RelativeMouseDelta{100.0F, -50.0F}, 0.004F);
    assert(std::abs(mouseX - 0.4F) < 0.0001F);
    assert(std::abs(mouseY - 0.2F) < 0.0001F);
    applyRelativeMouseSteering(mouseX, mouseY, RelativeMouseDelta{5000.0F, -5000.0F}, 0.004F);
    assert(mouseX == 3.1F && mouseY == 3.1F);
    applyRelativeMouseSteering(mouseX, mouseY, RelativeMouseDelta{1.0F, 1.0F}, -1.0F);
    assert(mouseX == 3.1F && mouseY == 3.1F);

    constexpr std::uint64_t seed = 0x123456789ABCDEF0ULL;
    const auto section = tunrun::sampleCourse(seed, 1.25);
    assert(section.radius >= tunrun::kCourseMinRadius && section.radius <= tunrun::kCourseMaxRadius);
    const tunrun::TunnelCrossSection centredSection{0.0F, 0.0F, section.radius, section.twist};
    assert(!tunrun::collidesWithTunnelWall(0.0F, 0.0F, centredSection));
    const float safeRadius = section.radius - tunrun::kCraftCollisionRadius;
    assert(tunrun::collidesWithTunnelWall(safeRadius + 0.01F, 0.0F, centredSection));

    assert(tunrun::courseHash(seed) == tunrun::courseHash(seed));
    assert(tunrun::courseHash(seed) != tunrun::courseHash(seed + 1U));
    assert(tunrun::deriveCourseSeed(seed, 0U) != tunrun::deriveCourseSeed(seed, 1U));
    const auto validation = tunrun::validateCourse(seed, 3600.0);
    assert(validation.valid && validation.samplesChecked > 4000U);
    assert(validation.minimumRadius >= tunrun::kCourseMinRadius);
    assert(validation.maximumRadius <= tunrun::kCourseMaxRadius);
    for (int node = 1; node < 30; ++node) {
        const double seam = static_cast<double>(node) * tunrun::kCourseNodeSpacing;
        const auto before = tunrun::sampleCourse(seed, seam - 0.001);
        const auto after = tunrun::sampleCourse(seed, seam + 0.001);
        assert(std::abs(before.centerX - after.centerX) < 0.01F);
        assert(std::abs(before.centerY - after.centerY) < 0.01F);
        assert(std::abs(before.radius - after.radius) < 0.01F);
        assert(std::abs(before.twist - after.twist) < 0.01F);
    }
    for (std::uint64_t sampleSeed = 0; sampleSeed < 24U; ++sampleSeed) {
        assert(tunrun::validateCourse(tunrun::deriveCourseSeed(seed, sampleSeed), 1800.0).valid);
    }

    tunrun::FlightState normal;
    tunrun::FlightState precision;
    float normalAccumulator = 0.0F;
    float precisionAccumulator = 0.0F;
    const tunrun::FlightInput steer{1.0F, 0.0F, false, false};
    for (int i = 0; i < 120; ++i) {
        tunrun::advanceFlight(normal, steer, 1.0F / 60.0F, normalAccumulator);
        tunrun::advanceFlight(precision, tunrun::FlightInput{1.0F, 0.0F, false, true},
                              1.0F / 60.0F, precisionAccumulator);
    }
    assert(normal.x > precision.x);

    tunrun::FlightState boost;
    float boostAccumulator = 0.0F;
    for (int i = 0; i < 120; ++i) {
        tunrun::advanceFlight(boost, tunrun::FlightInput{1.0F, 0.0F, true, false},
                              1.0F / 120.0F, boostAccumulator);
    }
    assert(boost.boostEnergy < 100.0F && boost.boostEnergy > 50.0F);

    const auto simulateAtRenderRate = [](int framesPerSecond) {
        tunrun::FlightState state;
        float accumulator = 0.0F;
        const float frameTime = 1.0F / static_cast<float>(framesPerSecond);
        for (int frame = 0; frame < framesPerSecond * 2; ++frame) {
            tunrun::advanceFlight(state, tunrun::FlightInput{0.6F, -0.4F, false, false},
                                  frameTime, accumulator);
        }
        return state;
    };
    const auto at30 = simulateAtRenderRate(30);
    const auto at60 = simulateAtRenderRate(60);
    const auto at120 = simulateAtRenderRate(120);
    assert(std::abs(at30.x - at60.x) < 0.01F);
    assert(std::abs(at30.x - at120.x) < 0.01F);
    assert(std::abs(at30.y - at60.y) < 0.01F);
    assert(std::abs(at30.y - at120.y) < 0.01F);
    assert(std::abs(at30.distance - at60.distance) < 0.01F);
    assert(std::abs(at30.distance - at120.distance) < 0.01F);

    std::cout << "TUNRUN input, physics, and procedural-course tests passed.\n";
}
