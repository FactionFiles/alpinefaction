#pragma once

#include "../math/vector.h"
#include "../math/matrix.h"
#include "../math/plane.h"
#include "../os/string.h"

namespace rf
{
    struct GSolid;
}

namespace rf::gr
{
    enum LightType
    {
        LT_NONE = 0x0,
        LT_DIRECTIONAL = 0x1,
        LT_POINT = 0x2,
        LT_SPOT = 0x3,
        LT_TUBE = 0x4,
    };

    enum LightShadowcastCondition
    {
        SHADOWCAST_NEVER = 0x0,
        SHADOWCAST_EDITOR = 0x1,
        SHADOWCAST_RUNTIME = 0x2,
    };

    struct Light
    {
        struct Light *next;
        struct Light *prev;
        LightType type;
        Vector3 vec;
        Vector3 vec2;
        Vector3 spotlight_dir;
        float spotlight_fov1;
        float spotlight_fov2;
        float spotlight_atten;
        float rad_2;
        float r;
        float g;
        float b;
        bool on;
        bool dynamic;
        bool use_squared_fov_falloff;
        char padding[1];
        LightShadowcastCondition shadow_condition;
        int attenuation_algorithm;
        int field_58;
        Vector3 local_vec;
        Vector3 local_vec2;
        Vector3 rotated_spotlight_dir;
        float rad2_squared;
        float spotlight_fov1_dot;
        float spotlight_fov2_dot;
        float spotlight_max_radius;
        int room;
        Plane spotlight_frustrum[6];
        int spotlight_nverts[6];
    };
    static_assert(sizeof(Light) == 0x10C);

    struct LevelLight
    {
        int uid;
        String script_name;
        Vector3 pos;
        Matrix3 orient;
        int state;
        gr::LightType type;
        float hue_r;
        float hue_g;
        float hue_b;
        float radius;
        float radius_dropoff;
        float fov;
        float fov_dropoff;
        float fov_attenuation;
        int fov_attenuation_algo;
        float tube_light_width;
        float on_intensity;
        float off_intensity;
        float on_time;
        float on_time_variance;
        float off_time;
        float off_time_variance;
        bool shadow_casting;
        bool runtime_shadows;
        bool dynamic;
        bool fade;
        float time_to_change;
        float current_state_time;
        bool is_on;
        bool is_enabled;
        bool is_used_at_runtime;
        bool is_used_by_clutter;
        int gr_light_handle;
    };
    static_assert(sizeof(LevelLight) == 0x98, "LevelLight struct size is incorrect!");

    static auto& level_light_lookup_from_uid = addr_as_ref<LevelLight*(int uid)>(0x0045D5E0);
    static auto& level_get_light_handle_from_uid = addr_as_ref<int(int uid)>(0x0045D580);
    static auto& light_get_from_handle = addr_as_ref<Light*(int handle)>(0x004D8DF0);

    static auto& light_filter_set_solid = addr_as_ref<int(GSolid *s, bool include_dynamic, bool include_static)>(0x004D9DD0);
    static auto& light_filter_reset = addr_as_ref<void()>(0x004D9FA0);
    static auto& light_get_ambient = addr_as_ref<void(float *r, float *g, float *b)>(0x004D8D10);

    static auto& num_relevant_lights = addr_as_ref<int>(0x00C9687C);
    static auto& relevant_lights = addr_as_ref<Light*[1100]>(0x00C4D588);
}
