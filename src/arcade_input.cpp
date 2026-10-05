// ely-arcade-sdk -- arcade_input.cpp
// See include/arcade_input.h for the binding table.

#include "arcade_input.h"

#include "raylib.h"

// Raw joystick access (raylib's gamepad mapping hides unmapped encoders).
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>

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

// ---------------------------------------------------------------------------
// Arcade encoders (Reyann Easyget Zero Delay / DragonRise USB encoders)
// ---------------------------------------------------------------------------

constexpr float  kAxisThreshold = 0.5f;   // +-0.5 on axes 0 (X) and 1 (Y)
constexpr double kRescanSeconds = 1.0;    // hot-plug detection interval

// Lower-case substrings of the device name that identify an encoder.
const char* const kEncoderNameHints[] =
{
    "usb gamepad", "zero delay", "easyget", "arcade", "dragonrise", "generic usb joystick",
};

// Encoder button used for each action, -1 = none (joystick axes). Order matches Action.
const int kEncoderActionButton[kActionCount] =
{
    /* Up      */ -1,
    /* Down    */ -1,
    /* Left    */ -1,
    /* Right   */ -1,
    /* Confirm */  1,   // Button 2
    /* Back    */  7,   // Button 8
    /* Restart */  0,   // Button 1
};

struct EncoderState
{
    bool connected = false;
    bool actionDown[kActionCount]           = {};
    bool actionPressed[kActionCount]        = {};
    bool buttonDown[kEncoderButtonCount]    = {};
    bool buttonPressed[kEncoderButtonCount] = {};
};

EncoderState g_encoder[kPlayerSlots];
int          g_autoJid[kPlayerSlots]   = { -1, -1 };   // result of the name scan
int          g_manualJid[kPlayerSlots] = { -1, -1 };   // AssignEncoder() overrides
bool         g_swapped                 = false;
bool         g_scanned                 = false;
double       g_lastScan                = 0.0;
std::string  g_name[kPlayerSlots];
bool         g_configLoaded            = false;        // saved assignment applied (or overridden)

// Saved player assignment. The two encoders have identical USB IDs, so the
// only thing we can persist portably is whether detection order is swapped.
std::string ConfigPath()
{
    std::string dir;
    const char* appData = std::getenv("APPDATA");
    if (appData && *appData) dir = appData;
    else
    {
        const char* home = std::getenv("HOME");
        dir = home ? (std::string(home) + "/.config") : ".";
    }
    return dir + "/ely-arcade/encoder_player_map.cfg";
}

bool LoadAssignment(bool& swapped)
{
    std::ifstream in(ConfigPath());
    if (!in) return false;
    std::string line;
    while (std::getline(in, line))
    {
        if (line.compare(0, 8, "swapped=") == 0)
        {
            swapped = (line.size() > 8 && line[8] == '1');
            return true;
        }
    }
    return false;
}

bool SaveAssignment(bool swapped)
{
    std::error_code ec;
    std::filesystem::path path(ConfigPath());
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;
    out << "swapped=" << (swapped ? 1 : 0) << "\n";
    return static_cast<bool>(out);
}

bool IsEncoderName(const char* name)
{
    if (!name) return false;
    // Lower-case and collapse whitespace: Windows reports "Generic   USB  Joystick  ".
    std::string lower;
    for (const char* p = name; *p; ++p)
    {
        unsigned char c = static_cast<unsigned char>(*p);
        if (std::isspace(c))
        {
            if (!lower.empty() && lower.back() != ' ') lower.push_back(' ');
        }
        else lower.push_back(static_cast<char>(std::tolower(c)));
    }
    while (!lower.empty() && lower.back() == ' ') lower.pop_back();

    for (const char* hint : kEncoderNameHints)
        if (lower.find(hint) != std::string::npos) return true;
    return false;
}

