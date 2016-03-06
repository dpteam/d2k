-------------------------------------------------------------------------------
--                                                                           --
-- D2K: A Doom Source Port for the 21st Century                              --
--                                                                           --
-- Copyright (C) 2014: See COPYRIGHT file                                    --
--                                                                           --
-- This file is part of D2K.                                                 --
--                                                                           --
-- D2K is free software: you can redistribute it and/or modify it under the  --
-- terms of the GNU General Public License as published by the Free Sofiware --
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
local TextWidget = require('text_widget')
local InputWidget = require('input_widget')
local RetractableTextWidget = require('retractable_text_widget')
local TableWidget = require('table_widget')
local InputInterface = require('input_interface')
local Fonts = require('fonts')
local ContainerWidget = require('container_widget')

local messages_widget = RetractableTextWidget.RetractableTextWidget({
  name = 'messages',
  z_index = 1,
  top_margin = 4,
  bottom_margin = 0,
  left_margin = 8,
  right_margin = 8,
  width = 1,
  bg_color = {0.0, 0.0, 0.0, 0.0},
  vertical_alignment = TextWidget.ALIGN_BOTTOM,
  outline_color = {0.0, 0.0, 0.0, 1.0},
  outline_text = true,
  outline_width = 1,
  line_height = 4,
  use_markup = true,
  font_description_text = Fonts.DEFAULT_MESSAGES_FONT,
  strip_ending_newline = true,
  retractable = RetractableTextWidget.RETRACT_UP,
  retraction_time = 500,
  retraction_timeout = 2000,
})

messages_widget:set_external_text_source(
  d2k.Game.get_consoleplayer_messages,
  d2k.Game.get_consoleplayer_messages_updated,
  d2k.Game.clear_consoleplayer_messages_updated
)

messages_widget:set_parent(d2k.hud)

local chat_widget = InputWidget.InputWidget({
  name = 'chat',
  z_index = 1,
  screen_reference_point = InputInterface.REFERENCE_POINT_SOUTHWEST,
  origin_reference_point = InputInterface.REFERENCE_POINT_SOUTHWEST,
  top_margin = 4,
  bottom_margin = 4,
  left_margin = 8,
  right_margin = 8,
  width = 1,
  line_height = 1,
  use_markup = false,
  strip_ending_newline = true,
  horizontal_alignment = TextWidget.ALIGN_LEFT,
  vertical_alignment = TextWidget.ALIGN_CENTER,
  fg_color = {1.0, 1.0, 1.0, 1.0},
  bg_color = {0.0, 0.0, 0.0, 0.85},
  input_handler = function(input) d2k.Client.say(input) end,
  deactivate_on_input = true
})

function chat_widget:render()
  if self:is_active() then
    InputWidget.InputWidget.render(self)
  end
end

function chat_widget:handle_event(event)
  if not self:is_active() then
    if event:is_key_press(d2k.Key.t) then
      self:activate()
      return true
    end

    return false
  end

  if event:is_key_press(d2k.Key.ESCAPE) then
    self:clear()
    self:deactivate()
    return true
  end

  return InputWidget.InputWidget.handle_event(self, event)
end

chat_widget:set_parent(d2k.hud)

local fps_widget = TextWidget.TextWidget({
  name = 'fps',
  z_index = 1,
  top_margin = 4,
  bottom_margin = 4,
  left_margin = 8,
  right_margin = 8,
  x = 0,
  y = 100,
  width = .15,
  height = .1,
  use_markup = false,
  horizontal_alignment = TextWidget.ALIGN_RIGHT,
  vertical_alignment = TextWidget.ALIGN_CENTER,
  fg_color = {1.0, 1.0, 1.0, 1.00},
  bg_color = {0.0, 0.0, 0.0, 0.65},
})

function fps_widget:get_last_time()
  return self.last_time
end

function fps_widget:set_last_time(last_time)
  self.last_time = last_time
end

function fps_widget:get_frame_count()
  return self.frame_count
end

