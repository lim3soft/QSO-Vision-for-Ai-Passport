#include "qso_ui.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#define BG 0x06111F
#define INK 0xFFFFFF
#define MUTED 0x93B8C8
#define CYAN 0x53DEEF
#define LIME 0xD3FA73
#define RED 0xF76F85
#define RULE 0x244864
#define ORANGE 0xFF8A18
#define GREEN 0x56EBAC

static lv_obj_t *plain(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *obj = lv_obj_create(parent);
    if (!obj) return NULL;
    lv_obj_remove_style_all(obj);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_scrollable(obj, false);
    return obj;
}

static lv_obj_t *label(lv_obj_t *parent, int x, int y, int w,
                        const lv_font_t *font, uint32_t color, const char *text)
{
    lv_obj_t *obj = lv_label_create(parent);
    if (!obj) return NULL;
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_text_line_space(obj, 1, 0);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_width(obj, w);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
    lv_label_set_text(obj, text);
    return obj;
}

static void stroke(lv_layer_t *layer, int x1, int y1, int x2, int y2, uint32_t color, int width)
{
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.p1 = (lv_point_precise_t){x1,y1}; d.p2 = (lv_point_precise_t){x2,y2};
    d.color = lv_color_hex(color); d.width = width;
    d.round_start = d.round_end = 1;
    lv_draw_line(layer, &d);
}

static void arc(lv_layer_t *layer, int x, int y, int radius, int start, int end, uint32_t color)
{
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.center = (lv_point_t){x,y}; d.radius = radius;
    d.start_angle = start; d.end_angle = end;
    d.width = 2; d.rounded = 1; d.color = lv_color_hex(color);
    lv_draw_arc(layer, &d);
}

static void wifi_draw(lv_event_t *event)
{
    qso_ui_t *ui = lv_event_get_user_data(event);
    lv_area_t a; lv_obj_get_coords(ui->wifi_icon, &a);
    lv_layer_t *layer = lv_event_get_layer(event);
    uint32_t color = ui->wifi_connected ? GREEN : RED;
    arc(layer,a.x1+7,a.y1+12,9,225,315,color);
    arc(layer,a.x1+7,a.y1+12,5,225,315,color);
    stroke(layer,a.x1+7,a.y1+12,a.x1+7,a.y1+12,color,2);
    if (!ui->wifi_connected) stroke(layer,a.x1+2,a.y1+2,a.x1+13,a.y1+13,RED,1);
}

static void speaker_draw(lv_event_t *event)
{
    qso_ui_t *ui = lv_event_get_user_data(event);
    lv_area_t a; lv_obj_get_coords(ui->audio_icon,&a);
    lv_layer_t *layer = lv_event_get_layer(event);
    uint32_t c = ui->audio_connected ? GREEN : RED;
    int x=a.x1+1, y=a.y1+7;
    stroke(layer,x,y-2,x,y+2,c,2); stroke(layer,x,y-2,x+3,y-2,c,2);
    stroke(layer,x,y+2,x+3,y+2,c,2); stroke(layer,x+3,y-2,x+5,y-5,c,2);
    stroke(layer,x+3,y+2,x+5,y+5,c,2); stroke(layer,x+5,y-5,x+5,y+5,c,2);
    if (ui->audio_connected && !ui->muted) {
        arc(layer,x+5,y,5,315,45,c); arc(layer,x+5,y,8,315,45,c);
    } else {
        stroke(layer,x+9,y-2,x+13,y+2,c,1);
        stroke(layer,x+9,y+2,x+13,y-2,c,1);
    }
}

static void battery_draw(lv_event_t *event)
{
    qso_ui_t *ui = lv_event_get_user_data(event);
    lv_area_t a; lv_obj_get_coords(ui->battery_icon,&a);
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_opa = LV_OPA_TRANSP; d.border_width = 1; d.radius = 2;
    uint32_t c = qso_battery_color(ui->battery_percent);
    d.border_color = lv_color_hex(c);
    lv_area_t body = {a.x1,a.y1+1,a.x1+32,a.y1+13};
    lv_draw_rect(lv_event_get_layer(event),&d,&body);
    if (ui->battery_percent > 0) {
        d.border_width = 0;
        d.bg_opa = ui->charging && (ui->phase / 10) % 2 ? LV_OPA_50 : LV_OPA_20;
        d.bg_color = lv_color_hex(c); d.radius = 1;
        int fill = ui->charging ? 1+(ui->phase/3)%29 : ui->battery_percent*29/100;
        body = (lv_area_t){a.x1+2,a.y1+3,a.x1+1+fill,a.y1+11};
        if(fill>0) lv_draw_rect(lv_event_get_layer(event),&d,&body);
    }
    stroke(lv_event_get_layer(event),a.x1+34,a.y1+5,a.x1+34,a.y1+9,c,2);
}

