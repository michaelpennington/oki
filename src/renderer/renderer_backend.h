#pragma once

#include "renderer_types.h"

struct platform_state;

bool renderer_backend_create(renderer_backend_type type,
                             struct platform_state *plat_state,
                             renderer_backend *out_renderer_backend);

void renderer_backend_destroy(renderer_backend *renderer_backend);
