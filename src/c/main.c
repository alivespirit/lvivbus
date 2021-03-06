#include <pebble.h>

#define STOPS_NUMBER 7

static Window *window;
static MenuLayer *menu_layer;
static TextLayer *text_layer;

//Buffers for received buses info
static char busnum_buf[32];
static char busroute_buf[256];
static char count_buf[8];
static char total_buf[8];

//Buffers for received geolocation data
static char geoname_buf[64];
static char geocode_buf[32];
static char geocount_buf[8];
static char geototal_buf[8];

//Simple menu layer vars for buses info
static Window *s_window;
static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[1];
static SimpleMenuItem s_menu_items[7];

//Simple menu layer vars for nearby stops info
static Window *geo_window;
static SimpleMenuLayer *geo_simple_menu_layer;
static SimpleMenuSection geo_menu_sections[1];
static SimpleMenuItem geo_menu_items[7];

static NumberWindow *number_window;

//Vars for settings from phone
typedef struct ClaySettings {
  char stop[32];
  char desc[64];
  char code[8];
} ClaySettings;

static ClaySettings stops[STOPS_NUMBER];

//Loading config from persistant memory, if available
static void load_config(){
  for (int i = 0; i < STOPS_NUMBER; i++) {
    persist_read_data(i+1, &stops[i], sizeof(ClaySettings));
  }
  if (strcmp(stops[0].stop, "") == 0){
    snprintf(stops[0].stop, sizeof(stops[0].stop), "%s", "Немає зупинок");
    snprintf(stops[0].desc, sizeof(stops[0].desc), "%s", "Потрібне налаштування");
  }
}

//Saving config to persistant memory
static void save_config(){
    for (int i = 0; i < STOPS_NUMBER; i++) {
      persist_write_data(i+1, &stops[i], sizeof(ClaySettings));
    }
}

