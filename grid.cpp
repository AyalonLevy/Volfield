constexpr float GRID_TO_SCREEN_RATION = .8f;
constexpr s32 GRID_OFFSET_X = 0;
constexpr s32 GRID_OFFSET_Y = -20;
constexpr s32 EDGE_THICKNESS = 2;
constexpr u32 COLOR_EMPTY = 0x555555;
constexpr u32 COLOR_FILLED = 0x0000FF;
constexpr u32 COLOR_EDGE = 0xFFFF00;
constexpr u32 COLOR_TRAIL = 0xFF0000;


enum TileState : u8 {
	EMPTY,	// Playeble area where enemies bounce
	FILLED,	// Cuptured area
	EDGE,	// The boundary between EMPTY and FILLED (where the player moves)
	TRAIL,	// The temporary line the player draws while cutting the Empty area
	BOSS,	// Temporary state for fillinf the non-boss area
};

struct Grid_Arena {
	int x_offset;
	int y_offset;
	int width;
	int height;

	TileState* memory;
};

global_variable Grid_Arena arena;

internal void
init_grid(int screen_width, int screen_height) {
	// Calculate proportional size
	arena.width = (int)(screen_width * GRID_TO_SCREEN_RATION);
	arena.height = (int)(screen_height * GRID_TO_SCREEN_RATION);

	// Place the arena on the screen
	arena.x_offset = (screen_width - arena.width) / 2 + GRID_OFFSET_X;
	arena.y_offset = (screen_height - arena.height) / 2 + GRID_OFFSET_Y;

	// Allocate memory
	int buffer_size = arena.width * arena.height * sizeof(TileState);

	if (arena.memory) VirtualFree(arena.memory, 0, MEM_RELEASE);

	arena.memory = (TileState*)VirtualAlloc(0, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	// Determine bounds
	for (int y = 0; y < arena.height; y++) {
		for (int x = 0; x < arena.width; x++) {
			bool is_edge = (x < EDGE_THICKNESS || x >= arena.width - EDGE_THICKNESS || y < EDGE_THICKNESS || y >= arena.height - EDGE_THICKNESS);

			int index = y * arena.width + x;

			if (is_edge) {
				arena.memory[index] = EDGE;
			} else {
				arena.memory[index] = EMPTY;
			}
		}
	}
}

internal void
draw_grid() {
	for (int y = 0; y < arena.height; y++) {
		for (int x = 0; x < arena.width; x++) {
			int index = y * arena.width + x;
			TileState state = arena.memory[index];

			int screen_x = arena.x_offset + x;
			int screen_y = arena.y_offset + y;

			if (state == EDGE) {
				draw_rect_in_pixels(screen_x, screen_y, screen_x + 1, screen_y + 1, COLOR_EDGE);
			} else if (state == EMPTY) {
				draw_rect_in_pixels(screen_x, screen_y, screen_x + 1, screen_y + 1, COLOR_EMPTY);
			} else if (state == FILLED) {
				draw_rect_in_pixels(screen_x, screen_y, screen_x + 1, screen_y + 1, COLOR_FILLED);
			} else if (state == TRAIL) {
				draw_rect_in_pixels(screen_x, screen_y, screen_x + 1, screen_y + 1, COLOR_TRAIL);
			} 
		}
	}
}

internal void
draw_trail(int x0, int y0, int x1, int y1) {
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (true) {
		int index = y0 * arena.width + x0;
		if (arena.memory[index] == EMPTY) arena.memory[index] = TRAIL;

		if (x0 == x1 && y0 == y1) break;

		int e2 = 2 * err;
		if (e2 > -dy) { err -= dy; x0 += sx; }
		if (e2 < dx) { err += dx; y0 += sy; }
	}
}

struct Point { int x, y; };

internal void
flood_fill(int start_x, int start_y, TileState target, TileState replacement) {
	// Safety Check: Prevent infinite loops and out-of-bounds start positions
	if (target == replacement) return;
	if (start_x < 0 || start_x >= arena.width || start_y < 0 || start_y >= arena.height) return;

	int start_index = start_y * arena.width + start_x;
	if (arena.memory[start_index] != target) return;

	// Create a temporary stack buffer
	Point* stack = (Point*)VirtualAlloc(0, arena.width * arena.height * sizeof(Point), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	int stack_ptr = 0;

	// Push starting point
	stack[stack_ptr++] = { start_x, start_y };
	arena.memory[start_index] = replacement;

	while (stack_ptr > 0) {
		Point p = stack[--stack_ptr];
		int index = p.y * arena.width + p.x;

		// Check neighbours. If ther are the target, mark them and push them
		// Right
		if (p.x + 1 < arena.width) {
			int idx = p.y * arena.width + (p.x + 1);
			if (arena.memory[idx] == target) {
				arena.memory[idx] = replacement;
				stack[stack_ptr++] = { p.x + 1, p.y };
			}
		}

		// Left
		if (p.x - 1 >= 0) {
			int idx = p.y * arena.width + (p.x - 1);
			if (arena.memory[idx] == target) {
				arena.memory[idx] = replacement;
				stack[stack_ptr++] = { p.x - 1, p.y };
			}
		}

		// Top
		if (p.y + 1 < arena.height) {
			int idx = (p.y + 1) * arena.width + p.x;
			if (arena.memory[idx] == target) {
				arena.memory[idx] = replacement;
				stack[stack_ptr++] = { p.x, p.y + 1 };
			}
		}

		// Bottom
		if (p.y - 1 >= 0) {
			int idx = (p.y - 1) * arena.width + p.x;
			if (arena.memory[idx] == target) {
				arena.memory[idx] = replacement;
				stack[stack_ptr++] = { p.x, p.y - 1 };
			}
		}
	}
	VirtualFree(stack, 0, MEM_RELEASE);
}

internal int
world_to_arena_x(float world_x, int screen_width, int screen_height) {
	float pixel_x = (world_x * screen_height * render_scale) + ((float)screen_width / 2.0f);
	return (int)pixel_x - arena.x_offset;
}

internal int
world_to_arena_y(float world_y, int screen_height) {
	float pixel_y = (world_y * screen_height * render_scale) + ((float)screen_height / 2.0f);
	return (int)pixel_y - arena.y_offset;
}