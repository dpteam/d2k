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


#ifndef V_VIDEO_H__
#define V_VIDEO_H__

//
// VIDEO
//

typedef enum
{
  patch_stretch_16x10,
  patch_stretch_4x3,
  patch_stretch_full,
  
  patch_stretch_max
} patch_stretch_t;

/* cph - from v_video.h, needed by gl_struct.h */
#define VPT_ALIGN_MASK 0xF
#define VPT_STRETCH_MASK 0x1F

enum patch_translation_e {
  // e6y: wide-res
  VPT_ALIGN_LEFT         = 1,
  VPT_ALIGN_RIGHT        = 2,
  VPT_ALIGN_TOP          = 3,
  VPT_ALIGN_LEFT_TOP     = 4,
  VPT_ALIGN_RIGHT_TOP    = 5,
  VPT_ALIGN_BOTTOM       = 6,
  VPT_ALIGN_WIDE         = 7,
  VPT_ALIGN_LEFT_BOTTOM  = 8,
  VPT_ALIGN_RIGHT_BOTTOM = 9,
  VPT_ALIGN_MAX          = 10,
  VPT_STRETCH            = 16, // Stretch to compensate for high-res

  VPT_NONE    = 128, // Normal
  VPT_FLIP    = 256, // Flip image horizontally
  VPT_TRANS   = 512, // Translate image via a translation table
  VPT_NOOFFSET = 1024,
};

typedef struct
{
   fixed_t     xstep, ystep;

   int width, height;

   // SoM 1-31-04: This will insure that scaled patches and such are put in the right places
   short x1lookup[321];
   short y1lookup[201];
   short x2lookup[321];
   short y2lookup[201];
} cb_video_t;

typedef struct stretch_param_s
{
  cb_video_t *video;
  int deltax1;
  int deltay1;
  int deltax2;
  int deltay2;
} stretch_param_t;

extern stretch_param_t stretch_params_table[3][VPT_ALIGN_MAX];
extern stretch_param_t *stretch_params;

extern cb_video_t video;
extern cb_video_t video_stretch;
extern cb_video_t video_full;
extern int patches_scalex;
extern int patches_scaley;

extern const char *render_aspects_list[];
extern const char *render_stretch_list[];

extern int render_stretch_hud;
extern int render_stretch_hud_default;
extern int render_patches_scalex;
extern int render_patches_scaley;

// DWF 2012-05-10
// SetRatio sets the following global variables based on window geometry and
// user preferences. The integer ratio is hardly used anymore, so further
// simplification may be in order.
void SetRatio(int width, int height);
extern bool tallscreen;
extern unsigned int ratio_multiplier, ratio_scale;
extern float gl_ratio;
extern int psprite_offset; // Needed for "tallscreen" modes

#define CENTERY     (SCREENHEIGHT/2)

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.

// array of pointers to color translation tables
extern const unsigned char *colrngs[];

// symbolic indices into color translation table pointer array
typedef enum
{
  CR_BRICK,   //0
  CR_TAN,     //1
  CR_GRAY,    //2
  CR_GREEN,   //3
  CR_BROWN,   //4
  CR_GOLD,    //5
  CR_RED,     //6
  CR_BLUE,    //7
  CR_ORANGE,  //8
  CR_YELLOW,  //9
  CR_BLUE2,   //10 // proff
  CR_LIMIT    //11 //jff 2/27/98 added for range check
} crange_idx_e;
//jff 1/16/98 end palette color range additions

#define CR_DEFAULT CR_RED   /* default value for out of range colors */

typedef struct {
  unsigned char *data;          // pointer to the screen content
  bool not_on_heap; // if set, no malloc or free is preformed and
                       // data never set to NULL. Used i.e. with SDL doublebuffer.
  int width;           // the width of the surface
  int height;          // the height of the surface, used when mallocing
  int byte_pitch;      // tha actual width of one line, used when mallocing
  int int_pitch;       // tha actual width of one line, used when mallocing
} screeninfo_t;

#define NUM_SCREENS 6
extern screeninfo_t screens[NUM_SCREENS];
extern int          usegamma;

// Varying bit-depth support -POPE
//
// For bilinear filtering, each palette color is pre-weighted and put in a
// table for fast blending operations. These macros decide how many weights
// to create for each color. The lower the number, the lower the blend
// accuracy, which can produce very bad artifacts in texture filtering.
#define VID_NUMCOLORWEIGHTS 64
#define VID_COLORWEIGHTMASK (VID_NUMCOLORWEIGHTS-1)
#define VID_COLORWEIGHTBITS 6

// Palettes for converting from 8 bit color to 16 and 32 bit. Also
// contains the weighted versions of each palette color for filtering
// operations
extern unsigned short *V_Palette15;
extern unsigned short *V_Palette16;
extern unsigned int *V_Palette32;

#define VID_PAL15(color, weight) V_Palette15[ (color)*VID_NUMCOLORWEIGHTS + (weight) ]
#define VID_PAL16(color, weight) V_Palette16[ (color)*VID_NUMCOLORWEIGHTS + (weight) ]
#define VID_PAL32(color, weight) V_Palette32[ (color)*VID_NUMCOLORWEIGHTS + (weight) ]

