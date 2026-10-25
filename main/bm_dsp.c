#include "bm_dsp.h"
static int16_t limit(int32_t scaled)
{
    int64_t magnitude=scaled<0?-(int64_t)scaled:scaled;
    if(magnitude>20000) {
        int64_t excess=magnitude-20000;
        magnitude=20000+excess*9000/(excess+9000);
    }
    return (int16_t)(scaled<0?-magnitude:magnitude);
}
int16_t bm_speech_boost(int16_t sample)
{
    return limit((int32_t)sample*2);
}

void bm_speech_level_reset(bm_speech_level_t *state) {state->gain_q12=4*4096;}

static uint32_t square_root(uint32_t n)
{
    uint32_t root=0,bit=1u<<30;
    while(bit>n)bit>>=2;
    while(bit) {
        if(n>=root+bit){n-=root+bit;root=(root>>1)+bit;}
        else root>>=1;
        bit>>=2;
    }
    return root;
}

void bm_speech_level(bm_speech_level_t *state,int16_t *pcm,size_t count)
{
    if(!state || !pcm || !count || count>960)return;
    uint64_t energy=0;
    uint32_t peak=0;
    for(size_t i=0;i<count;++i) {
        energy+=(int64_t)pcm[i]*pcm[i];
        uint32_t magnitude=pcm[i]<0?-(int32_t)pcm[i]:pcm[i];
        if(magnitude>peak)peak=magnitude;
    }
    uint32_t rms=square_root((uint32_t)(energy/count));
    int32_t target=rms>=80?(int32_t)(8000u*4096/rms):4096;
    if(target>16*4096)target=16*4096;
    if(target<4096)target=4096;
    int32_t before=state->gain_q12;
    if(before<1 || before>16*4096)before=4096;
    int32_t ceiling=peak?(int32_t)(28000u*4096/peak):16*4096;
    if(ceiling>16*4096)ceiling=16*4096;
    /* Inspect the entire block before applying gain. Loud transients constrain
     * both endpoints; no sample is passed through a waveform clipper. */
    if(before>ceiling)before=ceiling;
    int32_t after=before+(target-before)/(target<before?4:16);
    if(after>ceiling)after=ceiling;
    for(size_t i=0;i<count;++i) {
        int32_t gain=before+(int32_t)((int64_t)(after-before)*(int64_t)(i+1)/(int64_t)count);
        pcm[i]=(int16_t)((int64_t)pcm[i]*gain/4096);
    }
    state->gain_q12=after;
}
