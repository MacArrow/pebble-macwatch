#include "pebble.h"
#include "watch.h"

#define WATCH_MAX_CALLBACK_NUM 5

static TextLayer *text_date_layer;
static TextLayer *text_time_layer;

typedef struct {
  TickHandler cb;
  TimeUnits tu;
  uint32_t in;
} watch_callback_t;

static watch_callback_t callbacks[WATCH_MAX_CALLBACK_NUM] = { };

static void handle_day_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char date_text[] = "Xxxxxxxxx 00";
  
  strftime(date_text, sizeof(date_text), "%a, %b %e", tick_time);
  text_layer_set_text(text_date_layer, date_text);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char time_text[] = "00:00";
  char *time_format;

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Kludge to handle lack of non-padded hour format string
  // for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  }

  text_layer_set_text(text_time_layer, time_text);
 
  for(int i = 0; i < WATCH_MAX_CALLBACK_NUM; i++) {
    if (NULL != callbacks[i].cb) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "checking callback %d", i);
      switch(callbacks[i].tu) {
        case SECOND_UNIT:
          if (0 == (tick_time->tm_sec % callbacks[i].in)) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "executing second callback %d", i);
            callbacks[i].cb(tick_time, units_changed);
          }
          break;
        case MINUTE_UNIT:
          if ((0 == (tick_time->tm_min % callbacks[i].in)) 
            && (0 == tick_time->tm_sec)) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "executing minute callback %d", i);
            callbacks[i].cb(tick_time, units_changed);
          }
          break;
        case HOUR_UNIT:
          if ((0 == (tick_time->tm_hour % callbacks[i].in)) 
            && (0 == tick_time->tm_min)
            && (0 == tick_time->tm_sec)) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "executing hourly callback %d", i);
            callbacks[i].cb(tick_time, units_changed);
          }
          break;
        case DAY_UNIT:
          if ((0 == (tick_time->tm_mday % callbacks[i].in)) 
            && (0 == tick_time->tm_hour)
            && (0 == tick_time->tm_min)
            && (0 == tick_time->tm_sec)) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "executing dayly callback %d", i);
            callbacks[i].cb(tick_time, units_changed);
          }
          break;
        case MONTH_UNIT:
          if ((0 == (tick_time->tm_mon % callbacks[i].in)) 
            && (0 == tick_time->tm_mday)
            && (0 == tick_time->tm_hour)
            && (0 == tick_time->tm_min)
            && (0 == tick_time->tm_sec)) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "executing monthly callback %d", i);
            callbacks[i].cb(tick_time, units_changed);
          }
          break;
        case YEAR_UNIT:
          if ((0 == (tick_time->tm_year % callbacks[i].in)) 
            && (0 == tick_time->tm_mon)
            && (0 == tick_time->tm_mday)
            && (0 == tick_time->tm_hour)
            && (0 == tick_time->tm_min)
            && (0 == tick_time->tm_sec)) {
            APP_LOG(APP_LOG_LEVEL_DEBUG, "executing yearly callback %d", i);
            callbacks[i].cb(tick_time, units_changed);
          }
          break;
      }
    }
  }
 
  if ((0 == tick_time->tm_hour) && (0 == tick_time->tm_min)) {
    handle_day_tick(tick_time, units_changed);
  }
}

void watch_init(Layer *window_layer) {
  for (int i = 0; i < WATCH_MAX_CALLBACK_NUM; i++) {
    memset(&callbacks[i], 0, sizeof(watch_callback_t));
  }

  text_date_layer = text_layer_create(GRect(0, 102, 144, 22));
  text_layer_set_text_color(text_date_layer, GColorWhite);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  text_layer_set_text_alignment(text_date_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_date_layer));

  text_time_layer = text_layer_create(GRect(0, 112, 144, 50));
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_SUBSET_49)));
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));
  
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_minute_tick(current_time, MINUTE_UNIT);
  handle_day_tick(current_time, DAY_UNIT);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}

void watch_deinit(void) {
  tick_timer_service_unsubscribe();

  layer_remove_from_parent(text_layer_get_layer(text_time_layer));
  text_layer_destroy(text_time_layer);
  
  layer_remove_from_parent(text_layer_get_layer(text_date_layer));
  text_layer_destroy(text_date_layer);
}

int8_t watch_reg_callback(TickHandler cb, TimeUnits tu, uint32_t interval) {
  int8_t ret = -1;
  for (int i = 0; i < WATCH_MAX_CALLBACK_NUM; i++) {
    if (NULL == callbacks[i].cb) {
      callbacks[i].cb = cb;
      callbacks[i].tu = tu;
      callbacks[i].in = interval;
      ret = i;
      break;
    }
  }
  return ret;
}

void watch_dereg_callback(uint8_t cb_ind) {
  if (WATCH_MAX_CALLBACK_NUM > cb_ind) {
    callbacks[cb_ind].cb = NULL;
  }
}
