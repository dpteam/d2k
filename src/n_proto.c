/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 *
 *-----------------------------------------------------------------------------
 */

#include "z_zone.h"

#include <msgpack.h>

#include "n_peer.h"

#define MAX_PREF_NAME_SIZE 20

#define SERVER_ONLY(s)                                                         \
  if (!server) {                                                               \
    doom_printf(                                                               \
      "N_HandlePacket: Erroneously received packet [" #s "] from the server\n" \
    );                                                                         \
    return;                                                                    \
  }

#define CLIENT_ONLY(s)                                                         \
  if (server) {                                                                \
    doom_printf(                                                               \
      "N_HandlePacket: Erroneously received packet [" #s "] from a client\n"   \
    );                                                                         \
    return;                                                                    \
  }

#define CHECK_VALID_PLAYER(np, playernum)                                      \
  if (((np) == N_GetPlayerForPeer((playernum))) == NULL)                       \
    I_Error(__func__ ": Invalid player %d.\n", playernum)

#define CHECK_CONNECTION(np)                                                   \
  if (((np) = N_GetPeer(0)) == NULL) {                                         \
    doom_printf(__func__ ": Not connected\n");                                 \
    return;                                                                    \
  }

static buf_t message_recipients;

/*
 ##############################################################################
 # CG: C/S Message Handlers here
 ##############################################################################
*/
static void handle_state_delta(netpeer_t *np) {
  static buf_t delta_buffer;

  int from_tic, to_tic;

  if (N_UnpackStateDelta(np, &from_tic, &to_tic, &delta_buffer)) {
    N_ApplyStateDelta(from_tic, to_tic, &delta_buffer);
    G_LoadSaveData(N_GetCurrentState(), true, false);
  }
}

static void handle_full_state(netpeer_t *np) {
  static buf_t state_buffer;

  int tic;

  if (N_UnpackFullState(np, &tic, &state_buffer)) {
    N_SaveCurrentState(tic, &state_buffer);
    G_LoadSaveData(N_GetCurrentState(), true, false);
  }
}

static void handle_auth_response(netpeer_t *np) {
  auth_level_e level;

  if (N_UnpackAuthResponse(np, &level))
    N_SetLocalClientAuthorizationLevel(level);
}

static void handle_player_command_received(netpeer_t *np) {
  int tic;

  if (N_UnpackServerMessage(np, &tic))
    N_RemoveOldClientCommands(tic);
}

static void handle_server_message(netpeer_t *np) {
  static buf_t server_message_buffer;

  if (N_UnpackServerMessage(np, &server_message_buffer))
    doom_printf("[SERVER]: %s\n", server_message_buffer.data);
}

static void handle_player_message(netpeer_t *np) {
  static buf_t player_message_buffer;

  unsigned short sender = 0;
  size_t recipient_count = 0;
  dboolean unpacked_successfully = false;

  unpacked_successfully = N_UnpackPlayerMessage(
    np, &sender, &recipient_count, &message_recipients, &player_message_buffer
  );

  if (!unpacked_successfully)
    return

  if (server) {
    for (int i = 0; i < N_GetPeerCount(); i++) {
      netpeer_t *np = N_GetPeer(i);

      if (np != NULL) {
        N_PackPlayerMessage(
          np,
          sender,
          recipient_count,
          &message_recipients,
          player_message_buffer.data
        );
      }
    }
  }
  else {
    if (sender == -1) {
      doom_printf("[SERVER]: %s\n", player_message_buffer.data);
    }
    else {
      doom_printf(
        "%s: %s\n", players->objects[sender]->name, player_message_buffer.data
      );
    }
  }
}

static void handle_player_commands(netpeer_t *np) {
  N_UnpackPlayerCommands(np);
}

static void handle_auth_request(netpeer_t *np) {
  auth_level_e level;

  if (N_UnpackServerMessage(np, &level))
    N_SetLocalClientAuthorizationLevel(level);
}

