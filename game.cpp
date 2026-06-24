#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)



internal void
init_game(Game_State* state) {
	// Init player

	int target_pixel_x = state->arena.x_offset + (state->arena.width / 2);
	int target_pixel_y = state->arena.y_offset + (EDGE_THICKNESS / 2);

	state->speed = 30.0f;  // unitp per second

	state->player.x = pixels_to_world_x(target_pixel_x, render_state.width, render_state.height);
	state->player.y = pixels_to_world_y(target_pixel_y, render_state.height);
	state->player.half_size_x = 2;
	state->player.half_size_y = 2;
	state->player.type = ENTITY_PLAYER;
	state->player.is_active = true;

	// Init Boss
	state->boss.x = 0.0;
	state->boss.y = 0.0;
	state->boss.half_size_x = 6;
	state->boss.half_size_y = 6;
	state->boss.type = ENTITY_BOSS;
	state->boss.is_active = true;
}

internal void
finalize_capture(Game_State* state) {
	// Get boss position as grid index
	int boss_arena_x = world_to_arena_x(state, state->boss.x, render_state.width, render_state.height);
	int boss_arena_y = world_to_arena_y(state, state->boss.y, render_state.height);

	// Ensure boss indices are clamped
	boss_arena_x = clamp(0, boss_arena_x, state->arena.width - 1);
	boss_arena_y = clamp(0, boss_arena_y, state->arena.height - 1);

	// Flood fill the boss's side. Using 4 as a temporary invisible state.
	// The player's trail acts as a dam, stopping the fill from raching the captured area
	flood_fill(state, boss_arena_x, boss_arena_y, EMPTY, BOSS);

	// Evaluate the whole board
	for (int i = 0; i < state->arena.width * state->arena.height; i++) {
		if (state->arena.memory[i] == EMPTY) {
			// The area where the boss fill couldn't reach
			state->arena.memory[i] = FILLED;
		} else if (state->arena.memory[i] == TRAIL) {
			state->arena.memory[i] = EDGE;
		} else if (state->arena.memory[i] == BOSS) {
			state->arena.memory[i] = EMPTY;
		}
	}

	// Remove interior edges
	for (int y = 0; y < state->arena.height; y++) {
		for (int x = 0; x < state->arena.width; x++) {

			int index = y * state->arena.width + x;

			if (state->arena.memory[index] == EDGE) {
				bool touches_empty = false;

				// Check all 8 surrounding neighbors (3x3 grid)
				// This prevents diagonal visual glitches
				for (int dy = -1; dy <= 1; dy++) {
					for (int dx = -1; dx <= 1; dx++) {
						int nx = x + dx;
						int ny = y + dy;

						// Keep checks inside arena bounds
						if (nx >= 0 && nx < state->arena.width && ny >= 0 && ny < state->arena.height) {
							if (state->arena.memory[ny * state->arena.width + nx] == EMPTY) {
								touches_empty = true;
							}
						}
					}
				}

				// If it is completely buried and touches no empty space, it becomes filled territory!
				if (!touches_empty) {
					state->arena.memory[index] = FILLED;
				}
			}
		}
	}
}


