#include "audio_system.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "audio_controller.h"
#include "audio_decoder.h"
#include "audio_handles.h"
#include "audio_mixer.h"
#include "audio_source.h"
#include "audio_source_decoded.h"
#include "spsc_queue.h"

#include "decoders/mp3_audio_decoder.h"
#include "decoders/vorbis_audio_decoder.h"
#include "decoders/wav_audio_decoder.h"

AudioSystem::AudioSystem(std::unique_ptr<IAudioDevice> ad)
    : m_ad(std::move(ad)) {}

AudioSystem::~AudioSystem() {
    if (m_ad) {
        close();
    }
}

void AudioSystem::init() {
    m_audio_thread.mixer.set_voice_controller(m_voice_controller);
    m_buffer.fill(0.0f);

    m_ad->open({
        .format = AudioFormat::F32,
        .sample_rate = SAMPLE_RATE,
        .channels = CHANNELS,
    });

    m_ad->set_device_callback(
        AudioSystem::audio_device_callback,
        this);

    m_ad->start();
}

void AudioSystem::close() {
    m_ad->close();
    m_ad.reset();

    /* now that audio device is dead, we can
     * clear all loaded sources. */
    m_sources.clear();

    /* one last update */
    m_voice_controller.force_release_all();
}

void AudioSystem::update() {
    /* voice controller to free any instances */
    m_voice_controller.update();
}

/**
 * @brief Creates a audio file decoder from filepath extension
 *
 * For example:
 *   .wav -> WAV_AudioDecoder
 *   .ogg -> Vorbis_AudioDecoder
 *   .mp3 -> MP3_AudioDecoder
 *
 * @param path The filepath into the file
 * @return a unique ptr object with a IAudioDecoder instance.
 */
static std::unique_ptr<IAudioDecoder> get_decoder_from_path(const std::filesystem::path &path) {
    /* convert to lowercase (for case insensitive check) */
    std::string extension = path.extension().generic_string();
    std::ranges::transform(extension, extension.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    /* try to detect decoder from extension name */
    if (extension == ".wav") {
        return std::unique_ptr<IAudioDecoder>(new WAV_AudioDecoder(path));
    }
    if (extension == ".ogg") {
        return std::unique_ptr<IAudioDecoder>(new Vorbis_AudioDecoder(path));
    }
    if (extension == ".mp3") {
        return std::unique_ptr<IAudioDecoder>(new MP3_AudioDecoder(path));
    }

    /* failed, return nullptr */
    return {};
}

/**
 * @brief converts user-facing LoopPointsDesc into internal AudioLoopPoints type
 *
 * @param desc LoopPointsDesc object to convert from
 * @param info AudioSource information for sample rate query
 *
 * @return A AudioLoopPoints object
 */
static AudioLoopPoints to_audio_loop_points(const LoopPointsDesc &points, const AudioSourceInfo &info) {
    if (const auto *frames = std::get_if<LoopPointsFrames>(&points)) {
        return {.begin = frames->start, .end = frames->end};
    }

    const auto &seconds = std::get<LoopPointsSeconds>(points);
    const auto to_frames = [&](double s) -> size_t {
        if (s <= 0.0) {
            return 0;
        }
        const double frames = std::round(s * static_cast<double>(info.sample_rate));
        return frames >= static_cast<double>(info.frame_count)
                 ? info.frame_count
                 : static_cast<size_t>(frames);
    };

    return {.begin = to_frames(seconds.start), .end = to_frames(seconds.end)};
}

AudioSourceHandle AudioSystem::load_source(const std::filesystem::path &path) {
    std::unique_ptr<IAudioDecoder> decoder = get_decoder_from_path(path);
    if (!decoder) {
        /* unrecognized extension or unsupported format */
        return AudioSourceHandle{};
    }

    auto source = std::static_pointer_cast<IAudioSource>(
        AudioSourceDecoded::open(std::move(decoder)));

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

    m_sources.remove(handle);

    /* flag all voices to stop */
    m_voice_controller.stop_voices_of_source(handle);
}

std::optional<AudioSourceInfo> AudioSystem::get_source_info(AudioSourceHandle handle) const {
    if (const auto *source = m_sources.get(handle)) {
        return (*source)->info();
    }
    return std::nullopt;
}

uint64_t AudioSystem::get_source_length_frames(AudioSourceHandle handle) const {
    const auto info = get_source_info(handle);
    return info ? info->frame_count : 0;
}

double AudioSystem::get_source_length_seconds(AudioSourceHandle handle) const {
    const auto info = get_source_info(handle);
    return info ? info->duration_seconds() : 0.0;
}

size_t AudioSystem::get_source_sample_rate(AudioSourceHandle handle) const {
    const auto info = get_source_info(handle);
    return info ? info->sample_rate : 0;
}

size_t AudioSystem::get_source_channels(AudioSourceHandle handle) const {
    const auto info = get_source_info(handle);
    return info ? info->channels : 0;
}

VoiceHandle AudioSystem::play_audio(AudioSourceHandle source_handle, const PlayDesc &desc) {
    auto *source = m_sources.get(source_handle);
    if (!source) {
        return VoiceHandle{};
    }

    auto voice_handle = m_voice_controller.claim(desc, source_handle);
    if (!voice_handle) {
        return VoiceHandle{};
    }

    auto instance = (*source)->create_instance();

    instance->set_loop_points(desc.loop_points
                                  ? to_audio_loop_points(*desc.loop_points, (*source)->info())
                                  : AudioLoopPoints{});

    auto *inst = m_voice_controller.attach(voice_handle, std::move(instance));
    const bool queued = m_audio_thread.commands.try_emplace(PlaySoundCommand{
        .handle_index = voice_handle.index(),
        .instance = inst,
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
    ASSERT(m_voice_controller.is_playing(voice), "'voice' must be a valid voice!");
    m_voice_controller.stop_voice(voice);
}

bool AudioSystem::is_audio_playing(VoiceHandle voice) const {
    return m_voice_controller.is_playing(voice);
}

bool AudioSystem::is_audio_looping(VoiceHandle voice) const {
    ASSERT(m_voice_controller.is_playing(voice), "'voice' must be a valid voice!");
    return m_voice_controller.is_looping(voice);
}

void AudioSystem::set_audio_volume(VoiceHandle voice, float volume) {
    ASSERT(m_voice_controller.is_playing(voice), "'voice' must be a valid voice!");
    m_voice_controller.set_volume_gain(voice, volume);
}

void AudioSystem::set_audio_looping(VoiceHandle voice, bool looping) {
    ASSERT(m_voice_controller.is_playing(voice), "'voice' must be a valid voice!");
    m_voice_controller.set_looping(voice, looping);
}

std::span<float> AudioSystem::render_audio(size_t requested_frames) {
    /* process spsc commands */
    PlaySoundCommand cmd;
    while (m_audio_thread.commands.try_pop(cmd)) {
        m_audio_thread.mixer.create_voice({
            .instance = cmd.instance,
            .handle_index = cmd.handle_index,
        });
    }

    const auto frames = std::min(
        requested_frames, FRAME_SIZE);

    /* mix all voices into buffer */
    m_audio_thread.mixer.render_frames(
        m_buffer, frames);

    /* after mixing is done, apply hard-limiter */
    for (auto &sample : m_buffer) {
        sample = std::clamp(sample, -1.0f, 1.0f);
    }

    return std::span{m_buffer}.first(frames * CHANNELS);
}