static void move_x(void *obj, int32_t x) { lv_obj_set_x(obj,x); }

static void draw_text(lv_layer_t *layer, int x, int y, int width,
                       const lv_font_t *font, uint32_t color, const char *text)
{
    lv_draw_label_dsc_t d; lv_draw_label_dsc_init(&d);
    d.font=font;d.color=lv_color_hex(color);d.text=text;d.text_local=1;
    lv_area_t a={x,y,x+width-1,y+lv_font_get_line_height(font)-1};
    lv_draw_label(layer,&d,&a);
}

static void brand_draw(lv_event_t *event)
{
    lv_obj_t *obj=lv_event_get_target(event);
    lv_area_t a;lv_obj_get_coords(obj,&a);
    lv_layer_t *layer=lv_event_get_layer(event);
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);
    d.radius=LV_RADIUS_CIRCLE;d.bg_color=lv_color_hex(0x32DEF5);
    lv_area_t r={a.x1,a.y1+2,a.x1+12,a.y1+14};lv_draw_rect(layer,&d,&r);
    d.bg_color=lv_color_hex(BG);
    r=(lv_area_t){a.x1+2,a.y1+4,a.x1+10,a.y1+12};lv_draw_rect(layer,&d,&r);
    d.bg_color=lv_color_hex(0x32DEF5);
    r=(lv_area_t){a.x1+4,a.y1+6,a.x1+8,a.y1+10};lv_draw_rect(layer,&d,&r);
    draw_text(layer,a.x1+18,a.y1,35,&qso_bold_14,INK,"QSO");
    draw_text(layer,a.x1+52,a.y1,69,&qso_bold_14,0x52EDFA,"VISION");
}

static void card(lv_layer_t *layer, const lv_area_t *area, uint32_t border)
{
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);
    d.radius=12;d.border_width=1;d.border_color=lv_color_hex(border);
    d.bg_color=lv_color_hex(0x103949);
    d.bg_grad.dir=LV_GRAD_DIR_VER;d.bg_grad.stops_count=2;
    d.bg_grad.stops[0].color=lv_color_hex(0x103949);
    d.bg_grad.stops[0].opa=LV_OPA_COVER;d.bg_grad.stops[0].frac=0;
    d.bg_grad.stops[1].color=lv_color_hex(0x0D2438);
    d.bg_grad.stops[1].opa=LV_OPA_COVER;d.bg_grad.stops[1].frac=255;
    lv_draw_rect(layer,&d,area);
}

static void body_draw(lv_event_t *event)
{
    qso_ui_t *ui=lv_event_get_user_data(event);
    if(ui->page!=QSO_PAGE_MONITOR) return;
    lv_area_t a;lv_obj_get_coords(ui->body,&a);
    lv_area_t monitor={a.x1+8,a.y1+3,a.x1+231,a.y1+112};
    lv_area_t history={a.x1+8,a.y1+118,a.x1+231,a.y1+255};
    /* Website channel-one: #103949 -> #0d2438, #297d91 border.
     * A simple opaque gradient needs no whole-card/framebuffer layer. */
    card(lv_event_get_layer(event),&monitor,ui->live?CYAN:0x297D91);
    card(lv_event_get_layer(event),&history,0x297D91);
}

