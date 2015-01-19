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

#include "d_event.h"
#include "g_game.h"
#include "x_main.h"

static int XG_GameHandleEvent(lua_State *L) {
  event_t *ev = luaL_checkudata(L, -1, "InputEvent");
  bool event_handled = G_Responder(ev);

  lua_pushboolean(L, event_handled);

  return 1;
}

void XG_GameRegisterInterface(void) {
  X_RegisterObjects("Game", 1,
    "handle_event", X_FUNCTION, XG_GameHandleEvent
  );
}

/* vi: set et ts=2 sw=2: */

