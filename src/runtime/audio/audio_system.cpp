#include "audio_system.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>

#include "audio_mixer.h"
#include "audio_source_decoded.h"
#include "runtime/audio/audio_controller.h"
#include "spsc_queue.h"

AudioSystem::AudioSystem(std::unique_ptr<IAudioDevice> ad)
    : m_ad(std::move(ad)) {}

AudioSystem::~AudioSystem() {
    if (m_ad) {
        close();
    }
}

void AudioSystem::init() {
    m_ad->open({
        .format = AudioFormat::F32,
        .sample_rate = SAMPLE_RATE,
        .channels = CHANNELS,
    });

    m_ad->set_device_callback(
        AudioSystem::audio_device_callback,
        this);

    m_ad->start();

    m_audio_thread.mixer.set_voice_controller(m_voice_controller);
}

void AudioSystem::close() {
    m_ad->close();
    m_ad.reset();

    /* now that audio device is dead, we can
     * clear all loaded/unloaded sources. */
    m_sources.clear();
    m_unloaded.clear();
}

void AudioSystem::update() {
    /* TODO: make this faster by using swap-deletion */
    std::erase_if(m_unloaded, [](const std::shared_ptr<IAudioSource> &source) {
        return source.use_count() == 1;
    });
}

AudioSourceHandle AudioSystem::load_source(const std::filesystem::path &filepath) {
    auto source = std::static_pointer_cast<IAudioSource>(
        AudioSourceDecoded::open(filepath));

    if (!source) {
        return AudioSourceHandle{};
    }

    return m_sources.emplace(std::move(source));
}

void AudioSystem::unload_source(AudioSourceHandle handle) {
    auto *source = m_sources.get(handle);
    ASSERT(source, "Tried to unload non-existent source");

    if (!source) {
        return;
    }

    m_unloaded.emplace_back(std::move(*source));
    m_sources.remove(handle);

    /* flag all voices to stop */
    m_voice_controller.stop_voices_of_source(handle);
}

VoiceHandle AudioSystem::play_audio(AudioSourceHandle source, const PlayDesc &desc) {
    auto *source_ = m_sources.get(source);
    if (!source_) {
        return VoiceHandle{};
    }

    auto voice_handle = m_voice_controller.claim(desc);
    if (!voice_handle) {
        return VoiceHandle{};
    }

    const bool queued = m_audio_thread.commands.try_emplace(PlaySoundCommand{
        .handle_index = voice_handle.index(),
        .source = *source_,
    });

    /* failed to append to sound queue, free
     * the handle from the voice controller */
    if (!queued) {
        m_voice_controller.force_release(voice_handle);
        voice_handle = VoiceHandle{};
    }

    return voice_handle;
}

void AudioSystem::stop_audio(VoiceHandle voice) {
    m_voice_controller.stop_voice(voice);
}

bool AudioSystem::is_audio_playing(VoiceHandle voice) const {
    return m_voice_controller.is_playing(voice);
}

std::span<float> AudioSystem::render_audio(size_t requested_frames) {
    /* process spsc commands */
    PlaySoundCommand cmd;
    while (m_audio_thread.commands.try_pop(cmd)) {
        bool success = m_audio_thread.mixer.create_voice({
            .source = cmd.source,
            .handle_index = cmd.handle_index,
        });

        if (!success) {
            /* Failed to find or steal a voice candidate, so we must
             * flag voice controller that this voice won't play */
            m_voice_controller.mark_finished(cmd.handle_index);
        }
    }

    const auto frames = std::min(
        requested_frames, FRAME_SIZE);

    /* mix all voices into buffer */
    m_audio_thread.mixer.render_frames(
        m_buffer, frames, SAMPLE_RATE);

    /* after mixing is done, apply hard-limiter */
    for (auto &sample : m_buffer) {
        sample = std::clamp(sample, -1.0f, 1.0f);
    }

    return std::span{m_buffer}.first(frames * CHANNELS);
}
