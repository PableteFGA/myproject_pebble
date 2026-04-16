#include <pebble.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "converter_window.h"

#define MAX_ITEMS 20
#define MAX_ROW_LEN 96
#define HEADER_LEN 48

enum {
  KEY_REQUEST_REFRESH = 1,
  KEY_ITEM_INDEX = 2,
  KEY_ITEM_TEXT = 3,
  KEY_ITEM_COUNT = 4,
  KEY_HEADER_TEXT = 5,
  KEY_ITEM_CODE = 6,
  KEY_ITEM_LABEL = 7,
  KEY_ITEM_RATE_X100 = 8,
  KEY_ITEM_SUPPORTED = 9
};

typedef struct {
  char row_text[MAX_ROW_LEN];
  ConverterModel model;
} IndicatorRow;

static Window *s_main_window;
static TextLayer *s_header_layer;
static MenuLayer *s_menu_layer;

static char s_header_text[HEADER_LEN] = "Cargando...";
static IndicatorRow s_items[MAX_ITEMS];
static uint16_t s_row_count = 1;

static ConverterWindow *s_converter_window;

static void request_refresh(void) {
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (result != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_begin fallo: %d", result);
    return;
  }

  dict_write_uint8(iter, KEY_REQUEST_REFRESH, 1);
  app_message_outbox_send();
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return s_row_count;
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  return 52;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);
  const char *row = s_items[cell_index->row].row_text;
  const char *sep = strchr(row, '|');

  graphics_context_set_text_color(
    ctx,
    menu_cell_layer_is_highlighted(cell_layer) ? GColorWhite : GColorBlack
  );

  if (!sep) {
    graphics_draw_text(
      ctx,
      row,
      fonts_get_system_font(FONT_KEY_GOTHIC_18),
      bounds,
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft,
      NULL
    );
    return;
  }

  char title[40];
  char value[56];
  int title_len = (int)(sep - row);
  if (title_len >= (int)sizeof(title)) {
    title_len = (int)sizeof(title) - 1;
  }

  strncpy(title, row, title_len);
  title[title_len] = '\0';
  snprintf(value, sizeof(value), "%s", sep + 1);

  graphics_draw_text(
    ctx,
    title,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    GRect(5, 2, bounds.size.w - 10, 22),
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentLeft,
    NULL
  );

  graphics_draw_text(
    ctx,
    value,
    fonts_get_system_font(FONT_KEY_GOTHIC_14),
    GRect(5, 24, bounds.size.w - 10, 22),
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentLeft,
    NULL
  );
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  vibes_short_pulse();
  converter_window_set_model(s_converter_window, &s_items[cell_index->row].model);
  converter_window_push(s_converter_window, true);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *header_t = dict_find(iter, KEY_HEADER_TEXT);
  Tuple *count_t = dict_find(iter, KEY_ITEM_COUNT);
  Tuple *index_t = dict_find(iter, KEY_ITEM_INDEX);
  Tuple *text_t = dict_find(iter, KEY_ITEM_TEXT);

  Tuple *code_t = dict_find(iter, KEY_ITEM_CODE);
  Tuple *label_t = dict_find(iter, KEY_ITEM_LABEL);
  Tuple *rate_t = dict_find(iter, KEY_ITEM_RATE_X100);
  Tuple *supported_t = dict_find(iter, KEY_ITEM_SUPPORTED);

  if (header_t) {
    snprintf(s_header_text, sizeof(s_header_text), "%s", header_t->value->cstring);
    if (s_header_layer) {
      text_layer_set_text(s_header_layer, s_header_text);
    }
  }

  if (count_t) {
    s_row_count = count_t->value->uint8;
    if (s_row_count == 0) {
      s_row_count = 1;
      strncpy(s_items[0].row_text, "Sin datos", MAX_ROW_LEN - 1);
      s_items[0].row_text[MAX_ROW_LEN - 1] = '\0';
    } else if (s_row_count > MAX_ITEMS) {
      s_row_count = MAX_ITEMS;
    }
  }

  if (index_t) {
    int idx = index_t->value->uint8;
    if (idx >= 0 && idx < MAX_ITEMS) {
      if (text_t) {
        strncpy(s_items[idx].row_text, text_t->value->cstring, MAX_ROW_LEN - 1);
        s_items[idx].row_text[MAX_ROW_LEN - 1] = '\0';
      }

      if (code_t) {
        strncpy(s_items[idx].model.code, code_t->value->cstring, sizeof(s_items[idx].model.code) - 1);
        s_items[idx].model.code[sizeof(s_items[idx].model.code) - 1] = '\0';
      }

      if (label_t) {
        strncpy(s_items[idx].model.label, label_t->value->cstring, sizeof(s_items[idx].model.label) - 1);
        s_items[idx].model.label[sizeof(s_items[idx].model.label) - 1] = '\0';
      }

      if (rate_t) {
        s_items[idx].model.rate_to_pesos_x100 = strtoll(rate_t->value->cstring, NULL, 10);
      } else {
        s_items[idx].model.rate_to_pesos_x100 = 0;
      }

      if (supported_t) {
        s_items[idx].model.supported = supported_t->value->uint8 ? true : false;
      }
    }
  }

  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "inbox_dropped: %d", reason);
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "outbox_failed: %d", reason);
}

static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "outbox_sent");
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_header_layer = text_layer_create(GRect(0, 0, bounds.size.w, 14));
  text_layer_set_text(s_header_layer, s_header_text);
  text_layer_set_font(s_header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_09));
  text_layer_set_text_alignment(s_header_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_header_layer, GColorBlack);
  text_layer_set_text_color(s_header_layer, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(s_header_layer));

  s_menu_layer = menu_layer_create(GRect(0, 14, bounds.size.w, bounds.size.h - 14));
  menu_layer_set_click_config_onto_window(s_menu_layer, window);

  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = menu_get_num_rows_callback,
    .get_cell_height = menu_get_cell_height_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback
  });

  menu_layer_set_normal_colors(s_menu_layer, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(s_menu_layer, GColorRed, GColorWhite);

  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  request_refresh();
}

static void window_unload(Window *window) {
  if (s_menu_layer) {
    menu_layer_destroy(s_menu_layer);
    s_menu_layer = NULL;
  }

  if (s_header_layer) {
    text_layer_destroy(s_header_layer);
    s_header_layer = NULL;
  }
}

static void init(void) {
  for (int i = 0; i < MAX_ITEMS; i++) {
    s_items[i].row_text[0] = '\0';
    s_items[i].model.code[0] = '\0';
    s_items[i].model.label[0] = '\0';
    s_items[i].model.rate_to_pesos_x100 = 0;
    s_items[i].model.supported = false;
  }

  strncpy(s_header_text, "Cargando...", HEADER_LEN - 1);
  s_header_text[HEADER_LEN - 1] = '\0';

  strncpy(s_items[0].row_text, "Cargando...", MAX_ROW_LEN - 1);
  s_items[0].row_text[MAX_ROW_LEN - 1] = '\0';

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_register_outbox_sent(outbox_sent_handler);

  app_message_open(1024, 1024);

  s_converter_window = converter_window_create();

  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  converter_window_destroy(s_converter_window);
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}