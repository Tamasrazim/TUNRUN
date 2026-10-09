#include "app/input_state.hpp"
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

    std::cout << "TUNRUN input-state tests passed.\n";
}
