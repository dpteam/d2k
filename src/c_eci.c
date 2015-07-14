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

#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>

#include "c_main.h"
#include "n_net.h"
#include "n_uds.h"
#include "x_main.h"

/* CG [TODO]: Make this configurable */
#define D2K_SERVER_SOCKET_NAME "d2k.sock"

static uds_t uds;

static void handle_data(uds_t *uds, uds_peer_t *peer) {
  C_HandleInput(uds->input->str);
}

static void handle_oob(uds_t *uds) {
  D_Msg(MSG_ERROR, "ECI: Got out-of-band data [%s]\n", uds->oob->str);
}

static void eci_cleanup(void) {
  N_UDSFree(&uds);
}

bool eci_available(void) {
#ifdef G_OS_UNIX
  if (SERVER)
    return true;
#endif
  return false;
}

void C_ECIInit(void) {
  if (!eci_available())
    return;

  memset(&uds, 0, sizeof(uds_t));

  N_UDSInit(
    &uds,
    D2K_SERVER_SOCKET_NAME,
    handle_data,
    handle_oob,
    true
  );

  atexit(eci_cleanup);
}

void C_ECIService(void) {
  if (!eci_available())
    return;

  if (C_MessagesUpdated()) {
    GString *messages = C_GetMessages();

    N_UDSBroadcast(&uds, messages->str);
    C_ClearMessagesUpdated();
  }

  N_UDSService(&uds);
}

