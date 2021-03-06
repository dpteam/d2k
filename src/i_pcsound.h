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


#ifndef I_PCSOUND_H__
#define I_PCSOUND_H__

int I_PCS_StartSound(int id,
                     int channel,
                     int vol,
                     int sep,
                     int pitch,
                     int priority);
void I_PCS_StopSound(int handle);
int I_PCS_SoundIsPlaying(int handle);
void I_PCS_InitSound(void);

#endif /* #ifndef __I_PCSOUND_H__ */

/* vi: set et ts=2 sw=2: */

