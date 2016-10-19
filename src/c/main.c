#include <pebble.h>
#define EARTH_R 12
#define MOON_R 8
#define SUN_R 16
#define MOON_PATH_R 26
#define SUN_PATH_R 56
#define CPOS 72  

static Window *s_main_window;

static TextLayer *s_time_layer;

static BitmapLayer *s_earth_layer;
static BitmapLayer *s_moon_layer;
static BitmapLayer *s_sun_layer;

static Layer *s_canvas_layer;

static GBitmap *s_earth_bitmap;
static GBitmap *s_moon_bitmap;
static GBitmap *s_sun_bitmap;

static int time2int(char *time);
static void update_time();
static void minute_handler(struct tm *tick_time, TimeUnits units_changed);
static void battery_handler(BatteryChargeState new_state);
static void bt_handler(bool connected);
static void canvas_update_proc(Layer *this_layer, GContext *ctx);

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  
  s_canvas_layer = layer_create(GRect(0, 0, 144, 144));
  
  layer_add_child(window_layer, s_canvas_layer);
  // Set the update_proc
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
                                      
  // Create TextLayer to show numeric time
  s_time_layer = text_layer_create(GRect(0, 145, 144, 23));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_time_layer, "2015-01-01 00:00");
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Create GBitmap, then set to created BitmapLayer for earth
  s_earth_bitmap = gbitmap_create_with_resource(RESOURCE_ID_E01);
  s_earth_layer = bitmap_layer_create(GRect(CPOS - EARTH_R, CPOS - EARTH_R, EARTH_R*2, EARTH_R*2));
  bitmap_layer_set_bitmap(s_earth_layer, s_earth_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_earth_layer));
  
  // Create GBitmap, then set to created BitmapLayer for moon
  s_moon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_M01);
  s_moon_layer = bitmap_layer_create(GRect(CPOS - MOON_R, CPOS - MOON_PATH_R - MOON_R, MOON_R*2, MOON_R*2));
  bitmap_layer_set_bitmap(s_moon_layer, s_moon_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_moon_layer));

  // Create GBitmap, then set to created BitmapLayer for sun
  s_sun_bitmap = gbitmap_create_with_resource(RESOURCE_ID_S01);
  s_sun_layer = bitmap_layer_create(GRect(CPOS - SUN_R, CPOS - SUN_PATH_R - SUN_R, SUN_R*2, SUN_R*2));
  bitmap_layer_set_bitmap(s_sun_layer, s_sun_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_sun_layer));  
  
  // Show current connection state
  bt_handler(bluetooth_connection_service_peek());
  
  // Get the current battery level
  battery_handler(battery_state_service_peek());  

  // Update date and time at startup
  update_time();
}
                                
static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  
  // Destroy GBitmap
  gbitmap_destroy(s_earth_bitmap);
  gbitmap_destroy(s_moon_bitmap);
  gbitmap_destroy(s_sun_bitmap);
  
  // Destroy BitmapLayer
  bitmap_layer_destroy(s_earth_layer);
  bitmap_layer_destroy(s_moon_layer);
  bitmap_layer_destroy(s_sun_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, minute_handler);
  
  // Subscribe to the Battery State Service
  battery_state_service_subscribe(battery_handler);
  
  // Register BlueTooth Update 
  bluetooth_connection_service_subscribe(bt_handler);
}

static void deinit() {
  // Destroy main Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char tmbuffer[] = "2014-01-01 00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(tmbuffer, sizeof("2014-01-01 00:00"), "%Y-%m-%d %H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(tmbuffer, sizeof("2014-01-01 00:00"), "%Y-%m-%d %I:%M", tick_time);
  }
  
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, tmbuffer);
  
  static char tmTemp[] = "00";
  strftime(tmTemp, sizeof("00"), "%I", tick_time);
  int iHour = time2int(tmTemp);
  strftime(tmTemp, sizeof("00"), "%M", tick_time);
  int iMin = time2int(tmTemp);
  
  int x, y;

  int32_t minute_angle = TRIG_MAX_ANGLE * iMin / 60;
  y = (-cos_lookup(minute_angle) * MOON_PATH_R / TRIG_MAX_RATIO) + CPOS - MOON_R;
  x = (sin_lookup(minute_angle) * MOON_PATH_R / TRIG_MAX_RATIO) + CPOS - MOON_R;
  layer_set_frame(bitmap_layer_get_layer(s_moon_layer), GRect(x, y, MOON_R*2, MOON_R*2));  
  
  int32_t hour_angle = TRIG_MAX_ANGLE * (iHour % 12)/ 12;
  y = (-cos_lookup(hour_angle) * SUN_PATH_R / TRIG_MAX_RATIO) + CPOS - SUN_R;
  x = (sin_lookup(hour_angle) * SUN_PATH_R / TRIG_MAX_RATIO) + CPOS - SUN_R;
  layer_set_frame(bitmap_layer_get_layer(s_sun_layer), GRect(x, y, SUN_R*2, SUN_R*2));
}

static void minute_handler(struct tm *tick_time, TimeUnits units_changed) {
  //Update date and time display
  update_time();
  
}

static void battery_handler(BatteryChargeState new_state) {
  // Write to buffer and display
  int iLevel=  new_state.charge_percent;
  if (iLevel > 80) {
    s_moon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_M05);
  } else if (iLevel > 60) {
    s_moon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_M04);
  } else if (iLevel >40) {
    s_moon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_M03);
  } else if (iLevel > 20) {
    s_moon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_M02);
  } else {
    s_moon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_M01);
  }
  bitmap_layer_set_bitmap(s_moon_layer, s_moon_bitmap);  
}

static void bt_handler(bool connected) {
  if (connected) {
    s_sun_bitmap = gbitmap_create_with_resource(RESOURCE_ID_S02);
  } else {
    s_sun_bitmap = gbitmap_create_with_resource(RESOURCE_ID_S01);
    vibes_short_pulse();
  }
  
  bitmap_layer_set_bitmap(s_sun_layer, s_sun_bitmap);
}                                
                                
static int time2int(char *time) {
  int iRet = 0;
  iRet += (int)(time[0] - '0')*10;
  iRet += (int)(time[1] - '0');
  
  return iRet;
}

static void canvas_update_proc(Layer *this_layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(this_layer);
  // Draw a black filled rectangle with sharp corners
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  GPoint center = GPoint(CPOS, CPOS);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_circle(ctx, center, MOON_PATH_R);
  graphics_draw_circle(ctx, center, SUN_PATH_R);
  
}