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


#ifdef __GNUG__
#pragma implementation "doomstat.h"
#endif

#include "z_zone.h"

#include "doomdef.h"
#include "doomstat.h"
#include "d_event.h"
#include "p_user.h"
#include "g_game.h"

// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t gamemode = indetermined;
GameMission_t   gamemission = doom;

// Language.
Language_t   language = english;

// Set if homebrew PWAD stuff has been added.
bool modifiedgame;

//-----------------------------------------------------------------------------

// CPhipps - compatibility vars
complevel_t compatibility_level, default_compatibility_level;

// e6y
// it's required for demos recorded in "demo compatibility" mode by boom201 for example
int demover;

int comp[COMP_TOTAL], default_comp[COMP_TOTAL];    // killough 10/98
int /*comperr[COMPERR_NUM], */default_comperr[COMPERR_NUM];

// v1.1-like pitched sounds
int pitched_sounds;        // killough

int  default_translucency; // config file says           // phares
bool general_translucency; // true if translucency is ok // phares

int demo_insurance, default_demo_insurance;        // killough 1/16/98

int allow_pushers = 1;      // MT_PUSH Things              // phares 3/10/98
int default_allow_pushers;  // killough 3/1/98: make local to each game

int variable_friction = 1;      // ice & mud               // phares 3/10/98
int default_variable_friction;  // killough 3/1/98: make local to each game

int weapon_recoil;              // weapon recoil                   // phares
int default_weapon_recoil;      // killough 3/1/98: make local to each game

int player_bobbing;  // whether player bobs or not          // phares 2/25/98
int default_player_bobbing;  // killough 3/1/98: make local to each game

int leave_weapons;         // leave picked-up weapons behind?
int default_leave_weapons; // CG 08/19/2014

int monsters_remember;          // killough 3/1/98
int default_monsters_remember;

int monster_infighting=1;       // killough 7/19/98: monster<=>monster attacks
int default_monster_infighting=1;

int monster_friction=1;       // killough 10/98: monsters affected by friction
int default_monster_friction=1;

#ifdef DOGS
int dogs, default_dogs;         // killough 7/19/98: Marine's best friend :)
int dog_jumping, default_dog_jumping;   // killough 10/98
#endif

// killough 8/8/98: distance friends tend to move towards players
int distfriend = 128, default_distfriend = 128;

// killough 9/8/98: whether monsters are allowed to strafe or retreat
int monster_backing, default_monster_backing;

// killough 9/9/98: whether monsters are able to avoid hazards (e.g. crushers)
int monster_avoid_hazards, default_monster_avoid_hazards;

// killough 9/9/98: whether monsters help friends
int help_friends, default_help_friends;

int flashing_hom;     // killough 10/98

int doom_weapon_toggles; // killough 10/98

int monkeys, default_monkeys;

/* vi: set et ts=2 sw=2: */

