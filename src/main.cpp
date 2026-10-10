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

enum class CrashCause { Wall, Gate, Hazard };
enum class PendingRunAction { None, RestartSameSeed, ReturnToMainMenu };

struct AppState {
    tunrun::ScreenStack screens;
    int selectedShip = 0;
    int hangarPreviewShip = 0;
    std::string hangarMessage;
    std::string seedEntryText;
    std::string seedEntryMessage;
    bool seedEntryHasError = false;
    std::string modeMessage;
    PendingRunAction pendingRunAction = PendingRunAction::None;
    bool showFps = true;
    bool reduceMotion = false;
    bool fullscreen = false;
    bool hangarAxisLeftHeld = false;
    bool hangarAxisRightHeld = false;
    bool exitRequested = false;
    tunrun::FlightState flight;
    float flightAccumulator = 0.0F;
    float mouseAimX = 0.0F;
    float mouseAimY = 0.0F;
    bool mouseControlEngaged = false;
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
    app.mouseAimX = 0.0F;
    app.mouseAimY = 0.0F;
    app.mouseControlEngaged = false;
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
void activateHangarShip(AppState& app, std::uint32_t shipId) {
    if (shipId >= tunrun::kProfileShipCount) {
        app.hangarMessage = "Invalid ship selection.";
        return;
    }

    tunrun::Profile candidate = app.profile;
    const bool wasUnlocked = candidate.unlockedShips[shipId];
    if (!wasUnlocked) {
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
    }

    const auto equip = tunrun::equipShip(candidate, shipId);
    if (equip == tunrun::ShipTransactionStatus::AlreadyEquipped) {
        app.hangarMessage = "This ship is already active.";
        return;
    }
    if (equip != tunrun::ShipTransactionStatus::Equipped) {
        app.hangarMessage = "Locked ships cannot be equipped.";
        return;
    }

    const tunrun::Profile previousProfile = app.profile;
    const int previousShip = app.selectedShip;
    app.profile = candidate;
    app.selectedShip = static_cast<int>(shipId);
    if (!persistProfile(app)) {
        app.profile = previousProfile;
        app.selectedShip = previousShip;
        app.hangarMessage = "Save failed; purchase/equip was rolled back.";
        return;
    }
    app.hangarMessage = wasUnlocked
        ? std::string("Equipped: ") + tunrun::shipDefinition(shipId).name
        : std::string("Unlocked and equipped: ") + tunrun::shipDefinition(shipId).name;
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
    for(int i=0;i<segments;++i) {
        const float a0=static_cast<float>(i)*2.0F*PI/segments;
        const float a1=static_cast<float>(i+1)*2.0F*PI/segments;
        const Vector3 o0=p(a0,outerRadius,0,0),o1=p(a1,outerRadius,0,0);
        const Vector3 i0=p(a0,gate.apertureRadius,gate.offsetX,gate.offsetY);
        const Vector3 i1=p(a1,gate.apertureRadius,gate.offsetX,gate.offsetY);
        DrawLine3D(o0,o1,outer); DrawLine3D(i0,i1,inner);
        if(i%2==0) DrawLine3D(o0,i0,Color{76,99,125,205});
    }
    const auto halo=[&](float radius,Color c) {
        for(int i=0;i<segments;++i) {
            const float a0=static_cast<float>(i)*2.0F*PI/segments;
            const float a1=static_cast<float>(i+1)*2.0F*PI/segments;
            DrawLine3D(p(a0,radius,gate.offsetX,gate.offsetY),
                       p(a1,radius,gate.offsetX,gate.offsetY),c);
        }
    };
    if(gate.kind==tunrun::GateKind::Precision) {
        const Color c{255,171,131,180}; halo(gate.apertureRadius+0.22F,c);
        for(int i=0;i<segments;i+=3) {
            const float a=static_cast<float>(i)*2.0F*PI/segments;
            DrawLine3D(p(a,gate.apertureRadius,gate.offsetX,gate.offsetY),
                       p(a,gate.apertureRadius+0.22F,gate.offsetX,gate.offsetY),c);
        }
    } else if(gate.kind==tunrun::GateKind::Wide) {
        const Color c{150,245,213,180}; halo(gate.apertureRadius+0.34F,c);
        for(int i=0;i<segments;i+=6) {
            const float a=static_cast<float>(i)*2.0F*PI/segments;
            DrawLine3D(p(a,gate.apertureRadius,gate.offsetX,gate.offsetY),
                       p(a,gate.apertureRadius+0.34F,gate.offsetX,gate.offsetY),c);
        }
    } else if(gate.kind==tunrun::GateKind::Offset) {
        const Color c{194,172,255,235};
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
    const Color color=core?Color{255,174,108,255}:Color{111,225,255,255};
    const Color bright=core?Color{255,204,140,255}:Color{165,243,255,255};
    const Color dark=core?Color{132,57,29,255}:Color{27,107,153,255};
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
                    const tunrun::TunnelFrame* tunnelFrame = nullptr) {
    const Color hullColors[] = {
        kAccent, Color{190, 157, 255, 255}, Color{255, 186, 116, 255},
        Color{115, 238, 207, 255}, Color{255, 125, 145, 255},
        Color{187, 166, 255, 255}, Color{255, 218, 130, 255},
        Color{165, 190, 218, 255}
    };
    const Color color = hullColors[shipId < 8U ? shipId : 0U];
    const Color hullLight{
        static_cast<unsigned char>(std::min(255, static_cast<int>(color.r) + 34)),
        static_cast<unsigned char>(std::min(255, static_cast<int>(color.g) + 34)),
        static_cast<unsigned char>(std::min(255, static_cast<int>(color.b) + 34)), 255};
    const Color hullShade{
        static_cast<unsigned char>(color.r * 0.48F),
        static_cast<unsigned char>(color.g * 0.48F),
        static_cast<unsigned char>(color.b * 0.48F), 255};
    const float cp = std::cos(pitch), sp = std::sin(pitch);
    const float cy = std::cos(yaw), sy = std::sin(yaw);
    const float cr = std::cos(roll), sr = std::sin(roll);
    const auto v = [shipX, shipY, cp, sp, cy, sy, cr, sr, tunnelFrame](float x, float y, float z) {
        const float pitchedY = y * cp - z * sp;
        const float pitchedZ = y * sp + z * cp;
        const float yawedX = x * cy + pitchedZ * sy;
        const float yawedZ = -x * sy + pitchedZ * cy;
        const float rolledX = yawedX * cr - pitchedY * sr;
        const float rolledY = yawedX * sr + pitchedY * cr;
        const float localX = shipX + rolledX;
        const float localY = shipY + rolledY;
        if (tunnelFrame != nullptr) {
            return rayVector(tunrun::tunnelFramePoint(*tunnelFrame, localX, localY, yawedZ));
        }
        return Vector3{localX, localY, yawedZ};
    };
    const auto line = [color](Vector3 a, Vector3 b) { DrawLine3D(a, b, color); };
    // A shaded faceted fuselage under the distinct wireframe silhouette makes
    // each ship read as a solid 3D object instead of a flat HUD glyph.
    const auto nose = v(0.0F, 0.0F, -0.64F);
    const auto top = v(0.0F, 0.22F, 0.10F);
    const auto bottom = v(0.0F, -0.20F, 0.34F);
    const auto left = v(-0.30F, -0.01F, 0.28F);
    const auto right = v(0.30F, -0.01F, 0.28F);
    const auto tail = v(0.0F, 0.05F, 0.86F);
    DrawTriangle3D(nose, top, left, hullLight);
    DrawTriangle3D(nose, right, top, color);
    DrawTriangle3D(nose, bottom, right, hullShade);
    DrawTriangle3D(nose, left, bottom, color);
    DrawTriangle3D(top, tail, left, hullLight);
    DrawTriangle3D(top, right, tail, color);
    DrawTriangle3D(left, tail, bottom, hullShade);
    DrawTriangle3D(bottom, tail, right, hullShade);
    DrawSphere(v(-0.19F, -0.04F, 0.66F), 0.085F, Color{100, 224, 255, 255});
    DrawSphere(v(0.19F, -0.04F, 0.66F), 0.085F, Color{100, 224, 255, 255});
    switch (shipId) {
    case 0U: { // DRIFTWING: light delta wing.
        const auto nose = v(0.0F, 0.0F, -0.35F);
        const auto left = v(-0.78F, -0.30F, 0.62F);
        const auto right = v(0.78F, -0.30F, 0.62F);
        const auto tail = v(0.0F, 0.35F, 0.78F);
        line(nose, left); line(nose, right); line(left, tail);
        line(tail, right); line(left, right); line(nose, tail);
        break;
    }
    case 1U: { // WRAITH: long nose with swept rear fins.
        const auto nose = v(0.0F, 0.0F, -0.48F);
        const auto core = v(0.0F, 0.0F, 0.38F);
        const auto left = v(-0.43F, -0.05F, 0.55F);
        const auto right = v(0.43F, -0.05F, 0.55F);
        const auto leftTip = v(-0.82F, -0.28F, 0.82F);
        const auto rightTip = v(0.82F, -0.28F, 0.82F);
        const auto tail = v(0.0F, 0.26F, 0.94F);
        line(nose, core); line(core, left); line(core, right);
        line(left, leftTip); line(leftTip, tail); line(tail, rightTip);
        line(rightTip, right); line(leftTip, rightTip); line(core, tail);
        break;
    }
    case 2U: { // BULWARK: broad armored trapezoid.
        const auto nose = v(0.0F, 0.0F, -0.28F);
        const auto frontLeft = v(-0.50F, -0.28F, 0.18F);
        const auto frontRight = v(0.50F, -0.28F, 0.18F);
        const auto rearLeft = v(-0.80F, -0.10F, 0.70F);
        const auto rearRight = v(0.80F, -0.10F, 0.70F);
        const auto tail = v(0.0F, 0.30F, 0.90F);
        line(nose, frontLeft); line(frontLeft, rearLeft);
        line(rearLeft, tail); line(tail, rearRight);
        line(rearRight, frontRight); line(frontRight, nose);
        line(frontLeft, frontRight); line(rearLeft, rearRight);
        line(nose, tail); line(frontLeft, tail); line(frontRight, tail);
        break;
    }
    case 3U: { // MANTA: wide swept wings.
        const auto nose = v(0.0F, 0.0F, -0.40F);
        const auto leftTip = v(-1.00F, -0.02F, 0.35F);
        const auto rightTip = v(1.00F, -0.02F, 0.35F);
        const auto leftRear = v(-0.52F, -0.25F, 0.78F);
        const auto rightRear = v(0.52F, -0.25F, 0.78F);
        const auto tail = v(0.0F, 0.24F, 0.88F);
        line(nose, leftTip); line(leftTip, leftRear); line(leftRear, tail);
        line(tail, rightRear); line(rightRear, rightTip); line(rightTip, nose);
        line(leftTip, rightTip); line(nose, tail); line(leftRear, rightRear);
        break;
    }
    case 4U: { // COMET: long needle fuselage and twin exhaust rails.
        const auto nose = v(0.0F, 0.0F, -0.65F);
        const auto mid = v(0.0F, 0.0F, 0.28F);
        const auto left = v(-0.28F, -0.12F, 0.55F);
        const auto right = v(0.28F, -0.12F, 0.55F);
        const auto leftTail = v(-0.42F, 0.10F, 0.96F);
        const auto rightTail = v(0.42F, 0.10F, 0.96F);
        line(nose, mid); line(mid, left); line(mid, right);
        line(left, leftTail); line(right, rightTail);
        line(leftTail, rightTail); line(left, right);
        line(nose, leftTail); line(nose, rightTail);
        break;
    }
    case 5U: { // SPECTRE: split twin-prong silhouette.
        const auto leftNose = v(-0.22F, 0.02F, -0.45F);
        const auto rightNose = v(0.22F, 0.02F, -0.45F);
        const auto leftRear = v(-0.56F, -0.18F, 0.72F);
        const auto rightRear = v(0.56F, -0.18F, 0.72F);
        const auto center = v(0.0F, 0.20F, 0.78F);
        const auto leftWing = v(-0.86F, -0.24F, 0.40F);
        const auto rightWing = v(0.86F, -0.24F, 0.40F);
        line(leftNose, leftRear); line(leftRear, leftWing);
        line(leftWing, leftNose); line(rightNose, rightRear);
        line(rightRear, rightWing); line(rightWing, rightNose);
        line(leftRear, center); line(center, rightRear);
        line(leftNose, center); line(rightNose, center);
        break;
    }
    case 6U: { // VORTEX: interlocking diamond rails.
        const auto nose = v(0.0F, 0.0F, -0.42F);
        const auto left = v(-0.72F, 0.0F, 0.28F);
        const auto right = v(0.72F, 0.0F, 0.28F);
        const auto top = v(0.0F, 0.42F, 0.40F);
        const auto bottom = v(0.0F, -0.32F, 0.70F);
        const auto tail = v(0.0F, 0.12F, 0.92F);
        line(nose, left); line(left, bottom); line(bottom, right);
        line(right, nose); line(nose, top); line(top, right);
        line(right, tail); line(tail, left); line(left, top);
        line(top, bottom); line(bottom, tail); line(tail, nose);
        break;
    }
    case 7U: { // OBSIDIAN: angular heavy interceptor.
        const auto nose = v(0.0F, 0.0F, -0.34F);
        const auto leftFront = v(-0.45F, -0.22F, 0.05F);
        const auto rightFront = v(0.45F, -0.22F, 0.05F);
        const auto leftWing = v(-0.90F, -0.12F, 0.52F);
        const auto rightWing = v(0.90F, -0.12F, 0.52F);
        const auto leftRear = v(-0.50F, 0.16F, 0.83F);
        const auto rightRear = v(0.50F, 0.16F, 0.83F);
        const auto tail = v(0.0F, 0.35F, 0.97F);
        line(nose, leftFront); line(leftFront, leftWing);
        line(leftWing, leftRear); line(leftRear, tail);
        line(tail, rightRear); line(rightRear, rightWing);
        line(rightWing, rightFront); line(rightFront, nose);
        line(leftFront, rightFront); line(leftWing, rightWing);
        line(leftRear, rightRear); line(leftFront, tail); line(rightFront, tail);
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
    const Color color{255,103,91,255};
    const float phase=elapsedSeconds*(1.1F+hazard.frequency*0.4F)+
                      static_cast<float>(hazard.index)*0.73F;
    const Color blade{255,167,119,230};
    switch(hazard.index%4U) {
    case 0U:
        DrawSphere(center,hazard.radius*0.72F,Color{112,35,38,255});
        DrawSphereWires(center,hazard.radius,8,12,color);
        DrawLine3D(p(cx-hazard.radius,cy),p(cx+hazard.radius,cy),blade);
        DrawLine3D(p(cx,cy-hazard.radius),p(cx,cy+hazard.radius),blade);
        break;
    case 1U: {
        const Vector3 top=p(cx,cy+hazard.radius),right=p(cx+hazard.radius*0.78F,cy);
        const Vector3 bottom=p(cx,cy-hazard.radius),left=p(cx-hazard.radius*0.78F,cy);
        const Vector3 front=p(cx,cy,hazard.radius*0.55F),back=p(cx,cy,-hazard.radius*0.55F);
        const Color light{183,61,52,245},shade{79,27,36,255};
        DrawTriangle3D(top,right,front,light);DrawTriangle3D(right,bottom,front,color);
        DrawTriangle3D(bottom,left,front,shade);DrawTriangle3D(left,top,front,light);
        DrawTriangle3D(top,back,right,shade);DrawTriangle3D(right,back,bottom,light);
        DrawTriangle3D(bottom,back,left,color);DrawTriangle3D(left,back,top,shade);
        DrawLine3D(top,right,color);DrawLine3D(right,bottom,color);
        DrawLine3D(bottom,left,color);DrawLine3D(left,top,color);
        DrawLine3D(top,bottom,blade);DrawLine3D(left,right,blade);
        DrawSphereWires(center,hazard.radius*0.34F,6,8,color);
        break;
    }
    case 2U: {
        for(int i=0;i<3;++i) {
            const float a=phase+static_cast<float>(i)*2.0F*PI/3.0F;
            const float tx=cx+std::cos(a)*hazard.radius,ty=cy+std::sin(a)*hazard.radius;
            const Vector3 tip=p(tx,ty);
            const Vector3 shoulder=p(cx-std::cos(a)*hazard.radius*0.55F,
                cy-std::sin(a)*hazard.radius*0.55F,std::sin(phase)*hazard.radius*0.28F);
            const Vector3 back=p(tx,ty,-hazard.radius*0.22F);
            DrawTriangle3D(center,tip,shoulder,blade);
            DrawTriangle3D(center,shoulder,back,Color{142,43,39,255});
            DrawLine3D(center,tip,color);DrawLine3D(tip,shoulder,blade);DrawLine3D(shoulder,center,color);
        }
        DrawSphereWires(center,hazard.radius*0.24F,6,8,blade);
        break;
    }
    default: {
        const float cs=std::cos(phase),sn=std::sin(phase);
        const Vector3 a=p(cx+cs*hazard.radius,cy+sn*hazard.radius);
        const Vector3 b=p(cx-cs*hazard.radius,cy-sn*hazard.radius);
        const Vector3 c=p(cx-sn*hazard.radius,cy+cs*hazard.radius);
        const Vector3 d=p(cx+sn*hazard.radius,cy-cs*hazard.radius);
        const Vector3 front=p(cx,cy,hazard.radius*0.30F),back=p(cx,cy,-hazard.radius*0.30F);
        DrawTriangle3D(a,front,c,color);DrawTriangle3D(c,back,b,blade);
        DrawTriangle3D(b,front,d,Color{178,52,45,255});DrawTriangle3D(d,back,a,color);
        DrawSphere(center,hazard.radius*0.22F,Color{92,25,32,255});
        DrawLine3D(a,c,color);DrawLine3D(c,b,blade);DrawLine3D(b,d,color);DrawLine3D(d,a,color);
        DrawSphereWires(center,hazard.radius*0.42F,6,8,color);
        break;
    }
    }
}

void drawTunnel(std::uint64_t seed, float distance, float shipX, float shipY,
                std::uint32_t shipId, bool tpp, bool reduceMotion,
                float mouseAimX, float mouseAimY, bool showAimReticle,
                float pitch, float yaw, float roll,
                float boostEnergy, float dashCooldownRemaining,
                float dashRemaining, float elapsedSeconds,
                const tunrun::RunScore& score,
                std::uint64_t aetherPickedUp, std::uint64_t coresPickedUp) {
    const auto playerSection = tunrun::sampleCourse(seed, distance);
    const auto rearSection = tunrun::sampleCourse(seed, static_cast<double>(distance) - 6.0);
    const auto playerFrame = tunrun::sampleTunnelFrame(seed, distance, distance);
    const auto rearFrame = tunrun::sampleTunnelFrame(seed, distance, static_cast<double>(distance) - 6.0);
    const auto forwardFrame = tunrun::sampleTunnelFrame(seed, distance, static_cast<double>(distance) + 24.0);
    const auto cameraFrame = tunrun::sampleTunnelFrame(seed, distance, static_cast<double>(distance) - 1.25);
    const float forwardX = std::sin(yaw) * std::cos(pitch);
    const float forwardY = std::sin(pitch);
    const float forwardZ = -std::cos(yaw) * std::cos(pitch);
    Camera3D camera{};
    if (tpp) {
        const float rearCenterX = rearSection.centerX - playerSection.centerX;
        const float rearCenterY = rearSection.centerY - playerSection.centerY;
        const auto chasePose = tunrun::thirdPersonCameraPose(
            shipX, shipY, forwardX, forwardY, forwardZ,
            rearCenterX, rearCenterY, rearSection.radius);
        camera.position = rayVector(tunrun::tunnelFramePoint(
            rearFrame, chasePose.x - rearCenterX, chasePose.y - rearCenterY));
        camera.target = rayVector(forwardFrame.center);
    } else {
        camera.position = rayVector(tunrun::tunnelFramePoint(cameraFrame, shipX, shipY));
        camera.target = rayVector(forwardFrame.center);
    }
    camera.up = rayVector(tunrun::frameAdd(
        tunrun::frameScale(playerFrame.up, std::cos(roll)),
        tunrun::frameScale(playerFrame.right, -std::sin(roll))));
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
        const Color ringColor = ring % 4 == 0
            ? Color{164, 193, 225, 190} : Color{58, 78, 101, 140};
        const auto& ringFrame = tunnelFrames[static_cast<std::size_t>(ring - firstRing)];
        for (int side = 0; side < sideCount; ++side) {
            const int nextSide = (side + 1) % sideCount;
            const Vector3 a = tunnelPoint(ringFrame, side);
            const Vector3 b = tunnelPoint(ringFrame, nextSide);
            if (ring + 1 < lastRing) {
                const auto& nextFrame = tunnelFrames[static_cast<std::size_t>(ring + 1 - firstRing)];
                const Vector3 d = tunnelPoint(nextFrame, side);
                const Vector3 c = tunnelPoint(nextFrame, nextSide);
                const bool panelRidge = (side % 5 == 0) || (ring % 8 == 0);
                const Color panel = panelRidge
                    ? Color{29, 41, 56, 255} : Color{17, 23, 34, 255};
                // Fill the tunnel interior with actual 3D polygon faces, not
                // just projected grid lines. The brighter ribs stay visible.
                DrawTriangle3D(a, b, c, panel);
                DrawTriangle3D(a, c, d, panel);
                DrawLine3D(a, d, Color{48, 65, 83, 125});
            }
            DrawLine3D(a, b, ringColor);
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
            const float depth = -2.0F - travel;
            DrawLine3D(Vector3{radialX, radialY, depth},
                       Vector3{radialX * 0.92F, radialY * 0.92F, depth - 2.2F},
                       streakColor);
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
        drawProceduralGate(seed, distance,
                           tunrun::gateAt(seed, static_cast<std::uint32_t>(i)));
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
    if (tpp) {
        drawPlayerShip(shipId, shipX, shipY, pitch, yaw, roll, &playerFrame);
    }
    if (showAimReticle) {
        const Color aimColor{111, 225, 255, 235};
        constexpr float aimDepth = 1.9F;
        constexpr float halfWidth = 0.24F;
        constexpr float halfHeight = 0.24F;
        const auto aimPoint = [&](float x, float y) {
            return rayVector(tunrun::tunnelFramePoint(playerFrame, x, y, aimDepth));
        };
        const Vector3 aimCenter = aimPoint(mouseAimX, mouseAimY);
        DrawSphere(aimCenter, 0.045F, aimColor);
        DrawLine3D(aimPoint(mouseAimX - halfWidth, mouseAimY),
                   aimPoint(mouseAimX - 0.07F, mouseAimY), aimColor);
        DrawLine3D(aimPoint(mouseAimX + 0.07F, mouseAimY),
                   aimPoint(mouseAimX + halfWidth, mouseAimY), aimColor);
        DrawLine3D(aimPoint(mouseAimX, mouseAimY - halfHeight),
                   aimPoint(mouseAimX, mouseAimY - 0.07F), aimColor);
        DrawLine3D(aimPoint(mouseAimX, mouseAimY + 0.07F),
                   aimPoint(mouseAimX, mouseAimY + halfHeight), aimColor);
        DrawSphereWires(aimCenter, 0.15F, 6, 12, aimColor);
    }
    EndMode3D();
    DrawRectangle(22, 18, 344, 220, Color{10, 14, 21, 225});
    DrawRectangleLines(22, 18, 344, 220, kEdge);
    DrawText(TextFormat("TUNRUN / %s", tunrun::shipDefinition(shipId).name),
             35, 30, 15, kAccent);
    DrawText(tpp ? "CAMERA: TPP" : "CAMERA: FPP", 35, 52, 14, kText);
    DrawText(TextFormat("BOOST: %3.0f%%", boostEnergy), 35, 74, 13, kText);
    DrawRectangle(175, 78, 155, 8, Color{42, 51, 64, 255});
    DrawRectangle(175, 78, static_cast<int>(155.0F * boostEnergy / 100.0F), 8, kAccent);
    DrawText(TextFormat("NEXT GATE: %s", tunrun::gateKindName(nextGate.kind)), 35, 98, 12, kAccent);
    DrawText(TextFormat("SEED %016llX", static_cast<unsigned long long>(seed)), 35, 117, 11, kMuted);
    const auto hazardCue = tunrun::hazardHudCueAt(
        seed, static_cast<double>(distance), elapsedSeconds, shipX, shipY);
    if (hazardCue.valid) {
        DrawText(TextFormat("NEXT HAZARD: %.1f UNITS", hazardCue.distanceAhead),
                 35, 137, 10, Color{255, 153, 125, 255});
        const char* horizontalDirection = std::abs(hazardCue.offsetX) < 0.18F
            ? "CENTER" : hazardCue.offsetX < 0.0F ? "LEFT" : "RIGHT";
        const char* verticalDirection = std::abs(hazardCue.offsetY) < 0.18F
            ? "LEVEL" : hazardCue.offsetY > 0.0F ? "UP" : "DOWN";
        DrawText(TextFormat("MINE BEARING: %s %.1f / %s %.1f",
                 horizontalDirection, std::abs(hazardCue.offsetX),
                 verticalDirection, std::abs(hazardCue.offsetY)),
                 35, 157, 10, Color{255, 153, 125, 255});
    } else {
        DrawText("HAZARD CUE UNAVAILABLE", 35, 137, 10, kMuted);
    }
    DrawText(TextFormat("PICKUPS: +%llu AETHER / +%llu CORE",
             static_cast<unsigned long long>(aetherPickedUp),
             static_cast<unsigned long long>(coresPickedUp)), 35, 177, 10, kMuted);
    DrawText(TextFormat("SCORE %llu   COMBO x%.1f   CLEAN %llu",
             static_cast<unsigned long long>(score.total),
             1.0 + static_cast<double>(std::min<std::uint64_t>(score.combo, 40U)) / 10.0,
             static_cast<unsigned long long>(score.cleanPasses)), 35, 197, 10, kAccent);
    if (dashRemaining > 0.0F) {
        DrawText("DASH ACTIVE", 35, 217, 10, kAccent);
    } else if (dashCooldownRemaining > 0.0F) {
        DrawText(TextFormat("DASH RECHARGE: %.1fs", dashCooldownRemaining),
                 35, 217, 10, kMuted);
    } else if (boostEnergy >= tunrun::kDashEnergyCost) {
        DrawText("DASH READY: SPACE / PAD A", 35, 217, 10, kAccent);
    } else {
        DrawText(TextFormat("DASH NEEDS %.0f ENERGY", tunrun::kDashEnergyCost),
                 35, 217, 10, kMuted);
    }
    DrawRectangle(22, GetScreenHeight() - 48, GetScreenWidth() - 44, 26,
                  Color{10, 14, 21, 220});
    DrawText("WASD / MOUSE: STEER   ARROWS / RIGHT STICK: ROTATE   Q/E: ROLL   SPACE / A: DASH   SHIFT / RT: BOOST   V: CAMERA",
             36, GetScreenHeight() - 42, 11, kMuted);
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
        "PLAY / PROCEDURAL RUN", "HANGAR", "GAME MODES", "SEED LAB",
        "RECORDS / STATISTICS", "CONTROLS", "SETTINGS", "CREDITS", "EXIT"
    };
    const std::vector<std::string> modes{
        "CAMPAIGN (IN DEVELOPMENT)", "ENDLESS MODE (IN DEVELOPMENT)",
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
    const std::vector<std::string> exitItems{"CANCEL", "EXIT"};
    const std::vector<std::string> crashItems{
        "RETRY SAME SEED", "COPY SEED", "NEW SEED", "RETURN TO MAIN MENU"
    };
    const std::vector<std::string> seedLabItems{
        "ENTER CUSTOM SEED", "GENERATE NEW SEED", "START THIS SEED", "COPY SEED", "BACK"
    };
    const std::vector<std::string> seedEntryItems{"APPLY + START RUN", "CANCEL"};
    const std::vector<std::string> recoveryItems{"RESET PROFILE (PRESERVE DAMAGED FILES)", "EXIT WITHOUT RESET"};

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
                float steerX = (IsKeyDown(KEY_D) ? 1.0F : 0.0F) -
                               (IsKeyDown(KEY_A) ? 1.0F : 0.0F);
                float steerY = (IsKeyDown(KEY_W) ? 1.0F : 0.0F) -
                               (IsKeyDown(KEY_S) ? 1.0F : 0.0F);
                float rotateYaw = (IsKeyDown(KEY_RIGHT) ? 1.0F : 0.0F) -
                                  (IsKeyDown(KEY_LEFT) ? 1.0F : 0.0F);
                float rotatePitch = (IsKeyDown(KEY_UP) ? 1.0F : 0.0F) -
                                    (IsKeyDown(KEY_DOWN) ? 1.0F : 0.0F);
                float rollInput = (IsKeyDown(KEY_E) ? 1.0F : 0.0F) -
                                  (IsKeyDown(KEY_Q) ? 1.0F : 0.0F);
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
                    rotateYaw = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
                    rotatePitch = -GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
                    if (std::abs(rotateYaw) < 0.16F) rotateYaw = 0.0F;
                    if (std::abs(rotatePitch) < 0.16F) rotatePitch = 0.0F;
                }

                const bool manualFlightInput =
                    IsKeyDown(KEY_A) || IsKeyDown(KEY_D) ||
                    IsKeyDown(KEY_W) || IsKeyDown(KEY_S) ||
                    std::abs(padX) > 0.01F || std::abs(padY) > 0.01F ||
                    std::abs(rotateYaw) > 0.01F || std::abs(rotatePitch) > 0.01F ||
                    std::abs(rollInput) > 0.01F;
                if (manualFlightInput) {
                    app.mouseControlEngaged = false;
                    app.mouseAimX = app.flight.x;
                    app.mouseAimY = app.flight.y;
                }

                if (app.profile.mouseSteering) {
                    RelativeMouseDelta mouseDelta{};
                    if (rawMouse.installed()) {
                        mouseDelta = rawMouse.consume();
                    } else {
                        const Vector2 pointerDelta = GetMouseDelta();
                        mouseDelta = RelativeMouseDelta{pointerDelta.x, pointerDelta.y};
                    }
                    if (!manualFlightInput &&
                        (std::abs(mouseDelta.x) > 0.01F ||
                         std::abs(mouseDelta.y) > 0.01F)) {
                        applyRelativeMouseSteering(
                            app.mouseAimX, app.mouseAimY, mouseDelta,
                            app.profile.mouseSensitivity);
                        app.mouseControlEngaged = true;
                    }
                } else {
                    app.mouseControlEngaged = false;
                    app.mouseAimX = app.flight.x;
                    app.mouseAimY = app.flight.y;
                }

                if (app.profile.mouseSteering) {
                    const auto aimSection = tunrun::sampleCourse(
                        app.courseSeed, static_cast<double>(app.flight.distance));
                    const float safeAimRadius = std::max(
                        0.1F, aimSection.radius - tunrun::kCraftCollisionRadius - 0.08F);
                    (void)clampMouseTargetToRadius(
                        app.mouseAimX, app.mouseAimY, safeAimRadius);
                }

                // The target persists between raw-delta events and is tracked
                // with damping, so frame rate does not dictate steering strength.
                if (app.profile.mouseSteering && app.mouseControlEngaged &&
                    !manualFlightInput) {
                    steerX += mouseTargetSteering(
                        app.mouseAimX, app.flight.x, app.flight.velocityX);
                    steerY += mouseTargetSteering(
                        app.mouseAimY, app.flight.y, app.flight.velocityY);
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
                    rotateYaw, rotatePitch, rollInput
                };
                tunrun::advanceFlight(app.flight, flightInput, dt, app.flightAccumulator);
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
                       static_cast<std::uint32_t>(app.selectedShip), tpp, app.reduceMotion,
                       app.mouseAimX, app.mouseAimY,
                       app.profile.mouseSteering && app.mouseControlEngaged,
                       app.flight.pitch, app.flight.yaw, app.flight.roll,
                       app.flight.boostEnergy, app.flight.dashCooldownRemaining,
                       app.flight.dashRemaining, app.elapsed, app.runScore,
                       app.runAetherPickupReward, app.runSingularityCorePickupReward);
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
                case 0: resetFlight(app, tpp); chooseNextSeed(app); app.screens.push(tunrun::Screen::Preview); break;
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
            } else if (definition.aetherShardCost > 0U) {
                actionLabel = "UNLOCK FOR " + std::to_string(definition.aetherShardCost) + " AETHER SHARDS";
            } else {
                actionLabel = "UNLOCK FOR " + std::to_string(definition.singularityCoreCost) + " SINGULARITY CORES";
            }
            const Rectangle actionBounds{66.0F, 405.0F, 385.0F, 45.0F};
            const bool actionClicked = drawButton(actionBounds, actionLabel.c_str(), false);
            const bool actionConfirmed = confirmPressed();
            if (actionClicked || actionConfirmed) {
                activateHangarShip(app, static_cast<std::uint32_t>(previewShip));
            }

            if (drawButton(Rectangle{static_cast<float>(GetScreenWidth() - 365), 390, 48, 36}, "<", false))
                app.hangarPreviewShip = (app.hangarPreviewShip + 7) % 8;
            if (drawButton(Rectangle{static_cast<float>(GetScreenWidth() - 55), 390, 48, 36}, ">", false))
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
            if (drawMenu({"BACK"}, hangarSelection, GetScreenHeight() - 86,
                         !actionConfirmed) == 0) app.screens.pop();
            if (backPressed()) app.screens.pop();
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
                       "Keyboard, relative mouse and gamepad all control flight.");
            DrawText("KEYBOARD", 70, 184, 15, kAccent);
            DrawText("MOVE  W / A / S / D", 78, 218, 13, kText);
            DrawText("ROTATE  ARROW KEYS", 78, 246, 13, kText);
            DrawText("BARREL ROLL  Q / E", 78, 274, 13, kText);
            DrawText("BOOST  LEFT SHIFT", 78, 302, 13, kText);
            DrawText("PRECISION  LEFT CTRL", 78, 330, 13, kText);
            DrawText("DASH  SPACE  /  CAMERA  V", 78, 358, 13, kText);
            DrawText("MOUSE: RELATIVE X/Y STEERING", 78, 402, 12, kAccent);
            DrawText("PAUSE / BACK  ESC", 78, 426, 12, kMuted);
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
                case tunrun::GameModeChoice::Endless:
                    app.modeMessage = tunrun::modeUnavailableMessage(choice);
                    break;
                case tunrun::GameModeChoice::CustomSeedRun:
                    app.modeMessage.clear();
                    beginSeedEntry(app);
                    seedEntrySelection = 0;
                    app.screens.push(tunrun::Screen::SeedEntry);
                    break;
                case tunrun::GameModeChoice::PracticePreview:
                    app.modeMessage.clear();
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
                       "Adjust relative mouse steering, sensitivity, and display options.");
            auto labels = settingsItems;
            labels[0] = std::string("FULLSCREEN: ") + (app.fullscreen ? "ON" : "OFF");
            labels[1] = std::string("FPS COUNTER: ") + (app.showFps ? "ON" : "OFF");
            labels[2] = std::string("REDUCED MOTION: ") + (app.reduceMotion ? "ON" : "OFF");
            labels[3] = std::string("MOUSE FLIGHT: ") +
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
                app.mouseControlEngaged = false;
                app.mouseAimX = app.flight.x;
                app.mouseAimY = app.flight.y;
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
                app.mouseControlEngaged = false;
                app.mouseAimX = app.flight.x;
                app.mouseAimY = app.flight.y;
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
                        app.courseSeed=parsed.seed; resetFlight(app,tpp); app.screens.replace(tunrun::Screen::Preview);
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
            static tunrun::SimulatedRouteValidation cachedRouteState;
            if (!validationCacheValid || validationCacheSeed != app.courseSeed ||
                validationCacheShip != currentShipId) {
                cachedCourseValidation = tunrun::validateCourse(app.courseSeed, 360.0);
                cachedGateValidation = tunrun::validateObstacleSet(app.courseSeed, 32U);
                cachedHazardValidation = tunrun::validateHazardSet(app.courseSeed, 32U);
                cachedReachability = tunrun::validateGateReachability(
                    app.courseSeed, 32U, currentShipId);
                cachedRouteState = tunrun::validateSimulatedRouteReachability(
                    app.courseSeed, 32U, currentShipId);
                validationCacheSeed = app.courseSeed;
                validationCacheShip = currentShipId;
                validationCacheValid = true;
            }
            const auto& validation = cachedCourseValidation;
            const auto& gateValidation = cachedGateValidation;
            const auto& hazardValidation = cachedHazardValidation;
            const auto& reachability = cachedReachability;
            const auto& routeState = cachedRouteState;
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
            drawCentred(TextFormat("STATE ROUTE: %s   CLEARANCE %.2F   STEPS %u",
                                   routeState.valid ? "PASS" : "FAIL",
                                   routeState.minimumGateClearance,
                                   routeState.simulationSteps),
                        269.0F, 10, routeState.valid ? kAccent : kDanger);
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
                beginSeedEntry(app); seedEntrySelection=0; app.screens.push(tunrun::Screen::SeedEntry);
            } else if (picked == 1) chooseNextSeed(app);
            else if (picked == 2) { resetFlight(app,tpp); app.screens.push(tunrun::Screen::Preview); }
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
