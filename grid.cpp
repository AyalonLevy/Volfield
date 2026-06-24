constexpr float GRID_TO_SCREEN_RATION = .8f;
constexpr s32 GRID_OFFSET_X = 0;
constexpr s32 GRID_OFFSET_Y = 0;
constexpr s32 EDGE_THICKNESS = 2;
constexpr u32 COLOR_EMPTY = 0x555555;
constexpr u32 COLOR_FILLED = 0x333333;
constexpr u32 COLOR_EDGE = 0xFFFF00;
constexpr u32 COLOR_TRAIL = 0xFF0000;


internal void
init_grid(Game_State* state, int screen_width, int screen_height) {
	// Calculate proportional size
	state->arena.width = (int)(screen_width * GRID_TO_SCREEN_RATION);
	state->arena.height = (int)(screen_height * GRID_TO_SCREEN_RATION);

	// Place the arena on the screen
	state->arena.x_offset = (screen_width - state->arena.width) / 2 + GRID_OFFSET_X;
	state->arena.y_offset = (screen_height - state->arena.height) / 2 + GRID_OFFSET_Y;

	// Allocate memory
	int buffer_size = state->arena.width * state->arena.height * sizeof(TileState);

	if (state->arena.memory) VirtualFree(state->arena.memory, 0, MEM_RELEASE);

	state->arena.memory = (TileState*)VirtualAlloc(0, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	// Determine bounds
	for (int y = 0; y < state->arena.height; y++) {
		for (int x = 0; x < state->arena.width; x++) {
			bool is_edge = (x < EDGE_THICKNESS || x >= state->arena.width - EDGE_THICKNESS || y < EDGE_THICKNESS || y >= state->arena.height - EDGE_THICKNESS);

			int index = y * state->arena.width + x;

			if (is_edge) {
				state->arena.memory[index] = EDGE;
			} else {
				state->arena.memory[index] = EMPTY;
			}
		}
	}
}

internal void
draw_grid(Game_State* state) {
	int total_count = 0;
	int filled_count = 0;
	for (int y = 0; y < state->arena.height; y++) {
		for (int x = 0; x < state->arena.width; x++) {
			int index = y * state->arena.width + x;
			TileState tile_state = state->arena.memory[index];

			int screen_x = state->arena.x_offset + x;
			int screen_y = state->arena.y_offset + y;

			total_count++;

			if (tile_state == EDGE) {
				draw_rect_in_pixels(screen_x, screen_y, screen_x + 1, screen_y + 1, COLOR_EDGE);
			} else if (tile_state == EMPTY) {
				draw_rect_in_pixels(screen_x, screen_y, screen_x + 1, screen_y + 1, COLOR_EMPTY);
			} else if (tile_state == FILLED) {
				filled_count++;
				draw_rect_in_pixels(screen_x, screen_y, screen_x + 1, screen_y + 1, COLOR_FILLED);
			} else if (tile_state == TRAIL) {
				draw_rect_in_pixels(screen_x, screen_y, screen_x + 1, screen_y + 1, COLOR_TRAIL);
			} 
		}
	}

	state->filled_percent = (int)(((float)filled_count / (float)total_count) * 1000.0f);	// 1000 is for getting the first decimal point for the percentage
}

internal void
draw_trail(Game_State* state, int x0, int y0, int x1, int y1) {
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (true) {
		int index = y0 * state->arena.width + x0;
		if (state->arena.memory[index] == EMPTY) state->arena.memory[index] = TRAIL;

		if (x0 == x1 && y0 == y1) break;

		int e2 = 2 * err;
		if (e2 > -dy) { err -= dy; x0 += sx; }
		if (e2 < dx) { err += dx; y0 += sy; }
	}
}

struct Point { int x, y; };

internal void
flood_fill(Game_State* state, int start_x, int start_y, TileState target, TileState replacement) {
	// Safety Check: Prevent infinite loops and out-of-bounds start positions
	if (target == replacement) return;
	if (start_x < 0 || start_x >= state->arena.width || start_y < 0 || start_y >= state->arena.height) return;

	int start_index = start_y * state->arena.width + start_x;
	if (state->arena.memory[start_index] != target) return;

	// Create a temporary stack buffer
	Point* stack = (Point*)VirtualAlloc(0, state->arena.width * state->arena.height * sizeof(Point), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	int stack_ptr = 0;

	// Push starting point
	stack[stack_ptr++] = { start_x, start_y };
	state->arena.memory[start_index] = replacement;

	while (stack_ptr > 0) {
		Point p = stack[--stack_ptr];
		int index = p.y * state->arena.width + p.x;

		// Check neighbours. If ther are the target, mark them and push them
		// Right
		if (p.x + 1 < state->arena.width) {
			int idx = p.y * state->arena.width + (p.x + 1);
			if (state->arena.memory[idx] == target) {
				state->arena.memory[idx] = replacement;
				stack[stack_ptr++] = { p.x + 1, p.y };
			}
		}

		// Left
		if (p.x - 1 >= 0) {
			int idx = p.y * state->arena.width + (p.x - 1);
			if (state->arena.memory[idx] == target) {
				state->arena.memory[idx] = replacement;
				stack[stack_ptr++] = { p.x - 1, p.y };
			}
		}

		// Top
		if (p.y + 1 < state->arena.height) {
			int idx = (p.y + 1) * state->arena.width + p.x;
			if (state->arena.memory[idx] == target) {
				state->arena.memory[idx] = replacement;
				stack[stack_ptr++] = { p.x, p.y + 1 };
			}
		}

		// Bottom
		if (p.y - 1 >= 0) {
			int idx = (p.y - 1) * state->arena.width + p.x;
			if (state->arena.memory[idx] == target) {
				state->arena.memory[idx] = replacement;
				stack[stack_ptr++] = { p.x, p.y - 1 };
			}
		}
	}
	VirtualFree(stack, 0, MEM_RELEASE);
}

internal int
world_to_arena_x(Game_State* state, float world_x, int screen_width, int screen_height) {
	float pixel_x = (world_x * screen_height * render_scale) + ((float)screen_width / 2.0f);
	return (int)pixel_x - state->arena.x_offset;
}

internal int
world_to_arena_y(Game_State* state, float world_y, int screen_height) {
	float pixel_y = (world_y * screen_height * render_scale) + ((float)screen_height / 2.0f);
	return (int)pixel_y - state->arena.y_offset;
}