// The available bit-depth modes
typedef enum {
  /*
  VID_MODE8,
  VID_MODE15,
  VID_MODE16,
  */
  VID_MODE32,
  VID_MODEGL,
  VID_MODEMAX
} video_mode_t;

extern const char *default_videomode;

void V_InitMode(video_mode_t mode);

// video mode query interface
video_mode_t V_GetMode(void);
int V_GetModePixelDepth(video_mode_t mode);
int V_GetNumPixelBits(void);
int V_GetPixelDepth(void);

//jff 4/24/98 loads color translation lumps
void V_InitColorTranslation(void);

void V_InitFlexTranTable(void);

// Allocates buffer screens, call before R_Init.
void V_Init (void);

// V_CopyRect
typedef void (*V_CopyRect_f)(int srcscrn, int destscrn,
                             int x, int y,
                             int width, int height,
                             enum patch_translation_e flags);
extern V_CopyRect_f V_CopyRect;

// V_FillRect
typedef void (*V_FillRect_f)(int scrn, int x, int y,
                             int width, int height, unsigned char colour);
extern V_FillRect_f V_FillRect;

// CPhipps - patch drawing
// Consolidated into the 3 really useful functions:

// V_DrawNumPatch - Draws the patch from lump num
typedef void (*V_DrawNumPatch_f)(int x, int y, int scrn,
                                 int lump, int cm,
                                 enum patch_translation_e flags);
extern V_DrawNumPatch_f V_DrawNumPatch;

typedef void (*V_DrawNumPatchPrecise_f)(float x, float y, int scrn,
                                 int lump, int cm,
                                 enum patch_translation_e flags);
extern V_DrawNumPatchPrecise_f V_DrawNumPatchPrecise;

// V_DrawNamePatch - Draws the patch from lump "name"
#define V_DrawNamePatch(x,y,s,n,t,f) V_DrawNumPatch(x,y,s,W_GetNumForName(n),t,f)
#define V_DrawNamePatchPrecise(x,y,s,n,t,f) V_DrawNumPatchPrecise(x,y,s,W_GetNumForName(n),t,f)

/* cph -
 * Functions to return width & height of a patch.
 * Doesn't really belong here, but is often used in conjunction with
 * this code.
 */
#define V_NamePatchWidth(name) R_NumPatchWidth(W_GetNumForName(name))
#define V_NamePatchHeight(name) R_NumPatchHeight(W_GetNumForName(name))

// e6y
typedef void (*V_FillFlat_f)(int lump, int scrn, int x, int y, int width, int height, enum patch_translation_e flags);
extern V_FillFlat_f V_FillFlat;
#define V_FillFlatName(flatname, scrn, x, y, width, height, flags) \
  V_FillFlat(R_FlatNumForName(flatname), (scrn), (x), (y), (width), (height), (flags))

typedef void (*V_FillPatch_f)(int lump, int scrn, int x, int y, int width, int height, enum patch_translation_e flags);
extern V_FillPatch_f V_FillPatch;
#define V_FillPatchName(name, scrn, x, y, width, height, flags) \
  V_FillPatch(W_GetNumForName(name), (scrn), (x), (y), (width), (height), (flags))


/* cphipps 10/99: function to tile a flat over the screen */
typedef void (*V_DrawBackground_f)(const char* flatname, int scrn);
extern V_DrawBackground_f V_DrawBackground;

void V_DestroyUnusedTrueColorPalettes(void);
// CPhipps - function to set the palette to palette number pal.
void V_SetPalette(int pal);

// Alt-Enter: fullscreen <-> windowed
void V_ToggleFullscreen(void);
void V_ChangeScreenResolution(void);

// CPhipps - function to plot a pixel

// V_PlotPixel
typedef void (*V_PlotPixel_f)(int,int,int,unsigned char);
extern V_PlotPixel_f V_PlotPixel;

typedef struct
{
  int x, y;
  float fx, fy;
} fpoint_t;

typedef struct
{
  fpoint_t a, b;
} fline_t;

// V_DrawLine
typedef void (*V_DrawLine_f)(fline_t* fl, int color);
extern V_DrawLine_f V_DrawLine;

// V_DrawLineWu
typedef void (*V_DrawLineWu_f)(fline_t* fl, int color);
extern V_DrawLineWu_f V_DrawLineWu;

// V_PlotPixelWu
typedef void (*V_PlotPixelWu_f)(int scrn, int x, int y, unsigned char color, int weight);
extern V_PlotPixelWu_f V_PlotPixelWu;

void V_AllocScreen(screeninfo_t *scrn);
void V_AllocScreens();
void V_FreeScreen(screeninfo_t *scrn);
void V_FreeScreens();

const unsigned char* V_GetPlaypal(void);
void V_FreePlaypal(void);

// e6y: wide-res
void V_FillBorder(int lump, unsigned char color);

void V_GetWideRect(int *x, int *y, int *w, int *h, enum patch_translation_e flags);

int V_BestColor(const unsigned char *palette, int r, int g, int b);

#endif

/* vi: set et ts=2 sw=2: */

