#include "audio_eof_policy.h"

#include <stddef.h>

bool audio_eof_policy_should_finish(const audio_eof_policy_snapshot_t *snapshot)
{
    return snapshot != NULL &&
           snapshot->decoder_eof &&
           !snapshot->playback_finished &&
           snapshot->playing &&
           !snapshot->paused &&
           !snapshot->output_blocked &&
           snapshot->pending_frames == 0u;
}

bool audio_eof_policy_play_requires_rewind(bool playback_finished)
{
    return playback_finished;
}

bool audio_eof_policy_should_count_source_miss(bool decoder_eof)
{
    /* Once the producer has declared EOF, an empty PCM source is the expected
     * tail of the final, possibly partial output block. Mid-track starvation
     * still has decoder_eof=false and remains a real underrun. */
    return !decoder_eof;
}
