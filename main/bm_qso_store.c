#include "bm_qso_store.h"
#include <string.h>

void bm_qso_store_apply(bm_qso_store_t *store, const bm_qso_info_t *item,
                        int64_t monotonic_us, int64_t wall_seconds)
{
    if (!store || !item || !item->session[0] || item->started_us <= 0) return;
    size_t existing = store->count;
    for (size_t i = 0; i < store->count; ++i)
        if (!strcmp(item->session, store->history[i].session)) existing = i;
    bool same_current = !strcmp(item->session, store->current.session);
    if (item->active) {
        /* Startup history must never resurrect an old caller on the air. */
        if (item->historical || existing < store->count ||
            item->started_us < store->current.started_us) return;
        if (!same_current) {
            int64_t age = wall_seconds - item->started_us / 1000000;
            store->initial_seconds = age >= 0 && age < 300 ? (unsigned)age : 0;
            store->current_received_us = monotonic_us;
        }
        bm_qso_info_t current=*item;
        if(same_current) {
            if((!current.callsign[0] || !strncmp(current.callsign,"ID ",3)) && store->current.callsign[0])
                memcpy(current.callsign,store->current.callsign,sizeof(current.callsign));
            if(!current.name[0])memcpy(current.name,store->current.name,sizeof(current.name));
            if(!current.country[0])memcpy(current.country,store->current.country,sizeof(current.country));
            if(!current.source_id)current.source_id=store->current.source_id;
        }
        store->current = current;
        return;
    }
    bm_qso_info_t completed = *item;
    if (same_current) {
        if (!completed.source_id) completed.source_id=store->current.source_id;
        if ((!completed.callsign[0] || !strncmp(completed.callsign,"ID ",3)) && store->current.callsign[0])
            memcpy(completed.callsign, store->current.callsign, sizeof(completed.callsign));
        if (!completed.name[0]) memcpy(completed.name, store->current.name, sizeof(completed.name));
        if (!completed.country[0]) memcpy(completed.country, store->current.country, sizeof(completed.country));
        store->current.active = false;
    }
    if (existing < store->count) {
        /* Multiple BM masters can report one session with different Stop
         * values. Keep the longest completed report, with its caller fields. */
        bm_qso_history_row_t *old = &store->history[existing];
        if(!old->source_id)old->source_id=completed.source_id;
        if (completed.ended_us > old->ended_us) old->ended_us = completed.ended_us;
        if (completed.callsign[0] && (strncmp(completed.callsign,"ID ",3) || !old->callsign[0]))
            memcpy(old->callsign, completed.callsign, sizeof(old->callsign));
        if (completed.country[0]) memcpy(old->country, completed.country, sizeof(old->country));
        return;
    }
    size_t pos = 0;
    while (pos < store->count && store->history[pos].started_us > completed.started_us) ++pos;
    if (pos >= BM_QSO_HISTORY_MAX) return;
    size_t count = store->count < BM_QSO_HISTORY_MAX ? store->count + 1 : store->count;
    for (size_t i = count - 1; i > pos; --i) store->history[i] = store->history[i - 1];
    bm_qso_history_row_t *row=&store->history[pos];
    memset(row,0,sizeof(*row));
    row->started_us=completed.started_us;row->ended_us=completed.ended_us;
    row->source_id=completed.source_id;
    memcpy(row->session,completed.session,sizeof(row->session));
    memcpy(row->callsign,completed.callsign,sizeof(row->callsign));
    memcpy(row->country,completed.country,sizeof(row->country));
    store->count = count;
}

size_t bm_qso_store_page(const bm_qso_store_t *store, bm_qso_info_t *out,
    size_t capacity, size_t *offset, size_t *total, const char *anchor)
{
    if(!store || !offset || !total)return 0;
    *total=store->count;
    if(!out || !capacity)return 0;
    if(anchor && anchor[0])for(size_t i=0;i<store->count;++i)
        if(!strcmp(anchor,store->history[i].session)){*offset=i;break;}
    size_t max=store->count>capacity?store->count-capacity:0;
    if(*offset>max)*offset=max;
    size_t n=store->count-*offset;
    if(n>capacity)n=capacity;
    for(size_t i=0;i<n;++i) {
        const bm_qso_history_row_t *row=&store->history[*offset+i];
        memset(&out[i],0,sizeof(out[i]));
        out[i].historical=true;out[i].source_id=row->source_id;
        out[i].started_us=row->started_us;out[i].ended_us=row->ended_us;
        memcpy(out[i].session,row->session,sizeof(row->session));
        memcpy(out[i].callsign,row->callsign,sizeof(row->callsign));
        memcpy(out[i].country,row->country,sizeof(row->country));
    }
    return n;
}
