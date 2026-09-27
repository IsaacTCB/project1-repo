#include "fakes/audio_source_signal.h"
#include "fakes/audio_voice_controller_fake.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>

#include "runtime/audio/audio_mixer.h"
#include "runtime/audio/audio_source.h"
#include "runtime/audio/audio_source_decoded.h"

#include <gtest/gtest.h>
#include <optional>

class AudioMixerTest : public testing::Test {
protected:
    AudioMixerTest() {
        m_mixer.set_voice_controller(m_controller);
    }

public:
    std::unique_ptr<IAudioSourceInstance> get_signal(float value, size_t length) {
        auto decoder = std::make_unique<ConstSignal_AudioDecoder>(ConstSignalDesc{
            .value = value,
            .length = length,
            .channels = 1,
            .sample_rate = AudioLimits::OUTPUT_SAMPLE_RATE,
        });
        return AudioSourceDecoded::open(std::move(decoder))->create_instance();
    }

    std::unique_ptr<IAudioSourceInstance> get_ramp_signal(float value, size_t length) {
        auto decoder = std::make_unique<ConstSignal_AudioDecoder>(ConstSignalDesc{
            .value = value,
            .length = length,
            .channels = 2,
            .sample_rate = AudioLimits::OUTPUT_SAMPLE_RATE,
            .ramp_step = 1.0f,
        });
        return AudioSourceDecoded::open(std::move(decoder))->create_instance();
    }

    std::optional<size_t> add_voice(std::unique_ptr<IAudioSourceInstance> instance) {
        auto idx = m_controller.claim();
        if (!idx) {
            return std::nullopt;
        }

        auto *inst = m_controller.attach(*idx, std::move(instance));
        if (!inst) {
            m_controller.release_claim(*idx);
            return std::nullopt;
        }

        m_mixer.create_voice({
            .instance = inst,
            .handle_index = *idx,
        });

        return *idx;
    }

protected:
    TestVoiceControls m_controller{};
    AudioMixer m_mixer;
};

void expect_audio_near(std::span<const float> actual, std::span<const float> expected, float tolerance) {
    ASSERT_EQ(actual.size(), expected.size());

    for (size_t i = 0; i < actual.size(); ++i) {
        EXPECT_NEAR(actual[i], expected[i], tolerance)
            << "Mismatch at sample " << i;
    }
}

TEST_F(AudioMixerTest, MixingMonoVoicesWorks) {
    constexpr size_t RENDER_FRAMES = 50;
    constexpr size_t PCM_COUNT = RENDER_FRAMES * AudioLimits::OUTPUT_CHANNELS;

    std::array<float, PCM_COUNT> expected;
    expected.fill(0.0f);
    for (size_t i = 0; i < 20 * AudioLimits::OUTPUT_CHANNELS; ++i) {
        expected[i] = (i < 10 * AudioLimits::OUTPUT_CHANNELS) ? 2.0f : 1.0f;
    }

    /* add the voices that should match expected */
    this->add_voice(this->get_signal(1.0f, 10));
    this->add_voice(this->get_signal(1.0f, 20));

    /* render frames */
    std::array<float, PCM_COUNT> buffer;
    m_mixer.render_frames(buffer, RENDER_FRAMES);

    expect_audio_near(buffer, expected, 0.01f);
}

TEST_F(AudioMixerTest, MixingStereoVoicesWorks) {
    constexpr size_t RENDER_FRAMES = 50;
    constexpr size_t PCM_COUNT = RENDER_FRAMES * AudioLimits::OUTPUT_CHANNELS;

    std::array<float, PCM_COUNT> expected;
    expected.fill(0.0f);
    for (size_t i = 0; i < 10; ++i) {
        float s = 1.0f + static_cast<float>(i);
        expected[(i * AudioLimits::OUTPUT_CHANNELS) + 0] = s;
        expected[(i * AudioLimits::OUTPUT_CHANNELS) + 1] = s;
    }

    /* add the voices that should match expected */
    this->add_voice(this->get_ramp_signal(1.0f, 10));

    /* render frames */
    std::array<float, PCM_COUNT> buffer;
    m_mixer.render_frames(buffer, RENDER_FRAMES);

    expect_audio_near(buffer, expected, 0.01f);
}

TEST_F(AudioMixerTest, PcmValueIsHalvedByVolume) {
    constexpr size_t RENDER_FRAMES = 50;
    constexpr size_t PCM_COUNT = RENDER_FRAMES * AudioLimits::OUTPUT_CHANNELS;

    constexpr float VOLUME = 0.5f;

    std::array<float, PCM_COUNT> expected;
    expected.fill(2.0f * VOLUME);

    /* add the voices that should match expected */
    m_controller.set_volume(VOLUME);
    this->add_voice(this->get_signal(2.0f, RENDER_FRAMES));

    /* render frames */
    std::array<float, PCM_COUNT> buffer;
    m_mixer.render_frames(buffer, RENDER_FRAMES);

    expect_audio_near(buffer, expected, 0.01f);
}

