#include <regex>
#include <xlog/xlog.h>
#include <winsock2.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include "multi.h"
#include "multi_private.h"
#include "server_internal.h"
#include "../misc/misc.h"
#include "../rf/os/os.h"
#include "../rf/os/timer.h"
#include "../rf/multi.h"
#include "../rf/os/console.h"
#include "../rf/weapon.h"
#include "../rf/entity.h"
#include "../rf/ai.h"
#include "../rf/item.h"
#include "../main/main.h"
#include "../graphics/gr.h"

// Note: this must be called from DLL init function
// Note: we can't use global variable because that would lead to crash when launcher loads this DLL to check dependencies
static rf::CmdLineParam& get_url_cmd_line_param()
{
    static rf::CmdLineParam url_param{"-url", "", true};
    return url_param;
}

void handle_url_param()
{
    if (!get_url_cmd_line_param().found()) {
        return;
    }

    const char* url = get_url_cmd_line_param().get_arg();
    std::regex e{R"(^rf://([\w\.-]+):(\d+)/?(?:\?password=(.*))?$)"};
    std::cmatch cm;
    if (!std::regex_match(url, cm, e)) {
        xlog::warn("Unsupported URL: {}", url);
        return;
    }

    auto host_name = cm[1].str();
    auto port = static_cast<uint16_t>(std::stoi(cm[2].str()));
    auto password = cm[3].str();

    hostent* hp = gethostbyname(host_name.c_str());
    if (!hp) {
        xlog::warn("URL host lookup failed");
        return;
    }

    if (hp->h_addrtype != AF_INET) {
        xlog::warn("Unsupported address type (only IPv4 is supported)");
        return;
    }

    rf::console::print("Connecting to {}:{}...", host_name, port);
    auto host = ntohl(reinterpret_cast<in_addr *>(hp->h_addr_list[0])->S_un.S_addr);

    rf::NetAddr addr{host, port};
    start_join_multi_game_sequence(addr, password);
}

FunHook<void()> multi_limbo_init{
    0x0047C280,
    []() {
        multi_limbo_init.call_target();
        if (rf::is_server) {
            server_on_limbo_state_enter();
        }
    },
};

CodeInjection multi_start_injection{
    0x0046D5B0,
    []() {
        void debug_multi_init();
        debug_multi_init();
        void reset_restricted_cmds_on_init_multi();
        reset_restricted_cmds_on_init_multi();
        if (g_game_config.try_lightmaps_only) {
            evaluate_lightmaps_only();
        }        
    },
};

CodeInjection ctf_flag_return_fix{
    0x0047381D,
    [](auto& regs) {
        auto stack_frame = regs.esp + 0x1C;
        bool red = addr_as_ref<bool>(stack_frame + 4);
        if (red) {
            regs.eip = 0x00473827;
        }
        else {
            regs.eip = 0x00473822;
        }
    },
};

FunHook<void()> multi_ctf_level_init_hook{
    0x00472E30,
    []() {
        multi_ctf_level_init_hook.call_target();
        // Make sure CTF flag does not spin in new level if it was dropped in the previous level
        int info_index = rf::item_lookup_type("flag_red");
        if (info_index >= 0) {
            rf::item_info[info_index].flags &= ~rf::IIF_SPINS_IN_MULTI;
        }
        info_index = rf::item_lookup_type("flag_blue");
        if (info_index >= 0) {
            rf::item_info[info_index].flags &= ~rf::IIF_SPINS_IN_MULTI;
        }
    },
};

static rf::Timestamp select_weapon_done_timestamp[rf::multi_max_player_id];

bool multi_is_selecting_weapon(rf::Player* pp)
{
    auto& done_timestamp = select_weapon_done_timestamp[pp->net_data->player_id];
    return done_timestamp.valid() && !done_timestamp.elapsed();
}

void server_set_player_weapon(rf::Player* pp, rf::Entity* ep, int weapon_type)
{
    rf::player_make_weapon_current_selection(pp, weapon_type);
    ep->ai.current_primary_weapon = weapon_type;
    select_weapon_done_timestamp[pp->net_data->player_id].set(300);
}

FunHook<void(rf::Player*, rf::Entity*, int)> multi_select_weapon_server_side_hook{
    0x004858D0,
    [](rf::Player *pp, rf::Entity *ep, int weapon_type) {
        if (weapon_type == -1 || ep->ai.current_primary_weapon == weapon_type) {
            // Nothing to do
            return;
        }
        if (g_additional_server_config.gungame.enabled) {
            //send_chat_line_packet("Weapon switch denied. In GunGame, you get new weapons by getting frags.", pp);
            return;
        }
        bool has_weapon;
        if (weapon_type == rf::remote_charge_det_weapon_type) {
            has_weapon = rf::ai_has_weapon(&ep->ai, rf::remote_charge_weapon_type);
        }
        else {
            has_weapon = rf::ai_has_weapon(&ep->ai, weapon_type);
        }
        if (!has_weapon) {
            xlog::debug("Player {} attempted to select an unpossesed weapon {}", pp->name, weapon_type);
        }
        else if (multi_is_selecting_weapon(pp)) {
            xlog::debug("Player {} attempted to select weapon {} while selecting weapon {}",
                pp->name, weapon_type, ep->ai.current_primary_weapon);
        }
        else if (rf::entity_is_reloading(ep)) {
            xlog::debug("Player {} attempted to select weapon {} while reloading weapon {}",
                pp->name, weapon_type, ep->ai.current_primary_weapon);
        }
        else {
            rf::player_make_weapon_current_selection(pp, weapon_type);
            ep->ai.current_primary_weapon = weapon_type;
            select_weapon_done_timestamp[pp->net_data->player_id].set(300);
        }
    },
};

