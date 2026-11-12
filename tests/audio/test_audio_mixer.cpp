#include "fakes/audio_source_signal.h"
#include "fakes/audio_voice_controller_fake.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>

#include "runtime/audio/audio_limits.h"
#include "runtime/audio/audio_mixer.h"
#include "runtime/audio/audio_source.h"
#include "runtime/audio/audio_source_decoded.h"

#include <gtest/gtest.h>
#include <limits>
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

    std::optional<size_t> add_voice(std::unique_ptr<IAudioSourceInstance> instance, int priority = 0) {
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
            .priority = priority,
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

TEST_F(AudioMixerTest, NoActiveVoicesProducesSilence) {
    constexpr size_t RENDER_FRAMES = 50;
    constexpr size_t PCM_COUNT = RENDER_FRAMES * AudioLimits::OUTPUT_CHANNELS;

    std::array<float, PCM_COUNT> expected;
    expected.fill(0.0f);

    std::array<float, PCM_COUNT> buffer;
    m_mixer.render_frames(buffer, RENDER_FRAMES);

    expect_audio_near(buffer, expected, 0.0f);
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

TEST_F(AudioMixerTest, InitialVolumeIsAppliedWithoutRamp) {
    /* TODO: create tests for applying volumes with ramps */

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

TEST_F(AudioMixerTest, OutputsSilenceAfterSourceEnds) {
    /* create instance */
    auto voice = add_voice(get_signal(1.0f, 10));
    ASSERT_TRUE(voice);

    /* render to buffer */
    std::array<float, 50> buf;
    m_mixer.render_frames(buf, buf.size() / AudioLimits::OUTPUT_CHANNELS);

    /* compare results */

    std::array<float, 50> expected = expected_const_signal<50>(1.0f, 10);
    expect_audio_near(buf, expected, 0.0001f);
}

TEST_F(AudioMixerTest, MarksFinishedWhenSourceEnds) {
    /* create instance */
    auto voice = add_voice(get_signal(1.0f, 10));
    ASSERT_TRUE(voice);

    /* render to buffer */
    std::array<float, 50> buf;
    m_mixer.render_frames(buf, buf.size() / AudioLimits::OUTPUT_CHANNELS);

    /* check if voice is flagged as finished */
    EXPECT_TRUE(m_controller.was_marked_finished(*voice));
}

TEST_F(AudioMixerTest, NonPositivePitchHoldsFirstSample) {
    constexpr size_t RENDER_FRAMES = 50;
    constexpr size_t PCM_COUNT = RENDER_FRAMES * AudioLimits::OUTPUT_CHANNELS;

    /* NaN floats should be the same as negative floats */
    constexpr auto FLOAT_NAN = std::numeric_limits<float>::quiet_NaN();

    for (const float pitch : {-1.0f, FLOAT_NAN}) {
        AudioMixer mixer;
        TestVoiceControls controller;
        mixer.set_voice_controller(controller);
        controller.set_pitch(pitch);

        /* pitch is clamped to 0, so the voice barely advances */
        auto idx = controller.claim();
        ASSERT_TRUE(idx);
        auto *inst = controller.attach(*idx, get_signal(1.0f, RENDER_FRAMES));
        mixer.create_voice({.instance = inst, .handle_index = *idx, .priority = 0});

        std::array<float, PCM_COUNT> buf;
        mixer.render_frames(buf, RENDER_FRAMES);

        expect_audio_near(buf, expected_const_signal<PCM_COUNT>(1.0f, RENDER_FRAMES), 0.0001f);
        EXPECT_FALSE(controller.was_marked_finished(*idx)) << "pitch: " << pitch;
    }
}

TEST_F(AudioMixerTest, InfinitePitchIsClampedToMaxPitch) {
    constexpr size_t RENDER_FRAMES = 50;
    constexpr size_t PCM_COUNT = RENDER_FRAMES * AudioLimits::OUTPUT_CHANNELS;

    /* at MAX_PITCH, the source is consumed in exactly OUTPUT_FRAMES frames */
    constexpr size_t OUTPUT_FRAMES = 25;
    constexpr auto SOURCE_FRAMES = static_cast<size_t>(OUTPUT_FRAMES * AudioLimits::MAX_PITCH);

    m_controller.set_pitch(std::numeric_limits<float>::infinity());
    auto voice = add_voice(get_signal(1.0f, SOURCE_FRAMES));
    ASSERT_TRUE(voice);

    std::array<float, PCM_COUNT> buf;
    m_mixer.render_frames(buf, RENDER_FRAMES);

    expect_audio_near(buf, expected_const_signal<PCM_COUNT>(1.0f, OUTPUT_FRAMES), 0.0001f);
    EXPECT_TRUE(m_controller.was_marked_finished(*voice));
}

TEST_F(AudioMixerTest, StopRequestEventuallyMarksVoiceFinished) {
    constexpr size_t BUFFER_FRAMES = 25;
    constexpr size_t BUFFER_SIZE = BUFFER_FRAMES * 2;

    /* create instance */
    auto voice = add_voice(get_signal(1.0f, 1));

    /* loop to ensure that this voice wont stop by itself */
    m_controller.set_looping(true);

    /* run an iteration first */
    std::array<float, BUFFER_SIZE> buf;
    m_mixer.render_frames(buf, BUFFER_FRAMES);
    ASSERT_FALSE(m_controller.was_marked_finished(*voice));

    /* flags voice to stop, begin ramp curve process */
    m_controller.set_stop_request(true);

    /* mixer uses a ramp curve to prevent audio clicks.
     * The drawback is that killing a voice takes a
     * few milliseconds before marking as finished */
    constexpr auto VOLUME_RAMP_FRAMES = static_cast<size_t>(
        AudioMixer::VOLUME_RAMP_SECONDS * static_cast<float>(AudioLimits::OUTPUT_SAMPLE_RATE));
    constexpr size_t MAX_ITERATIONS = (VOLUME_RAMP_FRAMES + BUFFER_FRAMES - 1) / BUFFER_FRAMES;

    /* ensure that voice isn't marked as finished here */
    for (size_t i = 0; i < MAX_ITERATIONS; ++i) {
        m_mixer.render_frames(buf, buf.size() / AudioLimits::OUTPUT_CHANNELS);
        EXPECT_FALSE(m_controller.was_marked_finished(*voice)) << "iteration: " << i << " of " << MAX_ITERATIONS;
    }

    /* check if voice is flagged as finished */
    m_mixer.render_frames(buf, buf.size() / AudioLimits::OUTPUT_CHANNELS);
    EXPECT_TRUE(m_controller.was_marked_finished(*voice)) << "last iteration";
}

TEST_F(AudioMixerTest, DontMarkVoiceFinishedIfSourceIsNotOver) {
    constexpr size_t MIXER_FRAMES = 50;
    constexpr size_t MIXER_BUFFER_SIZE = MIXER_FRAMES * AudioLimits::OUTPUT_CHANNELS;

    /* create instance that's too long for a single mixer iteration */
    auto voice = add_voice(get_signal(1.0f, MIXER_FRAMES * 2));
    ASSERT_TRUE(voice);

    /* run iteration, this shouldn't kill voice */
    std::array<float, MIXER_BUFFER_SIZE> buf;
    m_mixer.render_frames(buf, buf.size() / AudioLimits::OUTPUT_CHANNELS);

    /* check if voice is flagged as finished */
    EXPECT_FALSE(m_controller.was_marked_finished(*voice));
}

class AudioMixerStealTest : public AudioMixerTest {
protected:
    AudioMixerStealTest() {
        m_controller.set_looping(true);
    }

    size_t add_looping_voice(int priority) {
        auto idx = add_voice(get_signal(1.0f, 10), priority);
        EXPECT_TRUE(idx);
        return idx.value_or(0);
    }

    /* fills every audible voice and returns handle indices */
    std::array<size_t, AudioLimits::MAX_VOICES> fill_all_voices(int priority) {
        std::array<size_t, AudioLimits::MAX_VOICES> handles{};
        for (auto &h : handles) {
            h = add_looping_voice(priority);
        }
        return handles;
    }

    /* renders until stolen voices ramp down is finished */
    void render_past_ramp() {
        constexpr size_t BUFFER_FRAMES = 64;
        constexpr auto RAMP_FRAMES = static_cast<size_t>(
            AudioMixer::VOLUME_RAMP_SECONDS * static_cast<float>(AudioLimits::OUTPUT_SAMPLE_RATE));
        constexpr size_t MAX_ITERATIONS = (RAMP_FRAMES / BUFFER_FRAMES) + 2;

        std::array<float, BUFFER_FRAMES * AudioLimits::OUTPUT_CHANNELS> buf;
        for (size_t i = 0; i < MAX_ITERATIONS; ++i) {
            m_mixer.render_frames(buf, BUFFER_FRAMES);
        }
    }

    size_t finished_count(std::span<const size_t> handles) {
        size_t finished = 0;
        for (const size_t handle : handles) {
            if (m_controller.was_marked_finished(handle)) {
                ++finished;
            }
        }
        return finished;
    }
};

TEST_F(AudioMixerStealTest, StealsLowPriorityVoice) {
    constexpr size_t LOW_PRIORITY_SLOT = 10;

    std::array<size_t, AudioLimits::MAX_VOICES> handles{};
    for (size_t i = 0; i < handles.size(); ++i) {
        handles[i] = add_looping_voice(i == LOW_PRIORITY_SLOT ? 1 : 5);
    }
    render_past_ramp();
    ASSERT_EQ(finished_count(handles), 0);

    const size_t newcomer = add_looping_voice(5);
    ASSERT_FALSE(m_controller.was_marked_finished(newcomer));
    render_past_ramp();

    /* the low priority voice was stolen, even though it isn't the oldest */
    EXPECT_TRUE(m_controller.was_marked_finished(handles[LOW_PRIORITY_SLOT]));
    EXPECT_EQ(finished_count(handles), 1);
    EXPECT_FALSE(m_controller.was_marked_finished(newcomer));
}

TEST_F(AudioMixerStealTest, StealsOldestVoiceWithSamePriority) {
    const auto handles = fill_all_voices(3);
    render_past_ramp();

    const size_t newcomer = add_looping_voice(3);
    render_past_ramp();

    EXPECT_TRUE(m_controller.was_marked_finished(handles.front()));
    EXPECT_EQ(finished_count(handles), 1);
    EXPECT_FALSE(m_controller.was_marked_finished(newcomer));
}

TEST_F(AudioMixerStealTest, ConsecutiveStealsPickDifferentVictims) {
    /* a voice that is already ramping down must not be picked again */
    const auto handles = fill_all_voices(3);
    render_past_ramp();

    const size_t first = add_looping_voice(3);
    const size_t second = add_looping_voice(3);
    render_past_ramp();

    EXPECT_TRUE(m_controller.was_marked_finished(handles[0]));
    EXPECT_TRUE(m_controller.was_marked_finished(handles[1]));
    EXPECT_EQ(finished_count(handles), 2);
    EXPECT_FALSE(m_controller.was_marked_finished(first));
    EXPECT_FALSE(m_controller.was_marked_finished(second));
}

TEST_F(AudioMixerStealTest, StealFailsWhenAllVoicesHaveHigherPriority) {
    const auto handles = fill_all_voices(5);
    render_past_ramp();

    const size_t rejected = add_looping_voice(1);

    /* this should be rejected immediately, no ramp involved */
    EXPECT_TRUE(m_controller.was_marked_finished(rejected));

    render_past_ramp();
    EXPECT_EQ(finished_count(handles), 0);
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