internal void
simulate_game(Input* input, float dt, Game_State* state) {
	// Initialize grid
	if (!state->is_initialized) {
		init_grid(state, render_state.width, render_state.height);

		init_game(state);

		// Get aren boundaries in world position
		state->min_pos_x = pixels_to_world_x(state->arena.x_offset + (EDGE_THICKNESS / 2), render_state.width, render_state.height);
		state->max_pos_x = pixels_to_world_x(state->arena.x_offset + state->arena.width - (EDGE_THICKNESS / 2), render_state.width, render_state.height);
		state->min_pos_y = pixels_to_world_y(state->arena.y_offset + (EDGE_THICKNESS / 2), render_state.height);
		state->max_pos_y = pixels_to_world_y(state->arena.y_offset + state->arena.height - (EDGE_THICKNESS / 2), render_state.height);

		state->is_initialized = true;
	}

	clear_screen();

	if (state->player.is_active) {
		// Calculate the proposed new position
		float new_player_x = state->player.x;
		float new_player_y = state->player.y;

		if (is_down(BUTTON_UP)) {
			new_player_y += state->speed * dt;
		}
		else if (is_down(BUTTON_DOWN)) {
			new_player_y -= state->speed * dt;
		}
		else if (is_down(BUTTON_LEFT)) {
			new_player_x -= state->speed * dt;
		}
		else if (is_down(BUTTON_RIGHT)) {
			new_player_x += state->speed * dt;
		}

		if (is_down(BUTTON_SHOOT)) {
			draw_rect(0, 0, 1, 1, 0xffffff);
		}

		// Convert current and proposed floats into Arena Grid Indices
		int current_arena_x = world_to_arena_x(state, state->player.x, render_state.width, render_state.height);
		int current_arena_y = world_to_arena_y(state, state->player.y, render_state.height);

		int target_arena_x = world_to_arena_x(state, new_player_x, render_state.width, render_state.height);
		int target_arena_y = world_to_arena_y(state, new_player_y, render_state.height);

		// Keep the target indices stricly inside the array bounds
		current_arena_x = clamp(0, current_arena_x, state->arena.width - 1);
		current_arena_y = clamp(0, current_arena_y, state->arena.height - 1);

		target_arena_x = clamp(0, target_arena_x, state->arena.width - 1);
		target_arena_y = clamp(0, target_arena_y, state->arena.height - 1);

		// --- SWEEP COLLISION LOGIC ---
		int step_x = (target_arena_x > current_arena_x) ? 1 : (target_arena_x < current_arena_x) ? -1 : 0;
		int step_y = (target_arena_y > current_arena_y) ? 1 : (target_arena_y < current_arena_y) ? -1 : 0;

		int check_x = current_arena_x;
		int check_y = current_arena_y;
		bool collided = false;
		bool hit_wall_while_drawing = false;
		TileState final_tile = state->arena.memory[current_arena_y * state->arena.width + current_arena_x];

		while (check_x != target_arena_x || check_y != target_arena_y) {
			check_x += step_x;
			check_y += step_y;
			TileState tile = state->arena.memory[check_y * state->arena.width + check_x];

			if (tile == EDGE) {
				if (state->player_is_drawing_trail) {
					target_arena_x = check_x;
					target_arena_y = check_y;
					final_tile = EDGE;
					collided = true;
					hit_wall_while_drawing = true;
					break;
				} else {
					final_tile = EDGE;
				}
			}
			else if (tile == FILLED || tile == TRAIL) {
				target_arena_x = check_x - step_x;
				target_arena_y = check_y - step_y;
				final_tile = state->arena.memory[target_arena_y * state->arena.width + target_arena_x];
				collided = true;
				break;
			}
			else {
				final_tile = EMPTY;
			}
		}

		// Apply Trailing and Capture
		// Draw trail if stepped into EMPTY or just closed a loop
		if (final_tile == EMPTY || hit_wall_while_drawing) {
			draw_trail(state, current_arena_x, current_arena_y, target_arena_x, target_arena_y);
		}

		// Capture trigger
		if(hit_wall_while_drawing) {
			finalize_capture(state);
			state->player_is_drawing_trail = false;
		}

		// Start drawing trigger
		if (final_tile == EMPTY && !state->player_is_drawing_trail) {
			state->player_is_drawing_trail = true;
		}

		// Apply Position
		if (collided) {
			state->player.x = pixels_to_world_x(target_arena_x + state->arena.x_offset, render_state.width, render_state.height);
			state->player.y = pixels_to_world_y(target_arena_y + state->arena.y_offset, render_state.height);
		}
		else {
			state->player.x = new_player_x;
			state->player.y = new_player_y;
		}

		state->player.x = clamp(state->min_pos_x, state->player.x, state->max_pos_x);
		state->player.y = clamp(state->min_pos_y, state->player.y, state->max_pos_y);
	}

	draw_grid(state);

	// Check Player vs Boss Collision
	if (rects_overlap(state->player.x, state->player.y, state->player.half_size_x, state->player.half_size_y, state->boss.x, state->boss.y, state->boss.half_size_x, state->boss.half_size_y)) {
		// GAME OVER - PLAYER DEATH
		state->player.is_active = false;
	}

	// Draw player
	if (state->player.is_active) draw_rect(state->player.x, state->player.y, state->player.half_size_x, state->player.half_size_y, COLOR_PLAYER);
	
	// Draw enemies
	draw_rect(state->boss.x, state->boss.y, state->boss.half_size_x, state->boss.half_size_y, COLOR_ENEMY);

	// Draw the percent filled
	draw_ui_percent(state);
}