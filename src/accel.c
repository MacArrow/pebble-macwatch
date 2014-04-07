#include "pebble.h"
#include "accel.h"

//#define ACCEL_ENABLE

static TextLayer *text_accel_layer;

static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  for(uint32_t i=0; i < num_samples; i++) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "vibrate %d time %llu X %d Y %d Z %d"
            , data[i].did_vibrate
            , data[i].timestamp
            , data[i].x
            , data[i].y
            , data[i].z
    );
  }
}

void accel_init(Layer *window_layer) {
#ifdef ACCEL_ENABLE
  text_accel_layer = text_layer_create(GRect(21, 131, 144-21, 168-131));
  text_layer_set_text_color(text_accel_layer, GColorWhite);
  text_layer_set_background_color(text_accel_layer, GColorClear);
  text_layer_set_font(text_accel_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21)));
  layer_add_child(window_layer, text_layer_get_layer(text_accel_layer));
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ);
  accel_data_service_subscribe(1, &accel_data_handler);
#endif
}

void accel_deinit() {
#ifdef ACCEL_ENABLE
  accel_data_service_unsubscribe();
  layer_remove_from_parent(text_layer_get_layer(text_accel_layer));
  text_layer_destroy(text_accel_layer);
#endif  
}
