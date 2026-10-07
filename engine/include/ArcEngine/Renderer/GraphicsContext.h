#pragma once

namespace Arc {

// Loads all OpenGL entry points via vendored glad2 (M2).
// MUST be called once after a GL context is current (Window creates it).
// Returns true when GL 4.6 Core entry points are available.
class GraphicsContext {
public:
    static bool Init();
    static const char* Version();
    static const char* Renderer();
};

} // namespace Arc
