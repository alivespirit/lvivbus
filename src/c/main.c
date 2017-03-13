#include <pebble.h>

#define KEY_REQUEST 0
#define KEY_RESPONSE 1
#define KEY_RESPONSE_TEXT 2

#define NUM_MENU_SECTIONS 1
#define NUM_FIRST_MENU_ITEMS 10

static Window *window;
static MenuLayer *menu_layer;
static TextLayer *text_layer;
static char response[128];
static char busnum_buf[128];
static char busroute_buf[128];

static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_FIRST_MENU_ITEMS];

static void menu_selectt_callback(int index, void *ctx) {
  s_first_menu_items[index].subtitle = "You've hit select here!";
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));

}

static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {

        // One menu section
	return 1;
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {

        // Six menu items
	return 6;
}

static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {

        // No header
	return 0;
}

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {

        // No header
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {

        // Menu items
	switch (cell_index->section) {
		case 0:
			switch (cell_index->row) {
				case 0:
					menu_cell_basic_draw(ctx, cell_layer, "Petliury", "0172", NULL);
					break;
				case 1:
					menu_cell_basic_draw(ctx, cell_layer, "Svobody Avenue", "0067", NULL);
					break;
				case 2: 
					menu_cell_basic_draw(ctx, cell_layer, "Weather", "Time/date/weather", NULL);
					break;
				case 3: 
					menu_cell_basic_draw(ctx, cell_layer, "Pacman", "Pacman animation", NULL);
					break;
				case 4: 
					menu_cell_basic_draw(ctx, cell_layer, "Rainbow", "Scrolling rainbow", NULL);
					break;
				case 5: 
					menu_cell_basic_draw(ctx, cell_layer, "Snow", "Snow animation", NULL);
					break;
			}
			break;
	}
}

static void SendRequest(char *data) {

        // Send request from watch to phone
	DictionaryIterator *iter1;
	app_message_outbox_begin(&iter1);
	dict_write_cstring(iter1, KEY_REQUEST, data);
	app_message_outbox_send();
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
	
        // Menu selection
	switch (cell_index->row) {
		case 0:
			SendRequest("0172");
			break;
		case 1:
			SendRequest("0067");
			break;
		case 2:
			SendRequest("weather");
			break;
		case 3:
			SendRequest("pacman");
			break;
		case 4:
			SendRequest("rainbow");
			break;
		case 5:
			SendRequest("0050");
			break;
	}
}


static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

        // Receive response from phone to watch
	//Tuple *t = dict_read_first(iterator);
  Tuple *busnum = dict_find(iterator, KEY_RESPONSE);
  Tuple *busroute = dict_find(iterator, KEY_RESPONSE_TEXT);
  int num_a_items = 0;
	while (busnum != NULL) {

		//switch(t->key) {
    if(busnum && busroute) {
			//case KEY_RESPONSE:
		  snprintf(busnum_buf, sizeof(busnum_buf), "%s", busnum->value->cstring);
      snprintf(busroute_buf, sizeof(busroute_buf), "%s", busroute->value->cstring);
			//text_layer_set_text(text_layer, busnum_buf);
      s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
        .title = busnum_buf,
        .subtitle = busroute_buf,
        .callback = menu_selectt_callback,
      };
      num_a_items=num_a_items+1;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Received busnum: %s", busnum_buf);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Received busroute: %s", busroute_buf);
		  break;
      //case KEY_RESPONSE_TEXT:
      //	snprintf(response, sizeof(response), "%s", t->value->cstring);
			//	text_layer_set_text(text_layer, response);
      // APP_LOG(APP_LOG_LEVEL_DEBUG, "Received: %s", response);
			//	break;
		}
		busnum = dict_read_next(iterator);
	}

  s_menu_sections[0] = (SimpleMenuSection) {
    .num_items = NUM_FIRST_MENU_ITEMS,
    .items = s_first_menu_items,
  };
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));

}

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
#ifdef PBL_COLOR
	menu_layer_set_highlight_colors(menu_layer, GColorFromRGB(0, 255, 255), GColorFromRGB(0, 0, 0));
#endif
	menu_layer_set_click_config_onto_window(menu_layer, window);
	text_layer_set_text(text_layer, "LvivBus");
	layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
	layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {

	menu_layer_destroy(menu_layer);
}

static void init(void) {
	
	response[0] = '\0';
	window = window_create();
	app_message_register_inbox_received(inbox_received_callback);
	app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	window_set_window_handlers(window, (WindowHandlers) {
		.load = window_load,
		.unload = window_unload,
	});
	const bool animated = true;
	window_stack_push(window, animated);
}

static void deinit(void) {
	
	window_destroy(window);
}

int main(void) {
	
	init();
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
	app_event_loop();
	deinit();
}
