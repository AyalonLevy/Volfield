constexpr u32 COLOR_BG = 0x000000;
constexpr u32 COLOR_PLAYER = 0x282828;
constexpr u32 COLOR_ENEMY = 0xFF0028;
constexpr u32 COLOR_TEXT = 0xFFFFFF;
constexpr u32 COLOR_LABEL = 0xFF0000;
constexpr s32 FONT_WIDTH = 5;
constexpr s32 FONT_HEIGHT = 7;
constexpr s32 FONT_PADDING = 1;
constexpr s32 SCORE_GAP = 2;
constexpr s32 UI_BASE_RESOLUTION_HEIGHT = 300; // Used to calculate proportional UI scaling
constexpr s32 UI_TOP_PADDING = 5;              // Margin from the top/bottom of the window
constexpr s32 UI_SIDE_MARGIN = 20;             // Margin from the left/right edges


internal const char* get_font_pattern_5x7(char c) {
	// 0-25: A-Z
	// 26-35: 0-9
	// 36: ':'
	// 37: '.'
	// 38: '&'
	// 39: ' ' (Space)
	const char* font_5x7[40] = {
		"01110100011000111111100011000110001", // A
		"11110100011000111110100011000111110", // B
		"01111100001000010000100001000001111", // C
		"11110100111000110001100011001111110", // D
		"11111100001000011110100001000011111", // E
		"11111100001000011110100001000010000", // F
		"01111100001000010011100011000101111", // G
		"10001100011000111111100011000110001", // H
		"01110001000010000100001000010001110", // I
		"00011000010000100001000011000101110", // J
		"10001100101010011000101001001010001", // K
		"10000100001000010000100001000011111", // L
		"10001110111010110001100011000110001", // M
		"10001110011010110011100011000110001", // N
		"01110100011000110001100011000101110", // O
		"11110100011000111110100001000010000", // P
		"01110100011000110001101011001001101", // Q
		"11110100011000111110101001001010001", // R
		"01111100001000001110000010000111110", // S
		"11111001000010000100001000010000100", // T
		"10001100011000110001100011000101110", // U
		"10001100011000110001100010101000100", // V
		"10001100011000110101110111000110001", // W
		"10001100010101000100010101000110001", // X
		"10001100010101000100001000010000100", // Y
		"11111000010001000100010001000011111", // Z
		"01110100011001110101110011000101110", // 0
		"00100011000010000100001000010001110", // 1
		"01110100010000100010001000100011111", // 2
		"01110100010000100110000011000101110", // 3
		"00010001100101010010111110001000010", // 4
		"11111100001111000001000011000101110", // 5
		"01110100001111010001100011000101110", // 6
		"11111000010001000100010000100001000", // 7
		"01110100011000101110100011000101110", // 8
		"01110100011000110001011110000101110", // 9
		"00000010000000000000010000000000000", // :
		"00000000000000000000010000000000000", // .
		"11001110010001000100010001001110011", // %
		"00000000000000000000000000000000000"  // Space
	};

	if (c >= 'A' && c <= 'Z') return font_5x7[c - 'A'];
	if (c >= 'a' && c <= 'z') return font_5x7[c - 'a']; // Handle lowercase gracefully
	if (c >= '0' && c <= '9') return font_5x7[26 + (c - '0')];
	if (c == ':') return font_5x7[36];
	if (c == '.') return font_5x7[37];
	if (c == '%') return font_5x7[38];

	return font_5x7[39]; // Space or unknown character
}

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
draw_char(char c, int x, int y, int scale, u32 color) {
	const char* pattern = get_font_pattern_5x7(c);

		for (int row = 0; row < FONT_HEIGHT; row++) {
			for (int col = 0; col < FONT_WIDTH; col++) {
				if (pattern[row * FONT_WIDTH + col] == '1') {
					int px = x + (col * scale);
					int py = y - (row * scale);
					draw_rect_in_pixels(px, py, px + scale, py + scale, color);
				}
			}
		}
}

internal int
measure_text(const char* text, int scale) {
	int len = 0;
	while (text[len]) len++;

	return len * (FONT_WIDTH * scale) + (len - 1) * (FONT_PADDING * scale);
}