void multi_reload_weapon_server_side(rf::Player* pp, int weapon_type)
{
    rf::Entity* ep = rf::entity_from_handle(pp->entity_handle);
    if (!ep) {
        // Entity is dead
    }
    else if (ep->ai.current_primary_weapon != weapon_type) {
        xlog::debug("Player {} attempted to reload unselected weapon {}", pp->name, weapon_type);
    }
    else if (multi_is_selecting_weapon(pp)) {
        xlog::debug("Player {} attempted to reload weapon {} while selecting it", pp->name, weapon_type);
    }
    else if (rf::entity_is_reloading(ep)) {
        xlog::debug("Player {} attempted to reload weapon {} while reloading it", pp->name, weapon_type);
    }
    else {
        rf::entity_reload_current_primary(ep, false, false);
    }
}

void multi_ensure_ammo_is_not_empty(rf::Entity* ep)
{
    int weapon_type = ep->ai.current_primary_weapon;
    auto& wi = rf::weapon_types[weapon_type];
    // at least ammo for 100 milliseconds
    int min_ammo = std::max(static_cast<int>(0.1f / wi.fire_wait), 1);
    if (rf::weapon_is_melee(weapon_type)) {
        return;
    }
    if (rf::weapon_uses_clip(weapon_type)) {
        auto& clip_ammo = ep->ai.clip_ammo[weapon_type];
        clip_ammo = std::max(clip_ammo, min_ammo);
    }
    else {
        auto& ammo = ep->ai.ammo[wi.ammo_type];
        ammo = std::max(ammo, min_ammo);
    }
}

void multi_turn_weapon_on(rf::Entity* ep, rf::Player* pp, bool alt_fire)
{
    // Note: pp is always null client-side
    auto weapon_type = ep->ai.current_primary_weapon;
    if (!rf::weapon_is_on_off_weapon(weapon_type, alt_fire)) {
        xlog::debug("Player {} attempted to turn on weapon {} which has no continous fire flag", ep->name, weapon_type);
    }
    else if (rf::is_server && multi_is_selecting_weapon(pp)) {
        xlog::debug("Player {} attempted to turn on weapon {} while selecting it", ep->name, weapon_type);
    }
    else if (rf::is_server && rf::entity_is_reloading(ep)) {
        xlog::debug("Player {} attempted to turn on weapon {} while reloading it", ep->name, weapon_type);
    }
    else {
        if (!rf::is_server) {
            // Make sure client-side ammo is not empty when we know that the player is currently shooting
            // It can be empty if a reload packet was lost or if it got desynced because of network delays
            multi_ensure_ammo_is_not_empty(ep);
        }
        rf::entity_turn_weapon_on(ep->handle, weapon_type, alt_fire);
    }
}

void multi_turn_weapon_off(rf::Entity* ep)
{
    auto current_primary_weapon = ep->ai.current_primary_weapon;
    if (rf::weapon_is_on_off_weapon(current_primary_weapon, false)
        || rf::weapon_is_on_off_weapon(current_primary_weapon, true)) {

        rf::entity_turn_weapon_off(ep->handle, current_primary_weapon);
    }
}

bool weapon_uses_ammo(int weapon_type, bool alt_fire)
{
    if (rf::weapon_is_detonator(weapon_type)) {
         return false;
    }
    if (rf::weapon_is_riot_stick(weapon_type) && alt_fire) {
        return true;
    }
    rf::WeaponInfo* winfo = &rf::weapon_types[weapon_type];
    return !(winfo->flags & rf::WTF_MELEE);
}

bool is_entity_out_of_ammo(rf::Entity *entity, int weapon_type, bool alt_fire)
{
    if (!weapon_uses_ammo(weapon_type, alt_fire)) {
        return false;
    }
    rf::WeaponInfo* winfo = &rf::weapon_types[weapon_type];
    if (winfo->clip_size == 0) {
        auto ammo = entity->ai.ammo[winfo->ammo_type];
        return ammo == 0;
    }
    auto clip_ammo = entity->ai.clip_ammo[weapon_type];
    return clip_ammo == 0;
}
void send_private_message_for_cancelled_shot(rf::Player* player, const std::string& reason)
{
    auto message = std::format("\xA6 Shot canceled: {}", reason);
    send_chat_line_packet(message.c_str(), player);
}

