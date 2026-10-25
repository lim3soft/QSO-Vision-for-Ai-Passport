#define _POSIX_C_SOURCE 200809L
#include "qso_ui.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint16_t framebuffer[240*320];
static uint16_t drawbuffer[240*20];
static qso_ui_t ui;
static unsigned backlight=100;
static time_t preview_wall=1792897445;
static unsigned history_draws;
static void count_history_draw(lv_event_t *event){(void)event;++history_draws;}
static time_t preview_clock_now(void) { return preview_wall; }
static void set_backlight(uint8_t value) { backlight=value; }

static void flush(lv_display_t *display,const lv_area_t *area,uint8_t *pixels)
{
    const uint16_t *src=(const uint16_t *)pixels;
    for(int y=area->y1;y<=area->y2;++y) for(int x=area->x1;x<=area->x2;++x) {
        int cx=x<30?30:(x>=210?209:x), cy=y<30?30:(y>=290?289:y);
        bool round=(x-cx)*(x-cx)+(y-cy)*(y-cy)<=30*30;
        if(x>=0&&x<240&&y>=0&&y<320) framebuffer[y*240+x]=round?*src:0;
        ++src;
    }
    lv_display_flush_ready(display);
}

static void ticks(unsigned count)
{
    for(unsigned i=0;i<count;++i) {lv_tick_inc(50);lv_timer_handler();}
    lv_refr_now(NULL);
}

static void dump(const char *name)
{
    char path[256];snprintf(path,sizeof(path),"/tmp/qso-ui-%s.ppm",name);
    FILE *out=fopen(path,"wb");assert(out);
    fprintf(out,"P6\n240 320\n255\n");
    for(unsigned i=0;i<240*320;++i) {
        uint16_t c=framebuffer[i];
        unsigned char rgb[]={((c>>11)&31)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};
        fwrite(rgb,1,3,out);
    }
    fclose(out);
}

