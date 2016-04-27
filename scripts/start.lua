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

package.path = package.path .. ';' .. d2k.script_search_path

local Fonts = require('fonts')

if not DEBUG then
    print = function(s)
        d2k.Messaging.print(d2k.Messaging.INFO, string.format('%s\n', s))
    end
end

mprint = function(s)
    d2k.Messaging.mecho(d2k.Messaging.INFO, s)
end

print('X_Init: Creating input handler.')
local input_handler = require('input_handler')
d2k.interfaces = input_handler.InputHandler()

if d2k.Video.is_enabled() then
  print('X_Init: Creating overlay.')
  local Overlay = require('overlay')
  d2k.overlay = Overlay.Overlay()

  print('X_Init: Creating Menu')
  local Menu = require('menu')
  d2k.menu = Menu.Menu()
  d2k.menu:set_parent(d2k.interfaces)

  print('X_Init: Creating console')
  local Console = require('console')
  d2k.console = Console.Console({
    font_description_text = Fonts.DEFAULT_CONSOLE_FONT
  })
  d2k.console:set_parent(d2k.interfaces)

  print('X_Init: Creating Game Interface')
  local GameInterface = require('game_interface')
  d2k.game_interface = GameInterface.GameInterface()
  d2k.game_interface:set_parent(d2k.interfaces)

  print('X_Init: Creating HUD')
  local HUD = require('hud')
  d2k.hud = HUD.HUD({
    font_description_text = Fonts.DEFAULT_HUD_FONT
  })
  d2k.hud:set_parent(d2k.interfaces)

  print('X_Init: Loading HUD widgets')
  func, err = loadfile(d2k.script_folder .. '/local_hud_widgets.lua', 't')
  if func then
    local worked, err = pcall(func)
    if not worked then
      mprint(string.format('<span color="red">%s</span>', err))
      print(err)
    end
  else
    mprint(string.format('<span color="red">%s</span>', err))
    print(err)
  end
else
  d2k.overlay = nil
  d2k.hud = nil
end

print('X_Init: Loading console shortcuts')
require('console_shortcuts')

-- vi: et ts=4 sw=4