static void data_draw(lv_event_t *event)
{
    qso_ui_t *ui=lv_event_get_user_data(event);
    lv_area_t a;lv_obj_get_coords(ui->data_panel,&a);
    lv_layer_t *layer=lv_event_get_layer(event);
    if(ui->page==QSO_PAGE_ABOUT) return; /* Real scrollable child label. */
    if(ui->page==QSO_PAGE_MONITOR) {
        if(!ui->history_count) {
            draw_text(layer,a.x1,a.y1+12,208,&lv_font_montserrat_10,MUTED,ui->history_empty);
            return;
        }
        for(size_t i=0;i<ui->history_count;++i) {
            const bm_qso_info_t *row=&ui->history[i];
            int y=a.y1+(int)i*24;
            lv_draw_rect_dsc_t bubble;lv_draw_rect_dsc_init(&bubble);
            bubble.radius=5;bubble.bg_color=lv_color_hex(0x0B1D30);
            bubble.border_width=1;bubble.border_color=lv_color_hex(0x25485E);
            lv_area_t bounds={a.x1,y,a.x2,y+22};lv_draw_rect(layer,&bubble,&bounds);
            /* Orange caller and cyan country remain distinct on the dark bubble. */
            lv_point_t measured;
            lv_text_get_size(&measured,row->callsign,&qso_bold_12,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
            int call_width=measured.x;
            if(call_width>114) call_width=114;
            draw_text(layer,a.x1+5,y,call_width+1,&qso_bold_12,ORANGE,row->callsign);
            char country[40];snprintf(country,sizeof(country)," . %s",row->country[0]?row->country:"Unknown");
            draw_text(layer,a.x1+call_width+7,y+1,196-call_width,&lv_font_montserrat_10,CYAN,country);
            struct tm tm;time_t epoch=(time_t)(row->started_us/1000000);localtime_r(&epoch,&tm);
            unsigned s=row->ended_us>row->started_us?(unsigned)((row->ended_us-row->started_us)/1000000):0;
            char date[48],clock[40],duration[16];
            snprintf(date,sizeof(date),"%02d/%02d/%04d",tm.tm_mday,tm.tm_mon+1,tm.tm_year+1900);
            snprintf(clock,sizeof(clock),"%02d:%02d:%02d",tm.tm_hour,tm.tm_min,tm.tm_sec);
            snprintf(duration,sizeof(duration),"%um%02us",s/60,s%60);
            draw_text(layer,a.x1+5,y+11,77,&qso_text_11,INK,date);
            draw_text(layer,a.x1+85,y+11,65,&qso_text_11,INK,clock);
            draw_text(layer,a.x1+155,y+11,48,&qso_text_11,INK,duration);
        }
        return;
    }
    if(ui->menu_selected>=0) {
        lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);
        d.bg_color=lv_color_hex(0x142D35);d.radius=5;
        lv_area_t r={a.x1,a.y1+ui->cursor_y,a.x2,a.y1+ui->cursor_y+24};
        lv_draw_rect(layer,&d,&r);
        stroke(layer,a.x1+3,a.y1+ui->cursor_y+6,a.x1+3,a.y1+ui->cursor_y+18,CYAN,2);
    }
    const char *text=lv_label_get_text(ui->directory);
    for(unsigned i=0;i<6&&*text;++i) {
        char row[100];size_t n=0;
        while(text[n]&&text[n]!='\n'&&n<sizeof(row)-1){row[n]=text[n];++n;}
        row[n]=0;text+=n;if(*text=='\n')++text;
        bool chosen=row[0]=='>';
        const char *display=row;
        if(row[0]=='>'||row[0]==' '){++display;if(*display==' ')++display;}
        /* Clip long list names here; the selected SSID scrolls in the heading. */
        char compact[32];snprintf(compact,sizeof(compact),"%.25s",display);
        draw_text(layer,a.x1+11,a.y1+4+(int)i*27,194,&qso_text_16,chosen?CYAN:INK,compact);
    }
}

static void cursor_move(void *ptr,int32_t y)
{
    qso_ui_t *ui=ptr;ui->cursor_y=y;lv_obj_invalidate(ui->data_panel);
}

static time_t system_clock_now(void) { return time(NULL); }

static void eq_draw(lv_event_t *event)
{
    qso_ui_t *ui=lv_event_get_user_data(event);
    lv_area_t area;lv_obj_get_coords(ui->eq,&area);
    lv_layer_t *layer=lv_event_get_layer(event);
    lv_draw_rect_dsc_t d;lv_draw_rect_dsc_init(&d);d.radius=1;
    for(unsigned i=0;i<QSO_EQ_BARS;++i) {
        for(unsigned j=0;j<QSO_EQ_SEGMENTS;++j) {
            bool lit=j<ui->eq_levels[i];
            d.bg_color=lv_color_hex(lit?(j<4?0xFFB52E:j<8?0xFF8B28:0xF64DA9):0x25485E);
            d.bg_opa=lit?LV_OPA_COVER:LV_OPA_20;
            int x=area.x1+2+(int)i*6,y=area.y2-2-(int)j*3;
            lv_area_t segment={x,y-1,x+3,y};
            lv_draw_rect(layer,&d,&segment);
        }
    }
}

