#pragma once
#include "ArcEngine/Core/Math.h"
#include <string>

struct GLFWwindow;

namespace Arc {

// Polling input (M3b) — keyboard + mouse via GLFW.
// Later (M5+): action mapping ("Jump" -> Space), gamepad, Android touch.
// Usage: Input::Init(window.NativeHandle()); Input::IsKeyDown(Key::W);
enum class Key {
    Unknown = -1,
    Space = 32, A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Escape = 256, Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up, PageUp, PageDown, Home, End,
    F1 = 290, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    LeftShift = 340, LeftControl, LeftAlt, RightShift, RightControl, RightAlt,
};

enum class MouseButton { Left = 0, Right = 1, Middle = 2 };

class Input {
public:
    static void Init(GLFWwindow* window);
    static void Shutdown();

    static bool IsKeyDown(Key key);
    static bool IsKeyUp(Key key);
    // Pressed = went down THIS frame (needs EndFrame each loop).
    static bool IsKeyPressed(Key key);

    static bool IsMouseDown(MouseButton btn);
    static bool IsMousePressed(MouseButton btn);
    static void MousePos(double& x, double& y);
    static Vec2 MousePos();

    // Call at END of frame after SwapBuffers+PollEvents so Pressed works.
    static void EndFrame();

private:
    static GLFWwindow* s_window;
    static bool s_keysNow[512];
    static bool s_keysPrev[512];
    static bool s_mouseNow[8];
    static bool s_mousePrev[8];

    static int ToGlfw(Key k) { return static_cast<int>(k); }
};

} // namespace Arc
