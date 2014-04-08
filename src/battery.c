#include "pebble.h"
#include "battery.h"

#define BATTERY_STATES_CHARGE 6

static BitmapLayer *image_battery_icon_layer = NULL;
static GBitmap *battery_icon = NULL;
static uint8_t last_charge = 0;
static const uint32_t battery_icons[] = {
  RESOURCE_ID_BATTERY_EMPTY_ICON_BLACK,
  RESOURCE_ID_BATTERY_20_ICON_BLACK,
  RESOURCE_ID_BATTERY_40_ICON_BLACK,
  RESOURCE_ID_BATTERY_60_ICON_BLACK,
  RESOURCE_ID_BATTERY_80_ICON_BLACK,
  RESOURCE_ID_BATTERY_100_ICON_BLACK,
  RESOURCE_ID_BATTERY_CHARGE_ICON_BLACK,
};

static void handle_battery(BatteryChargeState charge) {
  uint8_t cur_charge = (charge.charge_percent + 19) / 20;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "battery charge %d %scharging %sconnected"
                               , charge.charge_percent
                               , charge.is_charging ? "" : "not "
                               , charge.is_plugged ? "" : "not "
         );
  if (charge.is_charging)
    cur_charge = BATTERY_STATES_CHARGE; 
  if  (last_charge != cur_charge) {
    if (battery_icon) {
      gbitmap_destroy(battery_icon);
    }
    battery_icon = gbitmap_create_with_resource(battery_icons[cur_charge]);
    bitmap_layer_set_bitmap(image_battery_icon_layer, battery_icon);
    last_charge = cur_charge;
  }
}

void battery_init(Layer *window_layer) {
  image_battery_icon_layer = bitmap_layer_create(GRect(144-25, 0, 24, 24));
  bitmap_layer_set_compositing_mode(image_battery_icon_layer, GCompOpAssign);
  layer_add_child(window_layer, bitmap_layer_get_layer(image_battery_icon_layer));
  BatteryChargeState charge = battery_state_service_peek();
  handle_battery(charge);
  battery_state_service_subscribe(handle_battery);
}

void battery_deinit() {
  battery_state_service_unsubscribe();
  layer_remove_from_parent(bitmap_layer_get_layer(image_battery_icon_layer));
  bitmap_layer_destroy(image_battery_icon_layer);
  if (battery_icon) {
    gbitmap_destroy(battery_icon);
  }
}
