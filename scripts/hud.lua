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

local class = require('middleclass')
local lgi = require('lgi')
local Cairo = lgi.cairo

local InputInterface = require('input_interface')
local InputInterfaceContainer = require('input_interface_container')

HUD = class('HUD', InputInterface.InputInterface)
HUD:include(InputInterfaceContainer.InputInterfaceContainer)

function HUD:initialize(h)
  h = h or {}

  h.name = h.name or 'HUD'

  InputInterface.InputInterface.initialize(self, h)

  self.interfaces = {}

  self.font_description_text = h.font_description_text or
                                 'Noto Sans,Arial Unicode MS,Unifont 11'
end

function HUD:sort_interfaces()
  table.sort(self.interfaces, function(i1, i2)
    if i1:get_z_index() < i2:get_z_index() then
      return -1
    elseif i2:get_z_index() < i1:get_z_index() then
      return 1
    end

    return 0
  end)
end

return {HUD = HUD}

-- vi: et ts=2 sw=2