static void handle_client_preference_change(netpeer_t *np) {
  static buf_t pref_key_name;
  static buf_t pref_key_value;

  size_t pref_count = 0;
  player_t *player = &players[np->playernum];

  if (!N_UnpackClientPreferenceChange(np, &pref_count))
    return;

  for (size_t i = 0; i < pref_count; i++) {
    if (!N_UnpackClientPreferenceChangeName(np, i, &pref_key_name)) {
      doom_printf(
        "N_HandlePacket: Error unpacking client preference change\n"
      );
      return;
    }

    if (pref_key_name.size > MAX_PREF_NAME_SIZE) {
      doom_printf(
        "N_HandlePacket: Invalid client preference change packet: preference "
        "name exceeds maximum length\n"
      );
      return;
    }

    if (M_BufferEqualsString(&pref_key_name, "name")) {
      N_UnpackNameChange(np, &pref_key_value);

      if (player->name != NULL) {
        doom_printf("%s is now %s.\n", player->name, pref_key_value.data);
        free(player->name);
      }

      player->name = strdup(pref_key_value.data);
    }
    else if (M_BufferEqualsString(&pref_key_name, "team")) {
      byte new_team = 0;

      if (!N_UnpackTeamChange(np, &new_team))
        return;

      player->team = new_team;
    }
    else if (M_BufferEqualsString(&pref_key_name, "pwo")) {
    }
    else if (M_BufferEqualsString(&pref_key_name, "wsop")) {
    }
    else if (M_BufferEqualsString(&pref_key_name, "bobbing")) {
    }
    else if (M_BufferEqualsString(&pref_key_name, "autoaim")) {
    }
    else if (M_BufferEqualsString(&pref_key_name, "weapon_speed")) {
    }
    else if (M_BufferEqualsString(&pref_key_name, "color")) {
    }
    else if (M_BufferEqualsString(&pref_key_name, "skin_name")) {
    }
  }
}

static void handle_rcon(netpeer_t *np) {
}

static void handle_vote_request(netpeer_t *np) {
}

/*
 ##############################################################################
 # CG: P2P Message Handlers here
 ##############################################################################
*/

static void handle_init(netpeer_t *np) {
}

static void handle_setup(netpeer_t *np) {
}

static void handle_go(netpeer_t *np) {
}

static void handle_client_tic(netpeer_t *np) {
}

static void handle_server_tic(netpeer_t *np) {
}

static void handle_retransmission_request(netpeer_t *np) {
}

static void handle_color(netpeer_t *np) {
}

static void handle_save_game_name(netpeer_t *np) {
}

static void handle_quit(netpeer_t *np) {
}

static void handle_down(netpeer_t *np) {
}

static void handle_wad(netpeer_t *np) {
}

static void handle_backoff(netpeer_t *np) {
}

static void dispatch_p2p_message(netpeer_t *np, byte message_type) {
  switch(message_type) {
    case PKT_INIT:
      SERVER_ONLY("initialization");
      handle_init_request(np);
    break;
    case PKT_SETUP:
      CLIENT_ONLY("setup");
      handle_setup(np);
    break;
    case PKT_GO:
      /* CG: Both clients and servers receive PKT_GO messages */
      handle_go(np);
    break;
    case PKT_TICC:
      SERVER_ONLY("client tic");
      handle_client_tic(np);
    break;
    case PKT_TICS:
      CLIENT_ONLY("server tic")
      handle_server_tic(np);
    break;
    case PKT_RETRANS:
      /* CG: Both clients and servers receive PKT_RETRANS messages */
      handle_retransmission_request(np);
    break;
    case PKT_COLOR:
      /* CG: Both clients and servers receive PKT_COLOR messages */
      handle_color(np);
    break;
    case PKT_SAVEG:
      /* CG: Both clients and servers receive PKT_SAVEG messages */
      handle_save_game_name(np);
    break;
    case PKT_QUIT:
      /* CG: Both clients and servers receive PKT_QUIT messages */
      handle_quit(np);
    break;
    case PKT_DOWN:
      CLIENT_ONLY("down");
      handle_down(np);
    break;
    case PKT_WAD:
      /* CG: Both clients and servers receive PKT_WAD messages */
      handle_wad(np);
    break;
    case PKT_BACKOFF:
      CLIENT_ONLY("backoff");
      handle_backoff(np);
    break;
    default:
      doom_printf("Received unknown message type %u from peer %s:%u.\n"
        message_type,
        I_IPToString(np->peer->address.host),
        np->peer->address.peer
      );
    break;
  }
}

