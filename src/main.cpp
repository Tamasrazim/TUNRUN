#include "app/input_state.hpp"
#include "app/flight_physics.hpp"
#include "app/seed_text.hpp"
#include "app/game_modes.hpp"
#include "app/camera_rig.hpp"
#include "app/economy.hpp"
#include "app/rewards.hpp"
#include "app/hazards.hpp"
#include "app/scoring.hpp"
#include "app/run_results.hpp"
#include "app/raw_mouse.hpp"
#include "app/tunnel_frame.hpp"
#include "app/tunnel_visuals.hpp"
#include "app/gate_visuals.hpp"
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

Color tunnelDepthFog(Color color, float distanceAhead) {
    if (!std::isfinite(distanceAhead)) distanceAhead = 0.0F;
    // Far sections disappear into the wormhole haze much sooner than before.
    // The next constriction stays readable while later geometry fades away.
    const float amount = std::clamp((distanceAhead - 3.0F) / 16.0F, 0.0F, 1.0F);
    const float fog = amount * amount * (3.0F - 2.0F * amount);
    constexpr float visibilityFloor = 0.025F;
    const float visibility = 1.0F - fog * (1.0F - visibilityFloor);
    color.r = static_cast<unsigned char>(std::clamp(
        8.0F + (static_cast<float>(color.r) - 8.0F) * visibility, 0.0F, 255.0F));
    color.g = static_cast<unsigned char>(std::clamp(
        10.0F + (static_cast<float>(color.g) - 10.0F) * visibility, 0.0F, 255.0F));
    color.b = static_cast<unsigned char>(std::clamp(
        15.0F + (static_cast<float>(color.b) - 15.0F) * visibility, 0.0F, 255.0F));
    return color;
}

enum class CrashCause { Wall, Gate, Hazard };
enum class PendingRunAction { None, RestartSameSeed, ReturnToMainMenu };