bool multi_is_player_firing_too_fast(rf::Player* pp, int weapon_type)
{
    // do not consider melee weapons for click limiter
    if (rf::weapon_is_melee(weapon_type)) {
        return false;
    }

    int player_id = pp->net_data->player_id;
    int now = rf::timer_get(1000);

    static std::vector<int> last_weapon_id(rf::multi_max_player_id, 0);
    static std::vector<int> last_weapon_fire(rf::multi_max_player_id, 0);

    int fire_wait_ms = 0;

    if (rf::weapon_is_semi_automatic(weapon_type)) {
        // for semi-auto weapons, use the server's fire wait setting
        fire_wait_ms = g_additional_server_config.click_limiter_fire_wait;
    }
    else {
        // for automatic weapons, use the minimum fire wait between both modes in weapons.tbl
        fire_wait_ms = std::min(rf::weapon_get_fire_wait_ms(weapon_type, 0), // primary
                                rf::weapon_get_fire_wait_ms(weapon_type, 1)  // alt
        );
    }

    // apply a 10% buffer for auto weapons
    if (!rf::weapon_is_semi_automatic(weapon_type))
    {
        fire_wait_ms = static_cast<int>(fire_wait_ms * 0.9f);
        
    }

    // reset if weapon changed
    if (last_weapon_id[player_id] != weapon_type) {
        last_weapon_fire[player_id] = 0;
        last_weapon_id[player_id] = weapon_type;
    }

    // check if time since last shot is less than minimum wait time
    int time_since_last_shot = now - last_weapon_fire[player_id];

    if (time_since_last_shot < fire_wait_ms)
        {
        send_private_message_for_cancelled_shot(pp, std::format(
            "too fast! Time between shots: {}ms, server minimum: {}ms", time_since_last_shot, fire_wait_ms));

        return true;
    }

    // we fired
    last_weapon_fire[player_id] = now;
    return false;
}

bool multi_is_weapon_fire_allowed_server_side(rf::Entity *ep, int weapon_type, bool alt_fire)
{
    rf::Player* pp = rf::player_from_entity_handle(ep->handle);
    if (ep->ai.current_primary_weapon != weapon_type) {
        xlog::debug("Player {} attempted to fire unselected weapon {}", pp->name, weapon_type);
    }
    else if (is_entity_out_of_ammo(ep, weapon_type, alt_fire)) {
        xlog::debug("Player {} attempted to fire weapon {} without ammunition", pp->name, weapon_type);
    }
    else if (rf::weapon_is_on_off_weapon(weapon_type, alt_fire)) {
        xlog::debug("Player {} attempted to fire a single bullet from on/off weapon {}", pp->name, weapon_type);
    }
    else if (multi_is_selecting_weapon(pp)) {
        xlog::debug("Player {} attempted to fire weapon {} while selecting it", pp->name, weapon_type);
    }
    else if (rf::entity_is_reloading(ep)) {
        xlog::debug("Player {} attempted to fire weapon {} while reloading it", pp->name, weapon_type);
    }
    else if (!multi_is_player_firing_too_fast(pp, weapon_type)) {
        return true;
    }
    return false;
}

FunHook<void(rf::Entity*, int, rf::Vector3&, rf::Matrix3&, bool)> multi_process_remote_weapon_fire_hook{
    0x0047D220,
    [](rf::Entity *ep, int weapon_type, rf::Vector3& pos, rf::Matrix3& orient, bool alt_fire) {
        if (rf::is_server) {
            // Do some checks server-side to prevent cheating
            if (!multi_is_weapon_fire_allowed_server_side(ep, weapon_type, alt_fire)) {
                return;
            }
        }
        multi_process_remote_weapon_fire_hook.call_target(ep, weapon_type, pos, orient, alt_fire);
    },
};

void multi_init_player(rf::Player* player)
{
    multi_kill_init_player(player);
}

void multi_do_patch()
{
    multi_limbo_init.install();
    multi_start_injection.install();

    // Allow server to customize dropped flag return timer in ms
    AsmWriter(0x00473B88).push(g_additional_server_config.ctf_flag_return_time_ms); // blue flag
    AsmWriter(0x00473B28).push(g_additional_server_config.ctf_flag_return_time_ms); // red flag

    // Fix CTF flag not returning to the base if the other flag was returned when the first one was waiting
    ctf_flag_return_fix.install();

    // Weapon select server-side handling
    multi_select_weapon_server_side_hook.install();

    // Check ammo server-side when handling weapon fire packets
    multi_process_remote_weapon_fire_hook.install();

    // Make sure CTF flag does not spin in new level if it was dropped in the previous level
    multi_ctf_level_init_hook.install();

    multi_kill_do_patch();
    level_download_do_patch();
    network_init();
    multi_tdm_apply_patch();

    level_download_init();
    multi_ban_apply_patch();

    // Init cmd line param
    get_url_cmd_line_param();
}

void multi_after_full_game_init()
{
    handle_url_param();
}
