#include <pebble.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "converter_window.h"

#define CONVERTER_ACTION_BAR_W 20
#define HEADER_HEIGHT 14
#define CARD_MARGIN 8
#define CARD_GAP 8
#define STEP_X100 100

typedef struct ConverterWindow {
  Window *window;
  Layer *content_layer;
  TextLayer *header_layer;
  ActionBarLayer *action_bar;

  GBitmap *icon_up;
  GBitmap *icon_select;
  GBitmap *icon_down;

  ConverterModel model;

  int64_t left_value_x100;
  int64_t right_value_x100;

  bool left_selected;
  bool editing;

  GColor highlight_color;
} ConverterWindow;

static void prv_format_number_x100(int64_t value_x100, char *out, size_t out_size) {
  if (value_x100 < 0) {
    value_x100 = 0;
  }

  int64_t integer_part = value_x100 / 100;
  int64_t decimal_part = value_x100 % 100;

  char int_raw[32];
  snprintf(int_raw, sizeof(int_raw), "%" PRId64, integer_part);

  char grouped[48];
  size_t len = strlen(int_raw);
  size_t g = 0;
  size_t first = len % 3;
  if (first == 0 && len > 0) {
    first = 3;
  }

  for (size_t i = 0; i < len && g + 1 < sizeof(grouped); i++) {
    if (i > 0 && ((i - first) % 3 == 0)) {
      grouped[g++] = '.';
    }
    grouped[g++] = int_raw[i];
  }
  grouped[g] = '\0';

  snprintf(out, out_size, "%s,%02" PRId64, grouped, decimal_part);
}

static void prv_sync_values_from_left(ConverterWindow *cw) {
  cw->right_value_x100 = (cw->left_value_x100 * cw->model.rate_to_pesos_x100) / 100;
}

static void prv_sync_values_from_right(ConverterWindow *cw) {
  if (cw->model.rate_to_pesos_x100 <= 0) {
    cw->left_value_x100 = 0;
    return;
  }

  cw->left_value_x100 = (cw->right_value_x100 * 100) / cw->model.rate_to_pesos_x100;
}

static void prv_draw_card(GContext *ctx, GRect rect, const char *title, const char *value, bool active) {
  GColor fill = active ? GColorRed : GColorWhite;
  GColor text = active ? GColorWhite : GColorBlack;
  GColor border = active ? GColorRed : GColorBlack;

  graphics_context_set_fill_color(ctx, fill);
  graphics_fill_rect(ctx, rect, 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, border);
  graphics_draw_rect(ctx, rect);

  graphics_context_set_text_color(ctx, text);

  graphics_draw_text(
    ctx,
    title,
    fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
    GRect(rect.origin.x + 6, rect.origin.y + 4, rect.size.w - 12, 20),
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentLeft,
    NULL
  );

  graphics_draw_text(
    ctx,
    value,
    fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK),
    GRect(rect.origin.x + 6, rect.origin.y + 20, rect.size.w - 12, rect.size.h - 24),
    GTextOverflowModeTrailingEllipsis,
    GTextAlignmentCenter,
    NULL
  );
}

static void prv_update_layer(Layer *layer, GContext *ctx) {
  ConverterWindow *cw = *(ConverterWindow **)layer_get_data(layer);

  GRect bounds = layer_get_bounds(layer);
  int content_w = bounds.size.w;
  int content_h = bounds.size.h;

  if (cw->model.rate_to_pesos_x100 <= 0) {
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(
      ctx,
      "No disponible",
      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD),
      GRect(0, content_h / 2 - 10, content_w, 24),
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentCenter,
      NULL
    );

    graphics_draw_text(
      ctx,
      "Este indicador no tiene\nconversion directa a pesos",
      fonts_get_system_font(FONT_KEY_GOTHIC_14),
      GRect(10, content_h / 2 + 14, content_w - 20, 40),
      GTextOverflowModeWordWrap,
      GTextAlignmentCenter,
      NULL
    );
    return;
  }

  int card_w = content_w - (CARD_MARGIN * 2);
  int card_h = (content_h - (CARD_MARGIN * 2) - CARD_GAP) / 2;

  GRect left_rect = GRect(CARD_MARGIN, CARD_MARGIN, card_w, card_h);
  GRect right_rect = GRect(CARD_MARGIN, CARD_MARGIN + card_h + CARD_GAP, card_w, card_h);

  char left_value_buff[48];
  char right_value_buff[48];

  prv_format_number_x100(cw->left_value_x100, left_value_buff, sizeof(left_value_buff));
  prv_format_number_x100(cw->right_value_x100, right_value_buff, sizeof(right_value_buff));

  prv_draw_card(ctx, left_rect, cw->model.label, left_value_buff, cw->left_selected);
  prv_draw_card(ctx, right_rect, "Pesos", right_value_buff, !cw->left_selected);
}

