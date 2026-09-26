#include "wav_audio_decoder.h"

#include <cstdint>

/* TODO: move this into external/ */
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

#include "core/assertion.h"

namespace {

size_t drwav_read_callback(void *userdata, void *ptr, size_t size);
drwav_bool32 drwav_seek_callback(void *userdata, int offset, drwav_seek_origin whence);
drwav_bool32 drwav_tell_callback(void *userdata, drwav_int64 *cursor);

} // namespace

/* ===================== *
 *       WAV Decoder     *
 * ===================== */

WAV_AudioDecoder::WAV_AudioDecoder(const std::filesystem::path &filepath) {
    m_file.open(filepath.string(), std::ios::binary | std::ios::in);
}

WAV_AudioDecoder::~WAV_AudioDecoder() {
    if (m_open) {
        drwav_uninit(&m_wav);
        m_open = false;
    }
    m_file.close();
}

bool WAV_AudioDecoder::open() {
    if (m_open || !m_file) {
        return false;
    }

    if (!drwav_init(&m_wav, drwav_read_callback, drwav_seek_callback, drwav_tell_callback, &m_file, nullptr)) {
        return false;
    }

    m_open = true;

    m_info.channels = static_cast<size_t>(m_wav.channels);
    m_info.sample_rate = static_cast<size_t>(m_wav.sampleRate);

    drwav_uint64 total = 0;
    drwav_get_length_in_pcm_frames(&m_wav, &total);

    m_info.frame_count = total;

    return true;
}

uint64_t WAV_AudioDecoder::read_frames(std::span<float> out, uint64_t frame_count) {
    const uint64_t channels = m_info.channels;
    if (!m_open) {
        return uint64_t{};
    }
    if (out.size() < frame_count * channels) {
        ASSERT(false, "WAV_AudioDecoder::read_frames(): out size must be large enough: 'out.size() >= frame_count * channels' ");
        return uint64_t{};
    }

    return drwav_read_pcm_frames_f32(&m_wav, frame_count, out.data());
}

void WAV_AudioDecoder::seek(uint64_t offset) {
    if (!m_open) {
        return;
    }

    drwav_seek_to_pcm_frame(&m_wav, static_cast<drwav_uint64>(offset));
}

uint64_t WAV_AudioDecoder::tell() {
    if (!m_open) {
        return 0;
    }

    drwav_uint64 cursor = 0;
    drwav_get_cursor_in_pcm_frames(&m_wav, &cursor);

    return cursor;
}

/* ===================== *
 *    drwav callbacks    *
 * ===================== */

namespace {

size_t drwav_read_callback(void *userdata, void *ptr, size_t size) {
    auto *file = static_cast<std::ifstream *>(userdata);
    const auto out = std::span(static_cast<char *>(ptr), size);
    file->read(out.data(), out.size());
    return file->gcount();
}

drwav_bool32 drwav_seek_callback(void *userdata, int offset, drwav_seek_origin whence) {
    auto *file = static_cast<std::ifstream *>(userdata);
    file->clear();
    switch (whence) {
        case DRWAV_SEEK_SET:
            file->seekg(offset, std::ios::beg);
            break;
        case DRWAV_SEEK_CUR:
            file->seekg(offset, std::ios::cur);
            break;
        case DRWAV_SEEK_END:
            file->seekg(offset, std::ios::end);
            break;
        default:
            return DRWAV_FALSE;
    }
    return file->fail() ? DRWAV_FALSE : DRWAV_TRUE;
}

drwav_bool32 drwav_tell_callback(void *userdata, drwav_int64 *cursor) {
    auto *file = static_cast<std::ifstream *>(userdata);
    file->clear();

    auto pos = file->tellg();
    if (pos == -1) {
        return false;
    }

    *cursor = pos;
    return true;
}

} // namespace
