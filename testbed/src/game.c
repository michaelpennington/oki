#include "game.h"
#include "core/logger.h"

bool game_initialize(game *game_inst) {
  (void)game_inst;
  kdebug("game_initialize() called");
  return true;
}

bool game_update(game *game_inst, f32 delta_time) {
  (void)game_inst;
  (void)delta_time;
  return true;
}

bool game_render(game *game_inst, f32 delta_time) {
  (void)game_inst;
  (void)delta_time;
  return true;
}

void game_on_resize(game *game_inst, u32 width, u32 height) {
  (void)game_inst;
  (void)width;
  (void)height;
}
