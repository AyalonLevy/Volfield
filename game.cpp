#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)


static const Level_Data level_config[] = {
	{1, 3, BEHAVIOR_BOUNCE, 20.f},
	{2, 5, BEHAVIOR_BOUNCE, 25.f},
	{3, 8, BEHAVIOR_BOUNCE, 30.f},
};

internal int
get_completion_bonus(int percent) {
	if (percent < 800) return 0;

	static const int score_table[] = {
		10000,  // 80
		11000,  // 81
		12000,  // 82
		13000,  // 83
		14000,  // 84
		15000,  // 85
		16000,  // 86
		17000,  // 87
		18000,  // 88
		19000,  // 89
		20000,  // 90
		22000,  // 91
		24000,  // 92
		26000,  // 93
		28000,  // 94
		30000,  // 95
		35000,  // 96
		40000,  // 97
		50000,  // 98
		100000, // 99.0
		110000, // 99.1
		120000, // 99.2
		130000, // 99.3
		140000, // 99.4
		150000, // 99.5
		200000, // 99.6
		250000, // 99.7
		350000, // 99.8
		500000, // 99.9
	};

	if (percent <= 990) {

		int tens_digit = (percent / 10);
		return score_table[tens_digit - 80];
	}

	return score_table[19 + (percent - 990)];
}

internal void
init_game(Game_State* state) {
	// Get arena boundaries in world position
	state->min_pos_x = pixels_to_world_x(state->arena.x_offset + (EDGE_THICKNESS / 2), render_state.width, render_state.height);
	state->max_pos_x = pixels_to_world_x(state->arena.x_offset + state->arena.width - (EDGE_THICKNESS / 2), render_state.width, render_state.height);
	state->min_pos_y = pixels_to_world_y(state->arena.y_offset + (EDGE_THICKNESS / 2), render_state.height);
	state->max_pos_y = pixels_to_world_y(state->arena.y_offset + state->arena.height - (EDGE_THICKNESS / 2), render_state.height);

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
	state->player_can_exit_edge = false;

	// Init Boss
	state->boss.x = 0.0;
	state->boss.y = 0.0;
	state->boss.dx = 0.6;
	state->boss.dy = 0.8;
	state->boss.speed = 20;
	state->boss.half_size_x = 6;
	state->boss.half_size_y = 6;
	state->boss.type = ENTITY_BOSS;
	state->boss.is_active = true;
}


internal void
load_level(Game_State* state, int level_idx) {
	state->status = STATUS_PLAYING;
	state->current_level_index = level_idx % 3;  // TODO: Remove the % 3 -> for testing only

	Level_Data config = level_config[state->current_level_index];

	// Clear and reset arena
	init_grid(state, render_state.width, render_state.height);
	init_game(state);

	// Spawn Minions
	for (int i = 0; i < MAX_ENEMIES; i++) {
		if (i < config.minion_count) {
			state->enemies[i].is_active = true;
			state->enemies[i].speed = config.minion_speed;

			// Distribute minions in a circle around the boss area
			state->enemies[i].x = ((i % 3) - 1) * 1.5 * state->boss.half_size_x;
			state->enemies[i].y = ((i / 3) - 1) * 1.5 * state->boss.half_size_x;
			
			// Randomize direction
			state->enemies[i].dx = (i % 2 == 0) ? 1.0f : -1.0f;
			state->enemies[i].dy = (i % 3 == 0) ? 1.0f : -1.0f;

			state->enemies[i].half_size_x = 2;
			state->enemies[i].half_size_y = 2;
		}
		else {
			state->enemies[i].is_active = false;
		}
	}
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

	state->score += state->score_accumulator;
	state->score_accumulator = 0;
}

internal void
update_enemy_bounce(Entity* enemy, float dt, Game_State* state) {
	if (!enemy->is_active) return;

	// Calculate new powition
	float new_x = enemy->x + (enemy->dx * enemy->speed * dt);
	float new_y = enemy->y + (enemy->dy * enemy->speed * dt);

	// Check X-Axis collision
	int check_x = world_to_arena_x(state, new_x + (enemy->dx > 0 ? enemy->half_size_x : -enemy->half_size_x), render_state.width, render_state.height);
	int current_y_grid = world_to_arena_y(state, enemy->y, render_state.height);

	check_x = clamp(0, check_x, state->arena.width - 1);
	current_y_grid = clamp(0, current_y_grid, state->arena.height - 1);

	TileState tile_x = state->arena.memory[current_y_grid * state->arena.width + check_x];

	if (tile_x == FILLED || tile_x == EDGE || tile_x == TRAIL) {
		enemy->dx *= -1.0f; // Reverse horizontal direction
		new_x = enemy->x;   // Cancel movement on this axis for this frame
	}

	// Check Y-Axis collision
	int check_y = world_to_arena_y(state, new_y + (enemy->dy > 0 ? enemy->half_size_y : -enemy->half_size_y), render_state.height);
	int current_x_grid = world_to_arena_x(state, enemy->x, render_state.width, render_state.height);

	check_y = clamp(0, check_y, state->arena.height - 1);
	current_x_grid = clamp(0, current_x_grid, state->arena.width - 1);

	TileState tile_y = state->arena.memory[check_y * state->arena.width + current_x_grid];

	if (tile_y == FILLED || tile_y == EDGE || tile_y == TRAIL) {
		enemy->dy *= -1.0f; // Reverse horizontal direction
		new_y = enemy->y;   // Cancel movement on this axis for this frame
	}

	// Apply position
	enemy->x = clamp(state->min_pos_x, new_x, state->max_pos_x);
	enemy->y = clamp(state->min_pos_y, new_y, state->max_pos_y);
}

