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


#ifndef I_VIDEO_H__
#define I_VIDEO_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doomtype.h"
#include "v_video.h"

#ifdef __GNUG__
#pragma interface
#endif

#ifdef GL_DOOM
typedef struct SDL_Surface *PSDL_Surface;

typedef struct vid_8ingl_s {
  int enabled;
  PSDL_Surface screen;
  PSDL_Surface surface;
  GLuint texid;
  GLuint pboids[2];
  int width;
  int height;
  int size;
  unsigned char *buf;
  unsigned char *colours;
  int palette;
  float fU1;
  float fU2;
  float fV1;
  float fV2;
} vid_8ingl_t;
#endif

extern vid_8ingl_t vid_8ingl;
extern int use_gl_surface;
extern const char *screen_resolutions_list[];
extern const char *screen_resolution;

extern const char *sdl_videodriver;
extern const char *sdl_video_window_pos;

extern int use_doublebuffer;
extern int use_fullscreen;
extern int desired_fullscreen;

extern int process_affinity_mask;
extern int process_priority;
extern int try_to_reduce_cpu_cache_misses;
extern dboolean window_focused;

void        I_PreInitGraphics(void);
void        I_InitScreenResolution(void);
void        I_InitGraphics(void);
void        I_UpdateVideoMode(void);
void        I_ShutdownGraphics(void);
void        I_SetPalette(int pal);
void        I_UpdateNoBlit(void);
void        I_FinishUpdate(void);
int         I_ScreenShot(const char *fname);
void        I_LockOverlaySurface(void);
void        I_UnlockOverlaySurface(void);
void        I_StartTic (void);
void        I_StartFrame (void);
const char* I_GetKeyString(int keycode);

#endif

/* vi: set et ts=2 sw=2: */

