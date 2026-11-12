#include "mp3_audio_decoder.h"

#include <cstdint>

/* TODO: move this into external/ */
#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

#include "core/assertion.h"

namespace {

size_t drmp3_read_callback(void *userdata, void *ptr, size_t size);
drmp3_bool32 drmp3_seek_callback(void *userdata, int offset, drmp3_seek_origin whence);
drmp3_bool32 drmp3_tell_callback(void *userdata, drmp3_int64 *cursor);

} // namespace

/* ===================== *
 *       MP3 Decoder     *
 * ===================== */

MP3_AudioDecoder::MP3_AudioDecoder(const std::filesystem::path &filepath) {
    m_file.open(filepath.string(), std::ios::binary | std::ios::in);
}

MP3_AudioDecoder::~MP3_AudioDecoder() {
    if (m_open) {
        drmp3_uninit(&m_mp3);
        m_open = false;
    }
    m_file.close();
}

bool MP3_AudioDecoder::open() {
    if (m_open || !m_file) {
        return false;
    }

    if (!drmp3_init(&m_mp3, drmp3_read_callback, drmp3_seek_callback, drmp3_tell_callback, nullptr, &m_file, nullptr)) {
        return false;
    }

    m_open = true;

    m_info.channels = static_cast<size_t>(m_mp3.channels);
    m_info.sample_rate = static_cast<size_t>(m_mp3.sampleRate);

    drmp3_uint64 total = drmp3_get_pcm_frame_count(&m_mp3);
    m_info.frame_count = total;

    return true;
}

uint64_t MP3_AudioDecoder::read_frames(std::span<float> out, uint64_t frame_count) {
    const uint64_t channels = m_info.channels;
    if (!m_open) {
        return uint64_t{};
    }
    if (out.size() < frame_count * channels) {
        ASSERT(false, "MP3_AudioDecoder::read_frames(): out size must be large enough: 'out.size() >= frame_count * channels' ");
        return uint64_t{};
    }

    return drmp3_read_pcm_frames_f32(&m_mp3, frame_count, out.data());
}

void MP3_AudioDecoder::seek(uint64_t offset) {
    if (!m_open) {
        return;
    }

    drmp3_seek_to_pcm_frame(&m_mp3, static_cast<drmp3_uint64>(offset));
}

uint64_t MP3_AudioDecoder::tell() {
    if (!m_open) {
        return 0;
    }

    return m_mp3.currentPCMFrame;
}

/* ===================== *
 *    drmp3 callbacks    *
 * ===================== */

namespace {

size_t drmp3_read_callback(void *userdata, void *ptr, size_t size) {
    auto *file = static_cast<std::ifstream *>(userdata);
    const auto out = std::span(static_cast<char *>(ptr), size);
    file->read(out.data(), out.size());
    return file->gcount();
}

drmp3_bool32 drmp3_seek_callback(void *userdata, int offset, drmp3_seek_origin whence) {
    auto *file = static_cast<std::ifstream *>(userdata);
    file->clear();
    switch (whence) {
        case DRMP3_SEEK_SET:
            file->seekg(offset, std::ios::beg);
            break;
        case DRMP3_SEEK_CUR:
            file->seekg(offset, std::ios::cur);
            break;
        case DRMP3_SEEK_END:
            file->seekg(offset, std::ios::end);
            break;
        default:
            return DRMP3_FALSE;
    }
    return file->fail() ? DRMP3_FALSE : DRMP3_TRUE;
}

drmp3_bool32 drmp3_tell_callback(void *userdata, drmp3_int64 *cursor) {
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