static void dispatch_cs_message(netpeer_t *np, byte message_type) {
  switch (message_type) {
    case nm_statedelta:
      CLIENT_ONLY("state delta");
      handle_state_delta(np);
    break;
    case nm_fullstate:
      CLIENT_ONLY("game state");
      handle_full_state(np);
    break;
    case nm_authresponse:
      CLIENT_ONLY("authorization response");
      handle_auth_response(np);
    break;
    case nm_playercommandreceived:
      CLIENT_ONLY("player command received");
      handle_player_command_received(np);
    case nm_servermessage:
      CLIENT_ONLY("server message");
      handle_server_message(np);
    break;
    case nm_playermessage:
      /* CG: Both servers and clients receive player message messages */
      handle_player_message(np);
    break;
    case nm_playercommands:
      SERVER_ONLY("player commands");
      handle_player_commands(np);
    break;
    case nm_authrequest:
      SERVER_ONLY("authorization request");
      handle_auth_request(np);
    break;
    case nm_namechange:
      SERVER_ONLY("name change");
      handle_name_change(np);
    break;
    case nm_teamchange:
      SERVER_ONLY("team change");
      handle_team_change(np);
    break;
    case nm_pwochange:
      SERVER_ONLY("PWO change");
      handle_pwo_change(np);
    break;
    case nm_wsopchange:
      SERVER_ONLY("WSOP change");
      handle_wsop_change(np);
    break;
    case nm_bobbingchange:
      SERVER_ONLY("bobbing change");
      handle_bobbing_change(np);
    break;
    case nm_autoaimchange:
      SERVER_ONLY("autoaim change");
      handle_autoaim_change(np);
    break;
    case nm_weaponspeedchange:
      SERVER_ONLY("weapon speed change");
      handle_weapon_speed_change(np);
    break;
    case nm_colorchange:
      SERVER_ONLY("color change");
      handle_color_change(np);
    break;
    case nm_skinchange:
      SERVER_ONLY("skin change");
      handle_skin_change(np);
    break;
    case nm_rconcommand:
      SERVER_ONLY("RCON command");
      handle_rcon(np);
    break;
    case nm_voterequest:
      SERVER_ONLY("vote request");
      handle_vote_request(np);
    break;
    default:
      doom_printf("Received unknown message type %u from peer %s:%u.\n"
        message_type,
        I_IPToString(np->peer->address.host),
        np->peer->address.peer
      );
    break;
  }
}

void N_HandlePacket(int peernum, void *data, size_t data_size) {
  netpeer_t *np = N_GetPeer(peernum);
  byte message_type = 0;

  msgpack_unpacker_reserve_buffer(pac, data_size);
  memcpy(msgpack_unpacker_buffer(pac), data, data_size);
  msgpack_unpacker_buffer_consumed(pac, data_size);

  while (N_LoadNewMessage(np, &message_type)) {
    if (use_p2p_netcode)
      dispatch_p2p_message(np, message_type);
    else
      dispatch_cs_message(np, message_type);
  }
}

buf_t* N_GetMessageRecipientBuffer(void) {
  size_t capacity = (players->capacity + 1) * sizeof(short);

  M_BufferEnsureCapacity(&message_recipients, capacity);
  M_BufferZero(&message_recipients);

  return &message_recipients;
}

/*
 ##############################################################################
 # CG: C/S netcode interface here
 ##############################################################################
*/

