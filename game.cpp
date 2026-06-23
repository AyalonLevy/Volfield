#define is_down(b) input->buttons[b].is_down
#define pressed(b) (input->buttons[b].is_down && input->buttons[b].changed)
#define released(b) (!input->buttons[b].is_down && input->buttons[b].changed)

float player_pos_x = 0.0f;
float player_pos_y = 0.0f;

internal void
simulate_game(Input* input, float dt) {
	clear_screen(0xff5500);

	float speed = 50.f;  // unitp per second
	if (is_down(BUTTON_UP)) player_pos_y += speed * dt;
	if (is_down(BUTTON_DOWN)) player_pos_y -= speed * dt;
	if (is_down(BUTTON_LEFT)) player_pos_x -= speed * dt;
	if (is_down(BUTTON_RIGHT)) player_pos_x += speed * dt;
	if (is_down(BUTTON_SHOOT)) {
		draw_rect(0, 0, 1, 1, 0xffffff);
	}
	
	draw_rect(player_pos_x, player_pos_y, 3, 3, 0x00ff22);

	draw_rect(30, 10, 10, 50, 0xffff22);
}