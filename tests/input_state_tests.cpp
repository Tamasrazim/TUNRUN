#include "app/input_state.hpp"
#include <cassert>
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

    std::cout << "TUNRUN input-state tests passed.\n";
}