void SV_SendStateDelta(short playernum) {
  netpeer_t *np = NULL;
  CHECK_VALID_PLAYER(np, playernum);

  N_BuildStateDelta(np);
  N_PackStateDelta(np, np->last_state_received_tic, gametic, np->delta);
  np->last_state_sent_tic = gametic;
}

void SV_SendFullState(short playernum) {
  netpeer_t *np = NULL;
  CHECK_VALID_PLAYER(np, playernum);

  N_PackFullState(np, N_GetCurrentState());
  np->last_state_sent_tic = gametic;
}

void SV_SendAuthResponse(short playernum, auth_level_e auth_level) {
  netpeer_t *np = NULL;
  CHECK_VALID_PLAYER(np, playernum);

  N_PackMessage(np, auth_level);
}

void SV_SendMessage(short playernum, rune *message) {
  netpeer_t *np = NULL;
  CHECK_VALID_PLAYER(np, playernum);

  N_PackMessage(np, message);
}

void SV_BroadcastMessage(rune *message) {
  netpeer_t *np = NULL;

  for (int i = 0; i < N_GetPeerCount(); i++) {
    netpeer_t *np = N_GetPeer(i);

    if ((np = N_GetPeer(i)) != NULL)
      N_PackMessage(np, message);
  }
}

void CL_SendMessageToServer(rune *message) {
  CL_SendMessageToPlayer(-1, message);
}

void CL_SendMessageToPlayer(short recipient, rune *message) {
  buf_t *recipients = N_GetMessageRecipientBuffer();
  netpeer_t *np = NULL;
  CHECK_CONNECTION(np);

  ((short *)message_recipients.data)[0] = recipient;

  N_PackPlayerMessage(np, consoleplayer, 1, recipients, message);
}

void CL_SendMessageToTeam(byte team, rune *message) {
  buf_t *recipients = N_GetMessageRecipientBuffer();
  size_t recipient_count = 0;
  netpeer_t *np = NULL;
  CHECK_CONNECTION(np);

  for (int i = 0; i < players->size; i++) {
    if (players->objects[i]->team == team) {
      ((dboolean *)message_recipients.data)[i] = true;
      recipient_count++;
    }
  }

  N_PackPlayerMessage(np, consoleplayer, recipient_count, recipients, message);
}

void CL_SendMessageToCurrentTeam(rune *message) {
  CL_SendMessageToTeam(players->objects[consoleplayer]->team, message);
}

void CL_SendCommands(void) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackPlayerCommands(np);
}

void CL_SendAuthRequest(rune *password) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackAuthRequest(np, password);
}

void CL_SendNameChange(rune *new_name) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackNameChange(np, new_name);
}

void CL_SendTeamChange(byte new_team) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackTeamChange(np, new_team);
}

void CL_SendPWOChange(void) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);
  /* CG: TODO */
}

void CL_SendWSOPChange(byte new_wsop_flags) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackWSOPChange(np, new_wsop_flags);
}

void CL_SendBobbingChange(double new_bobbing_amount) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackBobbingChange(np, new_bobbing_amount);
}

void CL_SendAutoaimChange(dboolean new_autoaim_enabled) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackAutoaimChange(np, new_autoaim_enabled);
}

void CL_SendWeaponSpeedChange(byte new_weapon_speed) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackWeaponSpeedChange(np, new_weapon_speed);
}

void CL_SendColorChange(byte new_red, byte new_green, byte new_blue) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackColorChange(np, new_red, new_green, new_blue);
}

void CL_SendSkinChange(void) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);
  /* CG: TODO */
}

void CL_SendRCONCommand(rune *command) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackRCONCommand(np, command);
}

void CL_SendVoteRequest(rune *command) {
  netpeer_t *np = N_GetPeer(0);
  CHECK_CONNECTION(np);

  N_PackVoteRequest(np, command);
}

/* vi: set et ts=2 sw=2: */

