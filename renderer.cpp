constexpr u32 COLOR_BG = 0x000000;
constexpr u32 COLOR_PLAYER = 0x282828;

internal void
clear_screen(u32 color=COLOR_BG) {
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