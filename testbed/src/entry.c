#include "game.h"
#include <entry.h>

#define WIDTH 1280
#define HEIGHT 720
#define NAME "Kohi Engine Testbed"
#define X 100
#define Y 100

bool create_game(game *out_game) {
  out_game->app_config.start_pos_x = X;
  out_game->app_config.start_pos_y = Y;
  out_game->app_config.start_width = WIDTH;
  out_game->app_config.start_height = HEIGHT;
  out_game->app_config.name = NAME;
  out_game->initialize = game_initialize;
  out_game->update = game_update;
  out_game->render = game_render;
  out_game->on_resize = game_on_resize;

  game_state state = {.delta_time = 0.0F};
  out_game->state = &state;

  return true;
}
