-------------------------------------------------------------------------------
--                                                                           --
-- D2K: A Doom Source Port for the 21st Century                              --
--                                                                           --
-- Copyright (C) 2014: See COPYRIGHT file                                    --
--                                                                           --
-- This file is part of D2K.                                                 --
--                                                                           --
-- D2K is free software: you can redistribute it and/or modify it under the  --
-- terms of the GNU General Public License as published by the Free Software --
-- Foundation, either version 2 of the License, or (at your option) any      --
-- later version.                                                            --
--                                                                           --
-- D2K is distributed in the hope that it will be useful, but WITHOUT ANY    --
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS --
-- FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more    --
-- details.                                                                  --
--                                                                           --
-- You should have received a copy of the GNU General Public License along   --
-- with D2K.  If not, see <http://www.gnu.org/licenses/>.                    --
--                                                                           --
-------------------------------------------------------------------------------

package.path = package.path .. ';' .. d2k.script_path

print('X_Init: Init script engine.')

print('X_Init: Creating input handler.')
local input_handler = require('input_handler')
d2k.input_handler = input_handler.InputHandler:new()

print('X_Init: Creating overlay.')
local Overlay = require('overlay')
d2k.overlay = Overlay.Overlay:new()

print('X_Init: Creating HUD')
local HUD = require('hud')
d2k.hud = HUD.HUD:new()

print('X_Init: Creating console')
local Console = require('console')
d2k.console = Console.Console:new()

print('X_Init: Adding console to HUD')
d2k.hud:add_widget(d2k.console)

-- vi: et ts=2 sw=2

