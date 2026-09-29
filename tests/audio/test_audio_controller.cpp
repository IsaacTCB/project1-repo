#include <gtest/gtest.h>
#include <vector>

#include "core/utils/handles.h"
#include "runtime/audio/audio_controller.h"
#include "runtime/audio/audio_handles.h"
#include "runtime/audio/audio_limits.h"

class AudioControllerTest : public testing::Test {
protected:
    VoiceHandle play_dummy_voice() {
        const PlayDesc desc{
            .volume = 2.0f,
            .pitch = 1.5f,
            .looping = true,
        };
        const VoiceHandle voice = m_controller.claim(desc, {});

        EXPECT_TRUE(voice.valid());
        if (!voice.valid()) {
            return {};
        }

        EXPECT_TRUE(m_controller.is_playing(voice));
        if (!m_controller.is_playing(voice)) {
            return {};
        }

        return voice;
    }

    VoiceController m_controller;
};

/* is playing */

TEST_F(AudioControllerTest, IsPlayingReturnsTrueAfterClaiming) {
    const PlayDesc desc{
        .volume = 2.0f,
        .pitch = 1.5f,
        .looping = true,
    };

    VoiceHandle voice = m_controller.claim(desc, {});
    ASSERT_EQ(voice.valid(), true) << "Failed to claim voice";

    /* voice has to be playing now */
    EXPECT_EQ(m_controller.is_playing(voice), true);

    /* check parameters to see if they match */
    const auto params = m_controller.params(voice.index());
    EXPECT_FLOAT_EQ(params.volume, desc.volume);
    EXPECT_FLOAT_EQ(params.pitch, desc.pitch);
    EXPECT_EQ(params.looping, desc.looping);
    EXPECT_EQ(params.stop_requested, false);
}

TEST_F(AudioControllerTest, ClaimFailsWhenAllVoicesAreOccupied) {
    std::array<VoiceHandle, AudioLimits::MAX_VOICES> voices;

    for (auto &voice : voices) {
        voice = m_controller.claim({}, {});
        ASSERT_TRUE(voice.valid());
    }

    const VoiceHandle extra = m_controller.claim({}, {});
    EXPECT_FALSE(extra.valid());
}

TEST_F(AudioControllerTest, IsPlayingReturnsFalseAfterStopRequest) {
    VoiceHandle voice = play_dummy_voice();
    ASSERT_TRUE(voice);

    /* send voice stop request */
    m_controller.stop_voice(voice);

    /* voice shouldn't be playing now */
    EXPECT_EQ(m_controller.is_playing(voice), false);

    /* check voice parameters to see if stop requested is true */
    const auto params = m_controller.params(voice.index());
    EXPECT_EQ(params.stop_requested, true);
}

TEST_F(AudioControllerTest, IsPlayingReturnsFalseOnMarkFinished) {
    VoiceHandle voice = play_dummy_voice();
    ASSERT_TRUE(voice);

    /* mark voice as finished (done by the mixer)  */
    m_controller.mark_finished(voice.index());

    /* voice shouldn't be playing now */
    EXPECT_EQ(m_controller.is_playing(voice), false);
}

TEST_F(AudioControllerTest, StopAllVoicesWithHandle) {
    const AudioSourceHandle handle1 = AudioSourceHandle::from_index(0);
    const AudioSourceHandle handle2 = AudioSourceHandle::from_index(1);

    std::vector<VoiceHandle> voices_of_handle1;
    std::vector<VoiceHandle> voices_of_handle2;

    for (size_t i = 0; i < AudioLimits::MAX_VOICES; ++i) {
        const bool handle_target = (i % 3);

        const auto source = handle_target ? handle1 : handle2;
        auto &target = handle_target ? voices_of_handle1 : voices_of_handle2;

        /* create handle */
        auto handle = m_controller.claim(PlayDesc{}, source);
        ASSERT_EQ(handle.valid(), true);
        target.push_back(handle);
    }

    m_controller.stop_voices_of_source(handle1);

    /* check every voice of handle 1 to see if they stop has been requested */
    for (const auto &voice : voices_of_handle1) {
        auto param = m_controller.params(voice.index());
        EXPECT_EQ(param.stop_requested, true);
    }

    /* now, let's check every voice of handle 2 to see if they haven't been messed with */
    for (const auto &voice : voices_of_handle2) {
        auto param = m_controller.params(voice.index());
        EXPECT_EQ(param.stop_requested, false);
    }
}

TEST_F(AudioControllerTest, FinishedVoiceIsNotImmediatelyReusable) {
    std::array<VoiceHandle, AudioLimits::MAX_VOICES> voices;
    for (auto &voice : voices) {
        voice = m_controller.claim({}, {});
        ASSERT_TRUE(voice.valid());
    }

    m_controller.mark_finished(voices[0].index());

    const VoiceHandle replacement = m_controller.claim({}, {});
    EXPECT_FALSE(replacement.valid());
}

TEST_F(AudioControllerTest, FinishedVoiceCanBeClaimedAfterUpdate) {
    std::array<VoiceHandle, AudioLimits::MAX_VOICES> voices;
    for (auto &voice : voices) {
        voice = m_controller.claim({}, {});
        ASSERT_TRUE(voice.valid());
    }

    m_controller.mark_finished(voices[0].index());
    m_controller.update();

    const VoiceHandle replacement = m_controller.claim({}, {});
    EXPECT_TRUE(replacement.valid());
}

/* Releasing handles */

TEST_F(AudioControllerTest, ReleasedVoiceHandleBecomesInvalid) {
    std::array<VoiceHandle, AudioLimits::MAX_VOICES> voices;
    for (auto &voice : voices) {
        voice = m_controller.claim({}, {});
        ASSERT_TRUE(voice.valid());
    }

    /* free it up */
    m_controller.force_release(voices[0]);

    /* replacement should consume old handle's slot */
    auto replacement = m_controller.claim({}, {});
    ASSERT_TRUE(replacement.valid());
    ASSERT_TRUE(voices[0].index() == replacement.index());

    EXPECT_TRUE(voices[0] != replacement);
    EXPECT_FALSE(m_controller.is_playing(voices[0]));
}

TEST_F(AudioControllerTest, ReleaseAllInvalidatesAllHandles) {
    std::array<VoiceHandle, AudioLimits::MAX_VOICES> voices;
    for (auto &voice : voices) {
        voice = m_controller.claim({}, {});
        ASSERT_TRUE(voice.valid());
    }

    m_controller.force_release_all();

    for (auto &voice : voices) {
        EXPECT_FALSE(m_controller.is_playing(voice));
        voice = m_controller.claim({}, {});
        EXPECT_TRUE(voice.valid());
    }
}

/* setters */

TEST_F(AudioControllerTest, SetVolumeGainChangesVolume) {
    VoiceHandle voice = play_dummy_voice();

    const float old_volume = m_controller.params(voice.index()).volume;
    const float new_volume = 5.0f;

    m_controller.set_volume_gain(voice, new_volume);

    ASSERT_NE(old_volume, new_volume) << "Old and new volume are the same value";
    EXPECT_EQ(new_volume, m_controller.params(voice.index()).volume);
}

TEST_F(AudioControllerTest, SetLoopingGainChangesLoopStatus) {
    VoiceHandle voice = play_dummy_voice();

    const bool loop = m_controller.is_looping(voice);
    const bool new_loop = !loop;

    m_controller.set_looping(voice, new_loop);
    EXPECT_EQ(new_loop, m_controller.is_looping(voice));
}
