#pragma once
#include <chrono>

namespace Arc {

// Simple frame clock: call Tick() once per frame.
// Gives delta seconds + FPS (smoothed over 0.5s).
class Time {
public:
    Time();

    void Tick();
    float Delta() const { return m_delta; }
    float Fps() const { return m_fps; }
    double Elapsed() const { return m_elapsed; }
    unsigned long FrameCount() const { return m_frames; }

private:
    using Clock = std::chrono::steady_clock;
    Clock::time_point m_last;
    float m_delta = 0.0f;
    float m_fps = 0.0f;
    double m_elapsed = 0.0;
    unsigned long m_frames = 0;

    // fps smoothing
    double m_fpsAccum = 0.0;
    unsigned long m_fpsFrames = 0;
};

} // namespace Arc
