// ely-arcade-sdk -- arcade_input.h
//
// Device-independent input for arcade games and the arcade menu.
//
// Games ask for logical *actions* (Up, Confirm, Back, ...) for a given
// *player* instead of polling raw keys, so the same game works with the
// keyboard during development and with the cabinet's joysticks/buttons.
//
// Quick start:
//
//     #include "arcade_input.h"
//
//     while (!WindowShouldClose())
//     {
//         arcade::UpdateInput();                         // once per frame, first thing
//
//         if (arcade::IsActionDown(arcade::Player::One, arcade::Action::Up))   { ... }
//         if (arcade::IsActionPressed(arcade::Player::Any, arcade::Action::Confirm)) { ... }
//         if (arcade::IsButtonPressed(arcade::Player::Two, 3)) { ... }  // raw button 4
//     }
//
// Input sources (all are OR'd together, per player):
//
//   1. Arcade encoders (the cabinet hardware)
//        Reyann Easyget "Zero Delay" USB encoders (DragonRise chipset), one per
//        player. Each has a 4-way joystick on axes 0/1 and 8 buttons (0-7).
//        Joystick: X axis = Left/Right, Y axis = Up/Down, +-0.5 threshold,
//                  Up/Down wins on diagonals.
//        Buttons:  Button 1 (index 0) -> Restart
//                  Button 2 (index 1) -> Confirm
//                  Button 8 (index 7) -> Back
//                  All eight are also available raw via IsButtonPressed/Down.
//        Encoders are auto-detected by device name; the first one found is
//        Player::One and the second is Player::Two. See "Player assignment".
//   2. Keyboard
//        Player::One  W A S D, Space (confirm), Left Ctrl (back), R (restart)
//        Player::Two  arrow keys, Enter / Keypad Enter (confirm),
//                     Right Ctrl (back), R (restart)
//   3. Standard (mapped) gamepads, raylib gamepad 0 = One, 1 = Two
//        d-pad, A (confirm), B (back), X (restart)
//
//   Player::Any  true when either player triggers the action. Use it for
//                menus and single-player modes.
//
// Player assignment:
//   The two encoders share an identical USB ID, so the OS cannot tell them
//   apart. By default they are assigned in detection order. If the players
//   are the wrong way around, call SwapEncoders() (e.g. behind a config flag),
//   or pin a specific device with AssignEncoder(). Use EncoderCount(),
//   EncoderName() and IsEncoderConnected() for diagnostics / on-screen hints.
//
// Requirements / limitations:
//   - Call UpdateInput() exactly once per frame (after InitWindow). It is what
//     makes "pressed" edge detection work for encoder buttons. Keyboard and
//     gamepad input do not depend on it.
//   - Encoder access uses raylib's GLFW backend (the SDK default).
//
// Exiting a game with ESC is still handled by raylib (SetExitKey).

#pragma once

namespace arcade
{

enum class Player
{
    One,
    Two,
    Any
};

enum class Action
{
    Up,
    Down,
    Left,
    Right,
    Confirm,
    Back,
    Restart,
    Count
};

// Number of buttons on each arcade encoder (valid raw indices are 0..7).
constexpr int kEncoderButtonCount = 8;

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

// Polls the arcade encoders and computes press edges. Call once per frame,
// before any IsAction*/IsButton* query. Also re-scans for (un)plugged encoders
// about once per second.
void UpdateInput();

// ---------------------------------------------------------------------------
// Actions (keyboard + encoders + gamepads)
// ---------------------------------------------------------------------------

// True only on the frame the action was triggered.
bool IsActionPressed(Player player, Action action);

// True while the action is held.
bool IsActionDown(Player player, Action action);

// ---------------------------------------------------------------------------
// Raw encoder buttons (index 0..7 = physical Button 1..8)
// ---------------------------------------------------------------------------

// True only on the frame the encoder button went down. False for bad indices.
bool IsButtonPressed(Player player, int button);

// True while the encoder button is held. False for bad indices.
bool IsButtonDown(Player player, int button);

// ---------------------------------------------------------------------------
// Encoder management / diagnostics
// ---------------------------------------------------------------------------

// Number of arcade encoders currently detected (0, 1 or 2 are used).
int EncoderCount();

// True if an encoder is currently assigned to the player (Any = either).
bool IsEncoderConnected(Player player);

// Device name of the player's encoder, or "" if none. Player::Any returns the
// first connected encoder's name.
const char* EncoderName(Player player);

// Swap which physical encoder is Player::One and which is Player::Two.
// Calling it again swaps back. Returns the new state (true = swapped).
bool SwapEncoders();

// True if Player One/Two are currently swapped relative to detection order.
bool IsEncoderSwapped();

// Persist the current swap state so it is restored on the next launch
// (loaded automatically by the first UpdateInput()). Returns false on I/O error.
// File: %APPDATA%/ely-arcade/encoder_player_map.cfg (or ~/.config/ely-arcade/...).
// See arcade_input_setup.h for the interactive wizard that calls this.
bool SaveEncoderAssignment();

// True while the player's *encoder* joystick is held in that direction
// (Up/Down/Left/Right only). Ignores keyboard and gamepads; used by the
// hardware test screen. Player::Any = either encoder.
bool IsEncoderDirectionDown(Player player, Action direction);

// Pin a specific joystick id (0..15, as seen by GLFW/raylib) to a player.
// Pass -1 to return that player to automatic assignment. Player::Any is ignored.
void AssignEncoder(Player player, int joystickId);

} // namespace arcade
