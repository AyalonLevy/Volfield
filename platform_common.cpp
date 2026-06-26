constexpr int BOSS_KILL_BONUS = 100000;
constexpr int MINION_KILL_BONUS = 2000;
constexpr int CAPTURE_POINTS = 10;
constexpr int POWERUP_BONUS_POINTS = 100;

struct Button_State {
	bool is_down;
	bool changed;
};

enum {
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_ACTION,

	BUTTON_COUNT,  // Should be the last item
};

struct Input {
	Button_State buttons[BUTTON_COUNT];
};

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

#pragma once
struct Game_State {
	bool is_initialized;
	bool player_is_drawing_trail;
	bool player_can_exit_edge;

	int filled_percent;
	float speed;

	int score_accumulator = 0;
	int score;
	int high_score;

	int win_condition = 800;
	bool level_cleared = false;

	Entity player;
	Entity boss;
	Entity enemies[10];

	Grid_Arena arena;

	float min_pos_x, max_pos_x;
	float min_pos_y, max_pos_y;
};

enum PowerUpType {
	POWERUP_NONE,
	POWERUP_POINTS,		// '-' - Gives bonus points
	POWERUP_SPEED,		// 'S' - Increase speed
	POWERUP_LASER,		// 'L' - Shoot laser that kills minions
	POWERUP_TIME,		// 'T' - Stops the enemies for 10 seconds
	POWERUP_PAUSE,		// 'P' - Stops the shield countdown for 10 seconds
	POWERUP_DESTROY,	// 'C' - Kills all minions
	POWERUP_BOSS_KILL,	// 'R' - Bombs to shoot the boss (red circle)

	POWERUP_COUNT,		// Should be the last item
};

struct PowerUp {
	PowerUpType type;
	float x, y;
	bool is_active;
};