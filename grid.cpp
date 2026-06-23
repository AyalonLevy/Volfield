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
			bool is_edge = (x < EDGE_THICKNESS || x > arena.width - EDGE_THICKNESS || y < EDGE_THICKNESS || y > arena.height - EDGE_THICKNESS);

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