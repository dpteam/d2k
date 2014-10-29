/*****************************************************************************/
/* D2K: A Doom Source Port for the 21st Century                              */
/*                                                                           */
/* Copyright (C) 2014: See COPYRIGHT file                                    */
/*                                                                           */
/* This file is part of D2K.                                                 */
/*                                                                           */
/* D2K is free software: you can redistribute it and/or modify it under the  */
/* terms of the GNU General Public License as published by the Free Software */
/* Foundation, either version 2 of the License, or (at your option) any      */
/* later version.                                                            */
/*                                                                           */
/* D2K is distributed in the hope that it will be useful, but WITHOUT ANY    */
/* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS */
/* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more    */
/* details.                                                                  */
/*                                                                           */
/* You should have received a copy of the GNU General Public License along   */
/* with D2K.  If not, see <http://www.gnu.org/licenses/>.                    */
/*                                                                           */
/*****************************************************************************/


#include "z_zone.h"

#include "doomdef.h"
#include "doomstat.h"
#include "d_event.h"
#include "d_log.h"
#include "d_player.h"
#include "d_ticcmd.h"
#include "e6y.h"
#include "g_game.h"
#include "i_video.h"
#include "n_net.h"
#include "n_main.h"
#include "n_state.h"
#include "n_proto.h"
#include "cl_main.h"
#include "p_cmd.h"
#include "p_map.h"
#include "p_mobj.h"
#include "p_pspr.h"
#include "p_user.h"
#include "s_sound.h"
#include "sv_tbuf.h"

#define NOEXTRAPOLATION 0
#define PMOBJTHINKER 1
#define COPIED_COMMAND 2
#define EXTRAPOLATION PMOBJTHINKER
#define MISSED_COMMAND_MAX 3

static GQueue *blank_command_queue;

static void print_command(gpointer data, gpointer user_data) {
  netticcmd_t *ncmd = data;

  D_Log(LOG_SYNC, " %u/%u", ncmd->index, ncmd->tic);
}

