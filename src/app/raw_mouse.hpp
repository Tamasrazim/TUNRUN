#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>

struct RelativeMouseDelta {
    float x = 0.0F;
    float y = 0.0F;
};

inline void applyRelativeMouseSteering(float& x, float& y,
                                       RelativeMouseDelta delta,
                                       float sensitivity,
                                       float limit = 3.1F) noexcept {
    if (!std::isfinite(sensitivity) || sensitivity < 0.0F) sensitivity = 0.0F;
    if (!std::isfinite(limit) || limit <= 0.0F) limit = 3.1F;
    if (!std::isfinite(delta.x)) delta.x = 0.0F;
    if (!std::isfinite(delta.y)) delta.y = 0.0F;
    x = std::clamp(x + delta.x * sensitivity, -limit, limit);
    y = std::clamp(y - delta.y * sensitivity, -limit, limit);
}

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

class RawMouse {
public:
    RawMouse() = default;
    RawMouse(const RawMouse&) = delete;
    RawMouse& operator=(const RawMouse&) = delete;
    ~RawMouse() { uninstall(); }

    bool install(void* nativeWindow) noexcept {
        if (installed_) return true;
        window_ = static_cast<HWND>(nativeWindow);
        if (window_ == nullptr || (instance_ != nullptr && instance_ != this)) return false;

        SetLastError(ERROR_SUCCESS);
        const LONG_PTR previous = SetWindowLongPtrW(
            window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&RawMouse::windowProc));
        if (previous == 0 && GetLastError() != ERROR_SUCCESS) {
            window_ = nullptr;
            return false;
        }
        previousProc_ = reinterpret_cast<WNDPROC>(previous);
        instance_ = this;

        RAWINPUTDEVICE device{0x01, 0x02, RIDEV_INPUTSINK, window_};
        if (!RegisterRawInputDevices(&device, 1, sizeof(device))) {
            if (GetWindowLongPtrW(window_, GWLP_WNDPROC) ==
                reinterpret_cast<LONG_PTR>(&RawMouse::windowProc)) {
                SetWindowLongPtrW(window_, GWLP_WNDPROC,
                                  reinterpret_cast<LONG_PTR>(previousProc_));
            }
            instance_ = nullptr;
            previousProc_ = nullptr;
            window_ = nullptr;
            return false;
        }
        installed_ = true;
        return true;
    }

    [[nodiscard]] bool installed() const noexcept { return installed_; }

    // Relative WM_INPUT deltas are used without cursor-centering or SetCursorPos.
    void setActive(bool wanted) noexcept {
        if (!installed_) return;
        active_ = wanted && GetForegroundWindow() == window_;
        if (active_) {
            RECT client{};
            POINT topLeft{0, 0};
            POINT bottomRight{};
            if (GetClientRect(window_, &client)) {
                bottomRight.x = client.right;
                bottomRight.y = client.bottom;
                ClientToScreen(window_, &topLeft);
                ClientToScreen(window_, &bottomRight);
                const RECT bounds{topLeft.x, topLeft.y,
                                  bottomRight.x, bottomRight.y};
                ClipCursor(&bounds);
            }
            SetCursor(nullptr);
        } else {
            ClipCursor(nullptr);
            SetCursor(LoadCursorW(nullptr, IDC_ARROW));
            clear();
        }
    }

    [[nodiscard]] RelativeMouseDelta consume() noexcept {
        return RelativeMouseDelta{
            static_cast<float>(deltaX_.exchange(0, std::memory_order_relaxed)),
            static_cast<float>(deltaY_.exchange(0, std::memory_order_relaxed))
        };
    }

    void clear() noexcept {
        deltaX_.store(0, std::memory_order_relaxed);
        deltaY_.store(0, std::memory_order_relaxed);
    }

    void uninstall() noexcept {
        if (!installed_) return;
        setActive(false);
        RAWINPUTDEVICE removal{0x01, 0x02, RIDEV_REMOVE, nullptr};
        RegisterRawInputDevices(&removal, 1, sizeof(removal));
        if (window_ != nullptr && GetWindowLongPtrW(window_, GWLP_WNDPROC) ==
            reinterpret_cast<LONG_PTR>(&RawMouse::windowProc)) {
            SetWindowLongPtrW(window_, GWLP_WNDPROC,
                              reinterpret_cast<LONG_PTR>(previousProc_));
        }
        if (instance_ == this) instance_ = nullptr;
        installed_ = false;
        previousProc_ = nullptr;
        window_ = nullptr;
        clear();
    }

private:
    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message,
                                       WPARAM wParam, LPARAM lParam) {
        RawMouse* self = instance_;
        if (self != nullptr && self->window_ == hwnd) {
            if (message == WM_INPUT) self->collect(lParam);
            if (message == WM_KILLFOCUS ||
                (message == WM_ACTIVATEAPP && wParam == FALSE)) {
                self->setActive(false);
            }
            if (message == WM_SETCURSOR && self->active_ &&
                LOWORD(lParam) == HTCLIENT) {
                SetCursor(nullptr);
                return TRUE;
            }
            if (self->previousProc_ != nullptr) {
                return CallWindowProcW(self->previousProc_, hwnd, message, wParam, lParam);
            }
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    void collect(LPARAM inputHandle) noexcept {
        if (!active_ || GetForegroundWindow() != window_) return;
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

    inline static RawMouse* instance_ = nullptr;
    HWND window_ = nullptr;
    WNDPROC previousProc_ = nullptr;
    std::atomic<long long> deltaX_{0};
    std::atomic<long long> deltaY_{0};
    bool installed_ = false;
    bool active_ = false;
};
#else
class RawMouse {
public:
    bool install(void*) noexcept { return false; }
    [[nodiscard]] bool installed() const noexcept { return false; }
    void setActive(bool) noexcept {}
    [[nodiscard]] RelativeMouseDelta consume() noexcept { return {}; }
    void clear() noexcept {}
    void uninstall() noexcept {}
};
#endif
