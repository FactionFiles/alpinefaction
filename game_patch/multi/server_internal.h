#pragma once

#include <string_view>
#include <string>
#include <set>
#include <map>
#include <optional>
#include <vector>
#include "../rf/math/vector.h"
#include "../rf/math/matrix.h"
#include "../rf/os/string.h"

// Forward declarations
namespace rf
{
    struct Player;
}

struct VoteConfig
{
    bool enabled = false;
    // int min_voters = 1;
    // int min_percentage = 50;
    int time_limit_seconds = 60;
};

struct HitSoundsConfig
{
    bool enabled = true;
    int sound_id = 29;
    int rate_limit = 10;
};

struct NewSpawnLogicConfig // defaults match stock game
{
    bool respect_team_spawns = true;    
    bool try_avoid_players = true;
    bool always_avoid_last = false;
    bool always_use_furthest = false;
    bool only_avoid_enemies = false;
    std::map<std::string, std::optional<int>> allowed_respawn_items;
};

struct ServerAdditionalConfig
{
    VoteConfig vote_kick;
    VoteConfig vote_level;
    VoteConfig vote_extend;
    VoteConfig vote_restart;
    VoteConfig vote_next;
    VoteConfig vote_previous;
    NewSpawnLogicConfig new_spawn_logic;
    int spawn_protection_duration_ms = 1500;
    std::optional<float> spawn_life;
    std::optional<float> spawn_armor;
    int ctf_flag_return_time_ms = 25000;
    HitSoundsConfig hit_sounds;
    std::map<std::string, std::string> item_replacements;
    std::string default_player_weapon;
    std::optional<int> default_player_weapon_ammo;
    bool require_client_mod = true;
    float player_damage_modifier = 1.0f;
    bool saving_enabled = false;
    bool upnp_enabled = true;
    std::optional<int> force_player_character;
    std::optional<float> max_fov;
    bool allow_fullbright_meshes = false;
    bool allow_lightmaps_only = false;
    bool allow_disable_screenshake = false;
    int anticheat_level = 0;
    bool stats_message_enabled = true;
    std::string welcome_message;
    bool weapon_items_give_full_ammo = false;
    float kill_reward_health = 0.0f;
    float kill_reward_armor = 0.0f;
    float kill_reward_effective_health = 0.0f;
    bool kill_reward_health_super = false;
    bool kill_reward_armor_super = false;
};

extern ServerAdditionalConfig g_additional_server_config;
extern std::string g_prev_level;

void cleanup_win32_server_console();
void handle_vote_command(std::string_view vote_name, std::string_view vote_arg, rf::Player* sender);
void server_vote_do_frame();
void init_server_commands();
void extend_round_time(int minutes);
void restart_current_level();
void load_next_level();
void load_prev_level();
void server_vote_on_limbo_state_enter();
void process_delayed_kicks();
const ServerAdditionalConfig& server_get_df_config();