function fps_widget:set_frame_count(frame_count)
  self.frame_count = frame_count
end

function fps_widget:increment_frame_count()
  self.frame_count = self.frame_count + 1
end

function fps_widget:tick()
  local current_time = d2k.System.get_ticks()
  local time_elapsed = current_time - self:get_last_time()

  self:increment_frame_count()

  if time_elapsed >= 1000 then
    self:set_text(string.format(
      '%03.2f FPS', (self:get_frame_count() / time_elapsed) * 1000
    ))
    self:set_frame_count(0)
    self:set_last_time(current_time)
  end
end

fps_widget:set_last_time(d2k.System.get_ticks())
fps_widget:set_frame_count(0)
fps_widget:set_text('0 FPS')

fps_widget:set_parent(d2k.hud)

local netstats_widget = TextWidget.TextWidget({
  name = 'netstats',
  z_index = 1,
  top_margin = 4,
  bottom_margin = 4,
  left_margin = 8,
  right_margin = 8,
  screen_reference_point = InputInterface.REFERENCE_POINT_EAST,
  origin_reference_point = InputInterface.REFERENCE_POINT_EAST,
  y = -50,
  width = .35,
  height = .22,
  use_markup = false,
  horizontal_alignment = TextWidget.ALIGN_LEFT,
  vertical_alignment = TextWidget.ALIGN_TOP,
  fg_color = {1.0, 1.0, 1.0, 1.00},
  bg_color = {0.0, 0.0, 0.0, 0.65},
})

function netstats_widget:get_last_time()
  return self.last_time
end

function netstats_widget:set_last_time(last_time)
  self.last_time = last_time
end

function netstats_widget:tick()
  local current_time = d2k.System.get_ticks()
  local time_elapsed = current_time - self:get_last_time()

  if time_elapsed >= 1000 then
    local netstats = d2k.Client.get_netstats()

    if netstats ~= nil then
      netstats.upload = netstats.upload / 1000
      netstats.download = netstats.download / 1000
      netstats.packet_loss = netstats.packet_loss / 100
      netstats.packet_loss_jitter = netstats.packet_loss_jitter / 100

      self:set_text(''
        .. string.format('U/D:      %d KB/s | %d KB/s\n',
          netstats.upload, netstats.download
        )
        .. string.format('Ping:     %d ms\n', netstats.ping_average)
        .. string.format('Jitter:   %d\n', netstats.jitter_average)
        .. string.format('Commands: %d / %d\n',
          netstats.unsynchronized_commands, netstats.total_commands
        )
        .. string.format('PL:       %0.2f%% | %0.2f%%\n',
          netstats.packet_loss, netstats.packet_loss_jitter
        )
        .. string.format('Throttle: %d\n', netstats.throttle)
      )
    end

    self:set_last_time(current_time)
  end
end

netstats_widget:set_last_time(d2k.System.get_ticks())

netstats_widget:set_parent(d2k.hud)

local scoreboard_widget = ContainerWidget.ContainerWidget({
  name = 'scoreboard',
  z_index = 1,
  screen_reference_point = InputInterface.REFERENCE_POINT_CENTER,
  origin_reference_point = InputInterface.REFERENCE_POINT_CENTER,
  width = .7,
  height = .7,
  use_markup = true,
  fg_color = {1.0, 1.0, 1.0, 1.00},
  bg_color = {0.0, 0.0, 0.0, 0.65}
})

function scoreboard_widget:render()
  if self:is_active() then
    ContainerWidget.ContainerWidget.render(self)
  end
end

function scoreboard_widget:handle_event(event)
  if not self:is_active() then
    if event:is_key_press(d2k.Key.BACKSLASH) then
      self:activate()
      return true
    end
  elseif event:is_key_release(d2k.Key.BACKSLASH) then
    self:invalidate_render()
    self:deactivate()
    return true
  end

  return false
end

scoreboard_widget:set_parent(d2k.hud)

