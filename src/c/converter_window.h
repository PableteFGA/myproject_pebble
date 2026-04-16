#pragma once
#include <inttypes.h>
#include <pebble.h>

typedef struct {
  char code[24];
  char label[24];
  int64_t rate_to_pesos_x100;
  bool supported;
} ConverterModel;

typedef struct ConverterWindow ConverterWindow;

ConverterWindow *converter_window_create(void);
void converter_window_destroy(ConverterWindow *converter_window);

void converter_window_set_model(ConverterWindow *converter_window, const ConverterModel *model);
void converter_window_set_highlight_color(ConverterWindow *converter_window, GColor color);

void converter_window_push(ConverterWindow *converter_window, bool animated);
void converter_window_pop(ConverterWindow *converter_window, bool animated);
bool converter_window_get_topmost_window(ConverterWindow *converter_window);