static void prv_recalc_and_redraw(ConverterWindow *cw) {
  if (!cw->content_layer) {
    return;
  }
  layer_mark_dirty(cw->content_layer);
}

static void prv_adjust_selected_value(ConverterWindow *cw, bool increase) {
  if (cw->model.rate_to_pesos_x100 <= 0) {
    return;
  }

  int64_t delta = increase ? STEP_X100 : -STEP_X100;

  if (cw->left_selected) {
    cw->left_value_x100 += delta;
    if (cw->left_value_x100 < 0) {
      cw->left_value_x100 = 0;
    }
    prv_sync_values_from_left(cw);
  } else {
    cw->right_value_x100 += delta;
    if (cw->right_value_x100 < 0) {
      cw->right_value_x100 = 0;
    }
    prv_sync_values_from_right(cw);
  }

  prv_recalc_and_redraw(cw);
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  ConverterWindow *cw = (ConverterWindow *)context;
  if (cw->model.rate_to_pesos_x100 <= 0) return;

  if (cw->editing) {
    prv_adjust_selected_value(cw, true);
  } else {
    cw->left_selected = true;
    prv_recalc_and_redraw(cw);
  }
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  ConverterWindow *cw = (ConverterWindow *)context;
  if (cw->model.rate_to_pesos_x100 <= 0) return;

  if (cw->editing) {
    prv_adjust_selected_value(cw, false);
  } else {
    cw->left_selected = false;
    prv_recalc_and_redraw(cw);
  }
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  ConverterWindow *cw = (ConverterWindow *)context;
  if (cw->model.rate_to_pesos_x100 <= 0) return;

  cw->editing = !cw->editing;
  vibes_short_pulse();
  prv_recalc_and_redraw(cw);
}

static void prv_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  ConverterWindow *cw = (ConverterWindow *)context;

  if (cw->editing) {
    cw->editing = false;
    prv_recalc_and_redraw(cw);
    return;
  }

  converter_window_pop(cw, true);
}

static void prv_action_click_config_provider(void *context) {
  ConverterWindow *cw = (ConverterWindow *)context;
  window_set_click_context(BUTTON_ID_UP, cw);
  window_set_click_context(BUTTON_ID_SELECT, cw);
  window_set_click_context(BUTTON_ID_DOWN, cw);

  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void prv_window_click_config_provider(void *context) {
  ConverterWindow *cw = (ConverterWindow *)context;
  window_set_click_context(BUTTON_ID_BACK, cw);
  window_single_click_subscribe(BUTTON_ID_BACK, prv_back_click_handler);
}

static void prv_window_load(Window *window) {
  ConverterWindow *cw = window_get_user_data(window);
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  window_set_background_color(window, GColorWhite);

  cw->header_layer = text_layer_create(GRect(0, 0, bounds.size.w - CONVERTER_ACTION_BAR_W, HEADER_HEIGHT));
  text_layer_set_text(cw->header_layer, cw->model.label);
  text_layer_set_font(cw->header_layer, fonts_get_system_font(FONT_KEY_GOTHIC_09));
  text_layer_set_text_alignment(cw->header_layer, GTextAlignmentCenter);
  text_layer_set_background_color(cw->header_layer, GColorBlack);
  text_layer_set_text_color(cw->header_layer, GColorWhite);
  layer_add_child(root, text_layer_get_layer(cw->header_layer));

  cw->content_layer = layer_create_with_data(
    GRect(0, HEADER_HEIGHT, bounds.size.w - CONVERTER_ACTION_BAR_W, bounds.size.h - HEADER_HEIGHT),
    sizeof(ConverterWindow *)
  );

  ConverterWindow **layer_data = (ConverterWindow **)layer_get_data(cw->content_layer);
  *layer_data = cw;
  layer_set_update_proc(cw->content_layer, prv_update_layer);
  layer_add_child(root, cw->content_layer);

  cw->action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(cw->action_bar, window);
  action_bar_layer_set_context(cw->action_bar, cw);
  action_bar_layer_set_click_config_provider(cw->action_bar, prv_action_click_config_provider);

  cw->icon_up = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONVERTER_UP);
  cw->icon_select = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONVERTER_SELECT);
  cw->icon_down = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONVERTER_DOWN);

  action_bar_layer_set_icon(cw->action_bar, BUTTON_ID_UP, cw->icon_up);
  action_bar_layer_set_icon(cw->action_bar, BUTTON_ID_SELECT, cw->icon_select);
  action_bar_layer_set_icon(cw->action_bar, BUTTON_ID_DOWN, cw->icon_down);

  prv_recalc_and_redraw(cw);
}

