// ely-arcade-sdk -- arcade_input.h
//
// Device-independent input for arcade games and the arcade menu.
//
// Games ask for logical *actions* (Up, Confirm, Back, ...) for a given
// *player* instead of polling raw keys, so the same game works with the
// keyboard during development and with the cabinet's joysticks/buttons.
//
// Bindings:
//   Player::One  keyboard: W A S D, Space (confirm), Left Ctrl (back), R (restart)
//                gamepad 0: d-pad, A (confirm), B (back), X (restart)
//   Player::Two  keyboard: arrow keys, Enter / Keypad Enter (confirm),
//                Right Ctrl (back), R (restart)
//                gamepad 1: d-pad, A (confirm), B (back), X (restart)
//   Player::Any  true when either player triggers the action. Use it for
//                menus and single-player modes.
//
// NOTE: Arcade joystick encoders that present themselves as a keyboard work
// through the keyboard bindings above (map the encoder's outputs to those keys).
//
// Exiting a game with ESC is still handled by raylib (SetExitKey).

#pragma once

namespace ely
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

// True only on the frame the action was triggered.
bool IsActionPressed(Player player, Action action);

// True while the action is held.
bool IsActionDown(Player player, Action action);

} // namespace ely
