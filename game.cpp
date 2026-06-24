#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)

enum EntityType {
	ENTITY_PLAYER,
	ENTITY_BOSS,
	ENTITY_MINION
};

struct Entity {
	float x, y;						// World position
	float half_size_x, half_size_y;	// Collider size
	EntityType type;				// Tag for behavior
	bool is_active;					// For spawning/despawning
};

global_variable Entity player;
global_variable Entity boss;
global_variable Entity enemies[10];

float min_pos_x = 0.0f;
float max_pos_x = 0.0f;
float min_pos_y = 0.0f;
float max_pos_y = 0.0f;


global_variable bool is_initialized = false;
global_variable bool player_is_drawing_trail = false;

internal void
init_game() {
	// Init player

	int target_pixel_x = arena.x_offset + (arena.width / 2);
	int target_pixel_y = arena.y_offset + (EDGE_THICKNESS / 2);

	player.x = pixels_to_world_x(target_pixel_x, render_state.width, render_state.height);
	player.y = pixels_to_world_y(target_pixel_y, render_state.height);
	player.half_size_x = 2;
	player.half_size_y = 2;
	player.type = ENTITY_PLAYER;
	player.is_active = true;

	// Init Boss
	boss.x = 0.0;
	boss.y = 0.0;
	boss.half_size_x = 6;
	boss.half_size_y = 6;
	boss.type = ENTITY_BOSS;
	boss.is_active = true;
}

internal void
finalize_capture() {
	// Get boss position as grid index
	int boss_arena_x = world_to_arena_x(boss.x, render_state.width, render_state.height);
	int boss_arena_y = world_to_arena_y(boss.y, render_state.height);

	// Ensure boss indices are clamped
	boss_arena_x = clamp(0, boss_arena_x, arena.width - 1);
	boss_arena_y = clamp(0, boss_arena_y, arena.height - 1);

	// Flood fill the boss's side. Using 4 as a temporary invisible state.
	// The player's trail acts as a dam, stopping the fill from raching the captured area
	flood_fill(boss_arena_x, boss_arena_y, EMPTY, BOSS);

	// Evaluate the whole board
	for (int i = 0; i < arena.width * arena.height; i++) {
		if (arena.memory[i] == EMPTY) {
			// The area where the boss fill couldn't reach
			arena.memory[i] = FILLED;
		} else if (arena.memory[i] == TRAIL) {
			arena.memory[i] = EDGE;
		} else if (arena.memory[i] == BOSS) {
			arena.memory[i] = EMPTY;
		}
	}

	// Remove interior edges
	for (int y = 0; y < arena.height; y++) {
		for (int x = 0; x < arena.width; x++) {

			int index = y * arena.width + x;

			if (arena.memory[index] == EDGE) {
				bool touches_empty = false;

				// Check all 8 surrounding neighbors (3x3 grid)
				// This prevents diagonal visual glitches
				for (int dy = -1; dy <= 1; dy++) {
					for (int dx = -1; dx <= 1; dx++) {
						int nx = x + dx;
						int ny = y + dy;

						// Keep checks inside arena bounds
						if (nx >= 0 && nx < arena.width && ny >= 0 && ny < arena.height) {
							if (arena.memory[ny * arena.width + nx] == EMPTY) {
								touches_empty = true;
							}
						}
					}
				}

				// If it is completely buried and touches no empty space, it becomes filled territory!
				if (!touches_empty) {
					arena.memory[index] = FILLED;
				}
			}
		}
	}
}

