// ely-arcade-sdk -- arcade_input_setup.cpp
// See include/arcade_input_setup.h.

#include "arcade_input_setup.h"
#include "arcade_input.h"

#include "raylib.h"

#include <cstdio>
#include <initializer_list>

namespace arcade
{
namespace
{

const Color kBg     = { 30, 30, 30, 255 };
const Color kCyan   = { 40, 220, 220, 255 };
const Color kYellow = { 255, 220, 40, 255 };
const Color kGreen  = { 50, 220, 80, 255 };
const Color kGrey   = { 180, 180, 180, 255 };
const Color kDark   = { 80, 80, 80, 255 };
const Color kRed    = { 220, 50, 50, 255 };

constexpr Player kPlayers[2] = { Player::One, Player::Two };

// RAII: ESC cancels the screen instead of closing the game.
struct ExitKeyGuard
{
    ExitKeyGuard()  { SetExitKey(KEY_NULL); }
    ~ExitKeyGuard() { SetExitKey(KEY_ESCAPE); }
};

void DrawCentered(const char* text, int y, int size, Color color)
{
    int w = MeasureText(text, size);
    DrawText(text, (GetScreenWidth() - w) / 2, y, size, color);
}

void DrawHeader(const char* title)
{
    ClearBackground(kBg);
    DrawCentered(title, 30, 36, kCyan);
    DrawCentered("Press ESC to cancel", 76, 20, kGrey);
    DrawLine(40, 110, GetScreenWidth() - 40, 110, kDark);
}

bool AnyButtonDown(Player player)
{
    for (int b = 0; b < kEncoderButtonCount; ++b)
        if (IsButtonDown(player, b)) return true;
    return false;
}

bool AnyEncoderInput(Player player)
{
    if (AnyButtonDown(player)) return true;
    for (Action a : { Action::Up, Action::Down, Action::Left, Action::Right })
        if (IsEncoderDirectionDown(player, a)) return true;
    return false;
}

// Returns false if the user cancelled (ESC / window closed).
bool Frame()
{
    return !(WindowShouldClose() || IsKeyPressed(KEY_ESCAPE));
}

// Waits (with a safety timeout) until nothing is held on either encoder.
void WaitForRelease()
{
    double deadline = GetTime() + 2.0;
    while (GetTime() < deadline && !WindowShouldClose())
    {
        UpdateInput();
        BeginDrawing();
        DrawHeader("Arcade Encoder Setup");
        DrawCentered("Release all buttons...", 200, 28, kGrey);
        EndDrawing();
        if (!AnyEncoderInput(Player::Any)) break;
    }
}

void ShowMessage(const char* title, const char* line1, const char* line2, Color color, double seconds)
{
    double end = GetTime() + seconds;
    while (GetTime() < end && Frame())
    {
        UpdateInput();
        BeginDrawing();
        DrawHeader(title);
        DrawCentered(line1, 180, 30, color);
        if (line2) DrawCentered(line2, 240, 22, kGrey);
        EndDrawing();
    }
}

void DrawEncoderList()
{
    char buf[256];
    for (int i = 0; i < 2; ++i)
    {
        bool on = IsEncoderConnected(kPlayers[i]);
        snprintf(buf, sizeof(buf), "Encoder %d: %s", i + 1, on ? EncoderName(kPlayers[i]) : "not connected");
        DrawCentered(buf, 330 + i * 28, 20, on ? kGrey : kRed);
    }
}

} // namespace

bool RunEncoderSetup()
{
    ExitKeyGuard guard;
    const char* title = "Arcade Encoder Setup";

    // Poll once, then allow the 1s hot-plug rescan to find late devices.
    UpdateInput();
    double wait = GetTime() + 1.2;
    while (EncoderCount() < 2 && GetTime() < wait && Frame()) UpdateInput();
    if (EncoderCount() < 2)
    {
        ShowMessage(title, "Two encoders are required", "Connect both USB encoders and try again.", kRed, 3.0);
        return false;
    }

    // --- Step 1: Player 1 (left) ---
    int firstSlot = -1;
    while (firstSlot < 0)
    {
        if (!Frame()) return false;
        UpdateInput();

        BeginDrawing();
        DrawHeader(title);
        DrawCentered("Press any button on the", 170, 30, kGrey);
        DrawCentered("PLAYER 1 (LEFT) controller", 210, 40, kYellow);
        DrawEncoderList();
        EndDrawing();

        for (int s = 0; s < 2 && firstSlot < 0; ++s)
            if (AnyButtonDown(kPlayers[s])) firstSlot = s;
    }

    // The encoder currently acting as Player Two answered -> flip the assignment.
    if (firstSlot == 1) SwapEncoders();
    WaitForRelease();

    // --- Step 2: Player 2 (right) ---
    bool second = false;
    while (!second)
    {
        if (!Frame())
        {
            if (firstSlot == 1) SwapEncoders();   // revert on cancel
            return false;
        }
        UpdateInput();

        BeginDrawing();
        DrawHeader(title);
        DrawCentered("Player 1 assigned", 150, 24, kYellow);
        DrawCentered("Press any button on the", 195, 30, kGrey);
        DrawCentered("PLAYER 2 (RIGHT) controller", 235, 40, kGreen);
        DrawEncoderList();
        EndDrawing();

        second = AnyButtonDown(Player::Two);
    }
    WaitForRelease();

    bool saved = SaveEncoderAssignment();
    ShowMessage(title, saved ? "Players assigned and saved!" : "Assigned, but saving failed",
                saved ? "Player 1 = left, Player 2 = right." : "The assignment will reset on restart.",
                saved ? kGreen : kRed, 2.0);
    return saved;
}

bool RunEncoderTest()
{
    ExitKeyGuard guard;
    const char* title = "Arcade Encoder Input Test";

    UpdateInput();
    double wait = GetTime() + 1.2;
    while (EncoderCount() == 0 && GetTime() < wait && Frame()) UpdateInput();
    if (EncoderCount() == 0)
    {
        ShowMessage(title, "No encoders found", "Connect your USB encoders and try again.", kRed, 3.0);
        return false;
    }

    const char*  dirNames[4] = { "UP", "DOWN", "LEFT", "RIGHT" };
    const Action dirs[4]     = { Action::Up, Action::Down, Action::Left, Action::Right };
    constexpr int kSteps     = 4 + kEncoderButtonCount;

    for (int p = 0; p < 2; ++p)
    {
        Player player = kPlayers[p];
        if (!IsEncoderConnected(player)) continue;

        int  step = 0;
        bool waitingRelease = false;

        while (step < kSteps)
        {
            if (!Frame()) return false;
            UpdateInput();

            if (!waitingRelease)
            {
                bool pass = (step < 4) ? IsEncoderDirectionDown(player, dirs[step])
                                       : IsButtonDown(player, step - 4);
                if (pass) waitingRelease = true;
            }
            else if (!AnyEncoderInput(player))
            {
                waitingRelease = false;
                ++step;
                if (step >= kSteps) break;
            }

            BeginDrawing();
            DrawHeader(title);

            char buf[128];
            snprintf(buf, sizeof(buf), "PLAYER %d  (%s)", p + 1, p == 0 ? "left" : "right");
            DrawCentered(buf, 125, 28, p == 0 ? kYellow : kGreen);

            if (step < 4) snprintf(buf, sizeof(buf), "Push joystick %s", dirNames[step]);
            else          snprintf(buf, sizeof(buf), "Press Button %d", step - 4 + 1);
            DrawCentered(buf, 170, 36, waitingRelease ? kGreen : WHITE);

            // Joystick indicator
            int cx = GetScreenWidth() / 2, cy = 270;
            DrawCircleLines(cx, cy, 50, kGrey);
            if (IsEncoderDirectionDown(player, Action::Up))    DrawCircle(cx, cy - 36, 12, kGreen);
            if (IsEncoderDirectionDown(player, Action::Down))  DrawCircle(cx, cy + 36, 12, kGreen);
            if (IsEncoderDirectionDown(player, Action::Left))  DrawCircle(cx - 36, cy, 12, kGreen);
            if (IsEncoderDirectionDown(player, Action::Right)) DrawCircle(cx + 36, cy, 12, kGreen);

            // Button grid (4 x 2)
            int gridW = 4 * 64 + 3 * 16, gx = (GetScreenWidth() - gridW) / 2, gy = 350;
            for (int b = 0; b < kEncoderButtonCount; ++b)
            {
                int  x      = gx + (b % 4) * 80, y = gy + (b / 4) * 80;
                bool done   = (step > 3) && (b < step - 4);
                bool target = (step > 3) && (b == step - 4);
                Color fill  = IsButtonDown(player, b) ? kGreen : (done ? Color{ 30, 110, 50, 255 } : kDark);
                DrawRectangle(x, y, 64, 64, fill);
                DrawRectangleLines(x, y, 64, 64, target ? kYellow : kGrey);
                snprintf(buf, sizeof(buf), "%d", b + 1);
                DrawText(buf, x + 26, y + 22, 24, WHITE);
            }
            EndDrawing();
        }
    }

    ShowMessage(title, "All inputs confirmed!", "Encoders are working correctly.", kGreen, 2.0);
    return true;
}

} // namespace arcade

