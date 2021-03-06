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

#include <time.h>

#include "c_main.h"
#include "d_msg.h"
#include "m_file.h"

/*
 * CG [TODO]: Add an API to disable flushing a message channel's log file after
 *            writing every message.
 */

typedef struct message_channel_s {
  bool  active;
  FILE *fobj;
} message_channel_t;

static bool message_channels_initialized = false;
static message_channel_t message_channels[MSG_MAX + 1];

static void check_message_channel(msg_channel_e c) {
  if (!message_channels_initialized)
    I_Error("Messaging has not yet been initialized!");

  if (c < MSG_MIN)
    I_Error("Invalid message channel %d (valid: %d - %d)", c, MSG_MIN, MSG_MAX);

  if (c > MSG_MAX)
    I_Error("Invalid message channel %d (valid: %d - %d)", c, MSG_MIN, MSG_MAX);
}

static void deactivate_message_channels(void) {
  for (msg_channel_e chan = MSG_MIN; chan <= MSG_MAX; chan++) {
    D_MsgDeactivate(chan);
  }
}

void D_InitMessaging(void) {
  for (msg_channel_e chan = MSG_MIN; chan <= MSG_MAX; chan++) {
    message_channel_t *mc = &message_channels[chan];

    mc->active = false;
    mc->fobj = NULL;
  }
  atexit(deactivate_message_channels);

  message_channels_initialized = true;
}

bool D_MsgActive(msg_channel_e chan) {
  check_message_channel(chan);

  return message_channels[chan].active;
}

void D_MsgActivate(msg_channel_e chan) {
  check_message_channel(chan);

  message_channels[chan].active = true;
}

bool D_MsgActivateWithPath(msg_channel_e chan, const char *file_path) {
  check_message_channel(chan);

  if (!D_LogToPath(chan, file_path)) {
    return false;
  }

  D_MsgActivate(chan);

  return true;
}

bool D_MsgActivateWithFile(msg_channel_e chan, FILE *fobj) {
  check_message_channel(chan);

  if (!D_LogToFile(chan, fobj)) {
    return false;
  }

  D_MsgActivate(chan);

  return true;
}

bool D_MsgActivateWithFD(msg_channel_e chan, int fd) {
  check_message_channel(chan);

  if (!D_LogToFD(chan, fd)) {
    return false;
  }

  D_MsgActivate(chan);

  return true;
}

void D_MsgDeactivate(msg_channel_e chan) {
  message_channel_t *mc;

  check_message_channel(chan);

  mc = &message_channels[chan];

  if (mc->fobj != NULL) {
    fflush(mc->fobj);
    fclose(mc->fobj);
  }

  mc->active = false;
  mc->fobj = NULL;
}

void D_VMsg(msg_channel_e chan, const char *fmt, va_list args) {
  va_list log_args;
  va_list console_args;
  message_channel_t *mc;

  check_message_channel(chan);

  mc = &message_channels[chan];

  switch (chan) {
    case MSG_DEBUG:
    case MSG_DEH:
    case MSG_GAME:
    case MSG_SAVE:
    case MSG_INFO:
    case MSG_WARN:
    case MSG_ERROR:
      va_copy(console_args, args);
      C_MVPrintf(fmt, console_args);
      va_end(console_args);
      break;
    default:
      break;
  }

  if (mc->fobj) {
    va_copy(log_args, args);
    vfprintf(mc->fobj, fmt, log_args);
    va_end(log_args);
    fflush(mc->fobj);
  }
}

void D_Msg(msg_channel_e chan, const char *fmt, ...) {
  va_list args;
  va_list log_args;
  va_list console_args;
  va_list cmdline_args;
  message_channel_t *mc;

  check_message_channel(chan);

  mc = &message_channels[chan];

  if (!mc->active)
    return;

  va_start(args, fmt);

  if ((chan == MSG_DEBUG) ||
      (chan == MSG_DEH)   ||
      (chan == MSG_GAME)  ||
      (chan == MSG_SAVE)  ||
      (chan == MSG_INFO)  ||
      (chan == MSG_WARN)  ||
      (chan == MSG_ERROR)) {
    va_copy(console_args, args);
    C_MVPrintf(fmt, console_args);
    va_end(console_args);
  }

  if (mc->fobj) {
    va_copy(log_args, args);
    vfprintf(mc->fobj, fmt, log_args);
    va_end(log_args);
    fflush(mc->fobj);
  }

  va_copy(cmdline_args, args);
  vprintf(fmt, cmdline_args);
  va_end(cmdline_args);
  fflush(stdout);

  va_end(args);
}

bool D_LogToPath(msg_channel_e chan, const char *file_path) {
  check_message_channel(chan);

  if (message_channels[chan].fobj) {
    if (!M_CloseFile(message_channels[chan].fobj)) {
      D_MsgDeactivate(chan);
      return false;
    }
  }

  char *full_file_path = g_strdup_printf(
    "%u-%s",
    (unsigned char)time(NULL),
    file_path
  );

  message_channels[chan].fobj = M_OpenFile(full_file_path, "w");

  g_free(full_file_path);

  if (message_channels[chan].fobj)
    D_MsgActivate(chan);
  else
    D_MsgDeactivate(chan);

  return D_MsgActive(chan);
}

bool D_LogToFile(msg_channel_e chan, FILE *fobj) {
  check_message_channel(chan);

  if (message_channels[chan].fobj) {
    if (!M_CloseFile(message_channels[chan].fobj)) {
      D_MsgDeactivate(chan);
      return false;
    }
  }

  message_channels[chan].fobj = fobj;

  if (message_channels[chan].fobj)
    D_MsgActivate(chan);
  else
    D_MsgDeactivate(chan);

  return D_MsgActive(chan);
}

bool D_LogToFD(msg_channel_e chan, int fd) {
  message_channel_t *mc;

  check_message_channel(chan);

  mc = &message_channels[chan];

  if (!mc->active)
    return false;

  if (!mc->fobj)
    return false;

  if (!M_CloseFile(mc->fobj)) {
    I_Error("Error closing log file for channel %d: %s\n",
      chan,
      M_GetFileError()
    );
  }

  mc->fobj = M_OpenFD(fd, "w");

  if (!mc->fobj) {
    I_Error("Error logging channel %d to FD %d: %s\n",
      chan,
      fd,
      M_GetFileError()
    );
  }

  return true;
}

FILE* D_MsgGetFile(msg_channel_e chan) {
  message_channel_t *mc;

  check_message_channel(chan);

  mc = &message_channels[chan];

  if (!mc->active)
    return NULL;

  return mc->fobj;
}

int D_MsgGetFD(msg_channel_e chan) {
  int fd;
  FILE *fobj;

  fobj = D_MsgGetFile(chan);

  if (!fobj)
    return -1;

  fd = fileno(fobj);

  if (fd == -1) {
    I_Error("Error retrieving file descriptor from log channel %d: %s\n",
      chan,
      strerror(errno)
    );
  }

  return fd;
}

/* vi: set et ts=2 sw=2: */

