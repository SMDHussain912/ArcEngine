#include "ArcEngine/Audio/Audio.h"
#include "ArcEngine/Core/Log.h"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <algorithm>
#include <mutex>
#include <vector>

namespace Arc {

namespace {
ma_engine s_engine;
std::mutex s_mutex;
std::vector<ma_sound*> s_sounds; // owned; freed on Shutdown (one-shots are short)

void PruneFinished() {
    // Keep vector small: drop sounds that finished playing.
    s_sounds.erase(std::remove_if(s_sounds.begin(), s_sounds.end(), [](ma_sound* s) {
        if (!ma_sound_is_playing(s)) {
            ma_sound_uninit(s);
            delete s;
            return true;
        }
        return false;
    }), s_sounds.end());
}
} // namespace

bool Audio::s_ready = false;
float Audio::s_master = 1.0f;

bool Audio::Init() {
    if (s_ready) return true;
    ma_result r = ma_engine_init(nullptr, &s_engine);
    if (r != MA_SUCCESS) {
        Log::Error("Audio engine init failed. Continuing silent (no audio device?).");
        return false;
    }
    s_ready = true;
    ma_engine_set_volume(&s_engine, s_master);
    Log::Info("Audio ready (miniaudio).");
    return true;
}

void Audio::Shutdown() {
    if (!s_ready) return;
    std::lock_guard<std::mutex> lock(s_mutex);
    for (auto* s : s_sounds) {
        ma_sound_uninit(s);
        delete s;
    }
    s_sounds.clear();
    ma_engine_uninit(&s_engine);
    s_ready = false;
    Log::Info("Audio shutdown.");
}

bool Audio::Play(const std::string& path, float volume) {
    if (!s_ready) return false;
    std::lock_guard<std::mutex> lock(s_mutex);
    PruneFinished();
    auto* sound = new ma_sound();
    ma_result r = ma_sound_init_from_file(&s_engine, path.c_str(), 0, nullptr, nullptr, sound);
    if (r != MA_SUCCESS) {
        delete sound;
        Log::Error("Audio play failed (missing/unsupported?): " + path);
        return false;
    }
    ma_sound_set_volume(sound, volume * s_master);
    ma_sound_start(sound);
    s_sounds.push_back(sound);
    return true;
}

void Audio::SetMasterVolume(float v) {
    s_master = std::clamp(v, 0.0f, 1.0f);
    if (s_ready) ma_engine_set_volume(&s_engine, s_master);
}

} // namespace Arc