void ScanEncoders()
{
    g_autoJid[0] = g_autoJid[1] = -1;
    int next = 0;

    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid)
    {
        if (jid == g_manualJid[0] || jid == g_manualJid[1]) continue;
        if (glfwJoystickPresent(jid) != GLFW_TRUE) continue;
        if (!IsEncoderName(glfwGetJoystickName(jid))) continue;

        while (next < kPlayerSlots && g_manualJid[next] >= 0) ++next;
        if (next >= kPlayerSlots) break;
        g_autoJid[next++] = jid;
    }

    g_scanned  = true;
    g_lastScan = GetTime();
}

int ResolveJid(int slot)
{
    int s = g_swapped ? (slot ^ 1) : slot;
    return (g_manualJid[s] >= 0) ? g_manualJid[s] : g_autoJid[s];
}

void PollSlot(int slot)
{
    EncoderState& e = g_encoder[slot];

    bool prevActionDown[kActionCount];
    bool prevButtonDown[kEncoderButtonCount];
    std::memcpy(prevActionDown, e.actionDown, sizeof(prevActionDown));
    std::memcpy(prevButtonDown, e.buttonDown, sizeof(prevButtonDown));

    std::memset(e.actionDown, 0, sizeof(e.actionDown));
    std::memset(e.buttonDown, 0, sizeof(e.buttonDown));

    int jid = ResolveJid(slot);
    e.connected = (jid >= 0 && glfwJoystickPresent(jid) == GLFW_TRUE);
    g_name[slot].clear();

    if (e.connected)
    {
        const char* name = glfwGetJoystickName(jid);
        if (name) g_name[slot] = name;

        int axisCount = 0, buttonCount = 0;
        const float* axes = glfwGetJoystickAxes(jid, &axisCount);
        const unsigned char* buttons = glfwGetJoystickButtons(jid, &buttonCount);

        if (buttons)
        {
            int n = buttonCount < kEncoderButtonCount ? buttonCount : kEncoderButtonCount;
            for (int i = 0; i < n; ++i) e.buttonDown[i] = (buttons[i] == GLFW_PRESS);
        }

        // Y axis first so diagonals prefer up/down.
        if (axes && axisCount >= 2)
        {
            float ax = axes[0], ay = axes[1];
            if      (ay < -kAxisThreshold) e.actionDown[static_cast<int>(Action::Up)]    = true;
            else if (ay >  kAxisThreshold) e.actionDown[static_cast<int>(Action::Down)]  = true;
            else if (ax < -kAxisThreshold) e.actionDown[static_cast<int>(Action::Left)]  = true;
            else if (ax >  kAxisThreshold) e.actionDown[static_cast<int>(Action::Right)] = true;
        }

        for (int a = 0; a < kActionCount; ++a)
        {
            int b = kEncoderActionButton[a];
            if (b >= 0 && e.buttonDown[b]) e.actionDown[a] = true;
        }
    }

    for (int a = 0; a < kActionCount; ++a)
        e.actionPressed[a] = e.actionDown[a] && !prevActionDown[a];
    for (int b = 0; b < kEncoderButtonCount; ++b)
        e.buttonPressed[b] = e.buttonDown[b] && !prevButtonDown[b];
}

bool CheckSlot(int slot, Action action, bool pressedOnly)
{
    const Binding& b = kBindings[slot][static_cast<int>(action)];

    for (int i = 0; i < kMaxKeys; ++i)
    {
        int key = b.keys[i];
        if (key == KEY_NULL) continue;
        if (pressedOnly ? IsKeyPressed(key) : IsKeyDown(key)) return true;
    }

    // Gamepad index matches the player slot (0 = One, 1 = Two). Only devices with
    // a standard mapping count; the arcade encoders are unmapped and handled below.
    if (IsGamepadAvailable(slot) && glfwJoystickIsGamepad(slot) == GLFW_TRUE)
    {
        if (pressedOnly ? IsGamepadButtonPressed(slot, b.padButton)
                        : IsGamepadButtonDown(slot, b.padButton))
            return true;
    }

    // Arcade encoder.
    const EncoderState& e = g_encoder[slot];
    if (pressedOnly ? e.actionPressed[static_cast<int>(action)]
                    : e.actionDown[static_cast<int>(action)])
        return true;

    return false;
}

