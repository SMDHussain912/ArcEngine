#include "ArcEngine/Core/Input.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstring>

namespace Arc {

GLFWwindow* Input::s_window = nullptr;
bool Input::s_keysNow[512] = {};
bool Input::s_keysPrev[512] = {};
bool Input::s_mouseNow[8] = {};
bool Input::s_mousePrev[8] = {};

void Input::Init(GLFWwindow* window) {
    s_window = window;
    std::memset(s_keysNow, 0, sizeof(s_keysNow));
    std::memset(s_keysPrev, 0, sizeof(s_keysPrev));
    std::memset(s_mouseNow, 0, sizeof(s_mouseNow));
    std::memset(s_mousePrev, 0, sizeof(s_mousePrev));
}

void Input::Shutdown() {
    s_window = nullptr;
}

bool Input::IsKeyDown(Key key) {
    if (!s_window) return false;
    int code = ToGlfw(key);
    if (code < 0 || code >= 512) return false;
    // Poll live so it works even before first EndFrame.
    int state = glfwGetKey(s_window, code);
    s_keysNow[code] = (state == GLFW_PRESS || state == GLFW_REPEAT);
    return s_keysNow[code];
}

bool Input::IsKeyUp(Key key) {
    return !IsKeyDown(key);
}

bool Input::IsKeyPressed(Key key) {
    if (!s_window) return false;
    int code = ToGlfw(key);
    if (code < 0 || code >= 512) return false;
    return s_keysNow[code] && !s_keysPrev[code];
}

bool Input::IsMouseDown(MouseButton btn) {
    if (!s_window) return false;
    int b = static_cast<int>(btn);
    int state = glfwGetMouseButton(s_window, b);
    s_mouseNow[b] = (state == GLFW_PRESS);
    return s_mouseNow[b];
}

bool Input::IsMousePressed(MouseButton btn) {
    if (!s_window) return false;
    int b = static_cast<int>(btn);
    return s_mouseNow[b] && !s_mousePrev[b];
}

void Input::MousePos(double& x, double& y) {
    x = y = 0.0;
    if (s_window) glfwGetCursorPos(s_window, &x, &y);
}

Vec2 Input::MousePos() {
    double x = 0, y = 0;
    MousePos(x, y);
    return Vec2(static_cast<float>(x), static_cast<float>(y));
}

void Input::EndFrame() {
    if (!s_window) return;
    for (int i = 0; i < 512; i++) s_keysPrev[i] = s_keysNow[i];
    for (int i = 0; i < 8; i++) s_mousePrev[i] = s_mouseNow[i];
    // Refresh current from GLFW. Valid ranges: 32..96 (printable) and
    // 256..348 (special: Escape..Menu). 349+ are invalid and warn in log.
    for (int code = 32; code <= 96; code++) {
        int st = glfwGetKey(s_window, code);
        s_keysNow[code] = (st == GLFW_PRESS || st == GLFW_REPEAT);
    }
    for (int code = 256; code <= 348; code++) {
        int st = glfwGetKey(s_window, code);
        s_keysNow[code] = (st == GLFW_PRESS || st == GLFW_REPEAT);
    }
    for (int b = 0; b < 3; b++) {
        s_mouseNow[b] = (glfwGetMouseButton(s_window, b) == GLFW_PRESS);
    }
}

} // namespace Arc
