#pragma once
#include "ArcEngine/Core/Math.h"
#include <string>

namespace Arc {

// Render command queue (M4) — game code records intent, backend executes.
// Keeps M5 Scene code API-stable when we add GLES: same commands, new backend.
class RenderCommand {
public:
    static void SetClearColor(const Vec4& color);
    static void Clear();
    static void SetViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h);

    static const Vec4& ClearColor() { return s_clearColor; }

private:
    static Vec4 s_clearColor;
};

} // namespace Arc
