#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)

float player_pos_x = 0.0f;
float player_pos_y = 0.0f;

float min_player_pos_x = 0.0f;
float max_player_pos_x = 0.0f;
float min_player_pos_y = 0.0f;
float max_player_pos_y = 0.0f;

global_variable bool is_initialized = false;

internal void
simulate_game(Input* input, float dt) {
	// Initialize grid
	if (!is_initialized) {
		init_grid(render_state.width, render_state.height);

		// Start player at center of bottom edge
		int target_pixel_x = arena.x_offset + (arena.width / 2);
		int target_pixel_y = arena.y_offset + EDGE_THICKNESS;

		player_pos_x = pixels_to_world_x(target_pixel_x, render_state.width, render_state.height);
		player_pos_y = pixels_to_world_y(target_pixel_y, render_state.height);

		min_player_pos_x = pixels_to_world_x(arena.x_offset + EDGE_THICKNESS, render_state.width, render_state.height);
		max_player_pos_x = pixels_to_world_x(arena.x_offset + arena.width - EDGE_THICKNESS, render_state.width, render_state.height);
		min_player_pos_y = pixels_to_world_y(arena.y_offset + EDGE_THICKNESS, render_state.height);
		max_player_pos_y = pixels_to_world_y(arena.y_offset + arena.height - EDGE_THICKNESS, render_state.height);

		is_initialized = true;
	}

	// Clear screen
	clear_screen();

	float speed = 50.f;  // unitp per second
	if (is_down(BUTTON_UP)) player_pos_y += speed * dt;
	if (is_down(BUTTON_DOWN)) player_pos_y -= speed * dt;
	if (is_down(BUTTON_LEFT)) player_pos_x -= speed * dt;
	if (is_down(BUTTON_RIGHT)) player_pos_x += speed * dt;
	if (is_down(BUTTON_SHOOT)) {
		draw_rect(0, 0, 1, 1, 0xffffff);
	}

	player_pos_x = clamp(min_player_pos_x, player_pos_x, max_player_pos_x);
	player_pos_y = clamp(min_player_pos_y, player_pos_y, max_player_pos_y);

	draw_grid();

	draw_rect(player_pos_x, player_pos_y, 3, 3, COLOR_PLAYER);

	//draw_rect(30, 10, 10, 50, 0xffff22);
}