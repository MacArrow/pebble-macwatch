#include "pebble.h"
#include "watch.h"
#include "battery.h"
#include "bluetooth.h"
#include "accel.h"
#include "weather.h"

Window *window;

void handle_deinit(void) {
  weather_deinit();
  accel_deinit();
  bluetooth_deinit();
  battery_deinit();
  watch_deinit();
}

void handle_init(void) {
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorBlack);
  Layer *window_layer = window_get_root_layer(window);
  watch_init(window_layer);
  battery_init(window_layer);
  bluetooth_init(window_layer);
  accel_init(window_layer);
  weather_init(window_layer);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}

