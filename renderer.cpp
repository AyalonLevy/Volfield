constexpr u32 COLOR_BG = 0x000000;
constexpr u32 COLOR_PLAYER = 0x282828;
constexpr u32 COLOR_ENEMY = 0xFF0028;
constexpr u32 COLOR_TEXT = 0xFFFFFF;
constexpr s32 FONT_WIDTH = 3;
constexpr s32 FONT_HEIGHT = 5;
constexpr s32 FONT_PADDING = 1;

internal void
clear_screen(u32 color = COLOR_BG) {
	u32* pixel = (u32*)render_state.memory;
	for (int y = 0; y < render_state.height; y++) {
		for (int x = 0; x < render_state.width; x++) {
			*pixel++ = color;
		}
	}
}

internal void
draw_rect_in_pixels(int x0, int y0, int x1, int y1, u32 color) {

	x0 = clamp(0, x0, render_state.width);
	x1 = clamp(0, x1, render_state.width);
	y0 = clamp(0, y0, render_state.height);
	y1 = clamp(0, y1, render_state.height);

	for (int y = y0; y < y1; y++) {
		u32* pixel = (u32*)render_state.memory + x0 + y * render_state.width;
		for (int x = x0; x < x1; x++) {
			*pixel++ = color;
		}
	}
}

global_variable float render_scale = 0.01f;

internal void
draw_rect(float x, float y, float half_size_x, float half_size_y, u32 color) {

	x *= render_state.height * render_scale;
	y *= render_state.height * render_scale;
	half_size_x *= render_state.height * render_scale;
	half_size_y *= render_state.height * render_scale;

	x += render_state.width / 2.0f;
	y += render_state.height / 2.0f;

	// Change to pixels
	int x0 = x - half_size_x;
	int x1 = x + half_size_x;
	int y0 = y - half_size_y;
	int y1 = y + half_size_y;

	draw_rect_in_pixels(x0, y0, x1, y1, color);
}

internal void
draw_digit(int digit, int x, int y, int scale, u32 color) {
	// A 3x5 bitmap forn. 0-9, and index 10 is the '%' symbol
	const s8* font[12] = {
		"111101101101111",	// 0
		"010110010010111",	// 1
		"111001111100111",	// 2
		"111001111001111",	// 3
		"101101111001001",	// 4
		"111100111001111",	// 5
		"111100111101111",	// 6
		"111001010010010",	// 7
		"111101111101111",	// 8
		"111101111001111",	// 9
		"101001010100101",	// 10 (%)
		"000000000000010",	// 11 (.)
	};

	const s8* pattern = font[digit];

	// Loop through the FONT_HEIGHT rows and FONT_WIDTH columns
	for (int row = 0; row < FONT_HEIGHT; row++) {
		for (int col = 0; col < FONT_WIDTH; col++) {
			// If the map has a '1', draw a scaled-up pixel block
			if (pattern[row * 3 + col] == '1') {
				int px = x + (col * scale);
				int py = y - (row * scale);
				draw_rect_in_pixels(px, py, px + scale, py + scale, color);
			}
		}
	}
}

internal void
draw_ui_percent(Game_State* state) {
	// Calculate available space
	int available_height = state->arena.y_offset;

	// Calculate dynamic scale
	int scale = available_height / (FONT_HEIGHT + 2 * FONT_PADDING);
	if (scale < 2) scale = 2;	// Needs to be at least 2 so small_scale >= 1

	int small_scale = scale / 2;
	if (small_scale < 1) small_scale = 1;

	int spacing = 1 * scale;
	int small_spacing = 1 * small_scale;

	// Extract the digits
	int whole = state->filled_percent / 10;
	int frac = state->filled_percent % 10;
	int ones = whole % 10;
	int tens = (whole / 10) % 10;

	// Calculate total width for centering
	int total_width = 0;
	total_width += (FONT_WIDTH * scale) + spacing;				// Tens
	total_width += (FONT_WIDTH * scale) + spacing;				// Ones
	total_width += (FONT_WIDTH * small_scale) + small_spacing;	// Decimal Point
	total_width += (FONT_WIDTH * small_scale) + spacing;			// Fraction digit
	total_width += (FONT_WIDTH * scale);							// % sign

	int start_x = (render_state.width - total_width) / 2;

	// Calculate Y anchors to bottom-align the small and big text
	int start_y_main = available_height - 2 * scale;
	int start_y_small = start_y_main - (FONT_HEIGHT * small_scale);

	int current_x = start_x;

	// Draw percent
	// Tens
	draw_digit(tens, current_x, start_y_main, scale, COLOR_TEXT);
	current_x += (FONT_WIDTH * scale) + spacing;

	// Ones
	draw_digit(ones, current_x, start_y_main, scale, COLOR_TEXT);
	current_x += (FONT_WIDTH * scale) + spacing;

	// Decimal Point
	draw_digit(11, current_x, start_y_small, small_scale, COLOR_TEXT);
	current_x += (FONT_WIDTH * small_scale) + small_spacing;

	// Fraction Digit
	draw_digit(frac, current_x, start_y_small, small_scale, COLOR_TEXT);
	current_x += (FONT_WIDTH * small_scale) + spacing;

	// % sign
	draw_digit(10, current_x, start_y_main, scale, COLOR_TEXT);
}

internal float
pixels_to_world_x(int pixel_x, int screen_width, int screen_height) {
	float centered_x = (float)pixel_x - ((float)screen_width / 2.0f);
	return centered_x / ((float)screen_height * render_scale);
}

internal float
pixels_to_world_y(int pixel_y, int screen_height) {
	float centered_y = (float)pixel_y - ((float)screen_height / 2.0f);
	return centered_y / ((float)screen_height * render_scale);
}