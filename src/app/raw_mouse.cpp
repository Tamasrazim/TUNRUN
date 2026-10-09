#include "app/raw_mouse.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {
RawMouse* g_instance = nullptr;
HWND g_window = nullptr;
WNDPROC g_previousProc = nullptr;

LRESULT CALLBACK tunrunRawMouseWindowProc(HWND hwnd, UINT message,
                                           WPARAM wParam, LPARAM lParam) {
    RawMouse* const mouse = g_instance;
    if (mouse != nullptr && hwnd == g_window) {
        if (message == WM_INPUT) {
            mouse->collectNativeInput(static_cast<std::intptr_t>(lParam));
        }
        if (message == WM_KILLFOCUS ||
            (message == WM_ACTIVATEAPP && wParam == FALSE)) {
            mouse->setActive(false);
        }
        if (message == WM_SETCURSOR && mouse->active() &&
            LOWORD(lParam) == HTCLIENT) {
            SetCursor(nullptr);
            return TRUE;
        }
        if (g_previousProc != nullptr) {
            return CallWindowProcW(g_previousProc, hwnd, message, wParam, lParam);
        }
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
} // namespace

RawMouse::~RawMouse() {
    uninstall();
}

bool RawMouse::install(void* nativeWindow) noexcept {
    if (installed_) return true;
    if (nativeWindow == nullptr || (g_instance != nullptr && g_instance != this)) {
        return false;
    }
    g_window = static_cast<HWND>(nativeWindow);

    SetLastError(ERROR_SUCCESS);
    const LONG_PTR previous = SetWindowLongPtrW(
        g_window, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(&tunrunRawMouseWindowProc));
    if (previous == 0 && GetLastError() != ERROR_SUCCESS) {
        g_window = nullptr;
        return false;
    }
    g_previousProc = reinterpret_cast<WNDPROC>(previous);
    g_instance = this;

    RAWINPUTDEVICE device{0x01, 0x02, RIDEV_INPUTSINK, g_window};
    if (!RegisterRawInputDevices(&device, 1, sizeof(device))) {
        if (GetWindowLongPtrW(g_window, GWLP_WNDPROC) ==
            reinterpret_cast<LONG_PTR>(&tunrunRawMouseWindowProc)) {
            SetWindowLongPtrW(g_window, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(g_previousProc));
        }
        g_instance = nullptr;
        g_previousProc = nullptr;
        g_window = nullptr;
        return false;
    }
    installed_ = true;
    return true;
}

void RawMouse::setActive(bool wanted) noexcept {
    if (!installed_) return;
    active_ = wanted && GetForegroundWindow() == g_window;
    if (active_) {
        RECT client{};
        POINT topLeft{0, 0};
        POINT bottomRight{};
        if (GetClientRect(g_window, &client)) {
            bottomRight.x = client.right;
            bottomRight.y = client.bottom;
            ClientToScreen(g_window, &topLeft);
            ClientToScreen(g_window, &bottomRight);
            const RECT bounds{topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
            ClipCursor(&bounds);
        }
        SetCursor(nullptr);
    } else {
        ClipCursor(nullptr);
        SetCursor(LoadCursorW(nullptr, IDC_ARROW));
        clear();
    }
}

RelativeMouseDelta RawMouse::consume() noexcept {
    return RelativeMouseDelta{
        static_cast<float>(deltaX_.exchange(0, std::memory_order_relaxed)),
        static_cast<float>(deltaY_.exchange(0, std::memory_order_relaxed))
    };
}

void RawMouse::clear() noexcept {
    deltaX_.store(0, std::memory_order_relaxed);
    deltaY_.store(0, std::memory_order_relaxed);
}

void RawMouse::collectNativeInput(std::intptr_t inputHandle) noexcept {
    if (!active_ || GetForegroundWindow() != g_window) return;
    RAWINPUT input{};
    UINT bytes = sizeof(input);
    const UINT copied = GetRawInputData(
        reinterpret_cast<HRAWINPUT>(inputHandle), RID_INPUT,
        &input, &bytes, sizeof(RAWINPUTHEADER));
    if (copied == static_cast<UINT>(-1) ||
        copied < sizeof(RAWINPUTHEADER) ||
        input.header.dwType != RIM_TYPEMOUSE) return;
    if ((input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0) return;
    deltaX_.fetch_add(input.data.mouse.lLastX, std::memory_order_relaxed);
    deltaY_.fetch_add(input.data.mouse.lLastY, std::memory_order_relaxed);
}

void RawMouse::uninstall() noexcept {
    if (!installed_) return;
    setActive(false);
    const RAWINPUTDEVICE removal{0x01, 0x02, RIDEV_REMOVE, nullptr};
    RegisterRawInputDevices(&removal, 1, sizeof(removal));
    if (g_window != nullptr && GetWindowLongPtrW(g_window, GWLP_WNDPROC) ==
        reinterpret_cast<LONG_PTR>(&tunrunRawMouseWindowProc)) {
        SetWindowLongPtrW(g_window, GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(g_previousProc));
    }
    if (g_instance == this) g_instance = nullptr;
    g_previousProc = nullptr;
    g_window = nullptr;
    installed_ = false;
    active_ = false;
    clear();
}

#else

RawMouse::~RawMouse() {
    uninstall();
}
bool RawMouse::install(void*) noexcept { return false; }
void RawMouse::setActive(bool) noexcept {}
RelativeMouseDelta RawMouse::consume() noexcept { return {}; }
void RawMouse::clear() noexcept {
    deltaX_.store(0, std::memory_order_relaxed);
    deltaY_.store(0, std::memory_order_relaxed);
}
void RawMouse::collectNativeInput(std::intptr_t) noexcept {}
void RawMouse::uninstall() noexcept {
    installed_ = false;
    active_ = false;
    clear();
}

#endif