internal void
simulate_game(Input* input, float dt) {
	// Initialize grid
	if (!is_initialized) {
		init_grid(render_state.width, render_state.height);

		init_game();

		// Get aren boundaries in world position
		min_pos_x = pixels_to_world_x(arena.x_offset + (EDGE_THICKNESS / 2), render_state.width, render_state.height);
		max_pos_x = pixels_to_world_x(arena.x_offset + arena.width - (EDGE_THICKNESS / 2), render_state.width, render_state.height);
		min_pos_y = pixels_to_world_y(arena.y_offset + (EDGE_THICKNESS / 2), render_state.height);
		max_pos_y = pixels_to_world_y(arena.y_offset + arena.height - (EDGE_THICKNESS / 2), render_state.height);

		is_initialized = true;
	}

	clear_screen();

	if (player.is_active) {
		// Calculate the proposed new position
		float new_player_x = player.x;
		float new_player_y = player.y;

		float speed = 50.f;  // unitp per second
		if (is_down(BUTTON_UP)) {
			new_player_y += speed * dt;
		}
		else if (is_down(BUTTON_DOWN)) {
			new_player_y -= speed * dt;
		}
		else if (is_down(BUTTON_LEFT)) {
			new_player_x -= speed * dt;
		}
		else if (is_down(BUTTON_RIGHT)) {
			new_player_x += speed * dt;
		}

		if (is_down(BUTTON_SHOOT)) {
			draw_rect(0, 0, 1, 1, 0xffffff);
		}

		// Convert current and proposed floats into Arena Grid Indices
		int current_arena_x = world_to_arena_x(player.x, render_state.width, render_state.height);
		int current_arena_y = world_to_arena_y(player.y, render_state.height);

		int target_arena_x = world_to_arena_x(new_player_x, render_state.width, render_state.height);
		int target_arena_y = world_to_arena_y(new_player_y, render_state.height);

		// Keep the target indices stricly inside the array bounds
		current_arena_x = clamp(0, current_arena_x, arena.width - 1);
		current_arena_y = clamp(0, current_arena_y, arena.height - 1);

		target_arena_x = clamp(0, target_arena_x, arena.width - 1);
		target_arena_y = clamp(0, target_arena_y, arena.height - 1);

		// --- SWEEP COLLISION LOGIC ---
		int step_x = (target_arena_x > current_arena_x) ? 1 : (target_arena_x < current_arena_x) ? -1 : 0;
		int step_y = (target_arena_y > current_arena_y) ? 1 : (target_arena_y < current_arena_y) ? -1 : 0;

		int check_x = current_arena_x;
		int check_y = current_arena_y;
		bool collided = false;
		bool hit_wall_while_drawing = false;
		TileState final_tile = arena.memory[current_arena_y * arena.width + current_arena_x];

		while (check_x != target_arena_x || check_y != target_arena_y) {
			check_x += step_x;
			check_y += step_y;
			TileState tile = arena.memory[check_y * arena.width + check_x];

			if (tile == EDGE) {
				if (player_is_drawing_trail) {
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
				final_tile = arena.memory[target_arena_y * arena.width + target_arena_x];
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
			draw_trail(current_arena_x, current_arena_y, target_arena_x, target_arena_y);
		}

		// Capture trigger
		if(hit_wall_while_drawing) {
			finalize_capture();
			player_is_drawing_trail = false;
		}

		// Start drawing trigger
		if (final_tile == EMPTY && !player_is_drawing_trail) {
			player_is_drawing_trail = true;
		}

		// Apply Position
		if (collided) {
			player.x = pixels_to_world_x(target_arena_x + arena.x_offset, render_state.width, render_state.height);
			player.y = pixels_to_world_y(target_arena_y + arena.y_offset, render_state.height);
		}
		else {
			player.x = new_player_x;
			player.y = new_player_y;
		}

		player.x = clamp(min_pos_x, player.x, max_pos_x);
		player.y = clamp(min_pos_y, player.y, max_pos_y);
	}

	draw_grid();

	// Check Player vs Boss Collision
	if (rects_overlap(player.x, player.y, player.half_size_x, player.half_size_y, boss.x, boss.y, boss.half_size_x, boss.half_size_y)) {
		// GAME OVER - PLAYER DEATH
		player.is_active = false;
	}

	// Draw player
	if (player.is_active) draw_rect(player.x, player.y, player.half_size_x, player.half_size_y, COLOR_PLAYER);
	
	// Draw enemies
	draw_rect(boss.x, boss.y, boss.half_size_x, boss.half_size_y, COLOR_ENEMY);
}