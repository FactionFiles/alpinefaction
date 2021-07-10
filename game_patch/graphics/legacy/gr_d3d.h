#pragma once

#include "../../rf/bmpman.h"

bool gr_d3d_is_texture_format_supported(rf::bm::Format format);
bool gr_d3d_set_render_target(int bm_handle);
void gr_d3d_update_window_mode();
void gr_d3d_update_texture_filtering();
void gr_d3d_apply_patch();
void gr_d3d_texture_apply_patch();
void gr_d3d_capture_apply_patch();
void gr_d3d_gamma_apply_patch();