internal void
check_entity_collisions(Game_State* state) {
	// Get player grid position
	int px = world_to_arena_x(state, state->player.x, render_state.width, render_state.height);
	int py = world_to_arena_y(state, state->player.y, render_state.height);
	TileState pt = state->arena.memory[py * state->arena.width + px];

	// Safety check - Invincible on EDGE (while has shield)
	if (pt == EDGE) return;

	// Check Boss
	if (rects_overlap(state->player.x, state->player.y, state->player.half_size_x, state->player.half_size_y, state->boss.x, state->boss.y, state->boss.half_size_x, state->boss.half_size_y)) {
		state->player.is_active = false;
		state->status = STATUS_FAILED;
		return;
	}

	// Check Minions
	Level_Data config = level_config[state->current_level_index];
	for (int i = 0; i < config.minion_count; i++) {
		if (state->enemies[i].is_active) {
			if (rects_overlap(state->player.x, state->player.y, state->player.half_size_x, state->player.half_size_y, state->enemies[i].x, state->enemies[i].y, state->enemies[i].half_size_x, state->enemies[i].half_size_y)) {
				state->player.is_active = false;
				state->status = STATUS_FAILED;
				return;
			}
		}
	}
}

internal void
simulate_game(Input* input, float dt, Game_State* state) {
	// Initialize grid
	if (!state->is_initialized) {
		init_grid(state, render_state.width, render_state.height);

		load_level(state, 0);

		state->is_initialized = true;
	}

	clear_screen();

	if (state->status == STATUS_PLAYING) {
		// Player Movement
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

			if (is_down(BUTTON_ACTION)) {
				state->player_can_exit_edge = true;
				draw_rect(0, 0, 1, 1, 0xffffff);
			}

			if (released(BUTTON_ACTION)) {
				state->player_can_exit_edge = false;
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
					if (!state->player_is_drawing_trail && !state->player_can_exit_edge) {
						target_arena_x = check_x - step_x;
						target_arena_y = check_y - step_y;
						final_tile = state->arena.memory[target_arena_y * state->arena.width + target_arena_x];
						collided = true;
						break;
					} else {
						final_tile = EMPTY;
					}
				}
			}

			// Apply Trailing and Capture
			// Draw trail if stepped into EMPTY or just closed a loop
			if (final_tile == EMPTY || hit_wall_while_drawing) {
				draw_trail(state, current_arena_x, current_arena_y, target_arena_x, target_arena_y);

				// Calculate how many grid cells the player moved this frame
				int cells_traversed = abs(target_arena_x - current_arena_x) + abs(target_arena_y - current_arena_y);

				// Award 10 points per cell drawn in the void
				state->score_accumulator += cells_traversed * CAPTURE_POINTS;
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

		// Enemy Logic
		// Update Boss
		update_enemy_bounce(&state->boss, dt, state);

		// Update minions
		for (int i = 0; i < MAX_ENEMIES; i++) {
			update_enemy_bounce(&state->enemies[i], dt, state);
		}

		// Collision and Win/Loss
		check_entity_collisions(state);

		// Win condition
		if (state->filled_percent >= state->win_condition) {
			state->score += get_completion_bonus(state->filled_percent);
			state->status = STATUS_LEVEL_CLEARED;
		}
	} else {
		// Transition Logic
		if (pressed(BUTTON_ACTION)) {
			if (state->status == STATUS_LEVEL_CLEARED) load_level(state, state->current_level_index + 1);
			else load_level(state, 0);

			state->status = STATUS_PLAYING;
		}
	}

	// Drawing
	draw_grid(state);


	// Draw player
	if (state->player.is_active) draw_rect(state->player.x, state->player.y, state->player.half_size_x, state->player.half_size_y, COLOR_PLAYER);
	
	//// Draw enemies
	draw_rect(state->boss.x, state->boss.y, state->boss.half_size_x, state->boss.half_size_y, COLOR_ENEMY);

	Level_Data config = level_config[state->current_level_index];
	for (int i = 0; i < config.minion_count; i++) {
		draw_rect(state->enemies[i].x, state->enemies[i].y, state->enemies[i].half_size_x, state->enemies[i].half_size_y, COLOR_ENEMY);
	}

	// Draw the percent filled
	draw_ui_percent(state);

	// Draw the top scores
	draw_ui_top(state);

	// Draw overlay Text
	if (state->status != STATUS_PLAYING) {
		int scale = (render_state.height / UI_BASE_RESOLUTION_HEIGHT) * 2;
		int center_y = render_state.height / 2;

		const char* status_text = (state->status == STATUS_LEVEL_CLEARED) ? "LEVEL CLEARED" : "FAILED";

		draw_text(status_text, (render_state.width - measure_text(status_text, scale)) / 2, center_y, scale, COLOR_TEXT);
		draw_text("PRESS SPACE", (render_state.width - measure_text("PRESS SPACE", scale)) / 2, center_y - (10 * scale), scale, COLOR_TEXT);
	}
}