static void prv_window_unload(Window *window) {
  ConverterWindow *cw = window_get_user_data(window);

  if (cw->action_bar) {
    action_bar_layer_destroy(cw->action_bar);
    cw->action_bar = NULL;
  }

  if (cw->content_layer) {
    layer_destroy(cw->content_layer);
    cw->content_layer = NULL;
  }

  if (cw->header_layer) {
    text_layer_destroy(cw->header_layer);
    cw->header_layer = NULL;
  }

  if (cw->icon_up) {
    gbitmap_destroy(cw->icon_up);
    cw->icon_up = NULL;
  }

  if (cw->icon_select) {
    gbitmap_destroy(cw->icon_select);
    cw->icon_select = NULL;
  }

  if (cw->icon_down) {
    gbitmap_destroy(cw->icon_down);
    cw->icon_down = NULL;
  }
}

ConverterWindow *converter_window_create(void) {
  ConverterWindow *cw = malloc(sizeof(ConverterWindow));
  if (!cw) {
    return NULL;
  }

  *cw = (ConverterWindow) {
    .window = NULL,
    .content_layer = NULL,
    .header_layer = NULL,
    .action_bar = NULL,
    .icon_up = NULL,
    .icon_select = NULL,
    .icon_down = NULL,
    .left_value_x100 = 100,
    .right_value_x100 = 100,
    .left_selected = true,
    .editing = false,
    .highlight_color = GColorRed
  };

  cw->window = window_create();
  window_set_user_data(cw->window, cw);
  window_set_window_handlers(cw->window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload
  });
  window_set_click_config_provider(cw->window, prv_window_click_config_provider);

  return cw;
}

void converter_window_destroy(ConverterWindow *cw) {
  if (!cw) {
    return;
  }

  if (cw->window) {
    window_destroy(cw->window);
    cw->window = NULL;
  }

  free(cw);
}

void converter_window_set_highlight_color(ConverterWindow *cw, GColor color) {
  if (!cw) return;
  cw->highlight_color = color;
}

void converter_window_set_model(ConverterWindow *cw, const ConverterModel *model) {
  if (!cw || !model) return;

  cw->model = *model;
  cw->model.supported = (cw->model.rate_to_pesos_x100 > 0);
  cw->left_value_x100 = 100;
  cw->right_value_x100 = cw->model.rate_to_pesos_x100;
  cw->left_selected = true;
  cw->editing = false;

  if (cw->header_layer) {
    text_layer_set_text(cw->header_layer, cw->model.label);
  }

  prv_recalc_and_redraw(cw);
}

void converter_window_push(ConverterWindow *cw, bool animated) {
  if (cw && cw->window) {
    window_stack_push(cw->window, animated);
  }
}

void converter_window_pop(ConverterWindow *cw, bool animated) {
  if (cw && cw->window) {
    window_stack_remove(cw->window, animated);
  }
}

bool converter_window_get_topmost_window(ConverterWindow *cw) {
  if (!cw || !cw->window) {
    return false;
  }
  return window_stack_get_top_window() == cw->window;
}