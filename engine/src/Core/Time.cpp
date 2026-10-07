#include "ArcEngine/Core/Time.h"

namespace Arc {

Time::Time() : m_last(Clock::now()) {}

void Time::Tick() {
    auto now = Clock::now();
    std::chrono::duration<double> diff = now - m_last;
    m_last = now;

    m_delta = static_cast<float>(diff.count());
    // clamp huge hitches (debugger, alt-tab) so gameplay stays stable
    if (m_delta > 0.1f) m_delta = 0.1f;

    m_elapsed += m_delta;
    m_frames++;

    m_fpsAccum += m_delta;
    m_fpsFrames++;
    if (m_fpsAccum >= 0.5) {
        m_fps = static_cast<float>(m_fpsFrames / m_fpsAccum);
        m_fpsAccum = 0.0;
        m_fpsFrames = 0;
    }
}

} // namespace Arc