local scoreboard_title_widget = TextWidget.TextWidget({
  name = 'scoreboard_title',
  z_index = scoreboard_widget.z_index,
  top_margin = 12,
  bottom_margin = 12,
  left_margin = 12,
  right_margin = 12,
  screen_reference_point = InputInterface.REFERENCE_POINT_NORTH,
  origin_reference_point = InputInterface.REFERENCE_POINT_NORTH,
  width = 1,
  height = .18,
  use_markup = true,
  horizontal_alignment = TextWidget.ALIGN_CENTER,
  vertical_alignment = TextWidget.ALIGN_TOP,
  fg_color = {1.0, 1.0, 1.0, 1.00},
  bg_color = {0.0, 0.0, 0.0, 0.00}
})

scoreboard_title_widget:set_parent(scoreboard_widget)
scoreboard_title_widget:set_text('DEATHMATCH\nFrag Limit: 50\tTime Limit: 30')

local scoreboard_player_table_widget = TableWidget.TableWidget({
  name = 'scoreboard_player_table',
  z_index = scoreboard_widget.z_index,
  top_margin = 12,
  bottom_margin = 12,
  left_margin = 12,
  right_margin = 12,
  screen_reference_point = InputInterface.REFERENCE_POINT_NORTH,
  origin_reference_point = InputInterface.REFERENCE_POINT_NORTH,
  width = .7,
  height = .7,
  horizontal_alignment = TextWidget.ALIGN_CENTER,
  vertical_alignment = TextWidget.ALIGN_TOP,
  fg_color = {1.0, 1.0, 1.0, 1.00},
  bg_color = {0.0, 0.0, 0.0, 0.00}
})

-- Add header
scoreboard_player_table_widget:add_row()

-- Player name
scoreboard_player_table_widget:add_column()

-- Frags
scoreboard_player_table_widget:add_column()

-- Deaths
scoreboard_player_table_widget:add_column()

-- Ping
scoreboard_player_table_widget:add_column()

-- Time (minutes)
scoreboard_player_table_widget:add_column()

-- JKist3
scoreboard_player_table_widget:add_row()

-- Ladna
scoreboard_player_table_widget:add_row()

scoreboard_player_table_widget:get_cell(1, 2):set_text('Frags')
scoreboard_player_table_widget:get_cell(1, 3):set_text('Deaths')
scoreboard_player_table_widget:get_cell(1, 4):set_text('Ping')
scoreboard_player_table_widget:get_cell(1, 5):set_text('Time')

scoreboard_player_table_widget:get_cell(2, 1):set_text('JKist3')
scoreboard_player_table_widget:get_cell(2, 2):set_text('28')
scoreboard_player_table_widget:get_cell(2, 3):set_text('14')
scoreboard_player_table_widget:get_cell(2, 4):set_text('17')
scoreboard_player_table_widget:get_cell(2, 5):set_text('3')

scoreboard_player_table_widget:get_cell(3, 1):set_text('Ladna')
scoreboard_player_table_widget:get_cell(3, 2):set_text('14')
scoreboard_player_table_widget:get_cell(3, 3):set_text('28')
scoreboard_player_table_widget:get_cell(3, 4):set_text('59')
scoreboard_player_table_widget:get_cell(3, 5):set_text('2')

scoreboard_player_table_widget:set_cell_width(1.0 / 5.0)
scoreboard_player_table_widget:set_cell_height(1.0 / 9.0)

scoreboard_player_table_widget:set_column_horizontal_alignment(
  2, TextWidget.ALIGN_CENTER
)
scoreboard_player_table_widget:set_column_horizontal_alignment(
  3, TextWidget.ALIGN_CENTER
)
scoreboard_player_table_widget:set_column_horizontal_alignment(
  4, TextWidget.ALIGN_CENTER
)
scoreboard_player_table_widget:set_column_horizontal_alignment(
  5, TextWidget.ALIGN_CENTER
)

scoreboard_player_table_widget:set_parent(scoreboard_widget)
scoreboard_player_table_widget:set_y(scoreboard_title_widget:get_pixel_height() + 5)

-- vi: et ts=2 sw=2

