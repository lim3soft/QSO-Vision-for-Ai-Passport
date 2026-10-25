#define _POSIX_C_SOURCE 200809L
#include "bm_protocol.h"
#include "bm_qso_store.h"
#include <stdio.h>
#include <string.h>

/* Consume the read-only research probe's live output without saving caller
 * records. Only aggregate acceptance counts are printed. */
int main(void)
{
    char line[8192];bm_qso_store_t store={0};
    unsigned startup=0,live=0,stopped=0;
    while(fgets(line,sizeof(line),stdin)) {
        if(strncmp(line,"TG91 ",5)) continue;
        bm_qso_info_t item;
        if(!bm_decode_metadata(line+5,91,&item)) continue;
        if(item.historical)++startup;else ++live;
        if(!item.active)++stopped;
        bm_qso_store_apply(&store,&item,1000000,(item.started_us/1000000)+1);
    }
    printf("Live BM parser: startup=%u live=%u completed=%u history_rows=%u\n",
        startup,live,stopped,(unsigned)store.count);
    return startup>0 && store.count>0 ? 0 : 1;
}
