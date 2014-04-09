#include "pebble.h"
#include "weather.h"
#include "watch.h"

static TextLayer *city_text_layer;
static TextLayer *temp_now_text_layer;
static TextLayer *temp_tomorrow_text_layer;

static Layer *line_layer;

static InverterLayer *inv_layer;

static BitmapLayer *sky_now_icon_layer;
static BitmapLayer *sky_tomorrow_icon_layer;

static GBitmap *sky_now_icon = NULL;
static GBitmap *sky_tomorrow_icon = NULL;

static const uint32_t WEATHER_ICONS_DAY[] = {
  RESOURCE_ID_WEATHER_CLOUDS_ICON_BLACK,              //0
  RESOURCE_ID_WEATHER_PARTLY_CLOUDY_DAY_ICON_BLACK,   //1
  RESOURCE_ID_WEATHER_SUN_ICON_BLACK,                 //2
  RESOURCE_ID_WEATHER_LITTLE_SNOW_ICON_BLACK,         //3
  RESOURCE_ID_WEATHER_SNOW_ICON_BLACK,                //4
  RESOURCE_ID_WEATHER_SLEET_ICON_BLACK,               //5
  RESOURCE_ID_WEATHER_DOWNPOUR_ICON_BLACK,            //6
  RESOURCE_ID_WEATHER_RAIN_ICON_BLACK,                //7
  RESOURCE_ID_WEATHER_STORM_ICON_BLACK,               //8
};

static const uint32_t WEATHER_ICONS_NIGHT[] = {
  RESOURCE_ID_WEATHER_CLOUDS_ICON_BLACK,              //0
  RESOURCE_ID_WEATHER_PARTLY_CLOUDY_NIGHT_ICON_BLACK, //1
  RESOURCE_ID_WEATHER_MOON_ICON_BLACK,                //2
  RESOURCE_ID_WEATHER_LITTLE_SNOW_ICON_BLACK,         //3
  RESOURCE_ID_WEATHER_SNOW_ICON_BLACK,                //4
  RESOURCE_ID_WEATHER_SLEET_ICON_BLACK,               //5
  RESOURCE_ID_WEATHER_DOWNPOUR_ICON_BLACK,            //6
  RESOURCE_ID_WEATHER_RAIN_ICON_BLACK,                //7
  RESOURCE_ID_WEATHER_STORM_ICON_BLACK,               //8
};

enum WeatherKey {
  WEATHER_ISDAY_KEY = 0x0,                // TUPLE_INT
  WEATHER_NOW_KEY = 0x1,                  // TUPLE_INT
  WEATHER_SKY_NOW_KEY = 0x2,              // TUPLE_INT
  WEATHER_TEMP_NOW_KEY = 0x3,             // TUPLE_CSTRING
  WEATHER_CITY_KEY = 0x4,                 // TUPLE_CSTRING
  WEATHER_FORECAST_KEY = 0x5,             // TUPLE_INT
  WEATHER_SKY_TOMORROW_KEY = 0x6,         // TUPLE_INT
  WEATHER_TEMP_TOMORROW_KEY = 0x7,        // TUPLE_CSTRING
};

static char city[15];
static char temp_now[15];
static char temp_tomorrow[15];

static int8_t callback_id = -1;

static uint8_t is_day = 1;

static void fetch_cmd(void) {
  Tuplet weather_tuple = TupletInteger(WEATHER_NOW_KEY, 1);
  Tuplet forecast_tuple = TupletInteger(WEATHER_FORECAST_KEY, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &weather_tuple);
  dict_write_tuplet(iter, &forecast_tuple);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "received incoming message! iter 0x%p", iter);
  Tuple *tuple = dict_read_first(iter);
  while (NULL != tuple) {
    switch (tuple->key) {
      case WEATHER_CITY_KEY:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "WEATHER_CITY_KEY");
        // strncpy(city, tuple->value->cstring, 15);
        // text_layer_set_text(city_text_layer, city);
        break;
      case WEATHER_TEMP_NOW_KEY:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "WEATHER_TEMP_NOW_KEY");
        strncpy(temp_now, tuple->value->cstring, 15);
        text_layer_set_text(temp_now_text_layer, temp_now); 
        break;
      case WEATHER_SKY_NOW_KEY:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "WEATHER_SKY_NOW_KEY %d", tuple->value->uint8);
        if (tuple->value->uint8 < sizeof(WEATHER_ICONS_DAY)) {
          if (NULL == sky_now_icon) {
            gbitmap_destroy(sky_now_icon);
          }
          if (is_day) {
            sky_now_icon = gbitmap_create_with_resource(WEATHER_ICONS_DAY[tuple->value->uint8]);
          } else {
            sky_now_icon = gbitmap_create_with_resource(WEATHER_ICONS_NIGHT[tuple->value->uint8]);
          }
          bitmap_layer_set_bitmap(sky_now_icon_layer, sky_now_icon);
        }
        break;
      case WEATHER_TEMP_TOMORROW_KEY:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "WEATHER_TEMP_TOMORROW_KEY");
        strncpy(temp_tomorrow, tuple->value->cstring, 15);
        text_layer_set_text(temp_tomorrow_text_layer, temp_tomorrow);
        break;
      case WEATHER_SKY_TOMORROW_KEY:   
        APP_LOG(APP_LOG_LEVEL_DEBUG, "WEATHER_SKY_TOMORROW_KEY %d", tuple->value->uint8);
        if (tuple->value->uint8 < sizeof(WEATHER_ICONS_DAY)) {
          if (NULL == sky_tomorrow_icon) {
            gbitmap_destroy(sky_tomorrow_icon);
          }
          sky_tomorrow_icon = gbitmap_create_with_resource(WEATHER_ICONS_DAY[tuple->value->uint8]);
          bitmap_layer_set_bitmap(sky_tomorrow_icon_layer, sky_tomorrow_icon);
        }
        break;
      case WEATHER_ISDAY_KEY:
        APP_LOG(APP_LOG_LEVEL_DEBUG, "WEATHER_ISDAY_KEY %d", tuple->value->uint8);
        is_day = tuple->value->uint8;
        layer_set_hidden(inverter_layer_get_layer(inv_layer), is_day);
        break;
    }
    tuple = dict_read_next(iter);
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
  fetch_cmd();
}

