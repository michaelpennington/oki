#pragma once

#include "core/application.h"

/**
 * Represents the basic game state in a game.
 * Called for creation by the application.
 */
typedef struct game {
  // The application configuration
  application_config app_config;

  // Function pointer to game's initialization function.
  bool (*initialize)(struct game *game_inst);

  // Function pointer to game's update function.
  bool (*update)(struct game *game_inst, f32 delta_time);

  // Function pointer to game's render function.
  bool (*render)(struct game *game_inst, f32 delta_time);

  // Function pointer to handle resizes, if applicable.
  void (*on_resize)(struct game *game_inst, u32 width, u32 height);

  // Game-specific game state, created and managed by the game.
  void *state;
} game;