static void run_player_command(player_t *player) {
  ticcmd_t *cmd = &player->cmd;
  weapontype_t newweapon;

  /*
  if (CLIENT && player != &players[consoleplayer]) {
    D_Log(LOG_SYNC, "(%d) run_player_command: Running command for %td\n",
      gametic,
      player - players
    );
  }
  */

  // chain saw run forward
  if (player->mo->flags & MF_JUSTATTACKED) {
    cmd->angleturn = 0;
    cmd->forwardmove = 0xc800 / 512;
    cmd->sidemove = 0;
    player->mo->flags &= ~MF_JUSTATTACKED;
  }

  if (player->playerstate == PST_DEAD)
    return;

  if (player->jumpTics)
    player->jumpTics--;

  // Move around.
  // Reactiontime is used to prevent movement
  //  for a bit after a teleport.
  if (player->mo->reactiontime)
    player->mo->reactiontime--;
  else
    P_MovePlayer(player);

  P_SetPitch(player);

  P_CalcHeight(player); // Determines view height and bobbing

  // Check for weapon change.
  if (cmd->buttons & BT_CHANGE) {
    // The actual changing of the weapon is done
    //  when the weapon psprite can do it
    //  (read: not in the middle of an attack).

    newweapon = (cmd->buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT;

    // killough 3/22/98: For demo compatibility we must perform the fist
    // and SSG weapons switches here, rather than in G_BuildTiccmd(). For
    // other games which rely on user preferences, we must use the latter.

    // compatibility mode -- required for old demos -- killough
    if (demo_compatibility) {
      //e6y
      if (!prboom_comp[PC_ALLOW_SSG_DIRECT].state)
        newweapon = (cmd->buttons & BT_WEAPONMASK_OLD) >> BT_WEAPONSHIFT;

      if (newweapon == wp_fist && player->weaponowned[wp_chainsaw] &&
          (player->readyweapon != wp_chainsaw ||
           !player->powers[pw_strength])) {
        newweapon = wp_chainsaw;
      }

      if (gamemode == commercial &&
          newweapon == wp_shotgun &&
          player->weaponowned[wp_supershotgun] &&
          player->readyweapon != wp_supershotgun) {
        newweapon = wp_supershotgun;
      }
    }

    // killough 2/8/98, 3/22/98 -- end of weapon selection changes

    // Do not go to plasma or BFG in shareware, even if cheated.
    if (player->weaponowned[newweapon] &&
        newweapon != player->readyweapon &&
        ((newweapon != wp_plasma && newweapon != wp_bfg) ||
         (gamemode != shareware))) {
      player->pendingweapon = newweapon;
    }
  }

  // check for use

  if (cmd->buttons & BT_USE) {
    if (!player->usedown) {
      P_UseLines(player);
      player->usedown = true;
    }
  }
  else {
    player->usedown = false;
  }

  // cycle psprites

  P_MovePsprites(player);
}

/*
 * CG 09/28/2014: It is not guaranteed that servers or players will receive
 *                player commands on a timely basis, rather, it is likely
 *                commands will be spread out and then later bunched together.
 *                My current understanding of the issue leads me to believe
 *                that the only remedy is to predict the player's ultimate
 *                position in the absence of commands, in hopes that when their
 *                commands are received and subsequently run, the predicted
 *                position will be close to the ultimate position.
 *
 *                For now, this prediction consists of simply running
 *                P_MobjThinker on the player, continuing to simulate their
 *                momentum (and other attributes).  This will work for very
 *                small gaps (1-5 TICs).
 *
 *                A better solution is to predict a player's position base on
 *                their recent movement.  Essentially, this would mean saving
 *                their momx/y/z values and continuing to apply them inside
 *                command gaps.
 *
 *                Of course, this is a clientside enhancement only.  Were
 *                servers to apply something like this, it would irrevocably
 *                break clientside prediction.
 */

bool extrapolate_player_position(player_t *player) {
  if ((!DELTACLIENT) || (player == &players[consoleplayer]) || (!player->mo))
    return false;

  if (!cl_extrapolate_player_positions)
    return false;

#if EXTRAPOLATION == PMOBJTHINKER
  P_MobjThinker(player->mo);
  return false;
#elif EXTRAPOLATION == COPIED_COMMAND
  GQueue *commands;
  netticcmd_t *ncmd;

  if (player->missed_command_count > MISSED_COMMAND_MAX) {
    printf("(%d) %td over missed command limit\n", gametic, player - players);
    return false;
  }

  /* CG: [XXX] Should be in netsync_t? */
  player->missed_command_count++;

  if (!CL_GetCommandSync(player - players, NULL, NULL, NULL, &commands)) {
    P_Echo(consoleplayer, "Server disconnected");
    return false;
  }
  
  ncmd = P_GetNewBlankCommand();

  ncmd->cmd.forwardmove = player->cmd.forwardmove;
  ncmd->cmd.sidemove    = player->cmd.sidemove;
  ncmd->cmd.angleturn   = player->cmd.angleturn;
  ncmd->cmd.consistancy = 0;
  ncmd->cmd.chatchar    = 0;
  ncmd->cmd.buttons     = 0;

  if (player != &players[0] &&
      ncmd->cmd.forwardmove != 0 &&
      ncmd->cmd.sidemove != 0 &&
      ncmd->cmd.angleturn != 0) {
    printf("(%d) Re-running command: {%d, %d, %d}\n",
      gametic,
      ncmd->cmd.forwardmove,
      ncmd->cmd.sidemove,
      ncmd->cmd.angleturn
    );
  }

  g_queue_push_tail(commands, ncmd);

  return true;
#endif
  return false;
}

void run_queued_player_commands(int playernum) {
  GQueue *commands;
  player_t *player = &players[playernum];
  unsigned int command_limit;
  unsigned int commands_run = 0;

  if (CLIENT) {
    if (!CL_GetCommandSync(playernum, NULL, NULL, NULL, &commands)) {
      P_Printf(consoleplayer,
        "run_queued_player_commands: No peer for player #%d\n", playernum
      );
      return;
    }
  }
  else if (playernum == consoleplayer) {
    return;
  }
  else {
    if (!SV_GetCommandSync(playernum, playernum, NULL, NULL, NULL, &commands)) {
      P_Printf(consoleplayer,
        "run_queued_player_commands: No peer for player #%d\n", playernum
      );
      return;
    }
  }

  if (SERVER && sv_limit_player_commands)
    command_limit = SV_GetPlayerCommandLimit(playernum);

  if (CLIENT && playernum == consoleplayer)
    P_PrintCommands(commands);

  while (!g_queue_is_empty(commands)) {
    int saved_leveltime = leveltime;
    netticcmd_t *ncmd = g_queue_peek_head(commands);

    if (SERVER && sv_limit_player_commands && commands_run >= command_limit)
      break;

    CL_SetupCommandState(playernum, ncmd);

    memcpy(&player->cmd, &ncmd->cmd, sizeof(ticcmd_t));
    leveltime = ncmd->tic;
    N_LogCommand(ncmd);
    run_player_command(player);

    if (player->mo)
      P_MobjThinker(player->mo);

    /*
    if (CLIENT)
    */
    N_LogPlayerPosition(player);

    CL_ShutdownCommandState();

    leveltime = saved_leveltime;

    if (CLIENT) {
      CL_UpdateCommandsRunSync(playernum, ncmd->index);
    }
    else if (SERVER) {
      SV_UpdateCommandsRunSync(playernum, playernum, ncmd->index);

      if (sv_limit_player_commands)
        commands_run++;
    }

    P_RecycleCommand(g_queue_pop_head(commands));
  }
}

void P_InitCommands(void) {
  if (blank_command_queue == NULL)
    blank_command_queue = g_queue_new();
}

netticcmd_t* P_GetNewBlankCommand(void) {
  netticcmd_t *ncmd = g_queue_pop_head(blank_command_queue);

  if (ncmd == NULL)
    ncmd = calloc(1, sizeof(netticcmd_t));

  return ncmd;
}

void P_BuildCommand(void) {
  GQueue *run_commands;
  GQueue *sync_commands;
  netticcmd_t *run_ncmd;
  netticcmd_t *sync_ncmd;

  if (!CLIENT)
    return;

  if (!CL_GetCommandSync(consoleplayer,
                         NULL,
                         NULL,
                         &sync_commands,
                         &run_commands)) {
    P_Echo(consoleplayer, "Server disconnected");
    return;
  }

  run_ncmd = P_GetNewBlankCommand();
  sync_ncmd = P_GetNewBlankCommand();

  G_BuildTiccmd(&run_ncmd->cmd);
  run_ncmd->index = CL_GetNextCommandIndex();
  run_ncmd->tic = gametic;
  memcpy(sync_ncmd, run_ncmd, sizeof(netticcmd_t));

  g_queue_push_tail(run_commands, run_ncmd);
  g_queue_push_tail(sync_commands, sync_ncmd);

  /*
  D_Log(LOG_SYNC, "(%d) P_BuildCommand: ", gametic);
  N_LogCommand(run_ncmd);
  */

  if (CLIENT)
    CL_MarkServerOutdated();
}

unsigned int P_GetPlayerRunCommandCount(int playernum) {
  GQueue *commands;

  if (CLIENT) {
    if (!CL_GetCommandSync(playernum, NULL, NULL, NULL, &commands)) {
      P_Echo(consoleplayer, "Server disconnected");
      return 0;
    }
  }
  else {
    if (!SV_GetCommandSync(playernum, playernum, NULL, NULL, NULL, &commands)) {
      P_Printf(consoleplayer,
        "P_GetPlayerRunCommandCount: No peer for player #%d\n", playernum
      );
      return 0;
    }
  }

  return g_queue_get_length(commands);
}

unsigned int P_GetPlayerSyncCommandCount(int playernum) {
  GQueue *commands;

  if (CLIENT) {
    if (!CL_GetCommandSync(playernum, NULL, NULL, &commands, NULL)) {
      P_Echo(consoleplayer, "Server disconnected");
      return 0;
    }
  }
  else {
    if (!SV_GetCommandSync(playernum, playernum, NULL, NULL, &commands, NULL)) {
      P_Printf(consoleplayer,
        "P_GetPlayerRunCommandCount: No peer for player #%d\n", playernum
      );
      return 0;
    }
  }

  return g_queue_get_length(commands);
}

void P_RunPlayerCommands(int playernum) {
  player_t *player = &players[playernum];
  if (!(DELTACLIENT || DELTASERVER)) {
    run_player_command(player);
    return;
  }
  
  if (DELTACLIENT && playernum != consoleplayer && playernum != 0) {
    int command_count = P_GetPlayerRunCommandCount(playernum);

    if (command_count == 0) {
      if (!extrapolate_player_position(player))
        return;
    }
    else {
      player->missed_command_count = 0;
      /*
      printf("(%d) Got (%d) command(s) for %d\n",
        gametic, command_count, playernum
      );
      */
    }
  }

  run_queued_player_commands(playernum);
}

void P_ClearRunCommands(int playernum) {
  GQueue *commands;

  if (!CL_GetCommandSync(playernum, NULL, NULL, NULL, &commands)) {
    P_Printf(consoleplayer,
      "run_queued_player_commands: No peer for player #%d\n", playernum
    );
    return;
  }

  while (!g_queue_is_empty(commands))
    P_RecycleCommand(g_queue_pop_head(commands));
}

void P_RemoveOldCommands(int sync_index, GQueue *commands) {
  while (!g_queue_is_empty(commands)) {
    netticcmd_t *ncmd = g_queue_peek_head(commands);

    if (ncmd->index <= sync_index) {
      D_Log(LOG_SYNC, "P_RemoveOldCommands: Removing command %d\n",
        ncmd->index
      );

      P_RecycleCommand(g_queue_pop_head(commands));
    }
    else {
      break;
    }
  }
}

void P_RecycleCommand(netticcmd_t *ncmd) {
  memset(ncmd, 0, sizeof(netticcmd_t));
  g_queue_push_tail(blank_command_queue, ncmd);
}

void P_PrintCommands(GQueue *commands) {
  D_Log(LOG_SYNC, "{");
  g_queue_foreach(commands, print_command, NULL);
  D_Log(LOG_SYNC, " }\n");
}

/* vi: set et ts=2 sw=2: */