static void s_select_callback(int index, void *ctx){}

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {

        // Two menu sections
	return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {

        // Nine menu items (7 favorite stops + select-by-number + geolocation)
	return STOPS_NUMBER+2;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {

        // No header
	return 0;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {

        // No header
}

//Filling main menu_layer with favorite stops from config + select-by-number + geolocation
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {

        // Menu items
			switch (cell_index->row) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
          menu_cell_basic_draw(ctx, cell_layer, stops[cell_index->row].stop, stops[cell_index->row].desc, NULL);
					break;
				case 7: 
					menu_cell_basic_draw(ctx, cell_layer, "Номер зупинки", "Вибір за номером", NULL);
					break;
        case 8: 
					menu_cell_basic_draw(ctx, cell_layer, "Зупинки поряд", "Пошук за геолокацією", NULL);
					break;
			}
}

//Sending request to watch
static void SendRequest(char *data) {
	DictionaryIterator *iter1;
	app_message_outbox_begin(&iter1);
	dict_write_cstring(iter1, MESSAGE_KEY_REQUEST, data);
	app_message_outbox_send();
}

//Sending request to get buses info for stop chosen from geolocation (geo simple menu layer)
static void geo_select_callback(int index, void *ctx){
  char geostop[sizeof(geocode_buf)];
  snprintf(geostop, sizeof(geostop), "%s", geo_menu_items[index].title);
  SendRequest(geostop);
}

//Sending request to get buses info from NumberWindow
static void number_select_callback(struct NumberWindow *number_window, void *context) {
	int busstop_num = number_window_get_value(number_window);
  char busstop_num_temp[8];
  snprintf(busstop_num_temp, sizeof(busstop_num_temp), "%d", busstop_num);
	window_stack_pop(true);
  SendRequest(busstop_num_temp);
}

//Main menu_layer items selection
static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	
        // Menu selection
  text_layer_set_text(text_layer, "Loading...");
	switch (cell_index->row) {
		case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
			SendRequest(stops[cell_index->row].code);
			break;
		case 7:
      number_window = number_window_create("Номер зупинки", (NumberWindowCallbacks) { .selected = number_select_callback }, NULL);
    	number_window_set_min(number_window, 1);
	    number_window_set_max(number_window, 900);
	    number_window_set_step_size(number_window, 1);
      window_stack_push((Window*)number_window, true);
			break;
    case 8:
      SendRequest("geo");
      break;
	}
}

static void s_window_load(Window *window) {
}

static void geo_window_load(Window *window) {
}

static void s_window_unload(Window *window) {
  simple_menu_layer_destroy(s_simple_menu_layer);
  number_window_destroy(number_window);
  text_layer_set_text(text_layer, "LvivBus");
}

static void geo_window_unload(Window *window) {
  simple_menu_layer_destroy(geo_simple_menu_layer);
  text_layer_set_text(text_layer, "LvivBus");
}

//Creating simple menu layer with buses info received from API
void DrawResults(int total) {

  s_menu_sections[0] = (SimpleMenuSection) {
    .num_items = total,
    .items = s_menu_items,
  };
  
  s_window = window_create();

  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_frame(window_layer);

  s_simple_menu_layer = simple_menu_layer_create(bounds, s_window, s_menu_sections, 1, NULL);
  
  window_set_window_handlers(s_window, (WindowHandlers) {
		.load = s_window_load,
		.unload = s_window_unload,
	});
  
  window_stack_push(s_window, "true");
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
  
  vibes_short_pulse();
}

//Creating simple menu layer with buses info received from geolocation API
void GeoDrawResults(int total) {

  geo_menu_sections[0] = (SimpleMenuSection) {
    .num_items = total,
    .items = geo_menu_items,
  };
  
  geo_window = window_create();

  Layer *window_layer = window_get_root_layer(geo_window);
  GRect bounds = layer_get_frame(window_layer);

  geo_simple_menu_layer = simple_menu_layer_create(bounds, geo_window, geo_menu_sections, 1, NULL);
  
  window_set_window_handlers(geo_window, (WindowHandlers) {
		.load = geo_window_load,
		.unload = geo_window_unload,
	});
  
  window_stack_push(geo_window, "true");
  layer_add_child(window_layer, simple_menu_layer_get_layer(geo_simple_menu_layer));
  
  vibes_short_pulse();
}

//Receiving info from watch
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

        // Receive response from phone to watch
  
  Tuple *busnum = dict_find(iterator, MESSAGE_KEY_RESPONSE);
  Tuple *busroute = dict_find(iterator, MESSAGE_KEY_RESPONSE_TEXT);
  Tuple *count = dict_find(iterator, MESSAGE_KEY_RESPONSE_COUNT);
  Tuple *total = dict_find(iterator, MESSAGE_KEY_TOTAL);
  
  if (busnum && busroute && count){
      snprintf(busnum_buf, sizeof(busnum_buf), "%s", busnum->value->cstring);
      snprintf(busroute_buf, sizeof(busroute_buf), "%s", busroute->value->cstring);
      snprintf(count_buf, sizeof(count_buf), "%d", (int)count->value->int32);
      snprintf(total_buf, sizeof(total_buf), "%d", (int)total->value->int32);

      int total_num = atoi(total_buf);
  
      int counter = atoi(count_buf);
      char *busnum_temp = malloc(sizeof(busnum_buf));
      strcpy(busnum_temp, busnum_buf);
      char *busroute_temp = malloc(sizeof(busroute_buf));
      strcpy(busroute_temp, busroute_buf);
  
      s_menu_items[counter] = (SimpleMenuItem) {
        .title = busnum_temp,
        .subtitle = busroute_temp,
        .callback = s_select_callback,
      };
  
      if (counter == total_num-1){
        DrawResults(total_num);
        //free(busnum_temp);
        free(busroute_temp);
      }
  }
  
  for (int i = 0; i < STOPS_NUMBER; i++) {
    Tuple *Stop = dict_find(iterator, MESSAGE_KEY_Stop+i);
    Tuple *Desc = dict_find(iterator, MESSAGE_KEY_Desc+i);
    Tuple *Code = dict_find(iterator, MESSAGE_KEY_Code+i);
    if (Stop){
      snprintf(stops[i].stop, sizeof(stops[i].stop), "%s", Stop->value->cstring);
      snprintf(stops[i].desc, sizeof(stops[i].desc), "%s", Desc->value->cstring);
      snprintf(stops[i].code, sizeof(stops[i].code), "%s", Code->value->cstring);
    }
    if (i == STOPS_NUMBER-1){
      save_config();
      menu_layer_reload_data(menu_layer);
    }
  }
  
  Tuple *geoname = dict_find(iterator, MESSAGE_KEY_GEO_NAME);
  Tuple *geocode = dict_find(iterator, MESSAGE_KEY_GEO_CODE);
  Tuple *geocount = dict_find(iterator, MESSAGE_KEY_GEO_RESPONSE_COUNT);
  Tuple *geototal = dict_find(iterator, MESSAGE_KEY_GEO_TOTAL);
  
  if (geoname && geocode && geocount){
      snprintf(geoname_buf, sizeof(geoname_buf), "%s", geoname->value->cstring);
      snprintf(geocode_buf, sizeof(geocode_buf), "%s", geocode->value->cstring);
      snprintf(geocount_buf, sizeof(geocount_buf), "%d", (int)geocount->value->int32);
      snprintf(geototal_buf, sizeof(geototal_buf), "%d", (int)geototal->value->int32);

      int total_num = atoi(geototal_buf);
  
      int counter = atoi(geocount_buf);
      char *geoname_temp = malloc(sizeof(geoname_buf));
      strcpy(geoname_temp, geoname_buf);
      char *geocode_temp = malloc(sizeof(geocode_buf));
      strcpy(geocode_temp, geocode_buf);
      
      geo_menu_items[counter] = (SimpleMenuItem) {
        .title = geocode_temp,
        .subtitle = geoname_temp,
        .callback = geo_select_callback,
      };
  
      if (counter == total_num-1){
        GeoDrawResults(total_num);
        //free(busnum_temp);
        //free(geoname_temp);
      }
  }
}

