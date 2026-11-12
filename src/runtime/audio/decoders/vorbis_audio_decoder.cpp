#include "vorbis_audio_decoder.h"

#include <cstdint>
#include <cstdio>
#include <utility>

#include "core/assertion.h"

namespace {

size_t vorbis_read_callback(void *ptr, size_t size, size_t nmemb, void *userdata);
int vorbis_seek_callback(void *userdata, ogg_int64_t offset, int whence);
int vorbis_close_callback(void *userdata);
long vorbis_tell_callback(void *userdata);

} // namespace

/* ===================== *
 *       Ogg Decoder     *
 * ===================== */

Vorbis_AudioDecoder::Vorbis_AudioDecoder(const std::filesystem::path &filepath) {
    m_file.open(filepath.string(), std::ios::binary | std::ios::in);
}

Vorbis_AudioDecoder::~Vorbis_AudioDecoder() {
    if (m_open) {
        ov_clear(&m_vf);
        m_open = false;
    }
    m_file.close();
}

bool Vorbis_AudioDecoder::open() {
    if (m_open || !m_file) {
        return false;
    }

    const ov_callbacks callbacks{
        .read_func = vorbis_read_callback,
        .seek_func = vorbis_seek_callback,
        .close_func = vorbis_close_callback,
        .tell_func = vorbis_tell_callback,
    };

    if (ov_open_callbacks(&m_file, &m_vf, nullptr, 0, callbacks) < 0) {
        return false;
    }
    m_open = true;

    vorbis_info *vi = ov_info(&m_vf, -1);
    m_info.channels = static_cast<size_t>(vi->channels);
    m_info.sample_rate = static_cast<size_t>(vi->rate);

    const ogg_int64_t total = ov_pcm_total(&m_vf, -1);
    m_info.frame_count = total < 0 ? 0 : static_cast<uint64_t>(total);

    return true;
}

uint64_t Vorbis_AudioDecoder::read_frames(std::span<float> out, uint64_t frame_count) {
    const uint64_t channels = m_info.channels;
    if (!m_open) {
        return uint64_t{};
    }
    if (out.size() < frame_count * channels) {
        ASSERT(false, "Vorbis_AudioDecoder::read_frames(): out size must be large enough: 'out.size() >= frame_count * channels' ");
        return uint64_t{};
    }

    uint64_t frames_written = 0;

    while (frames_written < frame_count) {
        float **pcm = nullptr;
        int bitstream = 0;

        const int requested = static_cast<int>(
            std::min<uint64_t>(frame_count - frames_written, 4096));

        const long result = ov_read_float(&m_vf, &pcm, requested, &bitstream);

        if (result == 0) {
            /* EOF */
            break;
        }

        if (result < 0) {
            /* decode error */
            return uint64_t{};
        }

        if (static_cast<uint64_t>(bitstream) != 0 &&
            (std::cmp_not_equal(ov_info(&m_vf, -1)->channels, m_info.channels) ||
                std::cmp_not_equal(ov_info(&m_vf, -1)->rate, m_info.sample_rate))) {
            /* unsupported format */
            return uint64_t{};
        }

        for (long frame = 0; frame < result; ++frame) {
            for (uint64_t channel = 0; channel < channels; ++channel) {
                out[((frames_written + static_cast<uint64_t>(frame)) * channels) + channel] =
                    pcm[channel][frame];
            }
        }

        frames_written += static_cast<uint64_t>(result);
    }

    return frames_written;
}

void Vorbis_AudioDecoder::seek(uint64_t offset) {
    if (!m_open) {
        return;
    }
    ov_pcm_seek(&m_vf, static_cast<ogg_int64_t>(offset));
}

uint64_t Vorbis_AudioDecoder::tell() {
    if (!m_open) {
        return 0;
    }
    const ogg_int64_t pos = ov_pcm_tell(&m_vf);
    return pos < 0 ? 0 : static_cast<uint64_t>(pos);
}

/* ===================== *
 *    Vorbis callbacks   *
 * ===================== */

namespace {

size_t vorbis_read_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    auto *file = static_cast<std::ifstream *>(userdata);
    const auto out = std::span(static_cast<char *>(ptr), size * nmemb);
    file->read(out.data(), out.size());
    return size == 0 ? 0 : file->gcount() / size;
}

int vorbis_seek_callback(void *userdata, ogg_int64_t offset, int whence) {
    auto *file = static_cast<std::ifstream *>(userdata);
    file->clear();
    switch (whence) {
        case SEEK_SET:
            file->seekg(offset, std::ios::beg);
            break;
        case SEEK_CUR:
            file->seekg(offset, std::ios::cur);
            break;
        case SEEK_END:
            file->seekg(offset, std::ios::end);
            break;
        default:
            return -1;
    }
    return file->fail() ? -1 : 0;
}

int vorbis_close_callback(void *userdata) {
    /* no-op, Vorbis_AudioDecoder is responsible of freeing the file */
    (void)userdata;
    return 0;
}

long vorbis_tell_callback(void *userdata) {
    auto *file = static_cast<std::ifstream *>(userdata);
    file->clear();
    return static_cast<long>(file->tellg());
}

} // namespace
