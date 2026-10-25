#include "bm_quality.h"
#include <stdio.h>

void bm_quality_merge(bm_qso_quality_t *target, const bm_qso_quality_t *update)
{
    uint8_t fields=(update->reported | update->valid) & (BM_QUALITY_BER | BM_QUALITY_LOSS);
    /* Final counters can arrive after an earlier, longer placeholder stop.
     * Conversely, a late zero-sample report cannot erase measured results. */
    if(update->sample_count < target->sample_count)
        fields &= (uint8_t)~(target->reported | target->valid);
    if (fields & BM_QUALITY_BER) {
        target->ber_centi_pct=update->ber_centi_pct;
        target->valid=(target->valid & (uint8_t)~BM_QUALITY_BER) | (update->valid & BM_QUALITY_BER);
    }
    if (fields & BM_QUALITY_LOSS) {
        target->loss_centi_pct=update->loss_centi_pct;
        target->valid=(target->valid & (uint8_t)~BM_QUALITY_LOSS) | (update->valid & BM_QUALITY_LOSS);
    }
    target->reported |= fields;
    if(update->sample_count > target->sample_count)target->sample_count=update->sample_count;
}

static void percent_text(char *out, size_t size, uint16_t value, bool valid)
{
    if (!valid) snprintf(out,size,"--");
    else snprintf(out,size,"%u.%02u%%",(unsigned)(value/100),(unsigned)(value%100));
}

void bm_quality_format(char *out, size_t size, const bm_qso_quality_t *quality)
{
    if (!out || !size) return;
    char ber[12],loss[12];
    percent_text(ber,sizeof(ber),quality->ber_centi_pct,quality->valid & BM_QUALITY_BER);
    percent_text(loss,sizeof(loss),quality->loss_centi_pct,quality->valid & BM_QUALITY_LOSS);
    snprintf(out,size,"BER %s   LOSS %s",ber,loss);
}

