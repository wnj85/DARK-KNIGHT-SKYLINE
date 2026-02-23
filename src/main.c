#include <pebble.h>

#define BAT_SIGNAL_DURATION 3000  // ms

static Window *s_main_window;
static BitmapLayer *s_gotham_layer;
static BitmapLayer *s_batsignal_layer;
static TextLayer *s_time_layer;

static GBitmap *s_gotham_bitmap;
static GBitmap *s_batsignal_bitmap;

static bool s_showing_batsignal = false;
static AppTimer *s_batsignal_timer = NULL;

static void hide_batsignal_callback(void *data);

static void update_time() {
  if (s_showing_batsignal) {
    return;
  }

  static char s_buffer[8];

  time_t temp = time(NULL);
  struct tm *t = localtime(&temp);

  if (clock_is_24h_style()) {
    strftime(s_buffer, sizeof(s_buffer), "%H:%M", t);
  } else {
    strftime(s_buffer, sizeof(s_buffer), "%I:%M", t);
  }

  text_layer_set_text(s_time_layer, s_buffer);
}

static void hide_batsignal_callback(void *data) {
  s_showing_batsignal = false;
  layer_set_hidden(bitmap_layer_get_layer(s_batsignal_layer), true);
  layer_set_hidden(text_layer_get_layer(s_time_layer), false);
  update_time();
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  s_showing_batsignal = true;

  layer_set_hidden(text_layer_get_layer(s_time_layer), true);
  layer_set_hidden(bitmap_layer_get_layer(s_batsignal_layer), false);

  if (s_batsignal_timer) {
    app_timer_cancel(s_batsignal_timer);
    s_batsignal_timer = NULL;
  }

  s_batsignal_timer = app_timer_register(BAT_SIGNAL_DURATION, hide_batsignal_callback, NULL);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Background color for night sky (dark blue)
  window_set_background_color(window, GColorOxfordBlue);

  // Gotham skyline along the bottom
  s_gotham_bitmap = gbitmap_create_with_resource(BG_Buildings.png);
  s_gotham_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_gotham_layer, s_gotham_bitmap);
  bitmap_layer_set_compositing_mode(s_gotham_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_gotham_layer));

  // Time in the night sky (top)
  GRect time_frame = GRect(0, 10, bounds.size.w, 50);
  s_time_layer = text_layer_create(time_frame);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Bat Signal bitmap in the sky area
  s_batsignal_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BAT_SIGNAL);
  GRect bat_bounds = gbitmap_get_bounds(s_batsignal_bitmap);
  int16_t bat_x = (bounds.size.w - bat_bounds.size.w) / 2;
  int16_t bat_y = 5;
  GRect bat_frame = GRect(bat_x, bat_y, bat_bounds.size.w, bat_bounds.size.h);

  s_batsignal_layer = bitmap_layer_create(bat_frame);
  bitmap_layer_set_bitmap(s_batsignal_layer, s_batsignal_bitmap);
  bitmap_layer_set_compositing_mode(s_batsignal_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_batsignal_layer));

  layer_set_hidden(bitmap_layer_get_layer(s_batsignal_layer), true);

  update_time();
}

static void main_window_unload(Window *window) {
  if (s_batsignal_timer) {
    app_timer_cancel(s_batsignal_timer);
    s_batsignal_timer = NULL;
  }

  text_layer_destroy(s_time_layer);

  bitmap_layer_destroy(s_gotham_layer);
  gbitmap_destroy(s_gotham_bitmap);

  bitmap_layer_destroy(s_batsignal_layer);
  gbitmap_destroy(s_batsignal_bitmap);
}

static void init() {
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  accel_tap_service_subscribe(accel_tap_handler);

  update_time();
}

static void deinit() {
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
