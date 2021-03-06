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

#include "c_cvar.h"
#include "x_main.h"
#include "x_intern.h"

static void load_cvar(const char *section, const char *value) {
  lua_getglobal(X_GetState(), X_NAMESPACE);

  lua_getfield(X_GetState(), -1, X_CVAR_NAMESPACE);
  if (!lua_istable(X_GetState(), -1)) {
    I_Error("CVAR namespace [%s.%s] is not a table\n", 
      X_NAMESPACE, X_CVAR_NAMESPACE
    );
  }
  lua_remove(X_GetState(), -2);

  lua_getfield(X_GetState(), -1, section);
  if (!lua_istable(X_GetState(), -1)) {
    I_Error("CVAR section [%s.%s.%s] is not a table\n", 
      X_NAMESPACE, X_CVAR_NAMESPACE, section
    );
  }
  lua_remove(X_GetState(), -2);

  lua_getfield(X_GetState(), -1, value);
  if (lua_isnil(X_GetState(), -1)) {
    I_Error("CVAR [%s.%s] not found\n", section, value);
  }
  lua_remove(X_GetState(), -2);
}

bool C_CVarGetBool(const char *section, const char *value) {
  load_cvar(section, value);
  if (!lua_isboolean(X_GetState(), -1)) {
    I_Error("CVAR [%s.%s] is not a boolean\n", section, value);
  }

  return X_PopString(X_GetState());
}

int32_t C_CVarGetInt(const char *section, const char *value) {
  load_cvar(section, value);
  if (!lua_isnumber(X_GetState(), -1)) {
    I_Error("CVAR [%s.%s] is not a number\n", section, value);
  }

  return X_PopInteger(X_GetState());
}

uint32_t C_CVarGetUInt(const char *section, const char *value) {
  load_cvar(section, value);
  if (!lua_isnumber(X_GetState(), -1)) {
    I_Error("CVAR [%s.%s] is not a number\n", section, value);
  }

  return X_PopUInteger(X_GetState());
}

double C_CVarGetFloat(const char *section, const char *value) {
  load_cvar(section, value);
  if (!lua_isnumber(X_GetState(), -1)) {
    I_Error("CVAR [%s.%s] is not a number\n", section, value);
  }

  return X_PopDecimal(X_GetState());
}

const char* C_CVarGetString(const char *section, const char *value) {
  load_cvar(section, value);
  if (!lua_isstring(X_GetState(), -1)) {
    I_Error("CVAR [%s.%s] is not a string\n", section, value);
  }

  return X_PopString(X_GetState());
}

/* vi: set et ts=2 sw=2: */