struct AppState {
    tunrun::ScreenStack screens;
    int selectedShip = 0;
    int hangarPreviewShip = 0;
    std::uint32_t pendingHangarShip = tunrun::kStarterShipId;
    std::string hangarMessage;
    std::string seedEntryText;
    std::string seedEntryMessage;
    bool seedEntryHasError = false;
    std::string modeMessage;
    tunrun::GameModeChoice activeMode = tunrun::GameModeChoice::Endless;
    PendingRunAction pendingRunAction = PendingRunAction::None;
    bool showFps = true;
    bool reduceMotion = false;
    bool fullscreen = false;
    bool hangarAxisLeftHeld = false;
    bool hangarAxisRightHeld = false;
    bool exitRequested = false;
    tunrun::FlightState flight;
    float flightAccumulator = 0.0F;
    float cameraLookYaw = 0.0F;
    float cameraLookPitch = 0.0F;
    tunrun::CameraFollowState tppCameraFollow;
    float cameraModeBlend = 0.0F;
    std::uint64_t rootSeed = 0;
    std::uint64_t courseSeed = 0;
    std::uint64_t runSerial = 0;
    CrashCause crashCause = CrashCause::Wall;
    std::uint32_t lastHitObjectIndex = 0U;
    tunrun::ProfileStore profileStore;
    tunrun::Profile profile;
    bool profileRecoveryRequired = false;
    bool profileWritable = true;
    bool saveWarning = false;
    std::string saveWarningMessage;
    bool runRecorded = false;
    std::uint64_t runGatesCleared = 0U;
    tunrun::RunScore runScore;
    bool newBestScore = false;
    bool newBestCombo = false;
    bool seedCopiedNotice = false;
    std::uint64_t lastRunReward = 0U;
    std::uint64_t lastRunCoreReward = 0U;
    std::uint64_t runAetherPickupReward = 0U;
    std::uint64_t runSingularityCorePickupReward = 0U;
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
    return IsKeyPressed(KEY_ESCAPE) || padPressed(GAMEPAD_BUTTON_MIDDLE_RIGHT) ||
           padPressed(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
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
    app.profile.fullscreen = app.fullscreen;
    app.profile.selectedShip = static_cast<std::uint32_t>(
        std::clamp(app.selectedShip, 0, static_cast<int>(tunrun::kProfileShipCount) - 1));
    app.profile.rootSeed = app.rootSeed;
    app.profile.runSerial = app.runSerial;
    const auto result = app.profileStore.save(app.profile, preserveBackup);
    app.saveWarning = !result.success;
    app.saveWarningMessage = result.message;
    return result.success;
}
void beginSeedEntry(AppState& app) {
    while (GetCharPressed() > 0) {}
    app.seedEntryText.clear();
    app.seedEntryMessage.clear();
    app.seedEntryHasError = false;
}
void resetFlight(AppState& app, bool& tpp) {
    app.flight = {};
    app.newBestScore = false;
    app.newBestCombo = false;
    app.seedCopiedNotice = false;
    // A held confirm button may have just entered this screen. Treat it as
    // already held so entering/retrying a run never fires a surprise dash.
    app.flight.dashButtonWasDown = IsKeyDown(KEY_SPACE) ||
        (IsGamepadAvailable(0) &&
         IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
    app.flightAccumulator = 0.0F;
    app.cameraLookYaw = 0.0F;
    app.cameraLookPitch = 0.0F;
    app.tppCameraFollow = {};
    app.cameraModeBlend = 0.0F;
    app.elapsed = 0.0F;
    app.runRecorded = false;
    app.runGatesCleared = 0U;
    app.runScore = {};
    app.lastRunReward = 0U;
    app.lastRunCoreReward = 0U;
    app.lastHitObjectIndex = 0U;
    app.runAetherPickupReward = 0U;
    app.runSingularityCorePickupReward = 0U;
    tpp = false;
}
void copyCourseSeed(AppState& app) {
    const std::string seedText = tunrun::formatHexSeed(app.courseSeed);
    SetClipboardText(seedText.c_str());
    app.seedCopiedNotice = true;
}
void chooseNextSeed(AppState& app) {
    if (app.runSerial < std::numeric_limits<std::uint64_t>::max()) ++app.runSerial;
    app.courseSeed = tunrun::deriveCourseSeed(app.rootSeed, app.runSerial);
    app.seedCopiedNotice = false;
    (void)persistProfile(app);
}
void purchaseHangarShip(AppState& app, std::uint32_t shipId) {
    if (shipId >= tunrun::kProfileShipCount || shipId >= tunrun::kShipCatalog.size()) {
        app.hangarMessage = "Invalid ship selection.";
        return;
    }
    if (app.profile.unlockedShips[shipId]) {
        app.hangarMessage = "This ship is already unlocked; equip it separately.";
        return;
    }

    tunrun::Profile candidate = app.profile;
    const auto purchase = tunrun::purchaseShip(candidate, shipId);
    if (purchase == tunrun::ShipTransactionStatus::InsufficientAetherShards) {
        app.hangarMessage = "Not enough Aether Shards.";
        return;
    }
    if (purchase == tunrun::ShipTransactionStatus::InsufficientSingularityCores) {
        app.hangarMessage = "Not enough Singularity Cores.";
        return;
    }
    if (purchase != tunrun::ShipTransactionStatus::Purchased) {
        app.hangarMessage = "Ship purchase was rejected.";
        return;
    }

    const tunrun::Profile previousProfile = app.profile;
    app.profile = candidate;
    // A purchase unlocks the ship only. The currently equipped ship stays active.
    if (!persistProfile(app)) {
        app.profile = previousProfile;
        app.hangarMessage = "Save failed; unlock and wallet change were rolled back.";
        return;
    }
    app.hangarMessage = std::string("Unlocked: ") + tunrun::shipDefinition(shipId).name +
                        ". Equip it when ready.";
}

void activateHangarShip(AppState& app, std::uint32_t shipId) {
    if (shipId >= tunrun::kProfileShipCount || shipId >= tunrun::kShipCatalog.size()) {
        app.hangarMessage = "Invalid ship selection.";
        return;
    }

    tunrun::Profile candidate = app.profile;
    const auto equip = tunrun::equipShip(candidate, shipId);
    if (equip == tunrun::ShipTransactionStatus::AlreadyEquipped) {
        app.hangarMessage = "This ship is already active.";
        return;
    }
    if (equip != tunrun::ShipTransactionStatus::Equipped) {
        app.hangarMessage = "Unlock this ship before equipping it.";
        return;
    }

    const tunrun::Profile previousProfile = app.profile;
    const int previousShip = app.selectedShip;
    app.profile = candidate;
    app.selectedShip = static_cast<int>(shipId);
    if (!persistProfile(app)) {
        app.profile = previousProfile;
        app.selectedShip = previousShip;
        app.hangarMessage = "Save failed; equip change was rolled back.";
        return;
    }
    app.hangarMessage = std::string("Equipped: ") + tunrun::shipDefinition(shipId).name;
}

void finishRun(AppState& app) {
    if (app.runRecorded) return;
    app.runRecorded = true;
    if (app.profile.totalRuns < std::numeric_limits<std::uint64_t>::max()) ++app.profile.totalRuns;
    if (app.profile.totalCrashes < std::numeric_limits<std::uint64_t>::max()) ++app.profile.totalCrashes;
    app.profile.bestDistance = std::max(app.profile.bestDistance,
                                        static_cast<double>(std::max(0.0F, app.flight.distance)));
    const auto recordUpdate = tunrun::updateCareerBests(
        app.runScore, app.profile.bestScore, app.profile.bestCombo);
    app.newBestScore = recordUpdate.scoreImproved;
    app.newBestCombo = recordUpdate.comboImproved;
    const double rawReward = std::floor(std::max(0.0F, app.flight.distance) / 20.0F);
    const std::uint64_t distanceReward = static_cast<std::uint64_t>(
        std::clamp(rawReward, 0.0, 250000.0));
    const std::uint64_t shardRewardCap = 250000U;
    app.lastRunReward = distanceReward + std::min(
        app.runAetherPickupReward, shardRewardCap - distanceReward);
    app.lastRunCoreReward = std::min<std::uint64_t>(app.runGatesCleared / 10U, 1000U);
    const std::uint64_t coreRewardRoom = 1000U - app.lastRunCoreReward;
    app.lastRunCoreReward += std::min(
        app.runSingularityCorePickupReward, coreRewardRoom);
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
             int firstY, bool confirmEnabled = true, int dangerIndex = -1,
             bool navigationEnabled = true, float buttonHeight = kButtonHeight,
             float buttonGap = kButtonGap) {
    if (navigationEnabled) {
        moveSelection(static_cast<int>(labels.size()), selection);
    } else if (labels.empty()) {
        selection = 0;
    } else {
        selection = std::clamp(selection, 0, static_cast<int>(labels.size()) - 1);
    }
    const int rowHeight = static_cast<int>(buttonHeight + buttonGap);
    const int top = firstY < 0
        ? (GetScreenHeight() - static_cast<int>(labels.size()) * rowHeight) / 2
        : firstY;
    for (int i = 0; i < static_cast<int>(labels.size()); ++i) {
        const Rectangle bounds{
            (static_cast<float>(GetScreenWidth()) - kPanelWidth) / 2.0F,
            static_cast<float>(top + i * rowHeight),
            kPanelWidth, buttonHeight
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

Vector3 rayVector(tunrun::FrameVector3 p) { return Vector3{p.x,p.y,p.z}; }

Vector3 tunnelPoint(const tunrun::TunnelFrame& frame,int side) {
    constexpr int sides=20;
    const float angle=static_cast<float>(side)*2.0F*PI/sides;
    return rayVector(tunrun::tunnelFramePoint(frame,
        std::cos(angle)*frame.radius,std::sin(angle)*frame.radius));
}
void drawProceduralGate(std::uint64_t seed,float playerDistance,
                         const tunrun::ProceduralGate& gate) {
    const float ahead=static_cast<float>(gate.distance-static_cast<double>(playerDistance));
    if(ahead < -1.0F || ahead > 108.0F) return;
    const auto frame=tunrun::sampleTunnelFrame(seed,playerDistance,gate.distance);
    const float outerRadius=frame.radius-0.24F;
    constexpr int segments=24;
    const auto p=[&](float angle,float radius,float ox,float oy) {
        return rayVector(tunrun::tunnelFramePoint(frame,
            ox+std::cos(angle)*radius,oy+std::sin(angle)*radius));
    };
    Color outer{100,130,164,220},inner{200,229,255,255};
    switch(gate.kind) {
    case tunrun::GateKind::Standard: break;
    case tunrun::GateKind::Precision: outer={146,81,66,225}; inner={255,171,131,255}; break;
    case tunrun::GateKind::Offset: outer={108,92,154,225}; inner={194,172,255,255}; break;
    case tunrun::GateKind::Wide: outer={70,133,120,220}; inner={150,245,213,255}; break;
    }
    outer = tunnelDepthFog(outer, std::max(0.0F, ahead));
    inner = tunnelDepthFog(inner, std::max(0.0F, ahead));
    for(int i=0;i<segments;++i) {
        const float a0=static_cast<float>(i)*2.0F*PI/segments;
        const float a1=static_cast<float>(i+1)*2.0F*PI/segments;
        const Vector3 o0=p(a0,outerRadius,0,0),o1=p(a1,outerRadius,0,0);
        const Vector3 i0=p(a0,gate.apertureRadius,gate.offsetX,gate.offsetY);
        const Vector3 i1=p(a1,gate.apertureRadius,gate.offsetX,gate.offsetY);
        // Opaque bulkhead panels close the space outside the true aperture.
        // The old ring-only gate let the whole next section show through.
        const Color bulkhead = tunnelDepthFog((i % 4 == 0)
            ? Color{26, 38, 54, 255} : Color{13, 20, 31, 255},
            std::max(0.0F, ahead));
        DrawTriangle3D(o0, o1, i1, bulkhead);
        DrawTriangle3D(o0, i1, i0, bulkhead);
        DrawLine3D(o0,o1,outer); DrawLine3D(i0,i1,inner);
        if(i%2==0) DrawLine3D(o0,i0,tunnelDepthFog(
            Color{76,99,125,205}, std::max(0.0F, ahead)));
    }
    const auto halo=[&](float radius,Color c) {
        c = tunnelDepthFog(c, std::max(0.0F, ahead));
        for(int i=0;i<segments;++i) {
            const float a0=static_cast<float>(i)*2.0F*PI/segments;
            const float a1=static_cast<float>(i+1)*2.0F*PI/segments;
            DrawLine3D(p(a0,radius,gate.offsetX,gate.offsetY),
                       p(a1,radius,gate.offsetX,gate.offsetY),c);
        }
    };
    if(gate.kind==tunrun::GateKind::Precision) {
        const Color raw{255,171,131,180};
        const Color c=tunnelDepthFog(raw,std::max(0.0F,ahead));
        halo(gate.apertureRadius+0.22F,raw);
        for(int i=0;i<segments;i+=3) {
            const float a=static_cast<float>(i)*2.0F*PI/segments;
            DrawLine3D(p(a,gate.apertureRadius,gate.offsetX,gate.offsetY),
                       p(a,gate.apertureRadius+0.22F,gate.offsetX,gate.offsetY),c);
        }
    } else if(gate.kind==tunrun::GateKind::Wide) {
        const Color raw{150,245,213,180};
        const Color c=tunnelDepthFog(raw,std::max(0.0F,ahead));
        halo(gate.apertureRadius+0.34F,raw);
        for(int i=0;i<segments;i+=6) {
            const float a=static_cast<float>(i)*2.0F*PI/segments;
            DrawLine3D(p(a,gate.apertureRadius,gate.offsetX,gate.offsetY),
                       p(a,gate.apertureRadius+0.34F,gate.offsetX,gate.offsetY),c);
        }
    } else if(gate.kind==tunrun::GateKind::Offset) {
        const Color c=tunnelDepthFog(Color{194,172,255,235},
                                     std::max(0.0F,ahead));
        for(int i=0;i<4;++i) {
            const float a=static_cast<float>(i)*PI*0.5F;
            DrawLine3D(p(a,gate.apertureRadius+0.12F,gate.offsetX,gate.offsetY),
                       p(a,gate.apertureRadius+0.48F,gate.offsetX,gate.offsetY),c);
            DrawLine3D(p(a+0.27F,gate.apertureRadius+0.22F,gate.offsetX,gate.offsetY),
                       p(a,gate.apertureRadius+0.48F,gate.offsetX,gate.offsetY),c);
            DrawLine3D(p(a-0.27F,gate.apertureRadius+0.22F,gate.offsetX,gate.offsetY),
                       p(a,gate.apertureRadius+0.48F,gate.offsetX,gate.offsetY),c);
        }
    }

    // A second seeded channel changes the gate's physical-looking support
    // silhouette, but all details stay outside the true collision aperture.
    const auto structure = tunrun::gateStructureFamilyAt(seed, gate.index);
    const Color structureBright = tunnelDepthFog(inner, std::max(0.0F, ahead));
    const Color structureDim = tunnelDepthFog(outer, std::max(0.0F, ahead));
    const auto annulusPoint = [&](float angle, float offset) {
        return p(angle, gate.apertureRadius + offset, gate.offsetX, gate.offsetY);
    };
    constexpr float gap = tunrun::kGateStructureMinimumApertureGap;
    switch (structure) {
    case tunrun::GateStructureFamily::RadialCage: {
        halo(gate.apertureRadius + 0.78F, outer);
        for (int i = 0; i < segments; i += 4) {
            const float a = static_cast<float>(i) * 2.0F * PI / segments;
            DrawLine3D(annulusPoint(a, gap), annulusPoint(a, 0.78F), structureBright);
        }
        break;
    }
    case tunrun::GateStructureFamily::SegmentedCrown: {
        halo(gate.apertureRadius + gap, inner);
        halo(gate.apertureRadius + 0.48F, outer);
        for (int i = 0; i < segments; i += 2) {
            const float a = static_cast<float>(i) * 2.0F * PI / segments;
            DrawLine3D(annulusPoint(a, gap), annulusPoint(a, 0.46F), structureBright);
        }
        break;
    }
    case tunrun::GateStructureFamily::ChevronBrace: {
        constexpr float angleStep = 2.0F * PI / segments;
        for (int i = 0; i < segments; i += 4) {
            const float a0 = static_cast<float>(i) * angleStep;
            const float a1 = a0 + 2.0F * angleStep;
            const float a2 = a0 + 4.0F * angleStep;
            DrawLine3D(annulusPoint(a0, gap), annulusPoint(a1, 0.70F), structureBright);
            DrawLine3D(annulusPoint(a1, 0.70F), annulusPoint(a2, gap), structureBright);
        }
        break;
    }
    case tunrun::GateStructureFamily::TwinRails: {
        for (int i = 0; i < 4; ++i) {
            const float a = static_cast<float>(i) * PI * 0.5F;
            DrawLine3D(annulusPoint(a - 0.10F, gap), annulusPoint(a - 0.10F, 0.80F), structureBright);
            DrawLine3D(annulusPoint(a + 0.10F, gap), annulusPoint(a + 0.10F, 0.80F), structureBright);
            DrawLine3D(annulusPoint(a - 0.10F, 0.39F), annulusPoint(a + 0.10F, 0.39F), structureDim);
            DrawLine3D(annulusPoint(a - 0.10F, 0.66F), annulusPoint(a + 0.10F, 0.66F), structureDim);
        }
        break;
    }
    case tunrun::GateStructureFamily::SplitClamps: {
        for (int i = 0; i < 4; ++i) {
            const float a = static_cast<float>(i) * PI * 0.5F;
            DrawLine3D(annulusPoint(a - 0.16F, gap), annulusPoint(a - 0.16F, 0.62F), structureBright);
            DrawLine3D(annulusPoint(a - 0.16F, 0.62F), annulusPoint(a + 0.16F, 0.62F), structureBright);
            DrawLine3D(annulusPoint(a + 0.16F, 0.62F), annulusPoint(a + 0.16F, gap), structureBright);
            DrawLine3D(annulusPoint(a - 0.16F, 0.38F), annulusPoint(a + 0.16F, 0.38F), structureDim);
        }
        break;
    }
    }
}
void drawGateThroat(std::uint64_t seed, float playerDistance,
                    const tunrun::ProceduralGate& gate) {
    constexpr float halfLength = tunrun::kGateThroatHalfLength;
    const float ahead = static_cast<float>(
        gate.distance - static_cast<double>(playerDistance));
    if (ahead < -halfLength - 3.0F || ahead > halfLength + 24.0F) return;

    // A tapered opaque sleeve creates a deep passage through each aperture.
    // Its minimum radius and center match gate physics, so the visible opening
    // never promises extra collision clearance.
    constexpr std::array<float, 7> offsets{{-18.0F, -12.0F, -6.0F, 0.0F,
                                            6.0F, 12.0F, 18.0F}};
    constexpr int segments = 24;
    std::array<tunrun::TunnelFrame, offsets.size()> frames{};
    std::array<float, offsets.size()> radii{};
    std::array<float, offsets.size()> centerX{};
    std::array<float, offsets.size()> centerY{};
    for (std::size_t ring = 0U; ring < offsets.size(); ++ring) {
        const float offset = offsets[ring];
        frames[ring] = tunrun::sampleTunnelFrame(
            seed, playerDistance, gate.distance + static_cast<double>(offset));
        const auto section = tunrun::gateThroatSectionAtDistance(
            seed, gate, gate.distance + static_cast<double>(offset));
        radii[ring] = section.radius;
        centerX[ring] = section.centerX;
        centerY[ring] = section.centerY;
    }

    Color wallDark{9, 17, 28, 255};
    Color wallLight{22, 42, 59, 255};
    Color edge{61, 121, 157, 205};
    switch (gate.kind) {
    case tunrun::GateKind::Precision:
        wallLight = Color{56, 30, 35, 255}; edge = Color{255, 142, 113, 220}; break;
    case tunrun::GateKind::Offset:
        wallLight = Color{35, 29, 55, 255}; edge = Color{181, 147, 255, 220}; break;
    case tunrun::GateKind::Wide:
        wallLight = Color{19, 47, 43, 255}; edge = Color{107, 240, 207, 220}; break;
    case tunrun::GateKind::Standard:
        break;
    }

    const auto point = [](const tunrun::TunnelFrame& frame, float cx, float cy,
                          float radius, float angle) {
        return rayVector(tunrun::tunnelFramePoint(
            frame, cx + std::cos(angle) * radius,
            cy + std::sin(angle) * radius));
    };
    for (std::size_t ring = 0U; ring + 1U < offsets.size(); ++ring) {
        const auto& current = frames[ring];
        const auto& next = frames[ring + 1U];
        for (int side = 0; side < segments; ++side) {
            const int nextSide = (side + 1) % segments;
            const float a0 = static_cast<float>(side) * 2.0F * PI / segments;
            const float a1 = static_cast<float>(nextSide) * 2.0F * PI / segments;
            // A small axial twist turns the sleeve into a corkscrew passage.
            // At the gate plane the added twist is exactly zero, preserving
            // the aperture's alignment with the gameplay collision model.
            const float twistA = offsets[ring] * 0.022F;
            const float twistB = offsets[ring + 1U] * 0.022F;
            const Vector3 a = point(current, centerX[ring], centerY[ring], radii[ring], a0 + twistA);
            const Vector3 b = point(current, centerX[ring], centerY[ring], radii[ring], a1 + twistA);
            const Vector3 c = point(next, centerX[ring + 1U], centerY[ring + 1U],
                                    radii[ring + 1U], a1 + twistB);
            const Vector3 d = point(next, centerX[ring + 1U], centerY[ring + 1U],
                                    radii[ring + 1U], a0 + twistB);
            const float wallAhead = std::max(0.0F, static_cast<float>(
                gate.distance + static_cast<double>(offsets[ring]) -
                static_cast<double>(playerDistance)));
            const Color wall = tunnelDepthFog(
                ((side % 4) == 0 || (ring % 2U) == 0) ? wallLight : wallDark,
                wallAhead);
            DrawTriangle3D(a, b, c, wall);
            DrawTriangle3D(a, c, d, wall);
            if (ring % 2U == 1U) DrawLine3D(a, b, tunnelDepthFog(edge, wallAhead));
        }
    }
}

void drawProceduralReward(std::uint64_t seed,float playerDistance,
                          const tunrun::ProceduralReward& reward) {
    const float ahead=static_cast<float>(reward.distance-static_cast<double>(playerDistance));
    if(ahead<0.0F||ahead>108.0F) return;
    const auto frame=tunrun::sampleTunnelFrame(seed,playerDistance,reward.distance);
    const auto p=[&](float x,float y,float z=0.0F) {
        return rayVector(tunrun::tunnelFramePoint(frame,x,y,z));
    };
    const float x=reward.offsetX,y=reward.offsetY;
    const float size=reward.kind==tunrun::RewardKind::SingularityCore?0.48F:0.34F;
    const bool core=reward.kind==tunrun::RewardKind::SingularityCore;
    const Color color=tunnelDepthFog(
        core?Color{255,174,108,255}:Color{111,225,255,255}, ahead);
    const Color bright=tunnelDepthFog(
        core?Color{255,204,140,255}:Color{165,243,255,255}, ahead);
    const Color dark=tunnelDepthFog(
        core?Color{132,57,29,255}:Color{27,107,153,255}, ahead);
    const Vector3 top=p(x,y+size),right=p(x+size*0.72F,y);
    const Vector3 bottom=p(x,y-size),left=p(x-size*0.72F,y);
    const Vector3 front=p(x,y,size*0.42F),back=p(x,y,-size*0.42F);
    DrawTriangle3D(top,front,right,bright); DrawTriangle3D(top,left,front,color);
    DrawTriangle3D(top,back,left,dark); DrawTriangle3D(top,right,back,bright);
    DrawTriangle3D(bottom,right,front,dark); DrawTriangle3D(bottom,front,left,bright);
    DrawTriangle3D(bottom,left,back,dark); DrawTriangle3D(bottom,back,right,color);
    if(core) DrawSphereWires(p(x,y),size*0.82F,6,12,bright);
    DrawLine3D(top,right,color); DrawLine3D(right,bottom,color);
    DrawLine3D(bottom,left,color); DrawLine3D(left,top,color);
    DrawLine3D(top,front,color); DrawLine3D(right,front,color);
    DrawLine3D(bottom,front,color); DrawLine3D(left,front,color);
    DrawLine3D(top,back,color); DrawLine3D(right,back,color);
    DrawLine3D(bottom,back,color); DrawLine3D(left,back,color);
}

void drawPlayerShip(std::uint32_t shipId, float shipX, float shipY,
                    float pitch, float yaw, float roll,
                    const tunrun::TunnelFrame* tunnelFrame = nullptr,
                    float visibility = 1.0F) {
    if (!std::isfinite(visibility)) visibility = 1.0F;
    visibility = std::clamp(visibility, 0.0F, 1.0F);
    if (visibility <= 0.001F) return;
    const auto withVisibility = [visibility](Color c) {
        c.a = static_cast<unsigned char>(std::clamp(
            std::round(static_cast<float>(c.a) * visibility), 0.0F, 255.0F));
        return c;
    };
    const Color hullColors[] = {
        kAccent, Color{190, 157, 255, 255}, Color{255, 186, 116, 255},
        Color{115, 238, 207, 255}, Color{255, 125, 145, 255},
        Color{187, 166, 255, 255}, Color{255, 218, 130, 255},
        Color{165, 190, 218, 255}
    };
    Color color = hullColors[shipId < 8U ? shipId : 0U];
    color.a = static_cast<unsigned char>(std::round(255.0F * visibility));
    const Color hullLight{
        static_cast<unsigned char>(std::min(255, static_cast<int>(color.r) + 34)),
        static_cast<unsigned char>(std::min(255, static_cast<int>(color.g) + 34)),
        static_cast<unsigned char>(std::min(255, static_cast<int>(color.b) + 34)), color.a};
    const Color hullShade{
        static_cast<unsigned char>(color.r * 0.48F),
        static_cast<unsigned char>(color.g * 0.48F),
        static_cast<unsigned char>(color.b * 0.48F), color.a};
    const float cp = std::cos(pitch), sp = std::sin(pitch);
    const float cy = std::cos(yaw), sy = std::sin(yaw);
    const float cr = std::cos(roll), sr = std::sin(roll);
    const auto v = [shipX, shipY, cp, sp, cy, sy, cr, sr, tunnelFrame](float x, float y, float z) {
        const float pitchedY = y * cp - z * sp;
        const float pitchedZ = y * sp + z * cp;
        // Positive yaw is a right turn in the flight controller. The model's
        // nose points along -Z, so use this matching rotation to keep rendered
        // heading, horizontal drift, and the direction cue in agreement.
        const float yawedX = x * cy - pitchedZ * sy;
        const float yawedZ = x * sy + pitchedZ * cy;
        const float rolledX = yawedX * cr - pitchedY * sr;
        const float rolledY = yawedX * sr + pitchedY * cr;
        const float localX = shipX + rolledX;
        const float localY = shipY + rolledY;
        if (tunnelFrame != nullptr) {
            return rayVector(tunrun::tunnelFramePoint(*tunnelFrame, localX, localY, yawedZ));
        }
        return Vector3{localX, localY, yawedZ};
    };
    const float bodyWidthScales[]  = {0.92F, 0.78F, 1.25F, 1.08F, 0.68F, 0.91F, 0.86F, 1.16F};
    const float bodyHeightScales[] = {0.92F, 0.86F, 1.20F, 0.78F, 0.76F, 0.92F, 1.10F, 1.12F};
    const float bodyLengthScales[] = {1.00F, 1.10F, 1.04F, 0.99F, 1.28F, 1.08F, 1.02F, 1.12F};
    const std::size_t bodyIndex = shipId < 8U ? static_cast<std::size_t>(shipId) : 0U;
    const float bodyWidth = bodyWidthScales[bodyIndex];
    const float bodyHeight = bodyHeightScales[bodyIndex];
    const float bodyLength = bodyLengthScales[bodyIndex];
    const auto body = [&](float x, float y, float z) {
        return v(x * bodyWidth, y * bodyHeight, z * bodyLength);
    };
    const auto line = [color](Vector3 a, Vector3 b) { DrawLine3D(a, b, color); };
    const auto quad = [](Vector3 a, Vector3 b, Vector3 c, Vector3 d,
                         Color upper, Color lower) {
        DrawTriangle3D(a, b, c, upper);
        DrawTriangle3D(a, c, d, lower);
    };
    const auto wing = [&](float side, float rootFrontX, float rootFrontZ,
                          float tipFrontX, float tipFrontZ,
                          float tipBackX, float tipBackZ,
                          float rootBackX, float rootBackZ, float wingY) {
        const Vector3 rootFront = v(side * rootFrontX, 0.055F, rootFrontZ);
        const Vector3 tipFront = v(side * tipFrontX, wingY, tipFrontZ);
        const Vector3 tipBack = v(side * tipBackX, wingY - 0.025F, tipBackZ);
        const Vector3 rootBack = v(side * rootBackX, -0.045F, rootBackZ);
        quad(rootFront, tipFront, tipBack, rootBack, hullLight, color);
        const Vector3 lowerRootFront = v(side * rootFrontX, 0.010F, rootFrontZ);
        const Vector3 lowerTipFront = v(side * tipFrontX, wingY - 0.050F, tipFrontZ);
        const Vector3 lowerTipBack = v(side * tipBackX, wingY - 0.075F, tipBackZ);
        const Vector3 lowerRootBack = v(side * rootBackX, -0.090F, rootBackZ);
        quad(lowerRootFront, lowerRootBack, lowerTipBack, lowerTipFront, hullShade, color);
        DrawTriangle3D(rootFront, lowerRootFront, lowerTipFront, hullShade);
        DrawTriangle3D(rootFront, lowerTipFront, tipFront, hullLight);
        DrawTriangle3D(tipFront, lowerTipFront, lowerTipBack, color);
        DrawTriangle3D(tipFront, lowerTipBack, tipBack, hullShade);
        DrawTriangle3D(tipBack, lowerTipBack, lowerRootBack, color);
        DrawTriangle3D(tipBack, lowerRootBack, rootBack, hullShade);
        DrawTriangle3D(rootBack, lowerRootBack, lowerRootFront, hullShade);
        DrawTriangle3D(rootBack, lowerRootFront, rootFront, color);
        line(rootFront, tipFront);
        line(tipFront, tipBack);
        line(tipBack, rootBack);
        line(rootBack, rootFront);
        const Vector3 sparMid = v(side * ((rootFrontX + tipBackX) * 0.46F),
                                  wingY + 0.012F,
                                  rootFrontZ + (tipBackZ - rootFrontZ) * 0.62F);
        line(rootFront, sparMid);
        line(sparMid, tipBack);
    };
    // Filled, ship-specific wing planforms replace the old wireframe-only
    // silhouettes with eight visibly different 3D spacecraft.
    switch (shipId) {
    case 0U: // DRIFTWING — balanced swept delta wings.
        wing(-1.0F, .18F, -.12F, .96F, .30F, .48F, .82F, .22F, .55F, -.045F);
        wing( 1.0F, .18F, -.12F, .96F, .30F, .48F, .82F, .22F, .55F, -.045F);
        break;
    case 1U: // WRAITH — long, narrow swept interceptor wings.
        wing(-1.0F, .16F, -.23F, .78F, .35F, .42F, .92F, .18F, .58F, -.085F);
        wing( 1.0F, .16F, -.23F, .78F, .35F, .42F, .92F, .18F, .58F, -.085F);
        break;
    case 2U: // BULWARK — wide armored lifting planes.
        wing(-1.0F, .29F, -.04F, 1.08F, .22F, .91F, .84F, .31F, .62F, -.10F);
        wing( 1.0F, .29F, -.04F, 1.08F, .22F, .91F, .84F, .31F, .62F, -.10F);
        break;
    case 3U: // MANTA — broad flowing manta-shaped wings.
        wing(-1.0F, .19F, -.27F, 1.12F, .10F, .58F, .84F, .27F, .55F, -.035F);
        wing( 1.0F, .19F, -.27F, 1.12F, .10F, .58F, .84F, .27F, .55F, -.035F);
        break;
    case 4U: // COMET — needle hull with small high-speed stabilisers.
        wing(-1.0F, .12F, -.18F, .60F, .08F, .34F, .82F, .16F, .61F, .005F);
        wing( 1.0F, .12F, -.18F, .60F, .08F, .34F, .82F, .16F, .61F, .005F);
        break;
    case 5U: // SPECTRE — split nose prongs and dual swept planes.
        wing(-1.0F, .21F, -.04F, .91F, .23F, .52F, .78F, .23F, .57F, -.065F);
        wing( 1.0F, .21F, -.04F, .91F, .23F, .52F, .78F, .23F, .57F, -.065F);
        wing(-1.0F, .14F, -.52F, .57F, -.49F, .42F, -.20F, .15F, -.12F, .04F);
        wing( 1.0F, .14F, -.52F, .57F, -.49F, .42F, -.20F, .15F, -.12F, .04F);
        break;
    case 6U: // VORTEX — diamond wings with angular cross-bracing.
        wing(-1.0F, .15F, -.20F, .84F, .10F, .84F, .68F, .20F, .55F, .015F);
        wing( 1.0F, .15F, -.20F, .84F, .10F, .84F, .68F, .20F, .55F, .015F);
        break;
    case 7U: // OBSIDIAN — heavy angular interceptor planform.
        wing(-1.0F, .24F, -.20F, .98F, .25F, .55F, .94F, .27F, .59F, -.075F);
        wing( 1.0F, .24F, -.20F, .98F, .25F, .55F, .94F, .27F, .59F, -.075F);
        break;
    default: break;
    }
    // Broad faceted central fuselage, canopy, and rear engine bells.
    const auto nose = body(0.0F, 0.0F, -0.88F);
    const auto top = body(0.0F, 0.26F, 0.03F);
    const auto bottom = body(0.0F, -0.21F, 0.30F);
    const auto left = body(-0.36F, -0.015F, 0.23F);
    const auto right = body(0.36F, -0.015F, 0.23F);
    const auto tail = body(0.0F, 0.045F, 1.00F);
    DrawTriangle3D(nose, top, left, hullLight);
    DrawTriangle3D(nose, right, top, color);
    DrawTriangle3D(nose, bottom, right, hullShade);
    DrawTriangle3D(nose, left, bottom, color);
    DrawTriangle3D(top, tail, left, hullLight);
    DrawTriangle3D(top, right, tail, color);
    DrawTriangle3D(left, tail, bottom, hullShade);
    DrawTriangle3D(bottom, tail, right, hullShade);
    // Blue-glass canopy and engine bells add recognizable spacecraft details.
    DrawTriangle3D(body(-0.15F, 0.135F, -0.28F),
                   body(0.0F, 0.205F, -0.48F),
                   body(0.15F, 0.135F, -0.28F), withVisibility(Color{43, 90, 130, 255}));
    DrawTriangle3D(body(-0.15F, 0.135F, -0.28F),
                   body(0.15F, 0.135F, -0.28F),
                   body(0.0F, 0.155F, -0.04F), withVisibility(Color{26, 57, 88, 255}));
    Color engineGlow = shipId == 4U ? Color{255, 139, 96, 255}
        : shipId == 6U ? Color{179, 123, 255, 255}
        : shipId == 7U ? Color{255, 90, 110, 255} : Color{88, 226, 255, 255};
    engineGlow.a = color.a;
    const float engineScale = std::min(bodyWidth, bodyHeight);
    const auto enginePod = [&](float x, float y, float z, float radius, float length) {
        const Vector3 start = body(x, y, z);
        const Vector3 end = body(x, y, z + length);
        DrawCylinderEx(start, end, radius * engineScale,
                       radius * 0.76F * engineScale, 10, hullShade);
        DrawCylinderEx(start, end, radius * 0.46F * engineScale,
                       radius * 0.40F * engineScale, 10,
                       withVisibility(Color{14, 19, 29, 255}));
        DrawSphere(end, radius * 0.72F * engineScale, engineGlow);
        DrawSphere(end, radius * 0.36F * engineScale,
                   withVisibility(Color{215, 248, 255, 255}));
    };
    enginePod(-0.205F, -0.06F, 0.58F, 0.095F, 0.31F);
    enginePod( 0.205F, -0.06F, 0.58F, 0.095F, 0.31F);
    if (shipId == 2U) {
        // Bulwark: four-engine heavy lifter with wider-set auxiliary pods.
        enginePod(-0.43F, -0.10F, 0.50F, 0.070F, 0.25F);
        enginePod( 0.43F, -0.10F, 0.50F, 0.070F, 0.25F);
    } else if (shipId == 4U) {
        // Comet: an additional narrow centerline thruster for its racer profile.
        enginePod(0.0F, -0.10F, 0.61F, 0.068F, 0.38F);
    } else if (shipId == 6U) {
        // Vortex: lateral stabilizer thrusters sit outboard of the main pair.
        enginePod(-0.43F, -0.01F, 0.45F, 0.052F, 0.26F);
        enginePod( 0.43F, -0.01F, 0.45F, 0.052F, 0.26F);
    }
    const Color detailLight = withVisibility(Color{
        static_cast<unsigned char>(std::min(255, static_cast<int>(color.r) + 52)),
        static_cast<unsigned char>(std::min(255, static_cast<int>(color.g) + 52)),
        static_cast<unsigned char>(std::min(255, static_cast<int>(color.b) + 52)), 255});
    const Color detailDark = withVisibility(Color{
        static_cast<unsigned char>(color.r * 0.32F),
        static_cast<unsigned char>(color.g * 0.32F),
        static_cast<unsigned char>(color.b * 0.32F), 255});
    switch (shipId) {
    case 0U: { // DRIFTWING: twin dorsal fins and a broad rear delta.
        DrawTriangle3D(body(-0.17F, 0.07F, 0.34F), body(-0.04F, 0.35F, 0.86F),
                       body(-0.03F, 0.08F, 0.78F), detailLight);
        DrawTriangle3D(body(0.17F, 0.07F, 0.34F), body(0.03F, 0.08F, 0.78F),
                       body(0.04F, 0.35F, 0.86F), color);
        break;
    }
    case 1U: { // WRAITH: raised dorsal blade and paired front sensor rails.
        DrawTriangle3D(body(-0.08F, 0.13F, 0.18F), body(0.0F, 0.56F, 0.72F),
                       body(-0.02F, 0.12F, 0.82F), detailLight);
        DrawTriangle3D(body(0.08F, 0.13F, 0.18F), body(0.02F, 0.12F, 0.82F),
                       body(0.0F, 0.56F, 0.72F), color);
        line(body(-0.11F, 0.10F, -0.45F), body(-0.11F, 0.10F, 0.28F));
        line(body(0.11F, 0.10F, -0.45F), body(0.11F, 0.10F, 0.28F));
        break;
    }
    case 2U: { // BULWARK: layered armored cheek plates and squared shoulders.
        DrawTriangle3D(body(-0.30F, 0.02F, 0.02F), body(-0.48F, -0.02F, 0.47F),
                       body(-0.29F, 0.12F, 0.58F), detailLight);
        DrawTriangle3D(body(0.30F, 0.02F, 0.02F), body(0.29F, 0.12F, 0.58F),
                       body(0.48F, -0.02F, 0.47F), color);
        DrawTriangle3D(body(-0.30F, -0.05F, 0.04F), body(-0.29F, -0.13F, 0.52F),
                       body(-0.48F, -0.02F, 0.47F), detailDark);
        DrawTriangle3D(body(0.30F, -0.05F, 0.04F), body(0.48F, -0.02F, 0.47F),
                       body(0.29F, -0.13F, 0.52F), detailDark);
        break;
    }
    case 3U: { // MANTA: flowing shoulder fins and back vents.
        DrawTriangle3D(body(-0.18F, 0.10F, 0.18F), body(-0.68F, 0.06F, 0.45F),
                       body(-0.32F, 0.12F, 0.70F), detailLight);
        DrawTriangle3D(body(0.18F, 0.10F, 0.18F), body(0.32F, 0.12F, 0.70F),
                       body(0.68F, 0.06F, 0.45F), color);
        for (int vent = -1; vent <= 1; ++vent) {
            const float offset = static_cast<float>(vent) * 0.075F;
            line(body(offset, 0.11F, 0.28F), body(offset, 0.11F, 0.58F));
        }
        break;
    }
    case 4U: { // COMET: long nose ridge, small upright tail fins.
        DrawTriangle3D(body(-0.07F, 0.10F, -0.72F), body(0.0F, 0.24F, 0.38F),
                       body(0.0F, 0.11F, 0.74F), detailLight);
        DrawTriangle3D(body(0.07F, 0.10F, -0.72F), body(0.0F, 0.11F, 0.74F),
                       body(0.0F, 0.24F, 0.38F), color);
        DrawTriangle3D(body(-0.16F, 0.02F, 0.48F), body(-0.22F, 0.26F, 0.85F),
                       body(-0.12F, 0.02F, 0.86F), detailDark);
        DrawTriangle3D(body(0.16F, 0.02F, 0.48F), body(0.12F, 0.02F, 0.86F),
                       body(0.22F, 0.26F, 0.85F), detailDark);
        break;
    }
    case 5U: { // SPECTRE: split-prong nose leaves a visible center gap.
        DrawTriangle3D(body(-0.25F, 0.04F, -0.66F), body(-0.08F, 0.13F, -0.27F),
                       body(-0.19F, 0.00F, -0.08F), detailLight);
        DrawTriangle3D(body(0.25F, 0.04F, -0.66F), body(0.19F, 0.00F, -0.08F),
                       body(0.08F, 0.13F, -0.27F), color);
        DrawTriangle3D(body(-0.10F, 0.05F, -0.30F), body(0.0F, 0.16F, -0.44F),
                       body(0.10F, 0.05F, -0.30F), detailDark);
        break;
    }
    case 6U: { // VORTEX: elevated sensor crown and diamond dorsal planes.
        DrawTriangle3D(body(-0.12F, 0.13F, 0.05F), body(0.0F, 0.52F, 0.43F),
                       body(-0.03F, 0.13F, 0.72F), detailLight);
        DrawTriangle3D(body(0.12F, 0.13F, 0.05F), body(0.03F, 0.13F, 0.72F),
                       body(0.0F, 0.52F, 0.43F), color);
        line(body(-0.40F, 0.05F, 0.22F), body(0.0F, 0.30F, 0.48F));
        line(body(0.40F, 0.05F, 0.22F), body(0.0F, 0.30F, 0.48F));
        break;
    }
    case 7U: { // OBSIDIAN: reinforced spine and shielded tail fins.
        DrawTriangle3D(body(-0.20F, 0.13F, -0.28F), body(0.0F, 0.32F, 0.53F),
                       body(-0.13F, 0.12F, 0.78F), detailLight);
        DrawTriangle3D(body(0.20F, 0.13F, -0.28F), body(0.13F, 0.12F, 0.78F),
                       body(0.0F, 0.32F, 0.53F), color);
        DrawTriangle3D(body(-0.36F, -0.02F, 0.30F), body(-0.54F, 0.24F, 0.86F),
                       body(-0.30F, 0.03F, 0.77F), detailDark);
        DrawTriangle3D(body(0.36F, -0.02F, 0.30F), body(0.30F, 0.03F, 0.77F),
                       body(0.54F, 0.24F, 0.86F), detailDark);
        break;
    }
    default:
        break;
    }
}

void drawHangarShipPreview3D(std::uint32_t shipId, int centerX, int centerY,
                              double elapsedSeconds) {
    const Rectangle bounds{static_cast<float>(centerX - 150),
                           static_cast<float>(centerY - 92), 300.0F, 184.0F};
    DrawRectangleRounded(bounds, 0.06F, 8, Color{10, 14, 21, 245});
    DrawRectangleRoundedLinesEx(bounds, 0.06F, 8, 1.0F, kEdge);
    BeginScissorMode(static_cast<int>(bounds.x), static_cast<int>(bounds.y),
                     static_cast<int>(bounds.width), static_cast<int>(bounds.height));
    Camera3D camera{};
    camera.position = Vector3{2.8F, 2.0F, 4.8F};
    camera.target = Vector3{0.0F, 0.0F, 0.0F};
    camera.up = Vector3{0.0F, 1.0F, 0.0F};
    camera.fovy = 38.0F;
    camera.projection = CAMERA_PERSPECTIVE;
    BeginMode3D(camera);
    DrawLine3D(Vector3{-1.6F, -0.85F, -1.2F}, Vector3{1.6F, -0.85F, -1.2F},
               Color{42, 58, 74, 255});
    DrawLine3D(Vector3{-1.6F, -0.85F, 1.2F}, Vector3{1.6F, -0.85F, 1.2F},
               Color{42, 58, 74, 255});
    DrawLine3D(Vector3{-1.6F, -0.85F, -1.2F}, Vector3{-1.6F, -0.85F, 1.2F},
               Color{42, 58, 74, 255});
    DrawLine3D(Vector3{1.6F, -0.85F, -1.2F}, Vector3{1.6F, -0.85F, 1.2F},
               Color{42, 58, 74, 255});
    const float turn = static_cast<float>(elapsedSeconds);
    drawPlayerShip(shipId, 0.0F, 0.0F, 0.20F + std::sin(turn * 0.7F) * 0.12F,
                   turn * 0.65F, std::sin(turn * 0.4F) * 0.18F);
    EndMode3D();
    EndScissorMode();
    DrawText("LIVE 3D HOLOGRAM", static_cast<int>(bounds.x) + 12,
             static_cast<int>(bounds.y) + 10, 10, kAccent);
}

void drawProceduralHazard(std::uint64_t seed,float playerDistance,
                           float elapsedSeconds,const tunrun::ProceduralHazard& hazard) {
    const float ahead=static_cast<float>(hazard.distance-static_cast<double>(playerDistance));
    if(ahead < -1.5F || ahead > 108.0F) return;
    const auto frame=tunrun::sampleTunnelFrame(seed,playerDistance,hazard.distance);
    const auto moving=tunrun::hazardCenterAt(hazard,elapsedSeconds);
    const auto p=[&](float x,float y,float z=0.0F) {
        return rayVector(tunrun::tunnelFramePoint(frame,x,y,z));
    };
    const float cx=moving.x,cy=moving.y;
    const Vector3 center=p(cx,cy);
    const auto warning = tunrun::hazardWarningProfileAt(ahead, elapsedSeconds);
    Color color{}, blade{}, shell{}, facetLight{}, facetDark{};
    const auto family = tunrun::hazardVisualFamilyAt(seed, hazard.index);
    switch (family) {
    case tunrun::HazardVisualFamily::Orbital:
        color={255,103,91,255}; blade={255,190,145,240}; shell={112,35,38,255};
        facetLight={220,91,70,255}; facetDark={79,27,36,255}; break;
    case tunrun::HazardVisualFamily::Prism:
        color={207,119,255,255}; blade={246,185,255,245}; shell={64,27,88,255};
        facetLight={223,151,255,255}; facetDark={62,29,91,255}; break;
    case tunrun::HazardVisualFamily::Rotor:
        color={77,220,255,255}; blade={181,249,255,245}; shell={17,62,86,255};
        facetLight={123,239,255,255}; facetDark={21,83,110,255}; break;
    case tunrun::HazardVisualFamily::Cross:
        color={255,211,91,255}; blade={255,244,174,245}; shell={98,62,20,255};
        facetLight={255,226,122,255}; facetDark={112,65,17,255}; break;
    case tunrun::HazardVisualFamily::HaloArray:
        color={92,245,205,255}; blade={196,255,239,245}; shell={13,73,62,255};
        facetLight={109,255,218,255}; facetDark={17,86,78,255}; break;
    case tunrun::HazardVisualFamily::ShardCluster:
        color={255,151,91,255}; blade={255,221,172,245}; shell={91,39,19,255};
        facetLight={255,190,120,255}; facetDark={122,52,27,255}; break;
    }
    color=tunnelDepthFog(color,ahead);
    blade=tunnelDepthFog(blade,ahead);
    shell=tunnelDepthFog(shell,ahead);
    facetLight=tunnelDepthFog(facetLight,ahead);
    facetDark=tunnelDepthFog(facetDark,ahead);
    const float phase=elapsedSeconds*(1.1F+hazard.frequency*0.4F)+
                      static_cast<float>(hazard.index)*0.73F;
    switch (family) {
    case tunrun::HazardVisualFamily::Orbital:
        DrawSphere(center,hazard.radius*0.72F,shell);
        DrawSphereWires(center,hazard.radius,8,12,color);
        DrawLine3D(p(cx-hazard.radius,cy),p(cx+hazard.radius,cy),blade);
        DrawLine3D(p(cx,cy-hazard.radius),p(cx,cy+hazard.radius),blade);
        break;
    case tunrun::HazardVisualFamily::Prism: {
        const Vector3 top=p(cx,cy+hazard.radius),right=p(cx+hazard.radius*0.78F,cy);
        const Vector3 bottom=p(cx,cy-hazard.radius),left=p(cx-hazard.radius*0.78F,cy);
        const Vector3 front=p(cx,cy,hazard.radius*0.55F),back=p(cx,cy,-hazard.radius*0.55F);
        
        DrawTriangle3D(top,right,front,facetLight);DrawTriangle3D(right,bottom,front,color);
        DrawTriangle3D(bottom,left,front,facetDark);DrawTriangle3D(left,top,front,facetLight);
        DrawTriangle3D(top,back,right,facetDark);DrawTriangle3D(right,back,bottom,facetLight);
        DrawTriangle3D(bottom,back,left,color);DrawTriangle3D(left,back,top,facetDark);
        DrawLine3D(top,right,color);DrawLine3D(right,bottom,color);
        DrawLine3D(bottom,left,color);DrawLine3D(left,top,color);
        DrawLine3D(top,bottom,blade);DrawLine3D(left,right,blade);
        DrawSphereWires(center,hazard.radius*0.34F,6,8,color);
        break;
    }
    case tunrun::HazardVisualFamily::Rotor: {
        for(int i=0;i<3;++i) {
            const float a=phase+static_cast<float>(i)*2.0F*PI/3.0F;
            const float tx=cx+std::cos(a)*hazard.radius,ty=cy+std::sin(a)*hazard.radius;
            const Vector3 tip=p(tx,ty);
            const Vector3 shoulder=p(cx-std::cos(a)*hazard.radius*0.55F,
                cy-std::sin(a)*hazard.radius*0.55F,std::sin(phase)*hazard.radius*0.28F);
            const Vector3 back=p(tx,ty,-hazard.radius*0.22F);
            DrawTriangle3D(center,tip,shoulder,blade);
            DrawTriangle3D(center,shoulder,back,facetDark);
            DrawLine3D(center,tip,color);DrawLine3D(tip,shoulder,blade);DrawLine3D(shoulder,center,color);
        }
        DrawSphereWires(center,hazard.radius*0.24F,6,8,blade);
        break;
    }
    case tunrun::HazardVisualFamily::Cross: {
        const float cs=std::cos(phase),sn=std::sin(phase);
        const Vector3 a=p(cx+cs*hazard.radius,cy+sn*hazard.radius);
        const Vector3 b=p(cx-cs*hazard.radius,cy-sn*hazard.radius);
        const Vector3 c=p(cx-sn*hazard.radius,cy+cs*hazard.radius);
        const Vector3 d=p(cx+sn*hazard.radius,cy-cs*hazard.radius);
        const Vector3 front=p(cx,cy,hazard.radius*0.30F),back=p(cx,cy,-hazard.radius*0.30F);
        DrawTriangle3D(a,front,c,color);DrawTriangle3D(c,back,b,blade);
        DrawTriangle3D(b,front,d,facetDark);DrawTriangle3D(d,back,a,color);
        DrawSphere(center,hazard.radius*0.22F,shell);
        DrawLine3D(a,c,color);DrawLine3D(c,b,blade);DrawLine3D(b,d,color);DrawLine3D(d,a,color);
        DrawSphereWires(center,hazard.radius*0.42F,6,8,color);
        break;
    }
    case tunrun::HazardVisualFamily::HaloArray: {
        // Three intersecting, phase-driven hoops give this mine a mechanical
        // gyroscope silhouette rather than another solid sphere or blade set.
        constexpr int ringSegments=32;
        const float ringRadius=hazard.radius*0.88F;
        const auto drawLoop=[&](int axis,float phaseOffset) {
            Vector3 previous{};
            for(int segment=0;segment<=ringSegments;++segment) {
                const float angle=phase+phaseOffset+
                    static_cast<float>(segment)*2.0F*PI/ringSegments;
                const float cs=std::cos(angle),sn=std::sin(angle);
                Vector3 point{};
                if(axis==0) point=p(cx+cs*ringRadius,cy+sn*ringRadius,
                                    std::sin(angle*2.0F+phase)*ringRadius*0.10F);
                else if(axis==1) point=p(cx+cs*ringRadius,
                    cy+sn*ringRadius*0.18F,sn*ringRadius);
                else point=p(cx+sn*ringRadius*0.18F,
                    cy+cs*ringRadius,sn*ringRadius);
                if(segment>0) DrawLine3D(previous,point,
                    (segment%4==0)?blade:color);
                previous=point;
            }
        };
        drawLoop(0,0.0F);
        drawLoop(1,0.92F);
        drawLoop(2,2.04F);
        DrawSphere(center,hazard.radius*0.20F,shell);
        DrawSphereWires(center,hazard.radius*0.28F,6,8,facetLight);
        break;
    }
    case tunrun::HazardVisualFamily::ShardCluster: {
        // Six individual crystal fins orbit a small core. Their triangular
        // faces form a faceted cluster instead of a cross-shaped silhouette.
        DrawSphere(center,hazard.radius*0.18F,shell);
        constexpr int shardCount=6;
        for(int i=0;i<shardCount;++i) {
            const float a=phase*0.52F+static_cast<float>(i)*2.0F*PI/shardCount;
            const float cs=std::cos(a),sn=std::sin(a);
            const float midX=cx+cs*hazard.radius*0.34F;
            const float midY=cy+sn*hazard.radius*0.34F;
            const float baseZ=std::sin(a*2.0F+phase)*hazard.radius*0.18F;
            const Vector3 root=p(midX,midY,baseZ);
            const Vector3 tip=p(cx+cs*hazard.radius*0.91F,
                                cy+sn*hazard.radius*0.91F,
                                baseZ+std::cos(a+phase)*hazard.radius*0.21F);
            const Vector3 edgeA=p(midX-sn*hazard.radius*0.15F,
                                  midY+cs*hazard.radius*0.15F,
                                  baseZ+hazard.radius*0.19F);
            const Vector3 edgeB=p(midX+sn*hazard.radius*0.15F,
                                  midY-cs*hazard.radius*0.15F,
                                  baseZ-hazard.radius*0.19F);
            const Color shardFace=(i%3==0)?facetLight:(i%3==1)?color:facetDark;
            DrawTriangle3D(root,tip,edgeA,shardFace);
            DrawTriangle3D(root,edgeA,edgeB,blade);
            DrawTriangle3D(root,edgeB,tip,facetDark);
            DrawLine3D(root,tip,blade);
            DrawLine3D(tip,edgeA,shardFace);
            DrawLine3D(tip,edgeB,color);
        }
        DrawSphereWires(center,hazard.radius*0.44F,6,8,color);
        break;
    }
    }

    // An in-world segmented beacon follows the same course frame and moving
    // center as the mine. All of its geometry is decorative, outside the mine
    // model, and gets brighter/faster only as approach distance decreases.
    if (warning.visible) {
        Color warningColor = warning.urgent
            ? Color{255, 70, 64, 255}
            : Color{255, 184, 101, 255};
        warningColor.a = static_cast<unsigned char>(std::clamp(
            60.0F + warning.intensity * 175.0F, 0.0F, 255.0F));
        warningColor = tunnelDepthFog(warningColor, std::max(0.0F, ahead));
        const float ringRadius = hazard.radius + warning.ringOffset;
        constexpr int warningSegments = 32;
        for (int i = 0; i < warningSegments; ++i) {
            // Leave regular gaps so the beacon reads as a warning instrument,
            // not a solid obstacle that could be mistaken for its hitbox.
            if (i % 4 == 3) continue;
            const float a0 = static_cast<float>(i) * 2.0F * PI / warningSegments;
            const float a1 = static_cast<float>(i + 1) * 2.0F * PI / warningSegments;
            DrawLine3D(p(cx + std::cos(a0) * ringRadius,
                         cy + std::sin(a0) * ringRadius),
                       p(cx + std::cos(a1) * ringRadius,
                         cy + std::sin(a1) * ringRadius), warningColor);
            if (i % 4 == 0) {
                const float tickLength = warning.urgent ? 0.22F : 0.12F;
                DrawLine3D(p(cx + std::cos(a0) * ringRadius,
                             cy + std::sin(a0) * ringRadius),
                           p(cx + std::cos(a0) * (ringRadius + tickLength),
                             cy + std::sin(a0) * (ringRadius + tickLength)),
                           warningColor);
            }
        }
        if (warning.urgent) {
            const float innerRing = ringRadius + 0.16F;
            for (int i = 0; i < warningSegments; i += 2) {
                const float a0 = static_cast<float>(i) * 2.0F * PI / warningSegments;
                const float a1 = static_cast<float>(i + 1) * 2.0F * PI / warningSegments;
                DrawLine3D(p(cx + std::cos(a0) * innerRing,
                             cy + std::sin(a0) * innerRing),
                           p(cx + std::cos(a1) * innerRing,
                             cy + std::sin(a1) * innerRing), warningColor);
            }
        }
    }
}

void drawTunnel(std::uint64_t seed, float distance, float shipX, float shipY,
                std::uint32_t shipId, const char* modeName, bool tpp, bool reduceMotion,
                bool showAimReticle,
                float pitch, float yaw, float cameraLookYaw, float cameraLookPitch,
                float roll, float shipBank,
                float actualForwardSpeed, float boostEnergy,
                float dashCooldownRemaining, float dashRemaining, float elapsedSeconds,
                const tunrun::RunScore& score,
                std::uint64_t aetherPickedUp, std::uint64_t coresPickedUp,
                tunrun::CameraFollowState& tppCameraFollow,
                float& cameraModeBlend, float frameDeltaTime) {
    const auto playerSection = tunrun::sampleCourse(seed, distance);
    const auto playerFrame = tunrun::sampleTunnelFrame(seed, distance, distance);
    const auto throatGuidance = tunrun::gateThroatGuidanceForFlight(
        seed, static_cast<double>(distance), shipX, shipY, actualForwardSpeed);
    const float viewYaw = std::remainder(cameraLookYaw, 2.0F * PI);
    const float viewPitch = std::clamp(cameraLookPitch, -1.20F, 1.20F);
    const auto orbit = tunrun::cameraOrbitOffset(viewYaw, viewPitch);
    cameraModeBlend = tunrun::smoothCameraModeBlend(
        cameraModeBlend, tpp ? 1.0F : 0.0F, frameDeltaTime);
    const float modeBlend = cameraModeBlend;
    constexpr double cameraLookDistance = 8.0;

    // Keep the orbit path warm in both modes. Switching to TPP therefore
    // starts from the current look direction instead of resetting to a rear
    // camera pose for one frame.
    tunrun::smoothCameraOrbitDistance(
        tppCameraFollow, orbit.behindDistance, frameDeltaTime);
    const double fppEyeDistance = static_cast<double>(distance) +
        tunrun::kFirstPersonCockpitForwardOffset;
    const double tppEyeDistance = static_cast<double>(distance) -
        static_cast<double>(tppCameraFollow.behindDistance);
    const auto cameraFrame = tunrun::sampleTunnelFrame(seed, distance, fppEyeDistance);
    const auto tppRearFrame = tunrun::sampleTunnelFrame(seed, distance, tppEyeDistance);
    const auto tppRearSection = tunrun::sampleCourse(seed, tppEyeDistance);

    // Construct both views every frame, then blend their eye and focus poses.
    // This makes the V switch a short camera move, rather than a teleport.
    const double fppTargetDistance = static_cast<double>(distance) +
        tunrun::cameraLookCourseOffset(false, viewYaw, viewPitch, cameraLookDistance);
    const auto fppTargetFrame = tunrun::sampleTunnelFrame(
        seed, distance, fppTargetDistance);
    const auto fppLookOffset = tunrun::cameraLookOffset(
        viewYaw, viewPitch, fppTargetFrame.radius, static_cast<float>(cameraLookDistance));
    float fppTargetX = fppLookOffset.right;
    float fppTargetY = fppLookOffset.up;
    const auto fppTargetThroat = tunrun::gateThroatSectionAtDistance(seed, fppTargetDistance);
    if (fppTargetThroat.active) {
        const auto safeTarget = tunrun::cameraSafeOffset(
            fppTargetX - fppTargetThroat.centerX,
            fppTargetY - fppTargetThroat.centerY,
            fppTargetThroat.radius, 0.60F);
        fppTargetX = fppTargetThroat.centerX + safeTarget.right;
        fppTargetY = fppTargetThroat.centerY + safeTarget.up;
    } else {
        const auto safeTarget = tunrun::cameraSafeOffset(
            fppTargetX, fppTargetY, fppTargetFrame.radius, 0.60F);
        fppTargetX = safeTarget.right;
        fppTargetY = safeTarget.up;
    }

    float tppTargetX = shipX;
    float tppTargetY = shipY;
    const double tppTargetDistance = static_cast<double>(distance);
    const auto tppTargetThroat = tunrun::gateThroatSectionAtDistance(seed, tppTargetDistance);
    if (tppTargetThroat.active) {
        const auto safeTarget = tunrun::cameraSafeOffset(
            tppTargetX - tppTargetThroat.centerX,
            tppTargetY - tppTargetThroat.centerY,
            tppTargetThroat.radius, 0.60F);
        tppTargetX = tppTargetThroat.centerX + safeTarget.right;
        tppTargetY = tppTargetThroat.centerY + safeTarget.up;
    } else {
        const auto safeTarget = tunrun::cameraSafeOffset(
            tppTargetX, tppTargetY, playerFrame.radius, 0.60F);
        tppTargetX = safeTarget.right;
        tppTargetY = safeTarget.up;
    }

    Camera3D fppCamera{};
    float fppEyeX = shipX;
    float fppEyeY = shipY + tunrun::kFirstPersonCockpitUpOffset;
    const auto fppEyeThroat = tunrun::gateThroatSectionAtDistance(seed, fppEyeDistance);
    if (fppEyeThroat.active) {
        const auto safeEye = tunrun::cameraSafeOffset(
            fppEyeX - fppEyeThroat.centerX, fppEyeY - fppEyeThroat.centerY,
            fppEyeThroat.radius, 0.55F);
        fppEyeX = fppEyeThroat.centerX + safeEye.right;
        fppEyeY = fppEyeThroat.centerY + safeEye.up;
    } else {
        const auto safeEye = tunrun::cameraSafeOffset(
            fppEyeX, fppEyeY, cameraFrame.radius, 0.55F);
        fppEyeX = safeEye.right;
        fppEyeY = safeEye.up;
    }
    fppCamera.position = rayVector(tunrun::tunnelFramePoint(cameraFrame, fppEyeX, fppEyeY));
    fppCamera.target = rayVector(tunrun::tunnelFramePoint(
        fppTargetFrame, fppTargetX, fppTargetY));

    Camera3D tppCamera{};
    const float rearCenterX = tppRearSection.centerX - playerSection.centerX;
    const float rearCenterY = tppRearSection.centerY - playerSection.centerY;
    const auto chasePose = tunrun::thirdPersonCameraPose(
        shipX, shipY, 0.0F, 0.0F, -1.0F,
        rearCenterX, rearCenterY, tppRearSection.radius);
    float chaseX = chasePose.x - rearCenterX + orbit.right;
    float chaseY = chasePose.y - rearCenterY + orbit.up;
    const auto chaseThroat = tunrun::gateThroatSectionAtDistance(seed, tppEyeDistance);
    float chaseCenterX = 0.0F;
    float chaseCenterY = 0.0F;
    float chaseRadius = tppRearSection.radius;
    if (chaseThroat.active) {
        chaseCenterX = chaseThroat.centerX;
        chaseCenterY = chaseThroat.centerY;
        chaseRadius = chaseThroat.radius;
    }
    const auto desiredChase = tunrun::cameraSafeOffset(
        chaseX - chaseCenterX, chaseY - chaseCenterY, chaseRadius, 0.80F);
    tunrun::smoothCameraFollow(
        tppCameraFollow, desiredChase.right, desiredChase.up, frameDeltaTime);
    const auto smoothedChase = tunrun::cameraSafeOffset(
        tppCameraFollow.right, tppCameraFollow.up, chaseRadius, 0.80F);
    chaseX = chaseCenterX + smoothedChase.right;
    chaseY = chaseCenterY + smoothedChase.up;
    tppCamera.position = rayVector(tunrun::tunnelFramePoint(tppRearFrame, chaseX, chaseY));
    tppCamera.target = rayVector(tunrun::tunnelFramePoint(
        playerFrame, tppTargetX, tppTargetY));

    // Clip each candidate camera before interpolation. This first pass keeps
    // either endpoint safe; a second pass below also verifies the mixed ray.
    auto clipCameraRay = [&](Camera3D& candidate, double& eyeDistance,
                             double& targetDistance, bool thirdPerson,
                             float eyeClearance, float focusClearance) {
        const tunrun::FrameVector3 eye{
            candidate.position.x, candidate.position.y, candidate.position.z};
        const tunrun::FrameVector3 focus{
            candidate.target.x, candidate.target.y, candidate.target.z};
        const auto limit = tunrun::limitCameraRayInsideTunnel(
            seed, distance, eyeDistance, eye, targetDistance, focus,
            eyeClearance, 64U, focusClearance);
        if (!limit.clipped) return;
        if (thirdPerson) {
            const auto reverse = tunrun::limitCameraRayInsideTunnel(
                seed, distance, targetDistance, focus, eyeDistance, eye,
                focusClearance, 64U, eyeClearance);
            if (reverse.clipped) {
                const float fraction = std::clamp(reverse.safeFraction, 0.05F, 0.95F);
                candidate.position = Vector3{
                    focus.x + (eye.x - focus.x) * fraction,
                    focus.y + (eye.y - focus.y) * fraction,
                    focus.z + (eye.z - focus.z) * fraction
                };
                eyeDistance = targetDistance +
                    (eyeDistance - targetDistance) * static_cast<double>(fraction);
                return;
            }
        }
        const float fraction = limit.safeFraction;
        candidate.target = Vector3{
            eye.x + (focus.x - eye.x) * fraction,
            eye.y + (focus.y - eye.y) * fraction,
            eye.z + (focus.z - eye.z) * fraction
        };
        targetDistance = eyeDistance +
            (targetDistance - eyeDistance) * static_cast<double>(fraction);
    };

    double fppEyePositionDistance = fppEyeDistance;
    double tppEyePositionDistance = tppEyeDistance;
    double fppFocusDistance = fppTargetDistance;
    double tppFocusDistance = tppTargetDistance;
    Camera3D camera{};
    double cameraOriginDistance = fppEyePositionDistance;
    double cameraTargetDistance = fppFocusDistance;
    if (modeBlend <= 0.001F) {
        // The steady FPP case needs only one 64-sample ray test. Avoid doing
        // three full tunnel/throat scans on every high-refresh render frame.
        clipCameraRay(fppCamera, fppEyePositionDistance, fppFocusDistance,
                      false, 0.55F, 0.55F);
        camera = fppCamera;
        cameraOriginDistance = fppEyePositionDistance;
        cameraTargetDistance = fppFocusDistance;
    } else if (modeBlend >= 0.999F) {
        // Likewise, steady TPP needs just its own candidate-ray test.
        clipCameraRay(tppCamera, tppEyePositionDistance, tppFocusDistance,
                      true, 0.80F, 0.35F);
        camera = tppCamera;
        cameraOriginDistance = tppEyePositionDistance;
        cameraTargetDistance = tppFocusDistance;
    } else {
        clipCameraRay(fppCamera, fppEyePositionDistance, fppFocusDistance,
                      false, 0.55F, 0.55F);
        clipCameraRay(tppCamera, tppEyePositionDistance, tppFocusDistance,
                      true, 0.80F, 0.35F);
        const auto mix = [modeBlend](float a, float b) {
            return a + (b - a) * modeBlend;
        };
        camera.position = Vector3{
            mix(fppCamera.position.x, tppCamera.position.x),
            mix(fppCamera.position.y, tppCamera.position.y),
            mix(fppCamera.position.z, tppCamera.position.z)
        };
        camera.target = Vector3{
            mix(fppCamera.target.x, tppCamera.target.x),
            mix(fppCamera.target.y, tppCamera.target.y),
            mix(fppCamera.target.z, tppCamera.target.z)
        };
        cameraOriginDistance = fppEyePositionDistance +
            (tppEyePositionDistance - fppEyePositionDistance) * static_cast<double>(modeBlend);
        cameraTargetDistance = fppFocusDistance +
            (tppFocusDistance - fppFocusDistance) * static_cast<double>(modeBlend);
        const float blendEyeClearance = 0.55F + (0.80F - 0.55F) * modeBlend;
        const float blendFocusClearance = 0.55F + (0.35F - 0.55F) * modeBlend;
        const auto blendedEyeFrame = tunrun::sampleTunnelFrame(
            seed, distance, cameraOriginDistance);
        const auto blendedEyeThroat = tunrun::gateThroatSectionAtDistance(
            seed, cameraOriginDistance);
        const float eyeCenterX = blendedEyeThroat.active
            ? blendedEyeThroat.centerX : 0.0F;
        const float eyeCenterY = blendedEyeThroat.active
            ? blendedEyeThroat.centerY : 0.0F;
        const float eyeRadius = blendedEyeThroat.active
            ? blendedEyeThroat.radius : blendedEyeFrame.radius;
        camera.position = rayVector(tunrun::clampCameraEyeToCrossSection(
            blendedEyeFrame,
            tunrun::FrameVector3{camera.position.x, camera.position.y, camera.position.z},
            eyeRadius, eyeCenterX, eyeCenterY, blendEyeClearance));
        // A blend between two safe rays can still cut a corner at an S-bend.
        // Validate the final blended ray instead of trusting the endpoints.
        clipCameraRay(camera, cameraOriginDistance, cameraTargetDistance,
                      modeBlend >= 0.5F, blendEyeClearance, blendFocusClearance);
    }
    const auto cameraBasisFrame = tunrun::sampleTunnelFrame(
        seed, distance, cameraOriginDistance);
    const float cameraRoll = tunrun::cameraRollForMode(roll, modeBlend);
    camera.up = rayVector(tunrun::frameAdd(
        tunrun::frameScale(cameraBasisFrame.up, std::cos(cameraRoll)),
        tunrun::frameScale(cameraBasisFrame.right, -std::sin(cameraRoll))));
    camera.fovy = 70.0F;
    camera.projection = CAMERA_PERSPECTIVE;
    ClearBackground(kBackground);
    BeginMode3D(camera);
    constexpr int firstRing = -5;
    constexpr int lastRing = 37;
    constexpr int sideCount = 20;
    constexpr float ringSpacing = 3.0F;
    std::array<tunrun::TunnelFrame, lastRing - firstRing> tunnelFrames{};
    for (int ring = firstRing; ring < lastRing; ++ring) {
        tunnelFrames[static_cast<std::size_t>(ring - firstRing)] =
            tunrun::sampleTunnelFrame(seed, distance,
                static_cast<double>(distance) + static_cast<double>(ring) * ringSpacing);
    }
    for (int ring = firstRing; ring < lastRing; ++ring) {
        const double ringWorldDistance = static_cast<double>(distance) +
            static_cast<double>(ring) * static_cast<double>(ringSpacing);
        const auto visual = tunrun::tunnelVisualProfileAt(
            seed, tunrun::tunnelVisualSectionIndex(ringWorldDistance));
        const float ringAhead = std::max(0.0F, static_cast<float>(ring) * ringSpacing);
        const bool strongRib = (ring % (visual.family == tunrun::TunnelVisualFamily::RibbedMetal
            ? 2 : visual.family == tunrun::TunnelVisualFamily::Lattice ? 3 : 4)) == 0;
        const float intensity = visual.intensity;
        const auto& palette = visual.palette;
        const Color ringAccent{
            static_cast<unsigned char>(palette.red * (strongRib ? 0.93F : 0.52F) * intensity),
            static_cast<unsigned char>(palette.green * (strongRib ? 0.93F : 0.52F) * intensity),
            static_cast<unsigned char>(palette.blue * (strongRib ? 0.93F : 0.52F) * intensity),
            static_cast<unsigned char>(strongRib ? 188 : 112)};
        const Color ringColor = tunnelDepthFog(ringAccent, ringAhead);
        const auto& ringFrame = tunnelFrames[static_cast<std::size_t>(ring - firstRing)];
        const std::int64_t sectionIndex = tunrun::tunnelVisualSectionIndex(ringWorldDistance);
        for (int side = 0; side < sideCount; ++side) {
            const int nextSide = (side + 1) % sideCount;
            const Vector3 a = tunnelPoint(ringFrame, side);
            const Vector3 b = tunnelPoint(ringFrame, nextSide);
            if (ring + 1 < lastRing) {
                const auto& nextFrame = tunnelFrames[static_cast<std::size_t>(ring + 1 - firstRing)];
                const Vector3 d = tunnelPoint(nextFrame, side);
                const Vector3 c = tunnelPoint(nextFrame, nextSide);
                bool panelRidge = false;
                switch (visual.family) {
                case tunrun::TunnelVisualFamily::RibbedMetal:
                    panelRidge = (side % 4 == 0) || (ring % 2 == 0);
                    break;
                case tunrun::TunnelVisualFamily::PlasmaRails:
                    panelRidge = (side % 5 == 0) || (ring % 8 == 0);
                    break;
                case tunrun::TunnelVisualFamily::FracturedPanels:
                    panelRidge = ((side * 3 + ring + static_cast<int>(sectionIndex % 7)) % 7) < 2;
                    break;
                case tunrun::TunnelVisualFamily::SpiralConduits:
                    panelRidge = (side % 6 == 0) || (side % 6 == 1);
                    break;
                case tunrun::TunnelVisualFamily::Lattice:
                    panelRidge = ((side % 2 == 0) && (ring % 2 == 0)) ||
                                 (ring % 6 == 0);
                    break;
                }
                const Color panel = tunnelDepthFog(panelRidge
                    ? Color{static_cast<unsigned char>(12 + palette.red / 5),
                            static_cast<unsigned char>(14 + palette.green / 5),
                            static_cast<unsigned char>(18 + palette.blue / 5), 255}
                    : Color{static_cast<unsigned char>(8 + palette.red / 18),
                            static_cast<unsigned char>(10 + palette.green / 18),
                            static_cast<unsigned char>(15 + palette.blue / 18), 255}, ringAhead);
                const Color seam = tunnelDepthFog(Color{
                    static_cast<unsigned char>(palette.red * 0.30F * intensity),
                    static_cast<unsigned char>(palette.green * 0.30F * intensity),
                    static_cast<unsigned char>(palette.blue * 0.30F * intensity),
                    static_cast<unsigned char>(panelRidge ? 118 : 64)}, ringAhead);
                // Cosmetic motifs never alter the continuous wall or collision.
                DrawTriangle3D(a, b, c, panel);
                DrawTriangle3D(a, c, d, panel);
                if (visual.family == tunrun::TunnelVisualFamily::FracturedPanels &&
                    ((side + ring) % 2 == 0)) {
                    DrawLine3D(a, c, seam);
                } else {
                    DrawLine3D(a, d, seam);
                }
            }
            DrawLine3D(a, b, ringColor);
        }
    }
    // Decorative rail masks vary by zone. Every rail occupies one of six fixed
    // angular slots, so the pattern stays spatially anchored on style changes.
    constexpr int ribbonCount = 6;
    for (int ribbon = 0; ribbon < ribbonCount; ++ribbon) {
        const float phase = static_cast<float>(ribbon) * 2.0F * PI /
            static_cast<float>(ribbonCount) + elapsedSeconds * 0.38F;
        for (int ring = firstRing; ring + 1 < lastRing; ++ring) {
            const auto& frameA = tunnelFrames[static_cast<std::size_t>(ring - firstRing)];
            const auto& frameB = tunnelFrames[static_cast<std::size_t>(ring + 1 - firstRing)];
            const double midpointDistance = static_cast<double>(distance) +
                (static_cast<double>(ring) + 0.5) * static_cast<double>(ringSpacing);
            const auto visual = tunrun::tunnelVisualProfileAt(
                seed, tunrun::tunnelVisualSectionIndex(midpointDistance));
            if (!tunrun::tunnelVisualRailEnabled(visual, ribbon)) continue;
            const float angleA = phase + static_cast<float>(ring - firstRing) * 0.19F;
            const float angleB = phase + static_cast<float>(ring + 1 - firstRing) * 0.19F;
            const float radiusA = std::max(0.25F, frameA.radius - 0.16F);
            const float radiusB = std::max(0.25F, frameB.radius - 0.16F);
            const Vector3 a = rayVector(tunrun::tunnelFramePoint(
                frameA, std::cos(angleA) * radiusA, std::sin(angleA) * radiusA));
            const Vector3 b = rayVector(tunrun::tunnelFramePoint(
                frameB, std::cos(angleB) * radiusB, std::sin(angleB) * radiusB));
            const auto& palette = visual.palette;
            const Color rail{
                static_cast<unsigned char>(palette.red * visual.intensity),
                static_cast<unsigned char>(palette.green * visual.intensity),
                static_cast<unsigned char>(palette.blue * visual.intensity), 190};
            DrawLine3D(a, b, tunnelDepthFog(
                rail, std::max(0.0F, static_cast<float>(ring + 1) * ringSpacing)));
        }
    }
    if (tunrun::shouldDrawDashStreaks(reduceMotion, dashRemaining)) {
        const float intensity = std::clamp(
            dashRemaining / tunrun::kDashDuration, 0.0F, 1.0F);
        const auto alpha = static_cast<unsigned char>(
            std::clamp(intensity * 220.0F, 0.0F, 220.0F));
        const Color streakColor{122, 225, 255, alpha};
        constexpr int streakCount = 12;
        for (int i = 0; i < streakCount; ++i) {
            const float angle = static_cast<float>(i) * 2.0F * PI / streakCount;
            const float radialX = std::cos(angle) * 3.7F;
            const float radialY = std::sin(angle) * 3.7F;
            const float travel = std::fmod(elapsedSeconds * 12.0F +
                                           static_cast<float>(i) * 2.7F, 19.0F);
            // Dash streaks are sampled in the same curved local frame as the
            // tunnel skin; fixed world-space XYZ lines otherwise float away
            // from the wall when the wormhole bends or twists.
            const auto streakFrameA = tunrun::sampleTunnelFrame(
                seed, distance, static_cast<double>(distance) + travel + 2.0);
            const auto streakFrameB = tunrun::sampleTunnelFrame(
                seed, distance, static_cast<double>(distance) + travel + 4.2);
            const Vector3 a = rayVector(tunrun::tunnelFramePoint(
                streakFrameA, radialX, radialY));
            const Vector3 b = rayVector(tunrun::tunnelFramePoint(
                streakFrameB, radialX * 0.92F, radialY * 0.92F));
            DrawLine3D(a, b, tunnelDepthFog(streakColor, travel + 2.0F));
        }
    }
    const auto firstGate = tunrun::gateAt(seed, 0U);
    const int firstVisibleIndex = std::max(0, static_cast<int>(
        std::floor((static_cast<double>(distance) - firstGate.distance) / tunrun::kGateSpacing)));
    const int nextGateIndex = std::max(0, static_cast<int>(
        std::ceil((static_cast<double>(distance) - firstGate.distance) /
                  tunrun::kGateSpacing)));
    const auto nextGate = tunrun::gateAt(seed, static_cast<std::uint32_t>(nextGateIndex));
    for (int i = firstVisibleIndex; i < firstVisibleIndex + 4; ++i) {
        const auto gate = tunrun::gateAt(seed, static_cast<std::uint32_t>(i));
        drawGateThroat(seed, distance, gate);
        drawProceduralGate(seed, distance, gate);
    }
    const int firstRewardIndex = std::max(0, static_cast<int>(std::floor(
        (static_cast<double>(distance) - tunrun::kRewardStartDistance) /
        tunrun::kRewardSpacing)));
    for (int i = firstRewardIndex; i < firstRewardIndex + 6; ++i) {
        drawProceduralReward(seed, distance,
            tunrun::rewardAt(seed, static_cast<std::uint32_t>(i)));
    }
    const auto firstHazard = tunrun::hazardAt(seed, 0U);
    const int firstHazardIndex = std::max(0, static_cast<int>(std::floor(
        (static_cast<double>(distance) - firstHazard.distance) /
        tunrun::kHazardSpacing)));
    for (int i = firstHazardIndex; i < firstHazardIndex + 4; ++i) {
        drawProceduralHazard(seed, distance, elapsedSeconds,
            tunrun::hazardAt(seed, static_cast<std::uint32_t>(i)));
    }
    const auto shipOrigin = tunrun::tunnelFramePoint(playerFrame, shipX, shipY);
    const float cameraToShipDistance = std::sqrt(
        (camera.position.x - shipOrigin.x) * (camera.position.x - shipOrigin.x) +
        (camera.position.y - shipOrigin.y) * (camera.position.y - shipOrigin.y) +
        (camera.position.z - shipOrigin.z) * (camera.position.z - shipOrigin.z));
    const float shipVisibility = tunrun::cameraShipVisibility(modeBlend) *
        tunrun::cameraShipVisibilityFromEyeDistance(cameraToShipDistance);
    if (shipVisibility > 0.001F) {
        drawPlayerShip(shipId, shipX, shipY, pitch, yaw, roll + shipBank,
                       &playerFrame, shipVisibility);
    }
    if (showAimReticle && modeBlend < 0.05F) {
        const tunrun::FrameVector3 cameraEye{
            camera.position.x, camera.position.y, camera.position.z};
        const tunrun::FrameVector3 cameraTarget{
            camera.target.x, camera.target.y, camera.target.z};
        const tunrun::FrameVector3 cameraUp{
            camera.up.x, camera.up.y, camera.up.z};
        const auto reticle = tunrun::cameraReticlePose(
            cameraEye, cameraTarget, cameraUp);
        if (reticle.valid) {
            const Color aimColor{111, 225, 255, 235};
            // Keep the reticle physically small when a blocked view ray is
            // clipped close to the camera, instead of letting it fill the view.
            const float size = std::clamp(reticle.depth * 0.024F, 0.045F, 0.19F);
            const float inner = size * 0.30F;
            const auto aimPoint = [&](float right, float up) {
                return rayVector(tunrun::frameAdd(
                    reticle.center,
                    tunrun::frameAdd(tunrun::frameScale(reticle.right, right),
                                     tunrun::frameScale(reticle.up, up))));
            };
            const Vector3 aimCenter = rayVector(reticle.center);
            DrawSphere(aimCenter, size * 0.13F, aimColor);
            DrawLine3D(aimPoint(-size, 0.0F), aimPoint(-inner, 0.0F), aimColor);
            DrawLine3D(aimPoint(inner, 0.0F), aimPoint(size, 0.0F), aimColor);
            DrawLine3D(aimPoint(0.0F, -size), aimPoint(0.0F, -inner), aimColor);
            DrawLine3D(aimPoint(0.0F, inner), aimPoint(0.0F, size), aimColor);
            DrawSphereWires(aimCenter, size * 0.64F, 6, 12, aimColor);
        }
    }

    // Mark the centre of the upcoming S-bend inside the actual throat mesh.
    // This gives the player a spatial target in both FPP and TPP instead of
    // making them rely on a text instruction alone. The marker is only visual:
    // collision and navigation continue using the shared throat sampler.
    if (throatGuidance.valid && throatGuidance.distanceAhead > 1.0F) {
        const double guideDistance = static_cast<double>(distance) +
            static_cast<double>(throatGuidance.distanceAhead);
        const auto guideGate = tunrun::gateAt(seed, throatGuidance.gateIndex);
        const auto guideThroat = tunrun::gateThroatSectionAtDistance(
            seed, guideGate, guideDistance);
        if (guideThroat.active) {
            const auto guideFrame = tunrun::sampleTunnelFrame(
                seed, distance, guideDistance);
            const float markerRadius = std::clamp(
                std::min(guideThroat.radius * 0.32F, 0.42F), 0.14F, 0.42F);
            const Color markerColor{111, 225, 255, 225};
            const auto markerPoint = [&](float x, float y) {
                return rayVector(tunrun::tunnelFramePoint(
                    guideFrame, guideThroat.centerX + x,
                    guideThroat.centerY + y));
            };
            const Vector3 markerCenter = markerPoint(0.0F, 0.0F);
            DrawSphere(markerCenter, 0.055F, markerColor);
            constexpr int markerSegments = 12;
            for (int segment = 0; segment < markerSegments; ++segment) {
                const float a0 = static_cast<float>(segment) * 2.0F * PI /
                    static_cast<float>(markerSegments);
                const float a1 = static_cast<float>(segment + 1) * 2.0F * PI /
                    static_cast<float>(markerSegments);
                DrawLine3D(
                    markerPoint(std::cos(a0) * markerRadius,
                                std::sin(a0) * markerRadius),
                    markerPoint(std::cos(a1) * markerRadius,
                                std::sin(a1) * markerRadius),
                    markerColor);
            }
            const float tick = markerRadius * 1.55F;
            DrawLine3D(markerPoint(-tick, 0.0F),
                       markerPoint(-markerRadius * 0.66F, 0.0F), markerColor);
            DrawLine3D(markerPoint(markerRadius * 0.66F, 0.0F),
                       markerPoint(tick, 0.0F), markerColor);
            DrawLine3D(markerPoint(0.0F, -tick),
                       markerPoint(0.0F, -markerRadius * 0.66F), markerColor);
            DrawLine3D(markerPoint(0.0F, markerRadius * 0.66F),
                       markerPoint(0.0F, tick), markerColor);
        }
    }
    EndMode3D();
    DrawRectangle(22, 18, 344, 260, Color{10, 14, 21, 225});
    DrawRectangleLines(22, 18, 344, 260, kEdge);
    DrawText(TextFormat("TUNRUN / %s", tunrun::shipDefinition(shipId).name),
             35, 30, 15, kAccent);
    DrawText(TextFormat("%s / %s / %4.1f U/S",
             modeName ? modeName : "FLIGHT",
             cameraModeBlend > 0.5F ? "TPP" : "FPP", actualForwardSpeed),
             35, 52, 12, kText);
    DrawText(TextFormat("BOOST: %3.0f%%", boostEnergy), 35, 74, 13, kText);
    DrawRectangle(175, 78, 155, 8, Color{42, 51, 64, 255});
    DrawRectangle(175, 78, static_cast<int>(155.0F * boostEnergy / 100.0F), 8, kAccent);
    DrawText(TextFormat("GATE: %s / %s", tunrun::gateKindName(nextGate.kind),
             tunrun::gateStructureFamilyName(
                 tunrun::gateStructureFamilyAt(seed, nextGate.index))),
             35, 98, 12, kAccent);
    DrawText(TextFormat("SEED %016llX", static_cast<unsigned long long>(seed)), 35, 117, 11, kMuted);
    if (throatGuidance.valid) {
        const char* lateralCue = std::abs(throatGuidance.lateralError) < 0.22F
            ? "CENTRE" : throatGuidance.lateralError < 0.0F ? "LEFT" : "RIGHT";
        const char* verticalCue = std::abs(throatGuidance.verticalError) < 0.22F
            ? "LEVEL" : throatGuidance.verticalError < 0.0F ? "PITCH DOWN" : "PITCH UP";
        DrawText(TextFormat("THROAT AIM: %s / %s  +%.1fU",
                 lateralCue, verticalCue, throatGuidance.distanceAhead),
                 35, 137, 10, kAccent);
    } else {
        DrawText("THROAT AIM: UNAVAILABLE", 35, 137, 10, kMuted);
    }
    const auto hazardCue = tunrun::hazardHudCueAt(
        seed, static_cast<double>(distance), elapsedSeconds, shipX, shipY);
    if (hazardCue.valid) {
        const auto upcomingHazard = tunrun::hazardAt(seed, hazardCue.hazardIndex);
        const auto hazardFamily = tunrun::hazardVisualFamilyAt(
            seed, hazardCue.hazardIndex);
        const float timeToHazard = hazardCue.distanceAhead /
            std::max(0.1F, actualForwardSpeed);
        const auto warning = tunrun::hazardWarningProfileAt(
            hazardCue.distanceAhead, elapsedSeconds);
        const char* warningLabel = warning.urgent ? "IMMINENT"
            : warning.visible ? "WARNING" : "TRACKING";
        const Color warningText = warning.urgent ? Color{255, 91, 84, 255}
            : warning.visible ? Color{255, 184, 101, 255}
                              : Color{255, 153, 125, 255};
        DrawText(TextFormat("%s %s / %s: %.1fU / %.1fS", warningLabel,
                 tunrun::hazardVisualFamilyName(hazardFamily),
                 tunrun::hazardMotionFamilyName(upcomingHazard.motionFamily),
                 hazardCue.distanceAhead, timeToHazard),
                 35, 157, 10, warningText);
        const char* horizontalDirection = std::abs(hazardCue.offsetX) < 0.18F
            ? "CENTER" : hazardCue.offsetX < 0.0F ? "LEFT" : "RIGHT";
        const char* verticalDirection = std::abs(hazardCue.offsetY) < 0.18F
            ? "LEVEL" : hazardCue.offsetY > 0.0F ? "UP" : "DOWN";
        DrawText(TextFormat("MINE BEARING: %s %.1f / %s %.1f",
                 horizontalDirection, std::abs(hazardCue.offsetX),
                 verticalDirection, std::abs(hazardCue.offsetY)),
                 35, 177, 10, Color{255, 153, 125, 255});
    } else {
        DrawText("HAZARD CUE UNAVAILABLE", 35, 157, 10, kMuted);
    }
    DrawText(TextFormat("PICKUPS: +%llu AETHER / +%llu CORE",
             static_cast<unsigned long long>(aetherPickedUp),
             static_cast<unsigned long long>(coresPickedUp)), 35, 197, 10, kMuted);
    DrawText(TextFormat("SCORE %llu   COMBO x%.1f   CLEAN %llu",
             static_cast<unsigned long long>(score.total),
             1.0 + static_cast<double>(std::min<std::uint64_t>(score.combo, 40U)) / 10.0,
             static_cast<unsigned long long>(score.cleanPasses)), 35, 217, 10, kAccent);
    if (dashRemaining > 0.0F) {
        DrawText("DASH ACTIVE", 35, 237, 10, kAccent);
    } else if (dashCooldownRemaining > 0.0F) {
        DrawText(TextFormat("DASH RECHARGE: %.1fs", dashCooldownRemaining),
                 35, 237, 10, kMuted);
    } else if (boostEnergy >= tunrun::kDashEnergyCost) {
        DrawText("DASH READY: SPACE / PAD A", 35, 237, 10, kAccent);
    } else {
        DrawText(TextFormat("DASH NEEDS %.0f ENERGY", tunrun::kDashEnergyCost),
                 35, 237, 10, kMuted);
    }
    DrawRectangle(22, GetScreenHeight() - 48, GetScreenWidth() - 44, 26,
                  Color{10, 14, 21, 220});
    DrawText("W GO   S BRAKE   A/B STEER   MOUSE LOOK   ARROWS ROTATE   Q/E ROLL   SPACE DASH   SHIFT BOOST   V CAMERA",
             36, GetScreenHeight() - 42, 10, kMuted);
}
} // namespace

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 800, "TUNRUN | Procedural Tunnel Runner");
    RawMouse rawMouse;
    (void)rawMouse.install(GetWindowHandle());
    SetWindowMinSize(800, 560);
    SetExitKey(KEY_NULL);
    const int monitorRefreshRate = GetMonitorRefreshRate(GetCurrentMonitor());
    SetTargetFPS(tunrun::targetFpsForRefreshRate(monitorRefreshRate));
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

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
    app.courseSeed = app.runSerial == 0U ? app.rootSeed : tunrun::deriveCourseSeed(app.rootSeed, app.runSerial);
    app.showFps = app.profile.showFps;
    app.reduceMotion = app.profile.reduceMotion;
    app.fullscreen = app.profile.fullscreen;
    app.selectedShip = static_cast<int>(std::min<std::uint32_t>(
        app.profile.selectedShip, static_cast<std::uint32_t>(tunrun::kProfileShipCount - 1U)));
    if (!app.profile.unlockedShips[static_cast<std::size_t>(app.selectedShip)]) {
        app.selectedShip = static_cast<int>(tunrun::kStarterShipId);
        app.profile.selectedShip = tunrun::kStarterShipId;
        initializeProfile = true;
    }
    app.hangarPreviewShip = app.selectedShip;
    // Always launch fullscreen. Windowed mode can be selected in Settings for
    // the current session; the next launch returns to fullscreen.
    ToggleFullscreen();
    app.fullscreen = true;
    app.profile.fullscreen = true;
    initializeProfile = true;

    if (app.profileRecoveryRequired) {
        app.screens.replace(tunrun::Screen::SaveRecovery);
    } else if (loadedProfile.status == tunrun::ProfileLoadStatus::RecoveredBackup) {
        (void)persistProfile(app, true);
    } else if (initializeProfile && app.profileWritable) {
        (void)persistProfile(app);
    }

    int mainSelection = 0, hangarSelection = 0, recordsSelection = 0, controlsSelection = 0, runConfirmSelection = 0, modesSelection = 0;
    int settingsSelection = 0, settingsResetSelection = 0, pauseSelection = 0, exitSelection = 0, crashSelection = 0,
        seedLabSelection = 0, seedEntrySelection = 0, seedPickerIndex = 0, recoverySelection = 0;
    bool tpp = false;
    const std::vector<std::string> mainItems{
        "ENDLESS SURVIVAL", "HANGAR", "GAME MODES", "SEED LAB",
        "RECORDS / STATISTICS", "CONTROLS", "SETTINGS", "CREDITS", "EXIT"
    };
    const std::vector<std::string> modes{
        "CAMPAIGN (IN DEVELOPMENT)", "ENDLESS SURVIVAL",
        "CUSTOM SEED RUN", "PRACTICE PREVIEW", "BACK"
    };
    const std::vector<std::string> pauseItems{
        "RESUME", "RESTART SAME SEED", "CONTROLS", "SETTINGS", "RETURN TO MAIN MENU"
    };
    const std::vector<std::string> confirmRestartItems{
        "CANCEL", "DISCARD + RESTART SAME SEED"
    };
    const std::vector<std::string> confirmReturnItems{
        "CANCEL", "DISCARD + RETURN TO MENU"
    };
    const std::vector<std::string> settingsItems{
        "FULLSCREEN: ON", "FPS COUNTER: ON", "REDUCED MOTION: OFF",
        "MOUSE FLIGHT: ON", "MOUSE SENSITIVITY: 0.0040", "RESET OPTIONS", "BACK"
    };
    const std::vector<std::string> resetSettingsItems{"CANCEL", "RESET OPTIONS"};
    const std::vector<std::string> hangarPurchaseItems{"CANCEL", "CONFIRM UNLOCK"};
    const std::vector<std::string> exitItems{"CANCEL", "EXIT"};
    const std::vector<std::string> crashItems{
        "RETRY SAME SEED", "COPY SEED", "NEW SEED", "RETURN TO MAIN MENU"
    };
    const std::vector<std::string> seedLabItems{
        "ENTER CUSTOM SEED", "GENERATE NEW SEED", "START THIS SEED", "COPY SEED", "BACK"
    };
    const std::vector<std::string> seedEntryItems{"APPLY + START RUN", "CANCEL"};
    const std::vector<std::string> recoveryItems{"RESET PROFILE (PRESERVE DAMAGED FILES)", "EXIT WITHOUT RESET"};
    int hangarFocus = 0; // 0 = unlock/equip, 1 = Back
    int hangarPurchaseSelection = 0; // Default focus is Cancel to avoid accidental purchases

    while (!WindowShouldClose() && !app.exitRequested) {
        const bool mouseCaptureWanted =
            app.screens.current() == tunrun::Screen::Preview &&
            app.profile.mouseSteering && IsWindowFocused();
        if (rawMouse.installed()) {
            if (rawMouse.active() != mouseCaptureWanted) {
                rawMouse.setActive(mouseCaptureWanted);
            } else if (mouseCaptureWanted) {
                rawMouse.refreshClip();
            }
        }
        const float dt = std::min(GetFrameTime(), 0.05F);
        if (tunrun::shouldAdvanceRunClock(app.screens)) app.elapsed += dt;

        if (app.screens.current() == tunrun::Screen::Preview) {
            if (!IsWindowFocused()) {
                app.screens.push(tunrun::Screen::Pause);
            } else {
                const auto keyboard = tunrun::keyboardFlightIntentFromKeys(
                    IsKeyDown(KEY_W), IsKeyDown(KEY_S),
                    IsKeyDown(KEY_A), IsKeyDown(KEY_B), IsKeyDown(KEY_D),
                    IsKeyDown(KEY_LEFT), IsKeyDown(KEY_RIGHT),
                    IsKeyDown(KEY_UP), IsKeyDown(KEY_DOWN),
                    IsKeyDown(KEY_Q), IsKeyDown(KEY_E));
                float steerX = keyboard.lateral;
                float steerY = 0.0F;
                const float speedControl = keyboard.speed;
                float rotateYaw = keyboard.yaw;
                float rotatePitch = keyboard.pitch;
                float rollInput = keyboard.roll;
                float padX = 0.0F;
                float padY = 0.0F;
                if (IsGamepadAvailable(0)) {
                    padX = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
                    padY = -GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
                    if (std::abs(padX) < 0.18F) padX = 0.0F;
                    else padX = std::copysign((std::abs(padX) - 0.18F) / 0.82F, padX);
                    if (std::abs(padY) < 0.18F) padY = 0.0F;
                    else padY = std::copysign((std::abs(padY) - 0.18F) / 0.82F, padY);
                    steerX += padX;
                    steerY += padY;
                    float padRotateYaw = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
                    float padRotatePitch = -GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
                    if (std::abs(padRotateYaw) < 0.16F) padRotateYaw = 0.0F;
                    if (std::abs(padRotatePitch) < 0.16F) padRotatePitch = 0.0F;
                    // A connected controller must not disable arrow-key rotation.
                    rotateYaw += padRotateYaw;
                    rotatePitch += padRotatePitch;
                }

                // Mouse input now rotates the camera only. The flight path is
                // controlled by A/D, arrows, and the gamepad, never by the view.
                if (app.profile.mouseSteering) {
                    RelativeMouseDelta mouseDelta{};
                    if (rawMouse.installed()) {
                        mouseDelta = rawMouse.consume();
                    } else {
                        const Vector2 pointerDelta = GetMouseDelta();
                        mouseDelta = RelativeMouseDelta{pointerDelta.x, pointerDelta.y};
                    }
                    app.cameraLookYaw = std::remainder(
                        app.cameraLookYaw + mouseDelta.x * app.profile.mouseSensitivity,
                        2.0F * PI);
                    app.cameraLookPitch = std::clamp(
                        app.cameraLookPitch - mouseDelta.y * app.profile.mouseSensitivity,
                        -0.72F, 0.72F);
                } else if (rawMouse.installed()) {
                    (void)rawMouse.consume();
                }
                const float previousElapsed = std::max(0.0F, app.elapsed - dt);
                const float previousX = app.flight.x;
                const float previousY = app.flight.y;
                const double previousDistance = app.flight.distance;
                const bool padBoost = IsGamepadAvailable(0) &&
                    tunrun::triggerPressed(
                        GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_TRIGGER));
                const bool padPrecision = IsGamepadAvailable(0) &&
                    tunrun::triggerPressed(
                        GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_TRIGGER));
                const bool padDash = IsGamepadAvailable(0) &&
                    IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
                const tunrun::FlightInput flightInput{
                    std::clamp(steerX, -1.0F, 1.0F),
                    std::clamp(steerY, -1.0F, 1.0F),
                    IsKeyDown(KEY_LEFT_SHIFT) || padBoost,
                    IsKeyDown(KEY_LEFT_CONTROL) || padPrecision,
                    static_cast<std::uint32_t>(app.selectedShip),
                    IsKeyDown(KEY_SPACE) || padDash,
                    rotateYaw, rotatePitch, rollInput, speedControl
                };
                tunrun::advanceFlight(app.flight, flightInput, dt, app.flightAccumulator);
                const auto tunnelSection = tunrun::sampleCourse(
                    app.courseSeed, static_cast<double>(app.flight.distance));
                const tunrun::TunnelCrossSection centredSection{
                    0.0F, 0.0F, tunnelSection.radius, tunnelSection.twist
                };
                const auto firstGateForThroat = tunrun::gateAt(app.courseSeed, 0U);
                const double segmentMinimumDistance = std::min(
                    previousDistance, static_cast<double>(app.flight.distance));
                const double segmentMaximumDistance = std::max(
                    previousDistance, static_cast<double>(app.flight.distance));
                const int firstThroatGate = std::max(0, static_cast<int>(std::floor(
                    (segmentMinimumDistance - tunrun::kGateThroatHalfLength -
                     firstGateForThroat.distance) / tunrun::kGateSpacing)) - 1);
                const int lastThroatGate = std::max(firstThroatGate, static_cast<int>(std::ceil(
                    (segmentMaximumDistance + tunrun::kGateThroatHalfLength -
                     firstGateForThroat.distance) / tunrun::kGateSpacing)) + 1);
                std::uint32_t throatCollisionGate = std::numeric_limits<std::uint32_t>::max();
                for (int gateIndex = firstThroatGate; gateIndex <= lastThroatGate; ++gateIndex) {
                    const auto throatGate = tunrun::gateAt(
                        app.courseSeed, static_cast<std::uint32_t>(gateIndex));
                    if (tunrun::collidesWithGateThroatAlongSegment(
                            app.courseSeed, throatGate,
                            previousX, previousY, previousDistance,
                            app.flight.x, app.flight.y,
                            static_cast<double>(app.flight.distance))) {
                        throatCollisionGate = throatGate.index;
                        break;
                    }
                }
                if (throatCollisionGate != std::numeric_limits<std::uint32_t>::max()) {
                    app.crashCause = CrashCause::Gate;
                    app.lastHitObjectIndex = throatCollisionGate;
                    finishRun(app);
                    app.screens.replace(tunrun::Screen::Crash);
                } else if (tunrun::collidesWithTunnelWall(
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
                        const auto gateCrossing = tunrun::gatePointAtCourseCrossing(
                            app.courseSeed, previousX, previousY, previousDistance,
                            app.flight.x, app.flight.y, app.flight.distance, gate);
                        if (!gateCrossing.crossedPlane) continue;
                        if (tunrun::collidesWithGateAtCrossingPoint(gateCrossing, gate)) {
                            app.crashCause = CrashCause::Gate;
                            app.lastHitObjectIndex = gate.index;
                            finishRun(app);
                            app.screens.replace(tunrun::Screen::Crash);
                            break;
                        }
                        if (app.runGatesCleared < std::numeric_limits<std::uint64_t>::max()) {
                            ++app.runGatesCleared;
                        }
                        if (gateCrossing.valid) {
                            (void)tunrun::awardGatePass(
                                app.runScore, gate, gateCrossing.x, gateCrossing.y);
                        }
                    }
                }
                if (app.screens.current() == tunrun::Screen::Preview) {
                    const auto firstHazard = tunrun::hazardAt(app.courseSeed, 0U);
                    const double hazardLongitudinalReach =
                        tunrun::kHazardMaximumRadius + tunrun::kHazardCraftCollisionRadius;
                    const int firstCandidate = std::max(0, static_cast<int>(std::floor(
                        (previousDistance - hazardLongitudinalReach -
                         firstHazard.distance) / tunrun::kHazardSpacing)) - 1);
                    const int lastCandidate = std::max(firstCandidate, static_cast<int>(std::ceil(
                        (static_cast<double>(app.flight.distance) +
                         hazardLongitudinalReach - firstHazard.distance) /
                        tunrun::kHazardSpacing)) + 1);
                    for (int hazardIndex = firstCandidate;
                         hazardIndex <= lastCandidate; ++hazardIndex) {
                        const auto hazard = tunrun::hazardAt(app.courseSeed,
                            static_cast<std::uint32_t>(hazardIndex));
                        if (tunrun::sweptCollidesWithHazard(
                                app.courseSeed, previousX, previousY, previousDistance, previousElapsed,
                                app.flight.x, app.flight.y, app.flight.distance, app.elapsed,
                                hazard)) {
                            app.crashCause = CrashCause::Hazard;
                            app.lastHitObjectIndex = hazard.index;
                            finishRun(app);
                            app.screens.replace(tunrun::Screen::Crash);
                            break;
                        }
                    }
                }
                if (app.screens.current() == tunrun::Screen::Preview) {
                    const int firstRewardIndex = std::max(0, static_cast<int>(std::floor(
                        (previousDistance - tunrun::kRewardStartDistance) /
                        tunrun::kRewardSpacing)) - 1);
                    const int lastRewardIndex = std::max(firstRewardIndex,
                        static_cast<int>(std::floor(
                            (static_cast<double>(app.flight.distance) -
                             tunrun::kRewardStartDistance) / tunrun::kRewardSpacing)) + 1);
                    for (int rewardIndex = firstRewardIndex;
                         rewardIndex <= lastRewardIndex; ++rewardIndex) {
                        const auto reward = tunrun::rewardAt(app.courseSeed,
                            static_cast<std::uint32_t>(rewardIndex));
                        if (!tunrun::collectsRewardAtCourseCrossing(
                                app.courseSeed, previousX, previousY, previousDistance,
                                app.flight.x, app.flight.y, app.flight.distance, reward)) continue;
                        const auto maxReward = std::numeric_limits<std::uint64_t>::max();
                        if (reward.kind == tunrun::RewardKind::AetherShard) {
                            app.runAetherPickupReward =
                                maxReward - app.runAetherPickupReward < reward.shardValue
                                ? maxReward : app.runAetherPickupReward + reward.shardValue;
                        } else {
                            if (app.runSingularityCorePickupReward < maxReward) {
                                ++app.runSingularityCorePickupReward;
                            }
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
            drawTunnel(app.courseSeed, app.flight.distance, app.flight.x, app.flight.y,
                       static_cast<std::uint32_t>(app.selectedShip),
                       tunrun::gameModeName(app.activeMode), tpp, app.reduceMotion, !tpp,
                       app.flight.pitch, app.flight.yaw,
                       app.cameraLookYaw, app.cameraLookPitch, app.flight.roll,
                       app.flight.bank, app.flight.actualForwardSpeed,
                       app.flight.boostEnergy, app.flight.dashCooldownRemaining,
                       app.flight.dashRemaining, app.elapsed, app.runScore,
                       app.runAetherPickupReward, app.runSingularityCorePickupReward,
                       app.tppCameraFollow, app.cameraModeBlend, dt);
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
            drawCentred("SEEDED COURSE / NATIVE FLIGHT PROTOTYPE", 141.0F, 12, kMuted);
            DrawText(TextFormat("AETHER SHARDS  %llu",
                                static_cast<unsigned long long>(app.profile.aetherShards)),
                     52, 162, 14, kAccent);
            DrawText(TextFormat("SINGULARITY CORES  %llu",
                                static_cast<unsigned long long>(app.profile.singularityCores)),
                     330, 162, 14, kText);
            // Tighten rows at smaller window heights so every menu item stays visible.
            const bool compactMenu = GetScreenHeight() < 705;
            const float mainButtonHeight = compactMenu ? 36.0F : kButtonHeight;
            const float mainButtonGap = compactMenu ? 4.0F : kButtonGap;
            const int mainMenuTop = compactMenu ? 180 : 185;
            const int picked = drawMenu(mainItems, mainSelection, mainMenuTop,
                true, -1, true, mainButtonHeight, mainButtonGap);
            if (picked >= 0) {
                switch (picked) {
                case 0:
                    app.activeMode = tunrun::GameModeChoice::Endless;
                    resetFlight(app, tpp);
                    chooseNextSeed(app);
                    app.screens.push(tunrun::Screen::Preview);
                    break;
                case 1: app.screens.push(tunrun::Screen::Hangar); break;
                case 2: app.modeMessage.clear(); app.screens.push(tunrun::Screen::Modes); break;
                case 3: app.screens.push(tunrun::Screen::SeedLab); break;
                case 4: app.screens.push(tunrun::Screen::Records); break;
                case 5: app.screens.push(tunrun::Screen::Controls); break;
                case 6: app.screens.push(tunrun::Screen::Settings); break;
                case 7: app.screens.push(tunrun::Screen::Credits); break;
                case 8: app.screens.push(tunrun::Screen::ExitConfirm); break;
                default: break;
                }
            }
            break;
        }
        case tunrun::Screen::Hangar: {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN) ||
                padPressed(GAMEPAD_BUTTON_LEFT_FACE_UP) ||
                padPressed(GAMEPAD_BUTTON_LEFT_FACE_DOWN)) {
                hangarFocus = 1 - hangarFocus;
            }
            const int previewShip = app.hangarPreviewShip;
            const int previousPreviewShip = app.hangarPreviewShip;
            const auto& definition = tunrun::shipDefinition(static_cast<std::uint32_t>(previewShip));
            const bool unlocked = app.profile.unlockedShips[static_cast<std::size_t>(previewShip)];
            const bool active = app.selectedShip == previewShip;
            drawHeader("01 / COLLECTION", "HANGAR", "3D ship showroom / unlocks use local saved progression.");
            DrawText("SELECTED SHIP", 66, 190, 13, kAccent);
            DrawText(definition.name, 66, 218, 30, kText);
            const std::string status = unlocked
                ? (active ? "STATUS: ACTIVE / UNLOCKED" : "STATUS: UNLOCKED / NOT ACTIVE")
                : "STATUS: LOCKED";
            DrawText(status.c_str(), 66, 255, 14, unlocked ? kAccent : kMuted);
            DrawText(TextFormat("SPEED %.2fx", definition.speedMultiplier), 66, 292, 14, kText);
            DrawText(TextFormat("ACCELERATION %.2fx", definition.accelerationMultiplier), 66, 316, 14, kText);
            DrawText(TextFormat("BOOST DRAIN %.2fx", definition.boostDrainMultiplier), 66, 340, 14, kText);
            DrawText(TextFormat("ENERGY RECOVERY %.2fx", definition.energyRegenerationMultiplier),
                     66, 364, 14, kText);
            drawHangarShipPreview3D(static_cast<std::uint32_t>(previewShip),
                                    GetScreenWidth() - 210, 315, app.elapsed);

            std::string actionLabel;
            if (unlocked) {
                actionLabel = active ? "ACTIVE SHIP" : "EQUIP THIS SHIP";
            } else {
                actionLabel = "UNLOCK SHIP";
            }
            const float actionWidth = std::clamp(
                static_cast<float>(GetScreenWidth()) - 450.0F, 150.0F, 385.0F);
            const Rectangle actionBounds{66.0F, 405.0F, actionWidth, 45.0F};
            const bool actionClicked = drawButton(actionBounds, actionLabel.c_str(), hangarFocus == 0);
            const bool actionConfirmed = hangarFocus == 0 && confirmPressed();
            if (!active && (actionClicked || actionConfirmed)) {
                if (unlocked) {
                    activateHangarShip(app, static_cast<std::uint32_t>(previewShip));
                } else {
                    app.pendingHangarShip = static_cast<std::uint32_t>(previewShip);
                    hangarPurchaseSelection = 0;
                    app.screens.push(tunrun::Screen::HangarPurchaseConfirm);
                }
            }

            std::string priceLine;
            if (unlocked) {
                priceLine = active ? "EQUIPPED / FLIGHT READY" : "UNLOCKED / EQUIP IS A SEPARATE ACTION";
            } else if (definition.aetherShardCost > 0U) {
                priceLine = "PRICE: " + std::to_string(definition.aetherShardCost) + " AETHER SHARDS";
            } else {
                priceLine = "PRICE: " + std::to_string(definition.singularityCoreCost) + " SINGULARITY CORES";
            }
            DrawText(priceLine.c_str(), 66, 385, 12, unlocked ? kMuted : kAccent);

            const float previewLeft = static_cast<float>(GetScreenWidth()) - 360.0F;
            if (drawButton(Rectangle{previewLeft + 10.0F, 390, 34, 34}, "<", false))
                app.hangarPreviewShip = (app.hangarPreviewShip + 7) % 8;
            if (drawButton(Rectangle{static_cast<float>(GetScreenWidth()) - 104.0F, 390, 34, 34}, ">", false))
                app.hangarPreviewShip = (app.hangarPreviewShip + 1) % 8;
            if (leftPressed()) app.hangarPreviewShip = (app.hangarPreviewShip + 7) % 8;
            if (rightPressed()) app.hangarPreviewShip = (app.hangarPreviewShip + 1) % 8;
            if (IsGamepadAvailable(0)) {
                const float horizontalAxis = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
                if (horizontalAxis < -0.65F && !app.hangarAxisLeftHeld) {
                    app.hangarPreviewShip = (app.hangarPreviewShip + 7) % 8;
                    app.hangarAxisLeftHeld = true;
                } else if (horizontalAxis > 0.65F && !app.hangarAxisRightHeld) {
                    app.hangarPreviewShip = (app.hangarPreviewShip + 1) % 8;
                    app.hangarAxisRightHeld = true;
                } else if (std::abs(horizontalAxis) < 0.25F) {
                    app.hangarAxisLeftHeld = false;
                    app.hangarAxisRightHeld = false;
                }
            }
            if (app.hangarPreviewShip != previousPreviewShip && !actionClicked && !actionConfirmed) {
                app.hangarMessage.clear();
            }
            DrawText(TextFormat("AETHER SHARDS %llu  |  SINGULARITY CORES %llu",
                                static_cast<unsigned long long>(app.profile.aetherShards),
                                static_cast<unsigned long long>(app.profile.singularityCores)),
                     66, 466, 13, kAccent);
            if (!app.hangarMessage.empty()) {
                const std::string visibleMessage = app.hangarMessage.substr(0, 88);
                DrawText(visibleMessage.c_str(), 66, 489, 12, app.saveWarning ? kDanger : kMuted);
            }
            const Rectangle backBounds{
                (static_cast<float>(GetScreenWidth()) - kPanelWidth) / 2.0F,
                static_cast<float>(GetScreenHeight() - 86), kPanelWidth, 42.0F};
            const bool backClicked = drawButton(backBounds, "BACK", hangarFocus == 1);
            const bool backConfirmed = hangarFocus == 1 && confirmPressed();
            if (backClicked || backConfirmed || backPressed()) app.screens.pop();
            break;
        }
        case tunrun::Screen::HangarPurchaseConfirm: {
            const std::uint32_t shipId = std::min<std::uint32_t>(
                app.pendingHangarShip, static_cast<std::uint32_t>(tunrun::kShipCatalog.size() - 1U));
            const auto& definition = tunrun::shipDefinition(shipId);
            drawHeader("01 / COLLECTION", "CONFIRM SHIP UNLOCK",
                       "Confirming unlocks this ship only. Your active ship will not change.");
            drawCentred(definition.name, 188.0F, 28, kText);
            if (definition.aetherShardCost > 0U) {
                DrawText(TextFormat("COST: %llu AETHER SHARDS",
                    static_cast<unsigned long long>(definition.aetherShardCost)),
                    GetScreenWidth() / 2 - 170, 232, 17, kAccent);
                DrawText(TextFormat("YOUR BALANCE: %llu",
                    static_cast<unsigned long long>(app.profile.aetherShards)),
                    GetScreenWidth() / 2 - 170, 260, 14, kMuted);
            } else {
                DrawText(TextFormat("COST: %llu SINGULARITY CORES",
                    static_cast<unsigned long long>(definition.singularityCoreCost)),
                    GetScreenWidth() / 2 - 170, 232, 17, kAccent);
                DrawText(TextFormat("YOUR BALANCE: %llu",
                    static_cast<unsigned long long>(app.profile.singularityCores)),
                    GetScreenWidth() / 2 - 170, 260, 14, kMuted);
            }
            drawCentred("CANCEL LEAVES YOUR WALLET AND HANGAR UNCHANGED.", 294.0F, 12, kMuted);
            const int picked = drawMenu(hangarPurchaseItems, hangarPurchaseSelection, 330);
            if (picked == 0 || (picked < 0 && backPressed())) {
                app.screens.pop();
            } else if (picked == 1) {
                purchaseHangarShip(app, shipId);
                app.screens.pop();
            }
            break;
        }
        case tunrun::Screen::Records: {
            drawHeader("PROFILE / 01", "RECORDS & STATISTICS",
                       "Local career records from completed runs; no account or cloud sync.");
            DrawText("PERSONAL BESTS", 70, 184, 15, kAccent);
            DrawText(TextFormat("BEST DISTANCE     %.1f", app.profile.bestDistance), 78, 218, 18, kText);
            DrawText(TextFormat("BEST SCORE        %llu", static_cast<unsigned long long>(app.profile.bestScore)), 78, 251, 18, kText);
            DrawText(TextFormat("BEST GATE COMBO   %llu", static_cast<unsigned long long>(app.profile.bestCombo)), 78, 284, 18, kText);
            DrawLine(70, 316, GetScreenWidth() - 70, 316, kEdge);
            DrawText("RUN HISTORY", 70, 334, 15, kAccent);
            DrawText(TextFormat("RUNS RECORDED     %llu", static_cast<unsigned long long>(app.profile.totalRuns)), 78, 367, 17, kText);
            DrawText(TextFormat("CRASHES           %llu", static_cast<unsigned long long>(app.profile.totalCrashes)), 78, 397, 17, kText);
            DrawText(TextFormat("AETHER SHARDS     %llu", static_cast<unsigned long long>(app.profile.aetherShards)), 430, 218, 16, kAccent);
            DrawText(TextFormat("SINGULARITY CORES %llu", static_cast<unsigned long long>(app.profile.singularityCores)), 430, 251, 16, kText);
            const auto unlockedCount = std::count(app.profile.unlockedShips.begin(), app.profile.unlockedShips.end(), true);
            DrawText(TextFormat("SHIPS UNLOCKED    %d / %d", static_cast<int>(unlockedCount), static_cast<int>(tunrun::kProfileShipCount)), 430, 284, 16, kText);
            DrawText(TextFormat("ACTIVE SHIP       %s", tunrun::shipDefinition(static_cast<std::uint32_t>(app.selectedShip)).name), 430, 367, 15, kText);
            if (app.saveWarning) DrawText("SAVE WARNING: CAREER DATA MAY NOT BE PERSISTED", 78, 433, 11, kDanger);
            if (drawMenu({"BACK"}, recordsSelection, GetScreenHeight() - 86) == 0) app.screens.pop();
            if (backPressed()) app.screens.pop();
            break;
        }
        case tunrun::Screen::Controls: {
            drawHeader("SYSTEM / CONTROLS", "CONTROLS",
                       "W/S manage speed, A/D steer laterally, mouse moves the camera.");
            DrawText("KEYBOARD", 70, 184, 15, kAccent);
            DrawText("GO / ACCELERATE  W", 78, 218, 13, kText);
            DrawText("BRAKE / SLOW  S", 78, 246, 13, kText);
            DrawText("LEFT A / RIGHT B (D ALSO WORKS)", 78, 274, 12, kText);
            DrawText("MOUSE LOOK: CAMERA ONLY", 78, 302, 13, kAccent);
            DrawText("ROTATE  ARROW KEYS", 78, 330, 13, kText);
            DrawText("BARREL ROLL  Q / E", 78, 358, 13, kText);
            DrawText("BOOST SHIFT / PRECISION CTRL", 78, 386, 12, kText);
            DrawText("DASH SPACE / CAMERA V / ESC BACK", 78, 410, 12, kMuted);
            DrawLine(402, 184, 402, 448, kEdge);
            DrawText("GAMEPAD", 432, 184, 15, kAccent);
            DrawText("LEFT STICK  MOVE", 440, 218, 13, kText);
            DrawText("RIGHT STICK  ROTATE", 440, 246, 13, kText);
            DrawText("RT  BOOST  /  LT  PRECISION", 440, 274, 13, kText);
            DrawText("A  DASH  /  Y  CAMERA", 440, 302, 13, kText);
            DrawText("START / B  PAUSE / BACK", 440, 330, 13, kText);
            DrawText("ROTATION HAS INERTIA; Q/E BARREL ROLLS", 440, 380, 12, kMuted);
            DrawText(TextFormat("DASH %.2fs  /  COOLDOWN %.2fs",
                     tunrun::kDashDuration, tunrun::kDashCooldown), 440, 408, 12, kMuted);
            if (drawMenu({"BACK"}, controlsSelection, GetScreenHeight() - 86) == 0) {
                app.screens.pop();
            }
            if (backPressed()) app.screens.pop();
            break;
        }
        case tunrun::Screen::Modes: {
            drawHeader("02 / FLIGHT PLAN", "GAME MODES",
                       "Available modes start a run; unfinished modes explain their status.");
            const int picked = drawMenu(modes, modesSelection, 192);
            if (picked >= 0) {
                const auto choice = static_cast<tunrun::GameModeChoice>(picked);
                switch (choice) {
                case tunrun::GameModeChoice::Campaign:
                    app.modeMessage = tunrun::modeUnavailableMessage(choice);
                    break;
                case tunrun::GameModeChoice::Endless:
                    app.modeMessage.clear();
                    app.activeMode = tunrun::GameModeChoice::Endless;
                    resetFlight(app, tpp);
                    chooseNextSeed(app);
                    app.screens.push(tunrun::Screen::Preview);
                    break;
                case tunrun::GameModeChoice::CustomSeedRun:
                    app.modeMessage.clear();
                    app.activeMode = tunrun::GameModeChoice::CustomSeedRun;
                    beginSeedEntry(app);
                    seedEntrySelection = 0;
                    app.screens.push(tunrun::Screen::SeedEntry);
                    break;
                case tunrun::GameModeChoice::PracticePreview:
                    app.modeMessage.clear();
                    app.activeMode = tunrun::GameModeChoice::PracticePreview;
                    resetFlight(app, tpp);
                    chooseNextSeed(app);
                    app.screens.push(tunrun::Screen::Preview);
                    break;
                case tunrun::GameModeChoice::Back:
                    app.modeMessage.clear();
                    app.screens.pop();
                    break;
                }
            } else if (backPressed()) {
                app.modeMessage.clear();
                app.screens.pop();
            }
            if (!app.modeMessage.empty()) {
                drawCentred(app.modeMessage.c_str(), 492.0F, 11, kMuted);
            }
            break;
        }
        case tunrun::Screen::Settings: {
            drawHeader("03 / CONFIGURATION", "SETTINGS",
                       "Adjust mouse-look camera sensitivity and display options.");
            auto labels = settingsItems;
            labels[0] = std::string("FULLSCREEN: ") + (app.fullscreen ? "ON" : "OFF");
            labels[1] = std::string("FPS COUNTER: ") + (app.showFps ? "ON" : "OFF");
            labels[2] = std::string("REDUCED MOTION: ") + (app.reduceMotion ? "ON" : "OFF");
            labels[3] = std::string("MOUSE CAMERA: ") +
                (app.profile.mouseSteering ? "ON" : "OFF");
            labels[4] = std::string("MOUSE SENSITIVITY: ") +
                TextFormat("%.4f", app.profile.mouseSensitivity);
            const int settingsMenuY = std::max(
                130, std::min(185, GetScreenHeight() - 360));
            const float settingsButtonHeight = 40.0F;
            const float settingsButtonGap = 7.0F;
            const int picked = drawMenu(labels, settingsSelection, settingsMenuY,
                true, -1, true, settingsButtonHeight, settingsButtonGap);

            const float panelX = (static_cast<float>(GetScreenWidth()) - kPanelWidth) / 2.0F;
            const float sliderX = panelX + 18.0F;
            const float sliderWidth = kPanelWidth - 36.0F;
            const float sliderY = static_cast<float>(settingsMenuY) +
                4.0F * (settingsButtonHeight + settingsButtonGap) + 33.0F;
            const float sliderPosition = mouseSensitivitySliderPosition(
                app.profile.mouseSensitivity);
            DrawRectangle(static_cast<int>(sliderX), static_cast<int>(sliderY),
                static_cast<int>(sliderWidth), 3, Color{42, 51, 64, 255});
            DrawRectangle(static_cast<int>(sliderX), static_cast<int>(sliderY),
                static_cast<int>(sliderWidth * sliderPosition), 3, kAccent);
            DrawCircle(static_cast<int>(sliderX + sliderWidth * sliderPosition),
                static_cast<int>(sliderY + 1.0F), 5.0F, kAccent);

            const bool settingsBackRequested = backPressed();
            bool sensitivityChanged = false;
            const Rectangle sensitivityRow{
                panelX, static_cast<float>(settingsMenuY) +
                    4.0F * (settingsButtonHeight + settingsButtonGap),
                kPanelWidth, settingsButtonHeight
            };
            const bool pointerOnSensitivityRow =
                CheckCollisionPointRec(GetMousePosition(), sensitivityRow);
            const bool draggingSensitivity =
                IsMouseButtonDown(MOUSE_BUTTON_LEFT) && pointerOnSensitivityRow;
            if (draggingSensitivity) {
                const float normalized = std::clamp(
                    (GetMouseX() - sliderX) / sliderWidth, 0.0F, 1.0F);
                app.profile.mouseSensitivity = mouseSensitivityFromSlider(normalized);
                sensitivityChanged = true;
            } else if (settingsSelection == 4 && picked < 0 &&
                       (leftPressed() || rightPressed())) {
                app.profile.mouseSensitivity = adjustMouseSensitivity(
                    app.profile.mouseSensitivity, leftPressed() ? -1 : 1);
                sensitivityChanged = true;
            } else if (picked == 4) {
                if (leftPressed()) {
                    app.profile.mouseSensitivity = adjustMouseSensitivity(
                        app.profile.mouseSensitivity, -1);
                } else if (rightPressed() || confirmPressed()) {
                    app.profile.mouseSensitivity = adjustMouseSensitivity(
                        app.profile.mouseSensitivity, 1);
                }
                sensitivityChanged = true;
            }

            if (picked == 0) {
                app.fullscreen = !app.fullscreen;
                ToggleFullscreen();
            } else if (picked == 1) {
                app.showFps = !app.showFps;
            } else if (picked == 2) {
                app.reduceMotion = !app.reduceMotion;
            } else if (picked == 3) {
                app.profile.mouseSteering = !app.profile.mouseSteering;
                app.cameraLookYaw = 0.0F;
                app.cameraLookPitch = 0.0F;
                rawMouse.clear();
                if (!app.profile.mouseSteering && rawMouse.installed()) {
                    rawMouse.setActive(false);
                }
            } else if (picked == 5) {
                settingsResetSelection = 0;
                app.screens.push(tunrun::Screen::SettingsResetConfirm);
            } else if (picked == 6) {
                app.screens.pop();
            }
            if (picked >= 0 && picked <= 3) (void)persistProfile(app);
            if (sensitivityChanged) (void)persistProfile(app);
            if (settingsBackRequested && picked != 6) app.screens.pop();
            break;
        }
        case tunrun::Screen::SettingsResetConfirm: {
            drawHeader("03 / CONFIGURATION", "RESET OPTIONS?",
                       "Only configuration changes. Wallets, ships and career records stay intact.");
            drawCentred("THIS DOES NOT CHANGE GAME PROGRESSION.", 198.0F, 13, kMuted);
            const int picked = drawMenu(resetSettingsItems, settingsResetSelection, 276, true, 1);
            if (picked == 0 || (picked < 0 && backPressed())) {
                app.screens.pop();
            } else if (picked == 1) {
                if (!app.fullscreen) ToggleFullscreen();
                app.fullscreen = true;
                app.showFps = true;
                app.reduceMotion = false;
                app.profile.mouseSteering = true;
                app.profile.mouseSensitivity = kMouseSensitivityDefault;
                app.cameraLookYaw = 0.0F;
                app.cameraLookPitch = 0.0F;
                rawMouse.clear();
                (void)persistProfile(app);
                app.screens.pop();
            }
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
            const int picked = drawMenu(pauseItems, pauseSelection, 215);
            if (picked == 0) app.screens.pop();
            else if (picked == 1) {
                app.pendingRunAction = PendingRunAction::RestartSameSeed;
                runConfirmSelection = 0;
                app.screens.push(tunrun::Screen::RunConfirm);
            } else if (picked == 2) app.screens.push(tunrun::Screen::Controls);
            else if (picked == 3) app.screens.push(tunrun::Screen::Settings);
            else if (picked == 4) {
                app.pendingRunAction = PendingRunAction::ReturnToMainMenu;
                runConfirmSelection = 0;
                app.screens.push(tunrun::Screen::RunConfirm);
            }
            if (backPressed()) app.screens.pop();
            break;
        }
        case tunrun::Screen::RunConfirm: {
            if (app.pendingRunAction == PendingRunAction::None) {
                if (drawMenu({"BACK"}, runConfirmSelection, 320) == 0 || backPressed()) {
                    app.screens.pop();
                }
                break;
            }
            drawHeader("FLIGHT / CONFIRMATION", "DISCARD CURRENT RUN?",
                       "Unbanked rewards and the in-run score are not saved yet.");
            drawCentred("YOUR CURRENT RUN PROGRESS WILL BE LOST.", 196.0F, 13, kDanger);
            const auto& confirmRunItems =
                app.pendingRunAction == PendingRunAction::RestartSameSeed
                    ? confirmRestartItems : confirmReturnItems;
            const int picked = drawMenu(confirmRunItems, runConfirmSelection, 276, true, 1);
            if (picked == 0 || (picked < 0 && backPressed())) {
                app.pendingRunAction = PendingRunAction::None;
                app.screens.pop();
            } else if (picked == 1) {
                const auto action = app.pendingRunAction;
                app.pendingRunAction = PendingRunAction::None;
                app.screens.pop(); // Return to Pause before applying the choice.
                if (action == PendingRunAction::RestartSameSeed) {
                    if (app.screens.current() == tunrun::Screen::Pause) app.screens.pop();
                    resetFlight(app, tpp);
                    app.screens.replace(tunrun::Screen::Preview);
                } else {
                    app.screens.reset();
                }
            }
            break;
        }
        case tunrun::Screen::Crash: {
            drawHeader("SYSTEM / FLIGHT TERMINATED",
                       app.crashCause == CrashCause::Gate ? "GATE COLLISION" :
                           app.crashCause == CrashCause::Hazard ? "MOVING HAZARD IMPACT" :
                           "WALL COLLISION",
                       app.crashCause == CrashCause::Gate
                           ? "Craft crossed an obstacle plane outside its safe aperture."
                           : app.crashCause == CrashCause::Hazard
                               ? "Craft intersected a moving procedural mine."
                               : "Craft collision volume touched the tunnel boundary.");
            drawCentred(TextFormat("DISTANCE %.1f   TIME %.1fs   SEED %016llX",
                                   app.flight.distance, app.elapsed,
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
            drawCentred(TextFormat("RUN SCORE %llu   CAREER BEST %llu",
                                   static_cast<unsigned long long>(app.runScore.total),
                                   static_cast<unsigned long long>(app.profile.bestScore)),
                        280.0F, 12, kAccent);
            drawCentred(TextFormat("BEST COMBO %llu   CLEAN PASSES %llu",
                                   static_cast<unsigned long long>(app.profile.bestCombo),
                                   static_cast<unsigned long long>(app.runScore.cleanPasses)),
                        298.0F, 11, kMuted);
            const char* resultNotice = tunrun::runResultNoticeText(
                tunrun::classifyRunResultNotice(
                    app.saveWarning, app.newBestScore, app.newBestCombo));
            if (resultNotice != nullptr) {
                drawCentred(resultNotice, 312.0F, 11,
                            app.saveWarning ? kDanger : kAccent);
            }
            if (app.crashCause == CrashCause::Gate) {
                const auto hitGate = tunrun::gateAt(app.courseSeed, app.lastHitObjectIndex);
                drawCentred(TextFormat("GATE #%u   DISTANCE %.1f   TYPE %s",
                                       app.lastHitObjectIndex, hitGate.distance,
                                       tunrun::gateKindName(hitGate.kind)),
                            267.0F, 11, kDanger);
            } else if (app.crashCause == CrashCause::Hazard) {
                const auto hitMine = tunrun::hazardAt(app.courseSeed, app.lastHitObjectIndex);
                drawCentred(TextFormat("MINE #%u   DISTANCE %.1f   HASH %016llX",
                                       app.lastHitObjectIndex, hitMine.distance,
                                       static_cast<unsigned long long>(
                                           tunrun::hazardHash(app.courseSeed, 128U))),
                            267.0F, 11, kDanger);
            }
            const bool compactCrashMenu = GetScreenHeight() < 705;
            const float crashButtonHeight = compactCrashMenu ? 36.0F : kButtonHeight;
            const float crashButtonGap = compactCrashMenu ? 4.0F : kButtonGap;
            const int crashMenuTop = compactCrashMenu ? 334 : 328;
            const int picked = drawMenu(crashItems, crashSelection, crashMenuTop,
                true, -1, true, crashButtonHeight, crashButtonGap);
            if (app.seedCopiedNotice) {
                const int noticeY = crashMenuTop + static_cast<int>(crashItems.size()) *
                    static_cast<int>(crashButtonHeight + crashButtonGap) + 4;
                drawCentred("SEED COPIED TO CLIPBOARD", static_cast<float>(noticeY),
                            10, kAccent);
            }
            if (picked == 0) {
                resetFlight(app, tpp);
                app.screens.replace(tunrun::Screen::Preview);
            } else if (picked == 1) {
                copyCourseSeed(app);
            } else if (picked == 2) {
                chooseNextSeed(app);
                resetFlight(app, tpp);
                app.screens.replace(tunrun::Screen::Preview);
            } else if (picked == 3) {
                app.screens.reset();
            }
            if (backPressed()) app.screens.reset();
            break;
        }
        case tunrun::Screen::SeedEntry: {
            drawHeader("03 / GENERATION", "SEED ENTRY",
                       "Use reproducible text or a literal 64-bit hexadecimal seed.");
            bool typedThisFrame = false;
            const bool pasteRequested = IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V);
            if (pasteRequested) {
                const char* clipboard = GetClipboardText();
                if (clipboard) for (const char* cursor=clipboard; *cursor && app.seedEntryText.size()<64U; ++cursor) {
                    const unsigned char c=static_cast<unsigned char>(*cursor);
                    if (c>=32U && c<=126U) app.seedEntryText.push_back(*cursor);
                }
                // Discard the V character event associated with the paste chord.
                while (GetCharPressed() > 0) {}
                typedThisFrame = true;
                app.seedEntryMessage.clear(); app.seedEntryHasError=false;
            } else {
                while (true) {
                    const int character=GetCharPressed();
                    if (character<=0) break;
                    typedThisFrame = true;
                    if (character>=32 && character<=126 && app.seedEntryText.size()<64U) app.seedEntryText.push_back(static_cast<char>(character));
                    app.seedEntryMessage.clear(); app.seedEntryHasError=false;
                }
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                typedThisFrame = true;
                if (!app.seedEntryText.empty()) app.seedEntryText.pop_back();
                app.seedEntryMessage.clear(); app.seedEntryHasError=false;
            }
            if (IsKeyPressed(KEY_DELETE)) {
                typedThisFrame = true;
                app.seedEntryText.clear();
                app.seedEntryMessage.clear(); app.seedEntryHasError=false;
            }
            static constexpr char chars[]="abcdefghijklmnopqrstuvwxyz0123456789 _-";
            constexpr int charCount=static_cast<int>(sizeof(chars)-1U);
            if (IsGamepadAvailable(0)) {
                if (padPressed(GAMEPAD_BUTTON_LEFT_FACE_LEFT)) seedPickerIndex=(seedPickerIndex+charCount-1)%charCount;
                else if (padPressed(GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) seedPickerIndex=(seedPickerIndex+1)%charCount;
                if (padPressed(GAMEPAD_BUTTON_RIGHT_FACE_LEFT) && app.seedEntryText.size()<64U) {
                    app.seedEntryText.push_back(chars[seedPickerIndex]);
                    typedThisFrame = true;
                    app.seedEntryMessage.clear(); app.seedEntryHasError=false;
                }
                if (padPressed(GAMEPAD_BUTTON_RIGHT_FACE_UP)) {
                    if (!app.seedEntryText.empty()) app.seedEntryText.pop_back();
                    typedThisFrame = true;
                    app.seedEntryMessage.clear(); app.seedEntryHasError=false;
                }
            }
            const Rectangle box{static_cast<float>(GetScreenWidth()/2-270),220.0F,540.0F,62.0F};
            const bool hovered=CheckCollisionPointRec(GetMousePosition(),box);
            DrawText("RUN SEED",static_cast<int>(box.x),193,14,kAccent);
            DrawRectangleRounded(box,0.10F,8,hovered?Color{36,50,68,255}:kPanel);
            DrawRectangleRoundedLinesEx(box,0.10F,8,1.4F,app.seedEntryHasError?kDanger:kAccent);
            const std::string shown=app.seedEntryText.size()>30U?".."+app.seedEntryText.substr(app.seedEntryText.size()-30U):app.seedEntryText;
            const int tx=static_cast<int>(box.x+16),ty=static_cast<int>(box.y+22);
            DrawText(shown.c_str(),tx,ty,18,kText);
            const int caret=std::min(static_cast<int>(box.x+box.width-14),tx+MeasureText(shown.c_str(),18)+3);
            DrawLine(caret,ty-1,caret,ty+20,kAccent);
            DrawText("TYPE/PASTE: 1-64 characters; case and repeated spaces are normalized.",50,300,13,kMuted);
            DrawText("HEX: use 0x + up to 16 digits, or exactly 16 hexadecimal digits.",50,320,13,kMuted);
            DrawText(TextFormat("CONTROLLER PICKER: [%c]  D-PAD LEFT/RIGHT, X ADD, Y DELETE",chars[seedPickerIndex]),50,340,13,kMuted);
            if (!app.seedEntryMessage.empty()) {
                drawCentred(app.seedEntryMessage.c_str(),363,13,app.seedEntryHasError?kDanger:kAccent);
            } else {
                DrawText(TextFormat("CURRENT COURSE: %016llX",
                    static_cast<unsigned long long>(app.courseSeed)),50,363,12,kMuted);
            }
            const int picked=drawMenu(seedEntryItems,seedEntrySelection,390,false,-1,!typedThisFrame);
            const bool confirmAction = IsKeyPressed(KEY_ENTER) || padPressed(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
            const int action = picked >= 0 ? picked : (confirmAction ? seedEntrySelection : -1);
            if (action==0) {
                const auto parsed=tunrun::parseSeedText(app.seedEntryText);
                if (!parsed.valid) { app.seedEntryMessage=parsed.error; app.seedEntryHasError=true; }
                else if (parsed.seed==0U) { app.seedEntryMessage="Seed 0 is reserved; choose a non-zero seed."; app.seedEntryHasError=true; }
                else {
                    const auto oldRoot=app.rootSeed, oldProfileRoot=app.profile.rootSeed;
                    const auto oldSerial=app.runSerial, oldProfileSerial=app.profile.runSerial;
                    app.rootSeed=parsed.seed; app.runSerial=0U;
                    if (!persistProfile(app)) {
                        const std::string saveError=app.saveWarningMessage;
                        app.rootSeed=oldRoot; app.runSerial=oldSerial;
                        app.profile.rootSeed=oldProfileRoot; app.profile.runSerial=oldProfileSerial;
                        app.seedEntryMessage="Seed not saved: "+saveError; app.seedEntryHasError=true;
                    } else {
                        app.courseSeed=parsed.seed;
                        app.activeMode = tunrun::GameModeChoice::CustomSeedRun;
                        resetFlight(app,tpp);
                        app.screens.replace(tunrun::Screen::Preview);
                    }
                }
            } else if (action==1 || (action<0 && backPressed())) app.screens.pop();
            break;
        }
        case tunrun::Screen::SeedLab: {
            drawHeader("03 / GENERATION", "SEED LAB", "Inspect deterministic course, gate, mine, and reward data for this seed.");
            const auto currentShipId = static_cast<std::uint32_t>(app.selectedShip);
            // These deterministic checks are expensive enough to keep out of the
            // render loop. Recompute once when either seed or selected ship changes.
            static bool validationCacheValid = false;
            static std::uint64_t validationCacheSeed = 0U;
            static std::uint32_t validationCacheShip = 0U;
            static tunrun::CourseValidation cachedCourseValidation;
            static tunrun::ObstacleValidation cachedGateValidation;
            static tunrun::HazardValidation cachedHazardValidation;
            static tunrun::ReachabilityValidation cachedReachability;
            static tunrun::StateGraphRouteValidation cachedRouteGraph;
            if (!validationCacheValid || validationCacheSeed != app.courseSeed ||
                validationCacheShip != currentShipId) {
                cachedCourseValidation = tunrun::validateCourse(app.courseSeed, 360.0);
                cachedGateValidation = tunrun::validateObstacleSet(app.courseSeed, 32U);
                cachedHazardValidation = tunrun::validateHazardSet(app.courseSeed, 32U);
                cachedReachability = tunrun::validateGateReachability(
                    app.courseSeed, 32U, currentShipId);
                cachedRouteGraph = tunrun::validateStateGraphRouteReachability(
                    app.courseSeed, 12U, currentShipId);
                validationCacheSeed = app.courseSeed;
                validationCacheShip = currentShipId;
                validationCacheValid = true;
            }
            const auto& validation = cachedCourseValidation;
            const auto& gateValidation = cachedGateValidation;
            const auto& hazardValidation = cachedHazardValidation;
            const auto& reachability = cachedReachability;
            const auto& routeGraph = cachedRouteGraph;
            drawCentred(TextFormat("SEED  %016llX", static_cast<unsigned long long>(app.courseSeed)),
                        160.0F, 20, kText);
            drawCentred(TextFormat("COURSE V%u   HASH %016llX",
                                   tunrun::kCourseGeneratorVersion,
                                   static_cast<unsigned long long>(tunrun::courseHash(app.courseSeed))),
                        189.0F, 12, kAccent);
            drawCentred(TextFormat("COURSE %s / %u SAMPLES   RADIUS %.2f-%.2f   MAX OFFSET %.2f",
                                   validation.valid ? "PASS" : "FAIL", validation.samplesChecked,
                                   validation.minimumRadius, validation.maximumRadius,
                                   validation.maximumCenterOffset),
                        209.0F, 10, validation.valid ? kAccent : kDanger);
            drawCentred(TextFormat("GATES V%u %s / %u   MIX S%u P%u O%u W%u",
                                   tunrun::kObstacleGeneratorVersion,
                                   gateValidation.valid ? "PASS" : "FAIL",
                                   gateValidation.gatesChecked,
                                   gateValidation.standardGates, gateValidation.precisionGates,
                                   gateValidation.offsetGates, gateValidation.wideGates),
                        229.0F, 10, gateValidation.valid ? kAccent : kDanger);
            drawCentred(TextFormat("GATE HASH %016llX   PAIRWISE %s: %s   MIN SLACK %.2F",
                                   static_cast<unsigned long long>(
                                       tunrun::obstacleHash(app.courseSeed, 32U)),
                                   tunrun::shipDefinition(currentShipId).name,
                                   reachability.valid ? "PASS" : "FAIL",
                                   reachability.minimumReachableSlack),
                        249.0F, 10, reachability.valid ? kAccent : kDanger);
            drawCentred(TextFormat("ROUTE GRAPH: %s   GATES %u/12   PEAK %u   MULTI %u   CLR %.2F   PRUNED %u   MINES %u/%u",
                                   routeGraph.valid ? "WITNESS" : "NO WITNESS",
                                   routeGraph.gatesChecked,
                                   routeGraph.peakStateCount,
                                   routeGraph.gatesWithMultiplePassingStates,
                                   routeGraph.bestWitnessMinimumClearance,
                                   routeGraph.beamPrunedStates,
                                   routeGraph.hazardCollisionStates,
                                   routeGraph.hazardChecks),
                        269.0F, 10, routeGraph.valid ? kAccent : kText);
            if (!routeGraph.valid) {
                drawCentred(TextFormat("SEARCH NOTE: %s", routeGraph.failure),
                            280.0F, 8, kText);
            }
            drawCentred(TextFormat("MINES V%u %s %u/32 H %016llX   REWARDS V%u H %016llX",
                                   tunrun::kHazardGeneratorVersion,
                                   hazardValidation.valid ? "PASS" : "FAIL",
                                   hazardValidation.hazardsChecked,
                                   static_cast<unsigned long long>(
                                       tunrun::hazardHash(app.courseSeed, 32U)),
                                   tunrun::kRewardGeneratorVersion,
                                   static_cast<unsigned long long>(
                                       tunrun::rewardHash(app.courseSeed, 32U))),
                        291.0F, 9, hazardValidation.valid ? kAccent : kDanger);
            const int seedLabMenuY = std::max(
                310, std::min(430, GetScreenHeight() - 250));
            const bool compactSeedLabMenu = GetScreenHeight() < 760;
            const float seedLabButtonHeight = compactSeedLabMenu ? 36.0F : kButtonHeight;
            const float seedLabButtonGap = compactSeedLabMenu ? 4.0F : kButtonGap;
            const int picked = drawMenu(seedLabItems, seedLabSelection, seedLabMenuY,
                true, -1, true, seedLabButtonHeight, seedLabButtonGap);
            if (app.seedCopiedNotice) {
                const int noticeY = seedLabMenuY + static_cast<int>(seedLabItems.size()) *
                    static_cast<int>(seedLabButtonHeight + seedLabButtonGap) + 4;
                drawCentred("SEED COPIED TO CLIPBOARD", static_cast<float>(noticeY),
                            10, kAccent);
            }
            if (picked == 0) {
                app.activeMode = tunrun::GameModeChoice::CustomSeedRun;
                beginSeedEntry(app); seedEntrySelection=0; app.screens.push(tunrun::Screen::SeedEntry);
            } else if (picked == 1) chooseNextSeed(app);
            else if (picked == 2) {
                app.activeMode = tunrun::GameModeChoice::CustomSeedRun;
                resetFlight(app,tpp);
                app.screens.push(tunrun::Screen::Preview);
            }
            else if (picked == 3) copyCourseSeed(app);
            else if (picked == 4) app.screens.pop();
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
                    app.fullscreen = defaults.fullscreen;
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
