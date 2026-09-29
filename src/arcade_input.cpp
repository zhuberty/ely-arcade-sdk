// ely-arcade-sdk -- arcade_input.cpp
// See include/arcade_input.h for the binding table.

#include "arcade_input.h"

#include "raylib.h"

namespace arcade
{
namespace
{

constexpr int kMaxKeys      = 3;
constexpr int kActionCount  = static_cast<int>(Action::Count);
constexpr int kPlayerSlots  = 2;   // One, Two

struct Binding
{
    int keys[kMaxKeys];   // KEY_NULL (0) marks an unused slot
    int padButton;        // raylib GamepadButton
};

// Indexed [player slot][action]; order matches enum class Action.
const Binding kBindings[kPlayerSlots][kActionCount] =
{
    // ---- Player One: WASD + gamepad 0 ----
    {
        /* Up      */ { { KEY_W, KEY_NULL, KEY_NULL },              GAMEPAD_BUTTON_LEFT_FACE_UP    },
        /* Down    */ { { KEY_S, KEY_NULL, KEY_NULL },              GAMEPAD_BUTTON_LEFT_FACE_DOWN  },
        /* Left    */ { { KEY_A, KEY_NULL, KEY_NULL },              GAMEPAD_BUTTON_LEFT_FACE_LEFT  },
        /* Right   */ { { KEY_D, KEY_NULL, KEY_NULL },              GAMEPAD_BUTTON_LEFT_FACE_RIGHT },
        /* Confirm */ { { KEY_SPACE, KEY_NULL, KEY_NULL },          GAMEPAD_BUTTON_RIGHT_FACE_DOWN },
        /* Back    */ { { KEY_LEFT_CONTROL, KEY_NULL, KEY_NULL },   GAMEPAD_BUTTON_RIGHT_FACE_RIGHT },
        /* Restart */ { { KEY_R, KEY_NULL, KEY_NULL },              GAMEPAD_BUTTON_RIGHT_FACE_LEFT },
    },
    // ---- Player Two: arrow keys + gamepad 1 ----
    {
        /* Up      */ { { KEY_UP, KEY_NULL, KEY_NULL },             GAMEPAD_BUTTON_LEFT_FACE_UP    },
        /* Down    */ { { KEY_DOWN, KEY_NULL, KEY_NULL },           GAMEPAD_BUTTON_LEFT_FACE_DOWN  },
        /* Left    */ { { KEY_LEFT, KEY_NULL, KEY_NULL },           GAMEPAD_BUTTON_LEFT_FACE_LEFT  },
        /* Right   */ { { KEY_RIGHT, KEY_NULL, KEY_NULL },          GAMEPAD_BUTTON_LEFT_FACE_RIGHT },
        /* Confirm */ { { KEY_ENTER, KEY_KP_ENTER, KEY_NULL },      GAMEPAD_BUTTON_RIGHT_FACE_DOWN },
        /* Back    */ { { KEY_RIGHT_CONTROL, KEY_NULL, KEY_NULL },  GAMEPAD_BUTTON_RIGHT_FACE_RIGHT },
        /* Restart */ { { KEY_R, KEY_NULL, KEY_NULL },              GAMEPAD_BUTTON_RIGHT_FACE_LEFT },
    },
};

bool CheckSlot(int slot, Action action, bool pressedOnly)
{
    const Binding& b = kBindings[slot][static_cast<int>(action)];

    for (int i = 0; i < kMaxKeys; ++i)
    {
        int key = b.keys[i];
        if (key == KEY_NULL) continue;
        if (pressedOnly ? IsKeyPressed(key) : IsKeyDown(key)) return true;
    }

    // Gamepad index matches the player slot (0 = One, 1 = Two).
    if (IsGamepadAvailable(slot))
    {
        if (pressedOnly ? IsGamepadButtonPressed(slot, b.padButton)
                        : IsGamepadButtonDown(slot, b.padButton))
            return true;
    }
    return false;
}

bool Check(Player player, Action action, bool pressedOnly)
{
    switch (player)
    {
        case Player::One: return CheckSlot(0, action, pressedOnly);
        case Player::Two: return CheckSlot(1, action, pressedOnly);
        case Player::Any: return CheckSlot(0, action, pressedOnly) ||
                                 CheckSlot(1, action, pressedOnly);
    }
    return false;
}

} // namespace

bool IsActionPressed(Player player, Action action) { return Check(player, action, true);  }
bool IsActionDown(Player player, Action action)    { return Check(player, action, false); }

} // namespace arcade