bool Check(Player player, Action action, bool pressedOnly)
{
    if (static_cast<int>(action) < 0 || static_cast<int>(action) >= kActionCount) return false;
    switch (player)
    {
        case Player::One: return CheckSlot(0, action, pressedOnly);
        case Player::Two: return CheckSlot(1, action, pressedOnly);
        case Player::Any: return CheckSlot(0, action, pressedOnly) ||
                                 CheckSlot(1, action, pressedOnly);
    }
    return false;
}

bool CheckButtonSlot(int slot, int button, bool pressedOnly)
{
    if (button < 0 || button >= kEncoderButtonCount) return false;
    const EncoderState& e = g_encoder[slot];
    return pressedOnly ? e.buttonPressed[button] : e.buttonDown[button];
}

bool CheckButton(Player player, int button, bool pressedOnly)
{
    switch (player)
    {
        case Player::One: return CheckButtonSlot(0, button, pressedOnly);
        case Player::Two: return CheckButtonSlot(1, button, pressedOnly);
        case Player::Any: return CheckButtonSlot(0, button, pressedOnly) ||
                                 CheckButtonSlot(1, button, pressedOnly);
    }
    return false;
}

} // namespace

void UpdateInput()
{
    if (!g_configLoaded)
    {
        g_configLoaded = true;
        bool swapped = false;
        if (LoadAssignment(swapped)) g_swapped = swapped;
    }

    if (!g_scanned || (GetTime() - g_lastScan) >= kRescanSeconds)
        ScanEncoders();

    for (int slot = 0; slot < kPlayerSlots; ++slot)
        PollSlot(slot);
}

bool IsActionPressed(Player player, Action action) { return Check(player, action, true);  }
bool IsActionDown(Player player, Action action)    { return Check(player, action, false); }

bool IsButtonPressed(Player player, int button)    { return CheckButton(player, button, true);  }
bool IsButtonDown(Player player, int button)       { return CheckButton(player, button, false); }

int EncoderCount()
{
    int count = 0;
    for (int slot = 0; slot < kPlayerSlots; ++slot)
        if (g_encoder[slot].connected) ++count;
    return count;
}

bool IsEncoderConnected(Player player)
{
    switch (player)
    {
        case Player::One: return g_encoder[0].connected;
        case Player::Two: return g_encoder[1].connected;
        case Player::Any: return g_encoder[0].connected || g_encoder[1].connected;
    }
    return false;
}

const char* EncoderName(Player player)
{
    switch (player)
    {
        case Player::One: return g_name[0].c_str();
        case Player::Two: return g_name[1].c_str();
        case Player::Any: return g_encoder[0].connected ? g_name[0].c_str() : g_name[1].c_str();
    }
    return "";
}

bool SwapEncoders()
{
    g_configLoaded = true;   // an explicit call wins over the saved assignment
    g_swapped = !g_swapped;
    return g_swapped;
}

bool IsEncoderSwapped() { return g_swapped; }

bool SaveEncoderAssignment() { return SaveAssignment(g_swapped); }

bool IsEncoderDirectionDown(Player player, Action direction)
{
    int a = static_cast<int>(direction);
    if (a < static_cast<int>(Action::Up) || a > static_cast<int>(Action::Right)) return false;
    switch (player)
    {
        case Player::One: return g_encoder[0].actionDown[a];
        case Player::Two: return g_encoder[1].actionDown[a];
        case Player::Any: return g_encoder[0].actionDown[a] || g_encoder[1].actionDown[a];
    }
    return false;
}

void AssignEncoder(Player player, int joystickId)
{
    if (player == Player::Any) return;
    if (joystickId < 0 || joystickId > GLFW_JOYSTICK_LAST) joystickId = -1;

    g_manualJid[player == Player::One ? 0 : 1] = joystickId;
    g_scanned = false;   // re-scan on next UpdateInput()
}

} // namespace arcade