static void handle_weather_tick(struct tm *tick_time, TimeUnits units_changed) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tick!");
  fetch_cmd();
}

void line_layer_update_callback(Layer *me, GContext* ctx) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", __func__);
  GRect bounds = layer_get_bounds(me);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 1, GCornersAll); 
}

void weather_init(Layer *window_layer) {
  city_text_layer = text_layer_create(GRect(0, 84, 144, 22));
  text_layer_set_text_color(city_text_layer, GColorWhite);
  text_layer_set_background_color(city_text_layer, GColorClear);
  text_layer_set_font(city_text_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  text_layer_set_text_alignment(city_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(city_text_layer));
  
  sky_now_icon_layer = bitmap_layer_create(GRect(0, 28, 32, 32));
  bitmap_layer_set_compositing_mode(sky_now_icon_layer, GCompOpAssign);
  layer_add_child(window_layer, bitmap_layer_get_layer(sky_now_icon_layer));
    
  temp_now_text_layer = text_layer_create(GRect(33, 30, 144-33, 17));
  text_layer_set_text_color(temp_now_text_layer, GColorWhite);
  text_layer_set_background_color(temp_now_text_layer, GColorClear);
  text_layer_set_font(temp_now_text_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHER_16)));
  text_layer_set_text_alignment(temp_now_text_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(temp_now_text_layer));

  sky_tomorrow_icon_layer = bitmap_layer_create(GRect(144-32, 54, 32, 32));
  bitmap_layer_set_compositing_mode(sky_tomorrow_icon_layer, GCompOpAssign);
  layer_add_child(window_layer, bitmap_layer_get_layer(sky_tomorrow_icon_layer));

  temp_tomorrow_text_layer = text_layer_create(GRect(0, 64, 144-33, 17));
  text_layer_set_text_color(temp_tomorrow_text_layer, GColorWhite);
  text_layer_set_background_color(temp_tomorrow_text_layer, GColorClear);
  text_layer_set_font(temp_tomorrow_text_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WEATHER_16)));
  text_layer_set_text_alignment(temp_tomorrow_text_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(temp_tomorrow_text_layer));

  line_layer = layer_create(GRect(31, 57, 144 - 32 - 31, 2));
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(window_layer, line_layer);

  inv_layer = inverter_layer_create(GRect(0, 0, 144, 168));
  layer_set_hidden(inverter_layer_get_layer(inv_layer), true);
  layer_add_child(window_layer, inverter_layer_get_layer(inv_layer));
    
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_failed(out_failed_handler);
  app_message_open(app_message_inbox_size_maximum(), APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
  
  fetch_cmd();

  callback_id = watch_reg_callback(handle_weather_tick, MINUTE_UNIT, 30);
  if (-1 == callback_id) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "unable to regiser weater timer callback");
  }
}

void weather_deinit(void) {
  watch_dereg_callback(callback_id);
  
  layer_remove_from_parent(inverter_layer_get_layer(inv_layer));
  inverter_layer_destroy(inv_layer);
 
  layer_remove_from_parent(line_layer);
  layer_destroy(line_layer);
 
  layer_remove_from_parent(text_layer_get_layer(temp_tomorrow_text_layer));
  text_layer_destroy(temp_tomorrow_text_layer);
  
  if (NULL != sky_tomorrow_icon)
    gbitmap_destroy(sky_tomorrow_icon);
  layer_remove_from_parent(bitmap_layer_get_layer(sky_tomorrow_icon_layer));
  bitmap_layer_destroy(sky_tomorrow_icon_layer);

  layer_remove_from_parent(text_layer_get_layer(temp_now_text_layer));
  text_layer_destroy(temp_now_text_layer);

  if (NULL != sky_now_icon)
    gbitmap_destroy(sky_now_icon);
  layer_remove_from_parent(bitmap_layer_get_layer(sky_now_icon_layer));
  bitmap_layer_destroy(sky_now_icon_layer);

  layer_remove_from_parent(text_layer_get_layer(city_text_layer));
  text_layer_destroy(city_text_layer);
}

