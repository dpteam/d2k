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


#ifndef PL_CMD_H__
#define PL_CMD_H__

uint32_t     PL_GetLatestServerRunCommandIndex(player_t *player);
void         PL_UpdateCommandServerTic(player_t *player,
                                       uint32_t command_index,
                                       uint32_t server_tic);
void         PL_InitCommandQueues(void);
size_t       PL_GetCommandCount(player_t *player);
netticcmd_t* PL_GetCommand(player_t *player, unsigned int index);
void         PL_QueueCommand(player_t *player, netticcmd_t *ncmd);
void         PL_AppendNewCommand(player_t *player, netticcmd_t *tmp_ncmd);
uint32_t     PL_GetLatestCommandIndex(player_t *player);
void         PL_ForEachCommand(player_t *player, GFunc func,
                                                 gpointer user_data);
void         PL_ClearCommands(player_t *player);
void         PL_ResetCommands(player_t *player);
void         PL_IgnoreCommands(player_t *player);
void         PL_TrimCommands(player_t *player, TrimFunc should_trim,
                                               gpointer user_data);
void         PL_BuildCommand(void);
bool         PL_RunCommands(player_t *player);

#endif

/* vi: set et ts=2 sw=2: */
