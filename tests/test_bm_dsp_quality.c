#include "bm_dsp.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static void tone(int16_t *pcm,int amplitude)
{
    for(unsigned i=0;i<160;++i)pcm[i]=(int16_t)lround(amplitude*sin(2*3.141592653589793*i/8));
}
int main(void)
{
    bm_speech_level_t level;bm_speech_level_reset(&level);
    int16_t pcm[160],input[160];
    /* A falling envelope must not become unsigned arithmetic or reverse sign. */
    level.gain_q12=8*4096;
    for(unsigned i=0;i<160;++i)pcm[i]=3000;
    bm_speech_level(&level,pcm,160);
    for(unsigned i=0;i<160;++i)assert(pcm[i]>0 && pcm[i]<=28000);
    assert(pcm[0]>pcm[159]);
    bm_speech_level_reset(&level);
    for(unsigned block=0;block<96;++block){tone(pcm,25000);bm_speech_level(&level,pcm,160);}
    tone(input,25000);
    double xx=0,xy=0,yy=0;
    for(unsigned i=0;i<160;++i){xx+=(double)input[i]*input[i];xy+=(double)input[i]*pcm[i];yy+=(double)pcm[i]*pcm[i];}
    double gain=xy/xx,error=0;
    for(unsigned i=0;i<160;++i){double e=pcm[i]-input[i]*gain;error+=e*e;assert(pcm[i]>=-28000&&pcm[i]<=28000);}
    double residual_db=10*log10((error+1e-15)/yy);
    printf("Steady 1 kHz loud-tone residual after best linear fit: %.1f dB\n",residual_db);fflush(stdout);
    assert(residual_db < -60); /* steady waveform remains linear, without soft clipping */
    bm_speech_level_reset(&level);
    for(unsigned block=0;block<128;++block){tone(pcm,500);bm_speech_level(&level,pcm,160);}
    int peak=0;for(unsigned i=0;i<160;++i){int a=pcm[i]<0?-pcm[i]:pcm[i];if(a>peak)peak=a;}
    assert(peak>=7900 && peak<=8000);
    printf("Quiet 1 kHz tone: 500 -> %d peak; sign-safe envelope and peak bounds PASS\n",peak);
    return 0;
}
