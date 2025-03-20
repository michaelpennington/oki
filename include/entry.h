#pragma once

#include "core/application.h"
#include "core/kmemory.h"
#include "game_types.h"
#include <stdio.h>

extern bool create_game(game *out_game);

/**
 * The main entry point of the application.
 */
int main(void) {
  initialize_memory();

  game game_inst;
  if (!create_game(&game_inst)) {
    printf("Could not create game!\n");
    return -1;
  }

  if (!game_inst.render || !game_inst.update || !game_inst.initialize ||
      !game_inst.on_resize) {
    printf("FATAL: The game's function pointers must be assigned!\n");
    return -2;
  }

  if (!application_create(&game_inst)) {
    printf("FATAL: application failed to create\n");
    return 1;
  }

  if (!application_run()) {
    printf("FATAL: application failed to run\n");
    return 2;
  }

  kfree(game_inst.state);

  shutdown_memory();

  return 0;
}
