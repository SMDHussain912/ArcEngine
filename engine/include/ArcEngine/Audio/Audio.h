#pragma once
#include <cstdint>
#include <string>

namespace Arc {

// Simple audio (M6b) — init once, play WAV/MP3/FLAC via miniaudio.
// Usage: Audio::Init(); Audio::Play("assets/audio/jump.wav"); Audio::Shutdown();
// Non-blocking: sounds play on mixer threads. M7 editor gets volume sliders.
class Audio {
public:
    static bool Init();
    static void Shutdown();
    static bool IsReady() { return s_ready; }

    // Fire-and-forget one-shot. Returns false if engine not ready or file bad.
    static bool Play(const std::string& path, float volume = 1.0f);

    static void SetMasterVolume(float v);
    static float MasterVolume() { return s_master; }

private:
    static bool s_ready;
    static float s_master;
};

} // namespace Arc
