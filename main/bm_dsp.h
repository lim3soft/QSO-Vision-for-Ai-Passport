#pragma once
#include <stdint.h>
#include <stddef.h>
/* +6 dB for quiet PCM, soft peak compression for loud PCM, no state or allocation. */
int16_t bm_speech_boost(int16_t sample);
typedef struct { int32_t gain_q12; } bm_speech_level_t;
void bm_speech_level_reset(bm_speech_level_t *state);
/* 20 ms blocks: peak-aware linear leveling, up to 16x quiet gain. Whole-block
 * headroom prevents clipping without nonlinear waveform soft clipping. */
void bm_speech_level(bm_speech_level_t *state,int16_t *pcm,size_t count);
