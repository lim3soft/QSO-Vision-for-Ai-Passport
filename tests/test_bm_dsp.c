#include "bm_dsp.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    assert(bm_speech_boost(0)==0);
    assert(bm_speech_boost(1000)==2000 && bm_speech_boost(-1000)==-2000);
    assert(bm_speech_boost(10000)==20000);
    int previous=-29000;
    for(int sample=-32768;sample<=32767;++sample) {
        int output=bm_speech_boost((int16_t)sample);
        assert(output>=-29000 && output<=29000 && output>=previous);
        previous=output;
        if(sample>0)assert(output==-bm_speech_boost((int16_t)-sample));
    }
    puts("Speech boost: PASS (all PCM inputs, +6 dB low-level gain, monotonic soft limiting, symmetry/headroom)");
    bm_speech_level_t level;bm_speech_level_reset(&level);int16_t pcm[160];
    for(unsigned frame=0;frame<64;++frame) {
        for(unsigned i=0;i<160;++i)pcm[i]=i%2?500:-500;
        bm_speech_level(&level,pcm,160);
    }
    assert(pcm[159]>7800 && pcm[0]<-7800); /* quiet speech approaches 16x */
    for(unsigned i=0;i<160;++i)pcm[i]=i%2?32767:-32768;
    bm_speech_level(&level,pcm,160);
    for(unsigned i=0;i<160;++i)assert(pcm[i]>=-29000 && pcm[i]<=29000);
    assert(level.gain_q12<=4096); /* peak-safe reduction on a loud input */
    for(unsigned i=0;i<160;++i)pcm[i]=0;
    bm_speech_level(&level,pcm,160);
    for(unsigned i=0;i<160;++i)assert(pcm[i]==0);
    bm_speech_level_reset(&level);
    for(unsigned i=0;i<160;++i)pcm[i]=i%2?10:-10;
    for(unsigned j=0;j<32;++j){for(unsigned i=0;i<160;++i)pcm[i]=i%2?10:-10;bm_speech_level(&level,pcm,160);}
    assert(pcm[159]<=11 && level.gain_q12<=4100);
    for(int sample=-32768;sample<=32767;++sample) {
        for(unsigned i=0;i<160;++i)pcm[i]=(int16_t)sample;
        bm_speech_level(&level,pcm,160);
        for(unsigned i=0;i<160;++i)assert(pcm[i]>=-29000 && pcm[i]<=29000);
    }
    puts("Adaptive speech level: PASS (quiet gain, loud attack, silence/noise floor, all PCM levels, headroom)");
    return 0;
}
