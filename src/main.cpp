#include "app/input_state.hpp"
#include "app/flight_physics.hpp"
#include "app/raw_mouse.hpp"
#include "app/save_profile.hpp"
#include "raylib.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {
constexpr int kInitialWidth = 1280;
constexpr int kInitialHeight = 800;
constexpr float kPanelWidth = 540.0F;
constexpr float kButtonHeight = 48.0F;
constexpr float kButtonGap = 10.0F;
const Color kBackground{8, 10, 15, 255};
const Color kPanel{17, 21, 30, 245};
const Color kEdge{57, 68, 83, 255};
const Color kText{232, 238, 246, 255};
const Color kMuted{137, 151, 171, 255};
const Color kAccent{189, 222, 255, 255};
const Color kDanger{255, 142, 142, 255};

enum class CrashCause { Wall, Gate };

struct AppState {
    tunrun::ScreenStack screens;
    int selectedShip = 0;
    int selectedMode = 0;
    bool showFps = true;
    bool reduceMotion = false;
    bool fullscreen = false;
    bool mouseSteering = true;
    float mouseSensitivity = 0.004F;
    bool hangarAxisLeftHeld = false;
    bool hangarAxisRightHeld = false;
    bool exitRequested = false;
    tunrun::FlightState flight;
    float flightAccumulator = 0.0F;
    std::uint64_t rootSeed = 0;
    std::uint64_t courseSeed = 0;
    std::uint64_t runSerial = 0;
    CrashCause crashCause = CrashCause::Wall;
    tunrun::ProfileStore profileStore;
    tunrun::Profile profile;
    bool profileRecoveryRequired = false;
    bool profileWritable = true;
    bool saveWarning = false;
    std::string saveWarningMessage;
    bool runRecorded = false;
    std::uint64_t runGatesCleared = 0U;
    std::uint64_t lastRunReward = 0U;
    std::uint64_t lastRunCoreReward = 0U;
    float elapsed = 0.0F;
};