//Creating main menu_layer
static void window_load(Window *window) {
	
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);
	bounds.origin.y += MENU_CELL_BASIC_HEADER_HEIGHT;
	bounds.size.h -= MENU_CELL_BASIC_HEADER_HEIGHT;
	menu_layer = menu_layer_create(bounds);
	menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_sections = menu_get_num_sections_callback,
		.get_num_rows = menu_get_num_rows_callback,
		.get_header_height = menu_get_header_height_callback,
		.draw_header = menu_draw_header_callback,
		.draw_row = menu_draw_row_callback,
		.select_click = menu_select_callback,
	});
	bounds = layer_get_bounds(window_layer);
	bounds.size.h = MENU_CELL_BASIC_HEADER_HEIGHT;
	text_layer = text_layer_create(bounds);
	text_layer_set_text_color(text_layer, GColorFromRGB(255, 255, 255));
	text_layer_set_background_color(text_layer, GColorFromRGB(0, 0, 255));
	menu_layer_set_click_config_onto_window(menu_layer, window);
	text_layer_set_text(text_layer, "LvivBus");
	layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
	layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
	menu_layer_destroy(menu_layer);
}

static void init(void) {
	load_config();
	window = window_create();
	app_message_register_inbox_received(inbox_received_callback);
	app_message_open(app_message_inbox_size_maximum()/2, app_message_outbox_size_maximum()/4);
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	const bool animated = true;
	window_stack_push(window, animated);
}

static void deinit(void) {
	window_destroy(s_window);
  window_destroy(geo_window);
	window_destroy(window);
}

int main(void) {
	
	init();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
	app_event_loop();
	deinit();
}
