#ifndef __WATCH_H__
#define __WATCH_H__

#include "pebble.h"

void watch_init(Layer *window_layer);
void watch_deinit(void);
int8_t watch_reg_callback(TickHandler cb, TimeUnits tu, uint32_t interval);
void watch_dereg_callback(uint8_t cb_ind);

#endif //__WATCH_H__