bool padPressed(int button) {
    return IsGamepadAvailable(0) && IsGamepadButtonPressed(0, button);
}
bool upPressed() {
    return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) ||
           padPressed(GAMEPAD_BUTTON_LEFT_FACE_UP);
}
bool downPressed() {
    return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
           padPressed(GAMEPAD_BUTTON_LEFT_FACE_DOWN);
}
bool confirmPressed() {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
           padPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
}
bool leftPressed() {
    return IsKeyPressed(KEY_LEFT) || padPressed(GAMEPAD_BUTTON_LEFT_FACE_LEFT);
}
bool rightPressed() {
    return IsKeyPressed(KEY_RIGHT) || padPressed(GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
}
bool backPressed() {
    return IsKeyPressed(KEY_ESCAPE) || padPressed(GAMEPAD_BUTTON_MIDDLE_RIGHT);
}
bool persistProfile(AppState& app, bool preserveBackup = false) {
    if (!app.profileWritable || app.profileRecoveryRequired) {
        app.saveWarning = true;
        if (app.saveWarningMessage.empty()) {
            app.saveWarningMessage = "profile is read-only until recovery is completed";
        }
        return false;
    }
    app.profile.showFps = app.showFps;
    app.profile.reduceMotion = app.reduceMotion;
    app.profile.mouseSteering = app.mouseSteering;
    app.profile.fullscreen = app.fullscreen;
    app.profile.mouseSensitivity = app.mouseSensitivity;
    app.profile.selectedShip = static_cast<std::uint32_t>(
        std::clamp(app.selectedShip, 0, static_cast<int>(tunrun::kProfileShipCount) - 1));
    app.profile.rootSeed = app.rootSeed;
    app.profile.runSerial = app.runSerial;
    const auto result = app.profileStore.save(app.profile, preserveBackup);
    app.saveWarning = !result.success;
    app.saveWarningMessage = result.message;
    return result.success;
}
void resetFlight(AppState& app, bool& tpp) {
    app.flight = {};
    app.flightAccumulator = 0.0F;
    app.elapsed = 0.0F;
    app.runRecorded = false;
    app.runGatesCleared = 0U;
    app.lastRunReward = 0U;
    app.lastRunCoreReward = 0U;
    tpp = false;
}
void chooseNextSeed(AppState& app) {
    if (app.runSerial < std::numeric_limits<std::uint64_t>::max()) ++app.runSerial;
    app.courseSeed = tunrun::deriveCourseSeed(app.rootSeed, app.runSerial);
    (void)persistProfile(app);
}
void finishRun(AppState& app) {
    if (app.runRecorded) return;
    app.runRecorded = true;
    if (app.profile.totalRuns < std::numeric_limits<std::uint64_t>::max()) ++app.profile.totalRuns;
    if (app.profile.totalCrashes < std::numeric_limits<std::uint64_t>::max()) ++app.profile.totalCrashes;
    app.profile.bestDistance = std::max(app.profile.bestDistance,
                                        static_cast<double>(std::max(0.0F, app.flight.distance)));
    const double rawReward = std::floor(std::max(0.0F, app.flight.distance) / 20.0F);
    app.lastRunReward = static_cast<std::uint64_t>(std::clamp(rawReward, 0.0, 250000.0));
    app.lastRunCoreReward = std::min<std::uint64_t>(app.runGatesCleared / 10U, 1000U);
    const auto maxValue = std::numeric_limits<std::uint64_t>::max();
    app.profile.aetherShards = maxValue - app.profile.aetherShards < app.lastRunReward
        ? maxValue : app.profile.aetherShards + app.lastRunReward;
    app.profile.singularityCores = maxValue - app.profile.singularityCores < app.lastRunCoreReward
        ? maxValue : app.profile.singularityCores + app.lastRunCoreReward;
    (void)persistProfile(app);
}

void drawCentred(const char* text, float y, int fontSize, Color color) {
    DrawText(text, (GetScreenWidth() - MeasureText(text, fontSize)) / 2,
             static_cast<int>(y), fontSize, color);
}

void drawBackground(float time, bool reducedMotion) {
    ClearBackground(kBackground);
    const float drift = reducedMotion ? 0.0F : std::sin(time * 0.35F) * 10.0F;
    const int width = GetScreenWidth();
    const int height = GetScreenHeight();
    for (int i = 0; i < 12; ++i) {
        const float scale = static_cast<float>(i + 1) / 12.0F;
        DrawEllipseLines(width / 2 + static_cast<int>(drift * scale),
                         static_cast<int>(height * 0.54F),
                         18.0F + scale * static_cast<float>(std::min(width, height)) * 0.65F,
                         9.0F + scale * static_cast<float>(height) * 0.26F,
                         Color{42, 55, 72, static_cast<unsigned char>(18 + i * 4)});
    }
    for (int i = 0; i < 12; ++i) {
        const int x = width * i / 11;
        DrawLine(x, 0, width / 2 + (x - width / 2) / 5, height,
                 Color{28, 38, 53, 100});
    }
}

bool drawButton(Rectangle bounds, const char* label, bool selected, bool danger = false) {
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
    const bool focused = hovered || selected;
    DrawRectangleRounded(bounds, 0.10F, 8, focused ? Color{36, 50, 68, 255} : kPanel);
    DrawRectangleRoundedLinesEx(bounds, 0.10F, 8, 1.2F,
                                danger ? kDanger : focused ? kAccent : kEdge);
    DrawText(label,
        static_cast<int>(bounds.x + (bounds.width - MeasureText(label, 18)) / 2),
        static_cast<int>(bounds.y + (bounds.height - 18.0F) / 2), 18,
        danger ? kDanger : kText);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void moveSelection(int count, int& selection) {
    if (count <= 0) { selection = 0; return; }

    bool moved = false;
    if (upPressed()) {
        selection = (selection - 1 + count) % count;
        moved = true;
    } else if (downPressed()) {
        selection = (selection + 1) % count;
        moved = true;
    }

    // Rate-limited analogue navigation prevents held sticks racing through menus.
    static int previousStickDirection = 0;
    static float repeatDelay = 0.0F;
    float verticalAxis = 0.0F;
    if (IsGamepadAvailable(0)) {
        verticalAxis = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
    }
    const int direction = verticalAxis < -0.55F ? -1
        : verticalAxis > 0.55F ? 1 : 0;
    if (direction == 0) {
        previousStickDirection = 0;
        repeatDelay = 0.0F;
    } else if (!moved) {
        if (direction != previousStickDirection) {
            selection = (selection + direction + count) % count;
            previousStickDirection = direction;
            repeatDelay = 0.24F;
        } else {
            repeatDelay -= GetFrameTime();
            if (repeatDelay <= 0.0F) {
                selection = (selection + direction + count) % count;
                repeatDelay = 0.12F;
            }
        }
    } else {
        previousStickDirection = direction;
        repeatDelay = 0.24F;
    }
    selection = std::clamp(selection, 0, count - 1);
}

int drawMenu(const std::vector<std::string>& labels, int& selection,
             int firstY, bool confirmEnabled = true, int dangerIndex = -1) {
    moveSelection(static_cast<int>(labels.size()), selection);
    const int top = firstY < 0
        ? (GetScreenHeight() - static_cast<int>(labels.size()) *
           static_cast<int>(kButtonHeight + kButtonGap)) / 2
        : firstY;
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        const Rectangle bounds{
            (static_cast<float>(GetScreenWidth()) - kPanelWidth) / 2.0F,
            static_cast<float>(top + i * static_cast<int>(kButtonHeight + kButtonGap)),
            kPanelWidth, kButtonHeight
        };
        if (CheckCollisionPointRec(GetMousePosition(), bounds)) selection = i;
        const bool click = drawButton(bounds, labels[static_cast<std::size_t>(i)].c_str(),
                                      selection == i, i == dangerIndex);
        if (click || (confirmEnabled && selection == i && confirmPressed())) return i;
    }
    return -1;
}

void drawHeader(const char* number, const char* title, const char* subtitle) {
    DrawText(number, 48, 35, 15, kAccent);
    DrawText(title, 48, 62, 36, kText);
    DrawText(subtitle, 50, 108, 16, kMuted);
    DrawLine(50, 143, GetScreenWidth() - 50, 143, kEdge);
}

Vector3 tunnelPoint(std::uint64_t seed, float distance, int ring, int side,
                    const tunrun::TunnelCrossSection& playerSection) {
    constexpr int sides = 16;
    constexpr float ringSpacing = 3.0F;
    const float depth = static_cast<float>(ring) * ringSpacing;
    const auto section = tunrun::sampleCourse(seed, static_cast<double>(distance) + depth);
    const float angle = static_cast<float>(side) * 2.0F * PI / sides + section.twist;
    return Vector3{section.centerX - playerSection.centerX + std::cos(angle) * section.radius,
                   section.centerY - playerSection.centerY + std::sin(angle) * section.radius,
                   -depth};
}
void drawProceduralGate(std::uint64_t seed, float playerDistance,
                        const tunrun::TunnelCrossSection& playerSection,
                        const tunrun::ProceduralGate& gate) {
    const float ahead = static_cast<float>(gate.distance - static_cast<double>(playerDistance));
    if (ahead < -1.0F || ahead > 70.0F) return;
    const auto courseAtGate = tunrun::sampleCourse(seed, gate.distance);
    const float outerRadius = courseAtGate.radius - 0.24F;
    const float gateCenterX = courseAtGate.centerX - playerSection.centerX + gate.offsetX;
    const float gateCenterY = courseAtGate.centerY - playerSection.centerY + gate.offsetY;
    const float courseCenterX = courseAtGate.centerX - playerSection.centerX;
    const float courseCenterY = courseAtGate.centerY - playerSection.centerY;
    constexpr int segments = 24;
    const float z = -ahead;
    for (int side = 0; side < segments; ++side) {
        const float a0 = static_cast<float>(side) * 2.0F * PI / segments + courseAtGate.twist;
        const float a1 = static_cast<float>(side + 1) * 2.0F * PI / segments + courseAtGate.twist;
        const Vector3 outer0{courseCenterX + std::cos(a0) * outerRadius, courseCenterY + std::sin(a0) * outerRadius, z};
        const Vector3 outer1{courseCenterX + std::cos(a1) * outerRadius, courseCenterY + std::sin(a1) * outerRadius, z};
        const Vector3 inner0{gateCenterX + std::cos(a0) * gate.apertureRadius, gateCenterY + std::sin(a0) * gate.apertureRadius, z};
        const Vector3 inner1{gateCenterX + std::cos(a1) * gate.apertureRadius, gateCenterY + std::sin(a1) * gate.apertureRadius, z};
        DrawLine3D(outer0, outer1, Color{100, 130, 164, 220});
        DrawLine3D(inner0, inner1, Color{200, 229, 255, 255});
        if (side % 2 == 0) DrawLine3D(outer0, inner0, Color{76, 99, 125, 205});
    }
}
void drawTunnel(std::uint64_t seed, float distance, float shipX, float shipY,
                bool tpp, float boostEnergy) {
    const auto playerSection = tunrun::sampleCourse(seed, distance);
    Camera3D camera{};
    camera.position = tpp ? Vector3{shipX, shipY + 0.7F, 7.0F}
                           : Vector3{shipX, shipY, 1.5F};
    camera.target = Vector3{shipX * 0.3F, shipY * 0.3F, -24.0F};
    camera.up = Vector3{0.0F, 1.0F, 0.0F};
    camera.fovy = 70.0F;
    camera.projection = CAMERA_PERSPECTIVE;
    ClearBackground(kBackground);
    BeginMode3D(camera);
    constexpr int ringCount = 23;
    constexpr int sideCount = 16;
    for (int ring = 0; ring < ringCount; ++ring) {
        const Color ringColor = ring % 4 == 0
            ? Color{164, 193, 225, 190} : Color{58, 78, 101, 140};
        for (int side = 0; side < sideCount; ++side) {
            DrawLine3D(tunnelPoint(seed, distance, ring, side, playerSection),
                       tunnelPoint(seed, distance, ring, (side + 1) % sideCount, playerSection), ringColor);
            if (ring + 1 < ringCount) {
                DrawLine3D(tunnelPoint(seed, distance, ring, side, playerSection),
                           tunnelPoint(seed, distance, ring + 1, side, playerSection), Color{48, 65, 83, 125});
            }
        }
    }
    const auto firstGate = tunrun::gateAt(seed, 0U);
    const int firstVisibleIndex = std::max(0, static_cast<int>(
        std::floor((static_cast<double>(distance) - firstGate.distance) / tunrun::kGateSpacing)));
    for (int i = firstVisibleIndex; i < firstVisibleIndex + 4; ++i) {
        drawProceduralGate(seed, distance, playerSection,
                           tunrun::gateAt(seed, static_cast<std::uint32_t>(i)));
    }
    if (tpp) {
        const Vector3 nose{shipX, shipY, -0.2F};
        const Vector3 left{shipX - 0.75F, shipY - 0.28F, 0.65F};
        const Vector3 right{shipX + 0.75F, shipY - 0.28F, 0.65F};
        const Vector3 tail{shipX, shipY + 0.34F, 0.8F};
        DrawLine3D(nose, left, kAccent);
        DrawLine3D(nose, right, kAccent);
        DrawLine3D(left, tail, kAccent);
        DrawLine3D(tail, right, kAccent);
        DrawLine3D(left, right, kAccent);
    }
    EndMode3D();
    DrawRectangle(22, 18, 344, 106, Color{10, 14, 21, 225});
    DrawRectangleLines(22, 18, 344, 106, kEdge);
    DrawText("TUNRUN / M3 SEEDED COURSE", 35, 30, 15, kAccent);
    DrawText(tpp ? "CAMERA: TPP" : "CAMERA: FPP", 35, 52, 14, kText);
    DrawText(TextFormat("BOOST: %3.0f%%", boostEnergy), 35, 74, 13, kText);
    DrawRectangle(175, 78, 155, 8, Color{42, 51, 64, 255});
    DrawRectangle(175, 78, static_cast<int>(155.0F * boostEnergy / 100.0F), 8, kAccent);
    DrawText(TextFormat("SEED %016llX", static_cast<unsigned long long>(seed)), 35, 98, 12, kMuted);
    DrawRectangle(22, GetScreenHeight() - 48, GetScreenWidth() - 44, 26,
                  Color{10, 14, 21, 220});
    DrawText("WASD / ARROWS: STEER   SHIFT: BOOST   CTRL: PRECISION   V: CAMERA   ESC: PAUSE",
             36, GetScreenHeight() - 42, 13, kMuted);
}
} // namespace

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 800, "TUNRUN | Procedural Tunnel Runner");
    SetWindowMinSize(800, 560);
    SetExitKey(KEY_NULL);
    SetTargetFPS(144);
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

    RawMouse rawMouse;
    rawMouse.install(GetWindowHandle());

    AppState app;
    app.profileStore = tunrun::ProfileStore::forCurrentUser();
    const auto clockSeed = static_cast<std::uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    std::uint64_t fallbackSeed = tunrun::mixCourseBits(clockSeed);
    if (fallbackSeed == 0U) fallbackSeed = 1U;

    const auto loadedProfile = app.profileStore.load();
    bool initializeProfile = false;
    if (loadedProfile.status == tunrun::ProfileLoadStatus::Loaded ||
        loadedProfile.status == tunrun::ProfileLoadStatus::RecoveredBackup) {
        app.profile = loadedProfile.profile;
    } else {
        app.profile = tunrun::Profile{};
        app.profile.rootSeed = fallbackSeed;
        initializeProfile = loadedProfile.status == tunrun::ProfileLoadStatus::NotFound;
    }
    if (loadedProfile.status == tunrun::ProfileLoadStatus::RecoveryRequired) {
        app.profileRecoveryRequired = true;
        app.profileWritable = false;
        app.saveWarning = true;
        app.saveWarningMessage = loadedProfile.message;
    } else if (loadedProfile.status == tunrun::ProfileLoadStatus::Error) {
        app.profileWritable = false;
        app.saveWarning = true;
        app.saveWarningMessage = loadedProfile.message;
    }

    if (app.profile.rootSeed == 0U) {
        app.profile.rootSeed = fallbackSeed;
        initializeProfile = true;
    }
    app.rootSeed = app.profile.rootSeed;
    app.runSerial = app.profile.runSerial;
    app.courseSeed = tunrun::deriveCourseSeed(app.rootSeed, app.runSerial);
    app.showFps = app.profile.showFps;
    app.reduceMotion = app.profile.reduceMotion;
    app.mouseSteering = app.profile.mouseSteering;
    app.fullscreen = app.profile.fullscreen;
    app.mouseSensitivity = app.profile.mouseSensitivity;
    app.selectedShip = static_cast<int>(std::min<std::uint32_t>(
        app.profile.selectedShip, static_cast<std::uint32_t>(tunrun::kProfileShipCount - 1U)));
    if (app.fullscreen) ToggleFullscreen();

    if (app.profileRecoveryRequired) {
        app.screens.replace(tunrun::Screen::SaveRecovery);
    } else if (loadedProfile.status == tunrun::ProfileLoadStatus::RecoveredBackup) {
        (void)persistProfile(app, true);
    } else if (initializeProfile && app.profileWritable) {
        (void)persistProfile(app);
    }

    int mainSelection = 0, hangarSelection = 0, modesSelection = 0;
    int settingsSelection = 0, pauseSelection = 0, exitSelection = 0, crashSelection = 0, seedLabSelection = 0, recoverySelection = 0;
    bool tpp = false;
    const std::vector<std::string> mainItems{
        "PLAY / PROCEDURAL RUN", "HANGAR", "GAME MODES", "SEED LAB",
        "SETTINGS", "CREDITS", "EXIT"
    };
    const std::vector<std::string> ships{
        "DRIFTWING", "WRAITH", "BULWARK", "MANTA",
        "COMET", "SPECTRE", "VORTEX", "OBSIDIAN"
    };
    const std::vector<std::string> modes{
        "CAMPAIGN (PLANNED)", "ENDLESS (PLANNED)",
        "SEED CHALLENGE (PLANNED)", "PRACTICE PREVIEW", "BACK"
    };
    const std::vector<std::string> pauseItems{"RESUME", "SETTINGS", "RETURN TO MAIN MENU"};
    const std::vector<std::string> settingsItems{
        "TOGGLE FULLSCREEN", "TOGGLE FPS COUNTER", "TOGGLE REDUCED MOTION",
        "MOUSE STEERING", "BACK"
    };
    const std::vector<std::string> exitItems{"CANCEL", "EXIT"};
    const std::vector<std::string> crashItems{"RETRY SAME SEED", "NEW SEED", "RETURN TO MAIN MENU"};
    const std::vector<std::string> seedLabItems{"GENERATE NEW SEED", "START THIS SEED", "BACK"};
    const std::vector<std::string> recoveryItems{"RESET PROFILE (PRESERVE DAMAGED FILES)", "EXIT WITHOUT RESET"};

    while (!WindowShouldClose() && !app.exitRequested) {
        const float dt = std::min(GetFrameTime(), 0.05F);
        const bool wantsMouseCapture =
            app.screens.current() == tunrun::Screen::Preview &&
            app.mouseSteering && IsWindowFocused();
        rawMouse.setActive(wantsMouseCapture);
        RelativeMouseDelta mouseDelta = rawMouse.consume();
        if (!rawMouse.installed() && wantsMouseCapture) {
            const Vector2 fallbackDelta = GetMouseDelta();
            mouseDelta = RelativeMouseDelta{fallbackDelta.x, fallbackDelta.y};
        }
        if (app.screens.current() != tunrun::Screen::Pause &&
            app.screens.current() != tunrun::Screen::Settings &&
            app.screens.current() != tunrun::Screen::Crash) app.elapsed += dt;

        if (app.screens.current() == tunrun::Screen::Preview) {
            if (!IsWindowFocused()) {
                app.screens.push(tunrun::Screen::Pause);
            } else {
                float steerX = (IsKeyDown(KEY_D) ? 1.0F : 0.0F) -
                               (IsKeyDown(KEY_A) ? 1.0F : 0.0F);
                float steerY = (IsKeyDown(KEY_W) ? 1.0F : 0.0F) -
                               (IsKeyDown(KEY_S) ? 1.0F : 0.0F);
                if (IsKeyDown(KEY_RIGHT)) steerX += 1.0F;
                if (IsKeyDown(KEY_LEFT)) steerX -= 1.0F;
                if (IsKeyDown(KEY_UP)) steerY += 1.0F;
                if (IsKeyDown(KEY_DOWN)) steerY -= 1.0F;
                if (IsGamepadAvailable(0)) {
                    float padX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
                    float padY = -GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
                    if (std::abs(padX) < 0.18F) padX = 0.0F;
                    else padX = std::copysign((std::abs(padX) - 0.18F) / 0.82F, padX);
                    if (std::abs(padY) < 0.18F) padY = 0.0F;
                    else padY = std::copysign((std::abs(padY) - 0.18F) / 0.82F, padY);
                    steerX += padX;
                    steerY += padY;
                }
                const float previousX = app.flight.x;
                const float previousY = app.flight.y;
                const double previousDistance = app.flight.distance;
                const tunrun::FlightInput flightInput{
                    std::clamp(steerX, -1.0F, 1.0F),
                    std::clamp(steerY, -1.0F, 1.0F),
                    IsKeyDown(KEY_LEFT_SHIFT),
                    IsKeyDown(KEY_LEFT_CONTROL)
                };
                tunrun::advanceFlight(app.flight, flightInput, dt, app.flightAccumulator);
                applyRelativeMouseSteering(app.flight.x, app.flight.y, mouseDelta,
                                           app.mouseSensitivity, tunrun::kFlightLimit);
                const auto tunnelSection = tunrun::sampleCourse(
                    app.courseSeed, static_cast<double>(app.flight.distance));
                const tunrun::TunnelCrossSection centredSection{
                    0.0F, 0.0F, tunnelSection.radius, tunnelSection.twist
                };
                if (tunrun::collidesWithTunnelWall(
                        app.flight.x, app.flight.y, centredSection)) {
                    app.crashCause = CrashCause::Wall;
                    finishRun(app);
                    app.screens.replace(tunrun::Screen::Crash);
                } else {
                    const auto firstGate = tunrun::gateAt(app.courseSeed, 0U);
                    const int firstCandidate = std::max(0, static_cast<int>(
                        std::floor((previousDistance - firstGate.distance) /
                                   tunrun::kGateSpacing)) - 1);
                    const int lastCandidate = std::max(firstCandidate, static_cast<int>(
                        std::ceil((static_cast<double>(app.flight.distance) +
                                   tunrun::kGateDepthHalfThickness - firstGate.distance) /
                                  tunrun::kGateSpacing)) + 1);
                    for (int gateIndex = firstCandidate; gateIndex <= lastCandidate; ++gateIndex) {
                        const auto gate = tunrun::gateAt(app.courseSeed,
                            static_cast<std::uint32_t>(gateIndex));
                        if (!tunrun::crossesGatePlane(previousDistance, app.flight.distance, gate)) continue;
                        const double travel = static_cast<double>(app.flight.distance) - previousDistance;
                        const float fraction = travel > 1.0e-6
                            ? static_cast<float>(std::clamp((gate.distance - previousDistance) / travel, 0.0, 1.0))
                            : 0.0F;
                        const float crossingX = previousX + (app.flight.x - previousX) * fraction;
                        const float crossingY = previousY + (app.flight.y - previousY) * fraction;
                        if (tunrun::collidesWithGate(crossingX, crossingY, gate)) {
                            app.crashCause = CrashCause::Gate;
                            finishRun(app);
                            app.screens.replace(tunrun::Screen::Crash);
                            break;
                        }
                        if (app.runGatesCleared < std::numeric_limits<std::uint64_t>::max()) {
                            ++app.runGatesCleared;
                        }
                    }
                }
                if (app.screens.current() == tunrun::Screen::Preview &&
                    (IsKeyPressed(KEY_V) || padPressed(GAMEPAD_BUTTON_RIGHT_FACE_UP))) tpp = !tpp;
                if (backPressed()) app.screens.push(tunrun::Screen::Pause);
            }
        }

        BeginDrawing();
        if (app.screens.current() == tunrun::Screen::Preview) {
            drawTunnel(app.courseSeed, app.flight.distance, app.flight.x, app.flight.y, tpp, app.flight.boostEnergy);
            const Rectangle pauseBounds{
                static_cast<float>(GetScreenWidth() - 126), 22.0F, 102.0F, 40.0F
            };
            if (drawButton(pauseBounds, "PAUSE", false)) {
                app.screens.push(tunrun::Screen::Pause);
            }
            if (app.showFps) DrawFPS(GetScreenWidth() - 92, 72);
            EndDrawing();
            continue;
        }

        drawBackground(app.elapsed, app.reduceMotion);
        switch (app.screens.current()) {
        case tunrun::Screen::MainMenu: {
            drawCentred("T U N R U N", 56.0F, 54, kText);
            drawCentred("PROCEDURAL TUNNEL RUNNER", 116.0F, 16, kAccent);
            drawCentred("SEEDED PROCEDURAL FLIGHT / MILESTONE M3", 141.0F, 12, kMuted);
            DrawText(TextFormat("AETHER SHARDS  %llu",
                                static_cast<unsigned long long>(app.profile.aetherShards)),
                     52, 162, 14, kAccent);
            DrawText(TextFormat("SINGULARITY CORES  %llu",
                                static_cast<unsigned long long>(app.profile.singularityCores)),
                     330, 162, 14, kText);
            const int picked = drawMenu(mainItems, mainSelection, 185);
            if (picked >= 0) {
                switch (picked) {
                case 0: resetFlight(app, tpp); chooseNextSeed(app); app.screens.push(tunrun::Screen::Preview); break;
                case 1: app.screens.push(tunrun::Screen::Hangar); break;
                case 2: app.screens.push(tunrun::Screen::Modes); break;
                case 3: app.screens.push(tunrun::Screen::SeedLab); break;
                case 4: app.screens.push(tunrun::Screen::Settings); break;
                case 5: app.screens.push(tunrun::Screen::Credits); break;
                case 6: app.screens.push(tunrun::Screen::ExitConfirm); break;
                default: break;
                }
            }
            break;
        }
        case tunrun::Screen::Hangar: {
            const int previousShip = app.selectedShip;
            drawHeader("01 / COLLECTION", "HANGAR", "Selection persists locally; unlocks and purchases are next.");
            drawCentred("SHIP", 205.0F, 15, kAccent);
            const std::string selected = "[ " + ships[static_cast<std::size_t>(app.selectedShip)] + " ]";
            drawCentred(selected.c_str(), 245.0F, 32, kText);
            drawCentred("Ship models and handling profiles arrive in a later milestone.", 293.0F, 15, kMuted);
            if (drawButton(Rectangle{static_cast<float>(GetScreenWidth()/2 - 180), 350, 70, 45}, "<", false))
                app.selectedShip = (app.selectedShip + 7) % 8;
            if (drawButton(Rectangle{static_cast<float>(GetScreenWidth()/2 + 110), 350, 70, 45}, ">", false))
                app.selectedShip = (app.selectedShip + 1) % 8;
            if (drawMenu({"BACK"}, hangarSelection, GetScreenHeight() - 96) == 0) app.screens.pop();
            if (leftPressed()) app.selectedShip = (app.selectedShip + 7) % 8;
            if (rightPressed()) app.selectedShip = (app.selectedShip + 1) % 8;
            if (IsGamepadAvailable(0)) {
                const float horizontalAxis = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
                if (horizontalAxis < -0.65F && !app.hangarAxisLeftHeld) {
                    app.selectedShip = (app.selectedShip + 7) % 8;
                    app.hangarAxisLeftHeld = true;
                } else if (horizontalAxis > 0.65F && !app.hangarAxisRightHeld) {
                    app.selectedShip = (app.selectedShip + 1) % 8;
                    app.hangarAxisRightHeld = true;
                } else if (std::abs(horizontalAxis) < 0.25F) {
                    app.hangarAxisLeftHeld = false;
                    app.hangarAxisRightHeld = false;
                }
            }
            if (app.selectedShip != previousShip) (void)persistProfile(app);
            if (backPressed()) app.screens.pop();
            break;
        }
        case tunrun::Screen::Modes: {
            drawHeader("02 / FLIGHT PLAN", "GAME MODES", "Practice opens the current seeded procedural course.");
            const int picked = drawMenu(modes, modesSelection, 192);
            if (picked == 3) { resetFlight(app, tpp); chooseNextSeed(app); app.screens.push(tunrun::Screen::Preview); }
            else if (picked == 4) app.screens.pop();
            else if (picked >= 0) app.selectedMode = picked;
            if (backPressed()) app.screens.pop();
            break;
        }
        case tunrun::Screen::Settings: {
            drawHeader("03 / CONFIGURATION", "SETTINGS", "Settings are saved to the local TUNRUN profile.");
            auto labels = settingsItems;
            labels[0] = std::string("FULLSCREEN: ") + (app.fullscreen ? "ON" : "OFF");
            labels[1] = std::string("FPS COUNTER: ") + (app.showFps ? "ON" : "OFF");
            labels[2] = std::string("REDUCED MOTION: ") + (app.reduceMotion ? "ON" : "OFF");
            labels[3] = std::string("MOUSE STEERING: ") + (app.mouseSteering ? "ON" : "OFF");
            const int picked = drawMenu(labels, settingsSelection, 225);
            if (picked == 0) { app.fullscreen = !app.fullscreen; ToggleFullscreen(); }
            else if (picked == 1) app.showFps = !app.showFps;
            else if (picked == 2) app.reduceMotion = !app.reduceMotion;
            else if (picked == 3) app.mouseSteering = !app.mouseSteering;
            else if (picked == 4) app.screens.pop();
            if (picked >= 0 && picked <= 3) (void)persistProfile(app);
            if (backPressed()) app.screens.pop();
            break;
        }
        case tunrun::Screen::Credits:
            drawHeader("04 / PROJECT", "CREDITS", "An original project by Tamasrazim.");
            drawCentred("C++20  /  raylib 5.5  /  CMake", 235.0F, 20, kText);
            drawCentred("Third-party notices and license terms are included in the repository.", 275.0F, 15, kMuted);
            if (drawMenu({"BACK"}, mainSelection, GetScreenHeight() - 96) == 0) app.screens.pop();
            if (backPressed()) app.screens.pop();
            break;
        case tunrun::Screen::Pause: {
            drawHeader("SYSTEM / PAUSED", "PAUSED", "Preview input is frozen while this screen is open.");
            const int picked = drawMenu(pauseItems, pauseSelection, 260);
            if (picked == 0) app.screens.pop();
            else if (picked == 1) app.screens.push(tunrun::Screen::Settings);
            else if (picked == 2) app.screens.reset();
            if (backPressed()) app.screens.pop();
            break;
        }
        case tunrun::Screen::Crash: {
            drawHeader("SYSTEM / FLIGHT TERMINATED",
                       app.crashCause == CrashCause::Gate ? "GATE COLLISION" : "WALL COLLISION",
                       app.crashCause == CrashCause::Gate
                           ? "Craft crossed an obstacle plane outside its safe aperture."
                           : "Craft collision volume touched the tunnel boundary.");
            drawCentred(TextFormat("DISTANCE %.1f   SEED %016llX", app.flight.distance,
                                   static_cast<unsigned long long>(app.courseSeed)),
                        190.0F, 16, kAccent);
            drawCentred(TextFormat("CANONICAL COURSE HASH %016llX",
                                   static_cast<unsigned long long>(tunrun::courseHash(app.courseSeed))),
                        220.0F, 14, kMuted);
            drawCentred(TextFormat("GATES CLEARED: %llu   AETHER +%llu   CORES +%llu",
                                   static_cast<unsigned long long>(app.runGatesCleared),
                                   static_cast<unsigned long long>(app.lastRunReward),
                                   static_cast<unsigned long long>(app.lastRunCoreReward)),
                        246.0F, 14, kAccent);
            const int picked = drawMenu(crashItems, crashSelection, 285, true);
            if (picked == 0) {
                resetFlight(app, tpp);
                app.screens.replace(tunrun::Screen::Preview);
            } else if (picked == 1) {
                chooseNextSeed(app);
                resetFlight(app, tpp);
                app.screens.replace(tunrun::Screen::Preview);
            } else if (picked == 2) {
                app.screens.reset();
            }
            if (backPressed()) app.screens.reset();
            break;
        }
        case tunrun::Screen::SeedLab: {
            drawHeader("03 / GENERATION", "SEED LAB", "Regenerate identical geometry from a seed; change the seed to explore another course.");
            const auto validation = tunrun::validateCourse(app.courseSeed, 360.0);
            const auto gateValidation = tunrun::validateObstacleSet(app.courseSeed, 32U);
            drawCentred(TextFormat("SEED  %016llX", static_cast<unsigned long long>(app.courseSeed)),
                        175.0F, 22, kText);
            drawCentred(TextFormat("GENERATOR V%u   HASH %016llX", tunrun::kCourseGeneratorVersion,
                                   static_cast<unsigned long long>(tunrun::courseHash(app.courseSeed))),
                        215.0F, 15, kAccent);
            drawCentred(TextFormat("VALIDATOR: %s   SAMPLES: %u",
                                   validation.valid ? "PASS" : "FAIL", validation.samplesChecked),
                        243.0F, 14, validation.valid ? kAccent : kDanger);
            drawCentred(TextFormat("RADIUS %.2f-%.2f   MAX CENTRE OFFSET %.2f",
                                   validation.minimumRadius, validation.maximumRadius,
                                   validation.maximumCenterOffset),
                        266.0F, 13, kMuted);
            drawCentred(TextFormat("OBSTACLES: %s   GATES CHECKED: %u",
                                   gateValidation.valid ? "PASS" : "FAIL",
                                   gateValidation.gatesChecked),
                        288.0F, 13, gateValidation.valid ? kAccent : kDanger);
            const int picked = drawMenu(seedLabItems, seedLabSelection, 340, true);
            if (picked == 0) chooseNextSeed(app);
            else if (picked == 1) { resetFlight(app, tpp); app.screens.push(tunrun::Screen::Preview); }
            else if (picked == 2) app.screens.pop();
            if (IsKeyPressed(KEY_N)) chooseNextSeed(app);
            if (backPressed()) app.screens.pop();
            break;
        }
        case tunrun::Screen::SaveRecovery: {
            drawHeader("SYSTEM / SAVE RECOVERY", "PROFILE RECOVERY REQUIRED",
                       "Both profile copies are invalid or unreadable. They will not be overwritten automatically.");
            drawCentred("Reset keeps damaged files as .corrupt backups.", 195.0F, 16, kMuted);
            const int picked = drawMenu(recoveryItems, recoverySelection, 280, true, 0);
            if (picked == 0) {
                tunrun::Profile defaults{};
                defaults.rootSeed = tunrun::mixCourseBits(clockSeed ^ 0xA17E5EED1234ULL);
                if (defaults.rootSeed == 0U) defaults.rootSeed = 1U;
                const auto resetResult = app.profileStore.resetToDefaults(defaults);
                if (resetResult.success) {
                    app.profile = defaults;
                    app.profileWritable = true;
                    app.profileRecoveryRequired = false;
                    app.rootSeed = defaults.rootSeed;
                    app.runSerial = defaults.runSerial;
                    app.courseSeed = tunrun::deriveCourseSeed(app.rootSeed, app.runSerial);
                    app.showFps = defaults.showFps;
                    app.reduceMotion = defaults.reduceMotion;
                    app.mouseSteering = defaults.mouseSteering;
                    app.fullscreen = defaults.fullscreen;
                    app.mouseSensitivity = defaults.mouseSensitivity;
                    app.selectedShip = static_cast<int>(defaults.selectedShip);
                    app.saveWarning = false;
                    app.saveWarningMessage.clear();
                    resetFlight(app, tpp);
                    app.screens.reset();
                } else {
                    app.saveWarning = true;
                    app.saveWarningMessage = resetResult.message;
                }
            } else if (picked == 1) {
                app.exitRequested = true;
            }
            break;
        }
        case tunrun::Screen::ExitConfirm: {
            drawHeader("SYSTEM / CONFIRMATION", "EXIT TUNRUN?", "Settings and progression are stored in your local profile.");
            const int picked = drawMenu(exitItems, exitSelection, 320, true, 1);
            if (picked == 0) app.screens.pop();
            else if (picked == 1) app.exitRequested = true;
            if (backPressed()) app.screens.pop();
            break;
        }
        case tunrun::Screen::Preview:
            break;
        }
        if (app.saveWarning && !app.profileRecoveryRequired) {
            DrawText("SAVE WARNING - SOME CHANGES MAY NOT BE PERSISTED",
                     22, GetScreenHeight() - 48, 13, kDanger);
        }
        if (app.showFps) DrawFPS(GetScreenWidth() - 92, 16);
        DrawText("MAIN-ONLY DEVELOPMENT BUILD", 22, GetScreenHeight() - 25, 12, kMuted);
        EndDrawing();
    }
    if (!app.profileRecoveryRequired && app.profileWritable) (void)persistProfile(app);
    rawMouse.uninstall();
    CloseWindow();
    return 0;
}
