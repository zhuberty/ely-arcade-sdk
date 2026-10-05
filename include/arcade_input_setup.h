// ely-arcade-sdk -- arcade_input_setup.h
//
// Interactive screens for the arcade encoders, drawn with raylib in the
// game's existing window (call them after InitWindow):
//
//   RunEncoderSetup()  Player-assignment wizard. Player 1 (left) presses any
//                      button on their controller, then Player 2 (right).
//                      The result is applied immediately and saved with
//                      arcade::SaveEncoderAssignment(), so it persists.
//   RunEncoderTest()   Guided hardware test: for each connected encoder, push
//                      the joystick up/down/left/right and press buttons 1-8.
//
// Both block, running their own frame loop, and return when finished or
// cancelled (ESC / window closed). They temporarily disable raylib's exit key
// so ESC cancels the screen instead of closing the game, then restore it to
// KEY_ESCAPE. They call arcade::UpdateInput() themselves.
//
// Example (hotkeys in a game's main loop):
//
//     if (IsKeyPressed(KEY_F2)) arcade::RunEncoderSetup();
//     if (IsKeyPressed(KEY_F3)) arcade::RunEncoderTest();

#pragma once

namespace arcade
{

// Returns true if both players were assigned and the assignment was saved.
// Needs two connected encoders; otherwise shows a message and returns false.
bool RunEncoderSetup();

// Returns true if every input on every connected encoder was confirmed.
bool RunEncoderTest();

} // namespace arcade
