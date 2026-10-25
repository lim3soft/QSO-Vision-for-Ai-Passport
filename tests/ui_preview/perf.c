#define _POSIX_C_SOURCE 200809L
#include "qso_ui.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t drawbuffer[240*20];
static qso_ui_t ui;
static unsigned long pixels,flushes,history_draws;
static void flushed(lv_display_t *display,const lv_area_t *a,uint8_t *data)
{
    (void)data;pixels+=(unsigned long)(a->x2-a->x1+1)*(unsigned long)(a->y2-a->y1+1);
    ++flushes;lv_display_flush_ready(display);
}
static void history_draw(lv_event_t *event){(void)event;++history_draws;}
static void step(void){lv_tick_inc(20);lv_timer_handler();lv_refr_now(NULL);}
int main(int argc,char **argv)
{
    bool active=argc>1 && !strcmp(argv[1],"active");
    setenv("TZ","MYT-8",1);tzset();lv_init();
    lv_display_t *display=lv_display_create(240,320);assert(display);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,drawbuffer,NULL,sizeof(drawbuffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flushed);assert(qso_ui_create(&ui));
    lv_label_set_text(ui.title,"MONITORING TALK GROUP 91");lv_label_set_text(ui.selection,"TG 91");
    lv_label_set_text(ui.onair,"WAITING FOR QSO");lv_label_set_text(ui.help,"VOL-85%");
    bm_qso_info_t rows[5]={0};
    const char *names[]={"Russia","Russia","Australia","Japan","United States"};
    for(size_t i=0;i<5;++i){snprintf(rows[i].callsign,sizeof(rows[i].callsign),"TEST%u",(unsigned)i+1);
        snprintf(rows[i].country,sizeof(rows[i].country),"%s",names[i]);
        rows[i].started_us=1700000000000000LL;rows[i].ended_us=1700000012000000LL;}
    lv_obj_add_event_cb(ui.data_panel,history_draw,LV_EVENT_DRAW_MAIN,NULL);
    qso_ui_history(&ui,rows,5,"CONNECTED");qso_ui_history_position(&ui,0,15);
    qso_ui_status(&ui,true,true,false,86,false);qso_ui_live(&ui,active);
    for(unsigned i=0;i<25;++i)step();
    pixels=flushes=history_draws=0;
    for(unsigned i=0;i<500;++i){
        if(i%25==0){qso_ui_history(&ui,rows,5,"CONNECTED");qso_ui_history_position(&ui,0,15);qso_ui_status(&ui,true,true,false,86,false);}
        step();
    }
    printf("{\"mode\":\"%s\",\"simulated_ms\":10000,\"unchanged_snapshot_updates\":20,\"history_draws\":%lu,\"flushed_pixels\":%lu,\"flushes\":%lu}\n",active?"active":"idle",history_draws,pixels,flushes);
    return 0;
}