static void activity_tick(lv_timer_t *timer)
{
    qso_ui_t *ui=lv_timer_get_user_data(timer);
    ++ui->phase;
    if(ui->page==QSO_PAGE_MONITOR) {
        if(qso_eq_step(ui->eq_levels,ui->phase,ui->live))lv_obj_invalidate(ui->eq);
        time_t now=ui->clock_now();
        if(now!=ui->clock_second) {
            char text[40];qso_clock_format(text,sizeof(text),now);
            ui->clock_second=now;
            lv_label_set_text(ui->clock,text);
        }
    }
    if(ui->charging && ui->phase%5==0)lv_obj_invalidate(ui->battery_icon);
}

bool qso_ui_create(qso_ui_t *ui)
{
    memset(ui,0,sizeof(*ui));
    ui->page=(qso_page_t)-1; ui->battery_percent=-1; ui->menu_selected=-1;
    ui->brightness=ui->brightness_target=100;
    ui->clock_second=(time_t)-1;ui->clock_now=system_clock_now;
    ui->screen=lv_obj_create(NULL);
    if (!ui->screen) return false;
    lv_obj_remove_style_all(ui->screen);
    lv_obj_set_size(ui->screen,240,320);
    lv_obj_set_style_bg_color(ui->screen,lv_color_hex(BG),0);
    lv_obj_set_style_bg_opa(ui->screen,LV_OPA_COVER,0);
    lv_obj_set_scrollable(ui->screen,false);
    lv_obj_t *brand=plain(ui->screen,16,14,119,19);
    if(!brand) return false;
    lv_obj_add_event_cb(brand,brand_draw,LV_EVENT_DRAW_MAIN,NULL);
    lv_obj_t *rule=plain(ui->screen,16,37,208,1);
    if (!rule) return false;
    lv_obj_set_style_bg_color(rule,lv_color_hex(RULE),0); lv_obj_set_style_bg_opa(rule,LV_OPA_COVER,0);
    ui->wifi_icon=plain(ui->screen,139,15,16,16);
    ui->audio_icon=plain(ui->screen,162,15,16,16);
    ui->battery_icon=plain(ui->screen,188,15,36,15);
    if (!ui->wifi_icon||!ui->audio_icon||!ui->battery_icon) return false;
    lv_obj_add_event_cb(ui->wifi_icon,wifi_draw,LV_EVENT_DRAW_MAIN,ui);
    lv_obj_add_event_cb(ui->audio_icon,speaker_draw,LV_EVENT_DRAW_MAIN,ui);
    lv_obj_add_event_cb(ui->battery_icon,battery_draw,LV_EVENT_DRAW_MAIN,ui);
    ui->battery_text=label(ui->battery_icon,1,4,31,&qso_bold_11,INK,"--%");
    lv_obj_set_style_text_align(ui->battery_text,LV_TEXT_ALIGN_CENTER,0);
    ui->body=plain(ui->screen,0,38,240,258);
    if (!ui->body) return false;
    lv_obj_add_event_cb(ui->body,body_draw,LV_EVENT_DRAW_MAIN,ui);
    ui->title=label(ui->body,16,10,208,&lv_font_montserrat_10,CYAN,"MONITORING TALK GROUP 91");
    ui->status=label(ui->body,16,24,208,&lv_font_montserrat_10,MUTED,"Connecting...");
    ui->selection=label(ui->body,16,27,208,&qso_bold_28,ORANGE,"TG 91");
    ui->onair=label(ui->body,16,60,208,&lv_font_montserrat_10,CYAN,"WAITING FOR QSO");
    ui->info=label(ui->body,16,74,208,&lv_font_montserrat_10,INK,"");
    lv_obj_set_height(ui->info,24);
    lv_label_set_long_mode(ui->info,LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ui->info,142);
    ui->clock=label(ui->body,16,98,142,&qso_text_11,CYAN,"--:--:-- --- --/--/--");
    if(!ui->clock)return false;
    lv_label_set_long_mode(ui->clock,LV_LABEL_LONG_CLIP);
    ui->eq=plain(ui->body,164,67,60,42);
    if(!ui->eq)return false;
    lv_obj_add_event_cb(ui->eq,eq_draw,LV_EVENT_DRAW_MAIN,ui);
    ui->history_heading=label(ui->body,16,124,208,&lv_font_montserrat_10,CYAN,"RECENT QSOs / BRANDMEISTER");
    /* Shared menu text is a model label. List pages render bounded rows from it. */
    ui->directory=label(ui->body,16,88,208,&lv_font_montserrat_12,INK,"");
    lv_obj_set_height(ui->directory,166);
    lv_label_set_long_mode(ui->directory,LV_LABEL_LONG_WRAP);
    ui->data_panel=plain(ui->body,16,133,208,122);
    if (!ui->data_panel) return false;
    lv_obj_add_event_cb(ui->data_panel,data_draw,LV_EVENT_DRAW_MAIN,ui);
    ui->help=label(ui->screen,16,296,208,&lv_font_montserrat_8,MUTED,"");
    lv_obj_set_style_text_line_space(ui->help,0,0);
    lv_obj_set_height(ui->help,20); lv_label_set_long_mode(ui->help,LV_LABEL_LONG_WRAP);
    if (!ui->title||!ui->status||!ui->selection||!ui->onair||!ui->info||!ui->directory||!ui->help||!ui->battery_text) return false;
    ui->timer=lv_timer_create(activity_tick,50,ui);
    if (!ui->timer) return false;
    qso_ui_page(ui,QSO_PAGE_MONITOR);
    qso_ui_menu(ui);
    lv_screen_load(ui->screen);
    return true;
}