TEST_F(AudioMixerTest, ConstantSignalWhenLoopingIsOn) {
    constexpr size_t RENDER_FRAMES = 50;
    constexpr size_t PCM_COUNT = RENDER_FRAMES * AudioLimits::OUTPUT_CHANNELS;

    std::array<float, PCM_COUNT> expected;
    expected.fill(2.0f);

    /* toggle looping for ALL voices */
    m_controller.set_looping(true);

    /* with no explicit loop points, the source
     * should loop over its whole length by default */
    this->add_voice(this->get_signal(1.0f, 10));

    /* add voice with explicit endpoints.
     * should also loop over its length, but explicitly */
    auto sig = this->get_signal(1.0f, 20);
    sig->set_loop_points({});
    this->add_voice(std::move(sig));

    /* render frames */
    std::array<float, PCM_COUNT> buffer;
    m_mixer.render_frames(buffer, RENDER_FRAMES);

    expect_audio_near(buffer, expected, 0.01f);
}

TEST_F(AudioMixerTest, LoopPointsWhenLoopingIsOn) {
    constexpr size_t RENDER_FRAMES = 50;
    constexpr size_t PCM_COUNT = RENDER_FRAMES * AudioLimits::OUTPUT_CHANNELS;

    constexpr size_t LOOP_BEG = 2;
    constexpr size_t LOOP_END = 8;
    constexpr size_t LOOP_DIFF = LOOP_END - LOOP_BEG;

    /* 0, 1 -- loop beg -- 2, 3, 4, 2, 3, 4... */
    std::array<float, PCM_COUNT> expected;
    for (size_t i = 0; i < expected.size(); ++i) {
        size_t j = i / AudioLimits::OUTPUT_CHANNELS;
        if (j < LOOP_BEG) {
            expected[i] = static_cast<float>(j);
        } else {
            expected[i] = static_cast<float>(LOOP_BEG + ((j - LOOP_BEG) % LOOP_DIFF));
        }
    }

    /* toggle looping for ALL voices */
    m_controller.set_looping(true);

    auto sig = this->get_ramp_signal(0.0f, 10);
    sig->set_loop_points({.begin = LOOP_BEG, .end = LOOP_END});
    this->add_voice(std::move(sig));

    /* render frames */
    std::array<float, PCM_COUNT> buffer;
    m_mixer.render_frames(buffer, RENDER_FRAMES);

    expect_audio_near(buffer, expected, 0.01f);
}

template <size_t FrameCount>
constexpr auto expected_const_signal(float value, size_t frames) {
    std::array<float, FrameCount> arr{0.0f};
    std::ranges::fill(std::span{arr}.first(frames * AudioLimits::OUTPUT_CHANNELS), value);
    return arr;
}

/* TODO: Make this use fixture just like the other tests */
TEST(AudioMixer, OutputsSilenceAfterSourceEnds) {
    auto decoder = std::make_unique<ConstSignal_AudioDecoder>(ConstSignalDesc{
        .value = 1.0f,
        .length = 10,
        .channels = 1,
        .sample_rate = AudioLimits::OUTPUT_SAMPLE_RATE,
    });
    auto source = AudioSourceDecoded::open(std::move(decoder));

    AudioMixer mixer{};

    /* create & attach voice controller */
    TestVoiceControls controls;
    mixer.set_voice_controller(controls);

    /* create instance */
    auto idx = controls.claim();
    ASSERT_TRUE(idx.has_value()) << "Failed to claim audio voice";
    auto *inst = controls.attach(*idx, source->create_instance());
    ASSERT_TRUE(inst) << "Failed to instantiate audio source";

    mixer.create_voice({
        .instance = inst,
        .handle_index = *idx,
    });

    /* render to buffer */
    std::array<float, 50> buf;
    mixer.render_frames(buf, buf.size() / AudioLimits::OUTPUT_CHANNELS);

    /* compare results */
    std::array<float, 50> expected = expected_const_signal<50>(1.0f, 10);
    expect_audio_near(buf, expected, 0.0001f);

    /* check if source flagged as finished by the mixer */
    EXPECT_EQ(controls.was_marked_finished(*idx), true);
}

// TEST(AudioTest, ConstSignalSelfTest) {
//     constexpr float VALUE = 0.5f;
//     constexpr size_t LENGTH = 1000;
//     constexpr size_t CHANNELS = 2;
//     constexpr size_t SAMPLE_RATE = 44100;
//
//     constexpr size_t BUFFER_SIZE = 100;
//     constexpr size_t READ_FRAME_AMOUNT = 10;
//
//     ConstSignal_AudioDecoder signal{{
//         .value = VALUE,
//         .length = LENGTH,
//         .channels = CHANNELS,
//         .sample_rate = SAMPLE_RATE,
//     }};
//
//     ASSERT_EQ(signal.open(), true);
//
//     const auto &info = signal.info();
//     EXPECT_EQ(info.channels, CHANNELS);
//     EXPECT_EQ(info.frame_count, LENGTH);
//     EXPECT_EQ(info.sample_rate, SAMPLE_RATE);
//
//     /* check pos */
//     EXPECT_EQ(signal.tell(), 0);
//
//     /* read buffer */
//     std::array<float, BUFFER_SIZE> buffer;
//     buffer.fill(0.0f);
//     signal.read_frames(buffer, READ_FRAME_AMOUNT);
//
//     /* check pos */
//     EXPECT_EQ(signal.tell(), READ_FRAME_AMOUNT);
//
//     /* check read buffer output */
//     std::array<float, BUFFER_SIZE> expected_buffer{};
//     expected_buffer.fill(0.0f);
//     std::ranges::fill(std::span{expected_buffer}.first<READ_FRAME_AMOUNT * CHANNELS>(), VALUE);
//     expect_audio_near(buffer, expected_buffer, 0.0001f);
// }