internal void
draw_text(const char* text, int x, int y, int scale, u32 color) {
	int current_x = x;
	while (*text) {
		draw_char(*text, current_x, y, scale, color);
		current_x += (FONT_WIDTH + FONT_PADDING) * scale;
		text++;
	}
}

internal void
draw_ui_percent(Game_State* state) {
	// Calculate available space
	int available_height = state->arena.y_offset;

	// Calculate dynamic scale
	int scale = available_height / (FONT_HEIGHT + 2 * UI_TOP_PADDING);
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
	int start_y_main = available_height - 2 * scale - 2 * UI_TOP_PADDING;
	int start_y_small = start_y_main - (FONT_HEIGHT * small_scale);

	int current_x = start_x;

	// Draw percent
	// Tens
	char buffer[64];
	wsprintfA(buffer, "%d", tens);  // Use wsprintfA to format integer into text
	draw_text(buffer, current_x, start_y_main, scale, COLOR_TEXT);
	current_x += (FONT_WIDTH * scale) + spacing;

	// Ones
	wsprintfA(buffer, "%d", ones);  // Use wsprintfA to format integer into text
	draw_text(buffer, current_x, start_y_main, scale, COLOR_TEXT);
	current_x += (FONT_WIDTH * scale) + spacing;

	// Decimal Point
	draw_text(".", current_x, start_y_small, small_scale, COLOR_TEXT);
	current_x += (FONT_WIDTH * small_scale) + small_spacing;

	// Fraction Digit
	wsprintfA(buffer, "%d", frac);  // Use wsprintfA to format integer into text
	draw_text(buffer, current_x, start_y_small, small_scale, COLOR_TEXT);
	current_x += (FONT_WIDTH * small_scale) + spacing;

	// % sign
	draw_text("%", current_x, start_y_main, scale, COLOR_TEXT);
}

internal void
draw_ui_top(Game_State* state) {
	int available_height = render_state.height - state->arena.height - state->arena.y_offset;

	int scale = (available_height / (FONT_HEIGHT + SCORE_GAP + 2 * UI_TOP_PADDING));
	if (scale < 1) scale = 1;

	char num_buffer[64];

	int label_y = render_state.height - (UI_TOP_PADDING * scale);

	int line_gap = SCORE_GAP * scale;
	int num_y = label_y - (FONT_HEIGHT * scale) - line_gap;

	// High Score
	const char* hs_label = "HIGH SCORE";
	wsprintfA(num_buffer, "%d", state->high_score);

	// Measure both parts to find the total width for perfect centering
	int hs_label_width = measure_text(hs_label, scale);
	int hs_num_width = measure_text(num_buffer, scale);

	int max_width = hs_label_width > hs_num_width ? hs_label_width : hs_num_width;

	int screen_center_x = render_state.width / 2;

	int hs_label_x = screen_center_x - (hs_label_width / 2);
	int hs_num_x = screen_center_x - (hs_num_width / 2);

	// Draw the Label (Red) followed immediately by the Number (White)
	draw_text(hs_label, hs_label_x, label_y, scale, COLOR_LABEL);
	draw_text(num_buffer, hs_num_x, num_y, scale, COLOR_TEXT);

	// Score
	const char* score_label = "SCORE";
	wsprintfA(num_buffer, "%d", state->score);

	int score_label_width = measure_text(score_label, scale);
	int score_num_width = measure_text(num_buffer, scale);

	// Plant the label on the left margin
	int score_label_x = UI_SIDE_MARGIN * scale;

	// Find the center of the "SCORE" label so we can align the number directly beneath it
	int score_label_center_x = score_label_x + (score_label_width / 2);
	int score_num_x = score_label_center_x - (score_num_width / 2);

	// Prevent the number from clipping off the left edge if it gets too wide (e.g., 0 padding)
	if (score_num_x < 0) score_num_x = 0;

	draw_text(score_label, score_label_x, label_y, scale, COLOR_LABEL);
	draw_text(num_buffer, score_num_x, num_y, scale, COLOR_TEXT);
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