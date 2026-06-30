#include "types.hpp"

i32 window_width = 1024;
i32 window_height = 768;

f64 unit_scale = 0.1;

f64 snap_eps = 1e-4;

i32 cluster_num_samples = 10;
f64 cluster_eps = 1.0;

f32 ceil_height_meters = 5.0f;
f32 door_frac_top      = 0.8f * ceil_height_meters; // from 80% to 100%
f32 window_frac_bottom = 0.2f * ceil_height_meters; // from 0% to 20%
f32 window_frac_top    = 0.8f * ceil_height_meters; // from 80% to 100%

f32 floor_texture_scaling = 2.0f;
f32 wall_texture_scaling = 2.0f;