int main(void)
{
    setenv("TZ","MYT-8",1);tzset();
    lv_init();
    lv_display_t *display=lv_display_create(240,320);assert(display);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,drawbuffer,NULL,sizeof(drawbuffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    bool created=qso_ui_create(&ui);
    if (!created) {
        lv_mem_monitor_t mem;lv_mem_monitor(&mem);
        fprintf(stderr,"UI allocation failed: peak=%u free=%u largest=%u; body=%p rows=%p/%p timer=%p\n",
            (unsigned)mem.max_used,(unsigned)mem.free_size,(unsigned)mem.free_biggest_size,
            (void*)ui.body,(void*)ui.data_panel,(void*)ui.directory,(void*)ui.timer);
    }
    assert(created);
    qso_ui_set_backlight(&ui,set_backlight);
    ui.clock_now=preview_clock_now;
    qso_ui_status(&ui,true,true,false,86,false);
    assert(lv_obj_has_flag(ui.status,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_style_text_font(ui.selection,0)==&qso_bold_28);
    lv_label_set_text(ui.title,"MONITORING TALK GROUP 91");
    lv_label_set_text(ui.status,"SSID: Home Radio");
    lv_label_set_text(ui.selection,"9W3SPY");
    lv_label_set_text(ui.onair,"CURRENTLY ON AIR");
    lv_label_set_text(ui.info,"Sample operator\nMalaysia . 00:24");
    lv_label_set_text(ui.help,"VOL-85%");
    bm_qso_info_t history[5]={0};
    const char *calls[]={"UB3XCM","R2ADU","VK2TEST","JA1TEST","K1TEST"};
    const char *countries[]={"Russia","Russia","Australia","Japan","United States"};
    for(unsigned i=0;i<5;++i) {
        snprintf(history[i].callsign,sizeof(history[i].callsign),"%s",calls[i]);
        snprintf(history[i].country,sizeof(history[i].country),"%s",countries[i]);
        history[i].quality=(bm_qso_quality_t){.valid=i==4?0:3,
            .ber_centi_pct=i==3?10000:(uint16_t)(i*37),
            .loss_centi_pct=i==3?10000:(uint16_t)(i*125)};
        history[i].started_us=1792787948000000LL-i*65000000LL;
        history[i].ended_us=history[i].started_us+(17+i*3)*1000000LL;
    }
    qso_ui_history(&ui,history,5,"CONNECTED");
    assert(!lv_obj_has_flag(ui.help,LV_OBJ_FLAG_HIDDEN));
    lv_point_t footer_size;
    lv_text_get_size(&footer_size,"VOL-100%",&lv_font_montserrat_8,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    assert(footer_size.x<=50 && footer_size.y<=10);
    /* Approved one-row detail columns: maximum display values must fit. */
    const char *detail_values[]={"31/12/99","23:59:59","999:59","100.00%","100.00%"};
    const int detail_widths[]={43,43,28,34,38};
    for(unsigned column=0;column<5;++column) {
        lv_point_t size;
        lv_text_get_size(&size,detail_values[column],&lv_font_montserrat_8,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
        assert(size.x<=detail_widths[column] && size.y<=11);
    }
    qso_ui_history_position(&ui,0,15);
    assert(strstr(lv_label_get_text(ui.history_heading),"1-5 OF 15"));
    qso_ui_live(&ui,true);qso_ui_current_country(&ui,"Malaysia");ticks(8);dump("on-air");
    assert(ui.caller_flag==qso_flag_image("MY") && !lv_obj_has_flag(ui.caller_flag_view,LV_OBJ_FLAG_HIDDEN));
    lv_area_t flag_area,call_area;
    lv_obj_get_coords(ui.caller_flag_view,&flag_area);lv_obj_get_coords(ui.selection,&call_area);
    assert(flag_area.x2<call_area.x1 && call_area.x2<=224);
    lv_obj_add_event_cb(ui.data_panel,count_history_draw,LV_EVENT_DRAW_MAIN,NULL);
    ticks(1);unsigned history_before=history_draws;
    qso_ui_history(&ui,history,5,"CONNECTED");qso_ui_history_position(&ui,0,15);
    qso_ui_status(&ui,true,true,false,86,false);ticks(1);
    assert(history_draws==history_before); /* No full history redraw for unchanged snapshots. */
    lv_label_set_text(ui.help,"PREVIEW");
    ticks(1);dump("community-cover");
    lv_label_set_text(ui.help,"VOL-85%");
    ticks(1);
    lv_area_t eq_area,clock_area,info_area;
    lv_obj_get_coords(ui.eq,&eq_area);lv_obj_get_coords(ui.clock,&clock_area);
    lv_obj_get_coords(ui.info,&info_area);
    assert(eq_area.x1>=160 && eq_area.x2<=224 && eq_area.y2<149);
    assert(info_area.x2<eq_area.x1 && clock_area.x2<eq_area.x1);
    assert(clock_area.y1>=info_area.y2 && clock_area.y2<150);
    const char *days[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
    for(unsigned i=0;i<7;++i) {
        char text[40];snprintf(text,sizeof(text),"23:59:59 %s 31/12/26",days[i]);
        lv_point_t size;lv_text_get_size(&size,text,&qso_text_11,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
        assert(size.x<=142);
    }
    uint16_t eq_pixels[60*42];
    for(int y=0;y<42;++y)memcpy(eq_pixels+y*60,framebuffer+(eq_area.y1+y)*240+eq_area.x1,120);
    uint8_t levels[QSO_EQ_BARS];memcpy(levels,ui.eq_levels,sizeof(levels));
    ticks(2);assert(memcmp(levels,ui.eq_levels,sizeof(levels))!=0);dump("on-air-2");
    bool eq_changed=false;
    for(int y=0;y<42;++y)if(memcmp(eq_pixels+y*60,framebuffer+(eq_area.y1+y)*240+eq_area.x1,120))eq_changed=true;
    assert(eq_changed);
    char expected[40];qso_clock_format(expected,sizeof(expected),preview_wall);
    assert(!strcmp(lv_label_get_text(ui.clock),expected));
    ++preview_wall;ticks(1);qso_clock_format(expected,sizeof(expected),preview_wall);
    assert(!strcmp(lv_label_get_text(ui.clock),expected));
    uint16_t speaker_pixels[16*16];
    for(int y=0;y<16;++y) memcpy(speaker_pixels+y*16,framebuffer+(15+y)*240+162,32);
    qso_ui_status(&ui,true,true,true,86,false);lv_label_set_text(ui.help,"MUTED");ticks(2);dump("muted");
    bool speaker_changed=false;
    for(int y=0;y<16;++y) if(memcmp(speaker_pixels+y*16,framebuffer+(15+y)*240+162,32)) speaker_changed=true;
    assert(speaker_changed && ui.audio_connected && ui.muted);
    lv_label_set_text(ui.help,"VOL-85%");
    qso_ui_status(&ui,true,true,false,86,true);ticks(2);dump("charging");
    uint16_t battery_pixels[36*15];
    for(int y=0;y<15;++y) memcpy(battery_pixels+y*36,framebuffer+(15+y)*240+188,72);
    ticks(6);bool battery_changed=false;
    for(int y=0;y<15;++y) if(memcmp(battery_pixels+y*36,framebuffer+(15+y)*240+188,72)) battery_changed=true;
    assert(battery_changed);
    qso_ui_status(&ui,true,true,false,100,false);ticks(2);dump("full-battery");
    qso_ui_live(&ui,false);ticks(8);
    for(unsigned i=0;i<QSO_EQ_BARS;++i)assert(ui.eq_levels[i]==0);
    lv_label_set_text(ui.selection,"TG 91");lv_label_set_text(ui.onair,"WAITING FOR QSO");
    lv_label_set_text(ui.info,"No active transmission");ticks(1);dump("idle");
    for(int y=0;y<42;++y)memcpy(eq_pixels+y*60,framebuffer+(eq_area.y1+y)*240+eq_area.x1,120);
    ticks(8);
    for(int y=0;y<42;++y)assert(!memcmp(eq_pixels+y*60,framebuffer+(eq_area.y1+y)*240+eq_area.x1,120));
    preview_wall=0;ticks(1);
    assert(!strcmp(lv_label_get_text(ui.clock),"--:--:-- --- --/--/--"));
    preview_wall=1792897445;ticks(1);
    uint16_t card_pixel=framebuffer[55*240+10];
    lv_area_t a;lv_obj_get_coords(ui.data_panel,&a);
    lv_area_t footer_area;lv_obj_get_coords(ui.help,&footer_area);
    assert(a.x1>=0&&a.x2<240&&a.y2<footer_area.y1);
    assert(footer_area.y1==307 && footer_area.y2<320);
    lv_area_t hint_area;lv_obj_get_coords(ui.hint_view,&hint_area);
    assert(hint_area.x1>footer_area.x2 && hint_area.y1==footer_area.y1 && hint_area.x2<240);
    assert(!lv_obj_has_flag(ui.hint_view,LV_OBJ_FLAG_OVERFLOW_VISIBLE));
    assert(lv_obj_get_parent(ui.hint_text)==ui.hint_view);
    uint16_t status_pixels[50*10],hint_pixels[150*10];
    for(int y=0;y<10;++y) {
        memcpy(status_pixels+y*50,framebuffer+(307+y)*240+16,100);
        memcpy(hint_pixels+y*150,framebuffer+(307+y)*240+74,300);
    }
    int hint_start=ui.hint_x;ticks(20);
    assert(ui.hint_x==-(int)(((unsigned)-hint_start+25)%(unsigned)ui.hint_cycle));
    /* One second advances 25 pixels left, independent of callback timing. */
    bool hint_changed=false;
    for(int y=0;y<10;++y) {
        assert(!memcmp(status_pixels+y*50,framebuffer+(307+y)*240+16,100));
        if(memcmp(hint_pixels+y*150,framebuffer+(307+y)*240+74,300))hint_changed=true;
    }
    assert(hint_changed);dump("footer-scroll");
    int hint_before_cycle=ui.hint_x;
    lv_tick_inc((unsigned)ui.hint_cycle*40);lv_timer_handler();lv_refr_now(NULL);
    assert(ui.hint_x==hint_before_cycle); /* Complete loop, even after a delayed callback. */
    qso_ui_page(&ui,QSO_PAGE_OPTIONS);
    assert(!lv_obj_has_flag(ui.status,LV_OBJ_FLAG_HIDDEN));
    assert(!lv_obj_has_flag(ui.help,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(ui.hint_view,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(ui.caller_flag_view,LV_OBJ_FLAG_HIDDEN));
    int hint_menu_x=ui.hint_x;
    lv_label_set_text(ui.title,"Options");lv_label_set_text(ui.status,"QSO VISION / SETTINGS");
    lv_label_set_text(ui.selection,"Choose a setting");
    lv_label_set_text(ui.directory,"> Change talkgroup\n  Wi-Fi connection\n  Screen brightness\n  Screen timeout\n  About");
    lv_label_set_text(ui.help,"UP/DOWN select  OK open\nHOLD OK return");
    qso_ui_menu(&ui);ticks(6);dump("options");
    assert(ui.hint_x==hint_menu_x);
    assert(framebuffer[55*240+10]!=card_pixel); /* Monitor card is absent here. */
    assert(lv_obj_has_flag(ui.eq,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_has_flag(ui.clock,LV_OBJ_FLAG_HIDDEN));
    qso_ui_page(&ui,QSO_PAGE_TALKGROUP);
    lv_label_set_text(ui.title,"Talkgroups");lv_label_set_text(ui.status,"PRESET DIRECTORY");
    lv_label_set_text(ui.selection,"CURRENT: TG 91");
    lv_label_set_text(ui.directory,"  TG 91\n  TG 45401\n  TG 46001\n  TG 46020\n  TG 50210\n> Other / Custom TG");
    qso_ui_menu(&ui);ticks(6);dump("talkgroups");
    assert(ui.menu_selected==5&&ui.cursor_y==135);
    qso_ui_page(&ui,QSO_PAGE_WIFI);
    lv_label_set_text(ui.title,"Wi-Fi networks");lv_label_set_text(ui.status,"Wi-Fi connected / 18 APs");
    lv_label_set_text(ui.selection,"SELECT 2/18: Home Radio");
    lv_label_set_text(ui.directory,"  RESCAN NETWORKS\n> Home Radio [SAVED]\n  Club shack [SAVED]\n  Home Radio Guest\n  A long network name");
    qso_ui_menu(&ui);ticks(6);dump("wifi");
    qso_ui_page(&ui,QSO_PAGE_PASSWORD);
    lv_label_set_text(ui.title,"Wi-Fi password");lv_label_set_text(ui.status,"Wi-Fi ready / 12 characters");
    lv_label_set_text(ui.selection,"SSID: Home Radio Guest");
    lv_label_set_text(ui.directory,"PASSWORD\nexample12345\n\nCHARACTER:  [m]");
    lv_label_set_text(ui.help,"UP/DOWN char  OK add\n2xUP delete  2xOK connect  HOLD OK back");
    ticks(6);dump("password");
    qso_ui_page(&ui,QSO_PAGE_CUSTOM);
    lv_label_set_text(ui.title,"Custom talkgroup");lv_label_set_text(ui.status,"Talkgroup range: 91-16777215");
    lv_label_set_text(ui.selection,"ENTER CUSTOM TG ID");lv_label_set_text(ui.directory,"TG: 46020\nDIGIT: 2");
    lv_label_set_text(ui.help,"UP/DOWN digit  OK add\n2xUP delete  2xOK save  HOLD OK back");
    ticks(6);dump("custom");
    qso_ui_page(&ui,QSO_PAGE_ABOUT);
    bm_wifi_info_t wifi={.connected=true,.signal_valid=true,.ipv4_valid=true,.rssi=-57};
    snprintf(wifi.ssid,sizeof(wifi.ssid),"A very long Wi-Fi SSID 1234567890");
    snprintf(wifi.ip,sizeof(wifi.ip),"192.0.2.27");
    snprintf(wifi.subnet,sizeof(wifi.subnet),"255.255.255.0");
    snprintf(wifi.gateway,sizeof(wifi.gateway),"192.0.2.1");
    snprintf(wifi.dns[0],sizeof(wifi.dns[0]),"192.0.2.1");
    snprintf(wifi.dns[1],sizeof(wifi.dns[1]),"2001:db8:1234:5678:9abc:def0:1234:5678");
    qso_ui_status(&ui,true,true,false,86,false);
    qso_ui_about(&ui,&wifi,86);ticks(6);dump("about");
    assert(lv_obj_get_parent(ui.directory)==ui.data_panel);
    assert(lv_obj_get_height(ui.directory)>lv_obj_get_height(ui.data_panel));
    assert(lv_obj_get_scroll_y(ui.data_panel)==0);
    assert(!strcmp(lv_label_get_text(ui.selection),"9W3SPY"));
    assert(strstr(lv_label_get_text(ui.directory),wifi.ssid));
    qso_ui_about_scroll(&ui,1);ticks(8);assert(lv_obj_get_scroll_y(ui.data_panel)>0);
    int32_t scrolled=lv_obj_get_scroll_y(ui.data_panel);
    wifi.rssi=-61;qso_ui_about(&ui,&wifi,85);qso_ui_status(&ui,true,true,false,85,false);ticks(2);
    assert(lv_obj_get_scroll_y(ui.data_panel)==scrolled);
    qso_ui_about_scroll(&ui,1);ticks(1);
    wifi.rssi=-62;qso_ui_about(&ui,&wifi,85);ticks(8);
    assert(lv_obj_get_scroll_y(ui.data_panel)==scrolled+48);
    dump("about-scrolled");
    for(unsigned i=0;i<30;++i){qso_ui_about_scroll(&ui,1);ticks(8);}
    assert(lv_obj_get_scroll_bottom(ui.data_panel)==0);dump("about-bottom");
    bm_wifi_info_t disconnected={0};qso_ui_about(&ui,&disconnected,85);ticks(2);
    assert(lv_obj_get_scroll_bottom(ui.data_panel)>=0);
    for(unsigned i=0;i<30;++i){qso_ui_about_scroll(&ui,-1);ticks(8);}
    assert(lv_obj_get_scroll_y(ui.data_panel)==0);
    qso_ui_live(&ui,true);ticks(2);
    qso_ui_display_enabled(&ui,false,100);
    unsigned phase=ui.phase;ticks(5);
    assert(backlight>0 && backlight<100);
    unsigned last=backlight;
    for(unsigned i=0;i<15;++i){ticks(1);assert(backlight<=last);last=backlight;}
    assert(backlight==0 && ui.phase==phase && ui.display_off);
    qso_ui_display_enabled(&ui,true,100);ticks(5);
    assert(backlight>0 && backlight<100 && ui.phase!=phase && !ui.display_off);
    last=backlight;
    for(unsigned i=0;i<15;++i){ticks(1);assert(backlight>=last);last=backlight;}
    assert(backlight==100);
    qso_ui_display_enabled(&ui,false,100);ticks(4);int partial=ui.brightness;
    qso_ui_display_enabled(&ui,true,100);assert(ui.brightness==partial);ticks(20);assert(backlight==100);
    qso_ui_brightness(&ui,37);ticks(20);assert(backlight==37);
    qso_ui_brightness(&ui,1);ticks(8);assert(backlight>1 && backlight<37);
    qso_ui_brightness(&ui,1);ticks(12);assert(backlight==1);
    qso_ui_display_enabled(&ui,false,37);ticks(20);assert(backlight==0);
    qso_ui_brightness(&ui,37);ticks(20);assert(backlight==0);
    qso_ui_display_enabled(&ui,true,37);
    for(unsigned i=0;i<20;++i){ticks(1);assert(backlight<=37);}
    assert(backlight==37);
    /* A QSO or a key restores to the preset, never an intermediate 100%. */
    qso_ui_brightness(&ui,1);ticks(20);assert(backlight==1);
    qso_ui_brightness(&ui,37);
    for(unsigned i=0;i<20;++i){ticks(1);assert(backlight<=37);}
    assert(backlight==37);
    /* Even a 1% preset survives explicit off/wake. */
    qso_ui_display_enabled(&ui,false,1);ticks(20);assert(backlight==0);
    qso_ui_display_enabled(&ui,true,1);ticks(20);assert(backlight==1);
    qso_ui_page(&ui,QSO_PAGE_SCREEN_TIMEOUT);
    lv_label_set_text(ui.title,"Screen timeout");
    lv_label_set_text(ui.status,"AUDIO KEEPS PLAYING");
    lv_label_set_text(ui.selection,"Choose dimming rule");
    lv_label_set_text(ui.directory,"  After 1 minute\n  After 5 minutes\n  After 10 minutes\n> Smart: idle 5 seconds\n  Never");
    lv_label_set_text(ui.help,"UP/DOWN select  OK apply\nDIM TO 1%  HOLD OK back");
    qso_ui_menu(&ui);ticks(8);dump("screen-timeout");
    assert(ui.menu_selected==3);
    qso_ui_page(&ui,QSO_PAGE_BRIGHTNESS);
    lv_label_set_text(ui.title,"Brightness");
    lv_label_set_text(ui.selection,"BRIGHTNESS: 37%");
    lv_label_set_text(ui.directory,"Awake brightness: 1-100%\n\nUP/DOWN adjust 1%\nHOLD UP/DOWN faster\nOK save permanently");
    lv_label_set_text(ui.help,"Timeout dims to 1%, wakes to this\nHOLD OK cancel unsaved changes");
    ticks(8);dump("screen-brightness");
    qso_ui_live(&ui,false);ticks(2);
    /* A stable battery must not pulse when no charging signal is supplied. */
    qso_ui_status(&ui,true,true,false,78,false);ticks(2);
    for(int y=0;y<15;++y)memcpy(battery_pixels+y*36,framebuffer+(15+y)*240+188,72);
    ticks(30);
    for(int y=0;y<15;++y)assert(!memcmp(battery_pixels+y*36,framebuffer+(15+y)*240+188,72));
    assert(qso_battery_color(10)==0xFF4D60 && qso_battery_color(0)==0xFF4D60);
    assert(qso_battery_color(20)==0xFFD44D && qso_battery_color(11)==0xFFD44D);
    assert(qso_battery_color(21)==0x56EBAC && qso_battery_color(-1)==0x93B8C8);
    assert(lv_obj_get_style_text_font(ui.battery_text,0)==&qso_bold_11);
    lv_point_t percent_size;
    lv_text_get_size(&percent_size,"100%",&qso_bold_11,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    assert(percent_size.x<=31);
    assert(lv_font_get_line_height(&qso_text_12)>lv_font_get_line_height(&lv_font_montserrat_8));
    qso_ui_page(&ui,QSO_PAGE_ABOUT);
    memset(&wifi,0,sizeof(wifi));qso_ui_about(&ui,&wifi,-1);ticks(3);dump("about-offline");
    assert(strstr(lv_label_get_text(ui.directory),"Disconnected"));
    assert(!strstr(lv_label_get_text(ui.directory),"192.0.2.27"));
    qso_ui_page(&ui,QSO_PAGE_MONITOR);qso_ui_history(&ui,history,5,"CONNECTED");ticks(6);
    assert(!lv_obj_has_flag(ui.data_panel,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_parent(ui.directory)==ui.body);
    qso_ui_live(&ui,true);qso_ui_current_country(&ui,"Unknown");ticks(1);
    assert(lv_obj_has_flag(ui.caller_flag_view,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_x(ui.selection)==16 && lv_obj_get_width(ui.selection)==208);
    qso_ui_current_country(&ui,"Hong Kong");ticks(1);
    assert(ui.caller_flag==qso_flag_image("HK"));
    qso_ui_live(&ui,false);ticks(1);
    assert(lv_obj_has_flag(ui.caller_flag_view,LV_OBJ_FLAG_HIDDEN));
    assert(lv_obj_get_scroll_y(ui.data_panel)==0);
    assert(lv_obj_has_flag(ui.status,LV_OBJ_FLAG_HIDDEN));
    lv_label_set_text(ui.title,"MONITORING TALK GROUP 99999999");
    lv_label_set_text(ui.selection,"TG 99999999");
    lv_point_t title_size;
    lv_text_get_size(&title_size,lv_label_get_text(ui.title),&lv_font_montserrat_10,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    assert(title_size.x<=208);
    qso_ui_status(&ui,false,false,false,12,false);ticks(3);dump("offline");
    lv_mem_monitor_t mem;lv_mem_monitor(&mem);
    printf("UI preview: PASS; LVGL peak=%u, free=%u, largest=%u (24 KB pool)\n",
        (unsigned)mem.max_used,(unsigned)mem.free_size,(unsigned)mem.free_biggest_size);
    return 0;
}