void qso_ui_page(qso_ui_t *ui, qso_page_t page)
{
    bool changed=ui->page!=page;
    if (!changed) return;
    ui->page=page;
    bool monitor=page==QSO_PAGE_MONITOR;
    bool about=page==QSO_PAGE_ABOUT;
    bool list=page==QSO_PAGE_OPTIONS||page==QSO_PAGE_TALKGROUP||page==QSO_PAGE_WIFI||page==QSO_PAGE_SCREEN_TIMEOUT;
    /* Reuse the directory label inside a clipped viewport for About. */
    lv_obj_set_parent(ui->directory,about?ui->data_panel:ui->body);
    lv_obj_set_pos(ui->directory,about?0:16,about?0:88);
    lv_obj_set_width(ui->directory,about?200:208);
    lv_obj_set_height(ui->directory,about?LV_SIZE_CONTENT:166);
    lv_obj_set_style_text_line_space(ui->directory,about?3:1,0);
    lv_obj_set_scrollable(ui->data_panel,about);
    lv_obj_set_scroll_dir(ui->data_panel,LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui->data_panel,about?LV_SCROLLBAR_MODE_AUTO:LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(ui->data_panel,lv_color_hex(CYAN),LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(ui->data_panel,LV_OPA_60,LV_PART_SCROLLBAR);
    lv_obj_set_style_width(ui->data_panel,2,LV_PART_SCROLLBAR);
    lv_obj_scroll_to_y(ui->data_panel,0,LV_ANIM_OFF);
    lv_obj_set_style_text_font(ui->title,monitor?&lv_font_montserrat_10:&lv_font_montserrat_20,0);
    lv_obj_set_style_text_font(ui->selection,monitor?&qso_bold_28:about?&qso_bold_14:&qso_text_14,0);
    lv_obj_set_style_text_font(ui->status,monitor?&lv_font_montserrat_10:&qso_text_12,0);
    lv_obj_set_y(ui->title,monitor?10:6);
    lv_obj_set_hidden(ui->status,monitor);
    lv_obj_set_y(ui->status,monitor?24:36);
    lv_obj_set_y(ui->selection,monitor?27:55);
    lv_obj_set_style_text_color(ui->selection,lv_color_hex(monitor||about?ORANGE:CYAN),0);
    lv_label_set_long_mode(ui->selection,LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_hidden(ui->onair,!monitor); lv_obj_set_hidden(ui->info,!monitor);
    lv_obj_set_hidden(ui->eq,!monitor);lv_obj_set_hidden(ui->clock,!monitor); lv_obj_set_hidden(ui->history_heading,!monitor);
    lv_obj_set_hidden(ui->data_panel,!monitor&&!list&&!about);
    lv_obj_set_pos(ui->data_panel,monitor||about?16:12,monitor?133:86);
    lv_obj_set_size(ui->data_panel,monitor||about?208:216,monitor?122:166);
    lv_obj_set_hidden(ui->directory,monitor||list);
    lv_obj_set_style_text_font(ui->directory,&qso_text_14,0);
    lv_obj_invalidate(ui->body);
    if(changed) {
        /* Move existing widgets only: group opacity/transforms would require
         * an intermediate framebuffer which is too expensive on this device. */
        lv_anim_delete(ui->body,move_x);
        lv_anim_t a; lv_anim_init(&a); lv_anim_set_var(&a,ui->body);
        lv_anim_set_exec_cb(&a,move_x); lv_anim_set_values(&a,10,0);
        lv_anim_set_duration(&a,180); lv_anim_set_path_cb(&a,lv_anim_path_ease_out);
        lv_anim_start(&a);
    }
}

void qso_ui_status(qso_ui_t *ui, bool wifi, bool audio, bool muted, int battery, bool charging)
{
    ui->wifi_connected=wifi; ui->audio_connected=audio; ui->muted=muted; ui->charging=charging;
    ui->battery_percent=battery>100?100:battery;
    if(battery<0) lv_label_set_text(ui->battery_text,"--%");
    else lv_label_set_text_fmt(ui->battery_text,"%d%%",ui->battery_percent);
    lv_obj_invalidate(ui->wifi_icon); lv_obj_invalidate(ui->audio_icon); lv_obj_invalidate(ui->battery_icon);
}

void qso_ui_live(qso_ui_t *ui, bool active)
{
    if(ui->live!=active)lv_obj_invalidate(ui->body);
    ui->live=active;
    /* The lightweight timer settles existing bars to zero on QSO end. */
}

void qso_ui_about(qso_ui_t *ui, const bm_wifi_info_t *wifi, int battery)
{
    if(ui->page!=QSO_PAGE_ABOUT) return;
    char signal[24],level[24];
    if(wifi && wifi->signal_valid) snprintf(signal,sizeof(signal),"%d dBm",wifi->rssi);
    else snprintf(signal,sizeof(signal),"Unavailable");
    if(battery>=0) snprintf(level,sizeof(level),"%d%%",battery);
    else snprintf(level,sizeof(level),"Unavailable");
    char text[1000];
    snprintf(text,sizeof(text),
        "WI-FI / DEVICE\nWi-Fi: %s\nSSID: %s\nSignal: %s\nIP: %s\nSubnet: %s\nGateway: %s\nDNS 1: %s\nDNS 2: %s\nDNS 3: %s\nBattery: %s\n\n"
        "ABOUT / LICENSES\nQSO Vision for AI Passport\nBuild: 2.3 / ESP-IDF 5.5.3\n\n"
        "Independent, receive-only app.\nBrandMeister/Hoseline APIs, data and trademarks belong to their respective owners. "
        "Access is subject to their terms. No affiliation or endorsement. "
        "Service availability is not guaranteed. Use where permitted.",
        !wifi?"Reading...":wifi->connected?"Connected":"Disconnected",
        wifi&&wifi->connected?wifi->ssid:"--", signal,
        wifi&&wifi->ipv4_valid?wifi->ip:"--",wifi&&wifi->ipv4_valid?wifi->subnet:"--",
        wifi&&wifi->ipv4_valid?wifi->gateway:"--",
        wifi&&wifi->dns[0][0]?wifi->dns[0]:"Not configured",
        wifi&&wifi->dns[1][0]?wifi->dns[1]:"Not configured",
        wifi&&wifi->dns[2][0]?wifi->dns[2]:"Not configured",level);
    if(strcmp(text,lv_label_get_text(ui->directory))) {
        int32_t y=lv_obj_get_scroll_y(ui->data_panel);
        lv_label_set_text(ui->directory,text);
        lv_obj_update_layout(ui->data_panel);
        /* scroll_to_y cancels an in-flight key animation even at the same y.
         * A signal-strength update must not swallow the user's scroll step. */
        int32_t bottom=lv_obj_get_scroll_bottom(ui->data_panel);
        if(bottom<0)
            lv_obj_scroll_to_y(ui->data_panel,lv_obj_get_scroll_y(ui->data_panel)+bottom,LV_ANIM_OFF);
        else if(lv_obj_get_scroll_y(ui->data_panel)!=y)
            lv_obj_scroll_to_y(ui->data_panel,y,LV_ANIM_OFF);
    }
    lv_label_set_text(ui->title,"About QSO Vision");
    lv_label_set_text(ui->status,"Developed by");
    lv_label_set_text(ui->selection,"9W3SPY");
    lv_label_set_text(ui->help,"UP/DOWN scroll  LONG OK back");
}

void qso_ui_about_scroll(qso_ui_t *ui, int direction)
{
    if(ui->page==QSO_PAGE_ABOUT)
        lv_obj_scroll_by_bounded(ui->data_panel,0,direction>0?-48:48,LV_ANIM_ON);
}

static void fade_brightness(void *ptr, int32_t value)
{
    qso_ui_t *ui=ptr;ui->brightness=value;
    if(ui->backlight)ui->backlight((uint8_t)value);
}

void qso_ui_set_backlight(qso_ui_t *ui, void (*callback)(uint8_t)) { ui->backlight=callback; }

void qso_ui_display_enabled(qso_ui_t *ui, bool enabled, unsigned awake_percent)
{
    if(awake_percent<1)awake_percent=1;
    if(awake_percent>100)awake_percent=100;
    ui->display_off=!enabled;
    if(enabled) {
        lv_timer_resume(ui->timer);
        lv_obj_invalidate(ui->screen);
    } else lv_timer_pause(ui->timer);
    ui->brightness_target=enabled?(int)awake_percent:0;
    lv_anim_delete(ui,fade_brightness);
    lv_anim_t a;lv_anim_init(&a);lv_anim_set_var(&a,ui);
    lv_anim_set_exec_cb(&a,fade_brightness);
    lv_anim_set_values(&a,ui->brightness,ui->brightness_target);
    lv_anim_set_duration(&a,900);lv_anim_set_path_cb(&a,lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

void qso_ui_brightness(qso_ui_t *ui, unsigned percent)
{
    if(ui->display_off)return; /* Explicit screen-off always wins. */
    if(percent>100)percent=100;
    if(ui->brightness_target==(int)percent)return;
    ui->brightness_target=(int)percent;
    lv_anim_delete(ui,fade_brightness);
    lv_anim_t a;lv_anim_init(&a);lv_anim_set_var(&a,ui);
    lv_anim_set_exec_cb(&a,fade_brightness);
    lv_anim_set_values(&a,ui->brightness,(int)percent);
    lv_anim_set_duration(&a,900);lv_anim_set_path_cb(&a,lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}
void qso_ui_history_position(qso_ui_t *ui, size_t offset, size_t total)
{
    size_t first=ui->history_count?offset+1:0;
    lv_label_set_text_fmt(ui->history_heading,"RECENT QSOs  /  %u-%u OF %u",
        (unsigned)first,(unsigned)(offset+ui->history_count),(unsigned)total);
}

void qso_ui_history(qso_ui_t *ui, const bm_qso_info_t *history, size_t count, const char *state)
{
    if(count>5) count=5;
    lv_label_set_text_fmt(ui->history_heading,"RECENT QSOs  /  %u",(unsigned)count);
    ui->history_count=count;
    if(count)memcpy(ui->history,history,count*sizeof(*history));
    snprintf(ui->history_empty,sizeof(ui->history_empty),"%s",!strcmp(state,"CONNECTED")?
        "No recent QSOs on this TG.":"Loading BrandMeister history...");
    lv_obj_invalidate(ui->data_panel);
}

void qso_ui_menu(qso_ui_t *ui)
{
    bool list=ui->page==QSO_PAGE_OPTIONS||ui->page==QSO_PAGE_TALKGROUP||ui->page==QSO_PAGE_WIFI||ui->page==QSO_PAGE_SCREEN_TIMEOUT;
    if(!list) return;
    const char *text=lv_label_get_text(ui->directory);
    int selected=-1;
    for(unsigned i=0;i<6;++i) {
        char row[100]; size_t n=0;
        while(text[n]&&text[n]!='\n'&&n<sizeof(row)-1) {row[n]=text[n];++n;}
        row[n]=0; text+=n; if(*text=='\n')++text;
        bool chosen=row[0]=='>';
        if(chosen) selected=(int)i;
    }
    if(selected>=0&&selected!=ui->menu_selected) {
        lv_anim_delete(ui,cursor_move);
        lv_anim_t a;lv_anim_init(&a);lv_anim_set_var(&a,ui);lv_anim_set_exec_cb(&a,cursor_move);
        lv_anim_set_values(&a,ui->cursor_y,selected*27);
        lv_anim_set_duration(&a,110);lv_anim_set_path_cb(&a,lv_anim_path_ease_out);lv_anim_start(&a);
    }
    ui->menu_selected=selected;
    lv_obj_invalidate(ui->data_panel);
}
