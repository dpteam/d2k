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

#include <wchar.h>
#include <glib.h>
#include <fontconfig/fontconfig.h>

#include "d_event.h"
#include "c_main.h"
#include "i_font.h"
#include "lprintf.h"
#include "m_file.h"

void I_LoadCustomFonts(void) {
  gchar *cwd = g_get_current_dir();
  char *font_folder = M_PathJoin(cwd, FONT_FOLDER_NAME);

  lprintf(LO_INFO, "I_LoadCustomFonts: Loading custom fonts\n");

#ifdef _WIN32
  obuf_t *font_files = M_ListFiles(font_folder);

  if (font_files == NULL) {
    I_Error("  Error reading font folder '%s' (%s)",
      FONT_FOLDER_NAME,
      M_GetFileError()
    );
  }

  OBUF_FOR_EACH(font_files, entry) {
    char *font_path = M_LocalizePath((char *)entry.obj);
    int res = AddFontResourceEx(font_path, FR_PRIVATE, 0);

    if (res == 0)
      wprintf(L"  Failed to load font %ls\n", font_path);
    else
      wprintf(L"  Loaded %d fonts from %ls\n", res, font_path);

    free(font_path);
  }
#else
  FcConfig *config = FcInitLoadConfigAndFonts();

  if (!FcConfigAppFontAddDir(config, (FcChar8 *)font_folder)) {
    I_Error("  Error loading font folder '%s'",
      FONT_FOLDER_NAME
    );
  }

  free(font_folder);

  FcConfigSetCurrent(config);
#endif
}

/* vi: set et ts=2 sw=2: */

