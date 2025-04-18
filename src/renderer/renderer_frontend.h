#pragma once

#include "renderer_types.h"

struct static_mesh_data;
struct platform_state;

bool renderer_initialize(const char *application_name,
                         struct platform_state *plat_state);
void renderer_shutdown();

void renderer_on_resized(u16 width, u16 height);

bool renderer_draw_frame(render_packet *packet);
