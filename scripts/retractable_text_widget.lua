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

local lgi = require('lgi')
local Cairo = lgi.cairo
local GLib = lgi.GLib
local Pango = lgi.Pango
local PangoCairo = lgi.PangoCairo
local TextWidget = require('text_widget')

local RetractableTextWidget = TextWidget.TextWidget:new()

RetractableTextWidget.RETRACT_NONE = 0
RetractableTextWidget.RETRACT_UP   = 1
RetractableTextWidget.RETRACT_DOWN = 2

RetractableTextWidget.RETRACTION_TIME    = 2000
RetractableTextWidget.RETRACTION_TIMEOUT = 2000

--
-- [TODO]
--
-- - Throw an error during initialization if vertical_align isn't ALIGN_TOP or
--   ALIGN_BOTTOM.
-- - Rename to RetractableTextWidget
-- - Only override methods that are actually different from TextWidget
-- - Pre-set fields properly, and only either new fields or fields whose
--   default value differs from TextWidget's default values
-- - On resize, we need to set the last retraction target to the last line
--

function RetractableTextWidget:new(rtw)
  rtw = rtw or {}

  rtw.retractable = rtw.retractable or RetractableTextWidget.RETRACT_NONE
  rtw.retraction_time = rtw.retraction_time or
                        RetractableTextWidget.RETRACTION_TIME
  rtw.retraction_timeout = rtw.retraction_timeout or
                           RetractableTextWidget.RETRACTION_TIMEOUT

  rtw.retracting = false
  rtw.retracting_line_number = -1
  rtw.last_retracted_line_number = 0
  rtw.retraction_target = 0
  rtw.retraction_start_time = 0

  setmetatable(rtw, self)
  self.__index = self

  rtw:build_layout()

  if rtw.word_wrap then
    rtw:set_word_wrap(rtw.word_wrap)
  else
    rtw:set_word_wrap(TextWidget.WRAP_NONE)
  end

  if rtw.horizontal_alignment then
    rtw:set_horizontal_alignment(rtw.horizontal_alignment)
  else
    rtw:set_horizontal_alignment(TextWidget.ALIGN_LEFT)
  end

  if rtw.vertical_alignment then
    rtw:set_vertical_alignment(rtw.vertical_alignment)
  else
    rtw:set_vertical_alignment(TextWidget.ALIGN_TOP)
  end

  if rtw.ellipsize then
    rtw:set_ellipsize(rtw.ellipsize)
  else
    rtw:set_ellipsize(TextWidget.ELLIPSIZE_NONE)
  end

  return rtw
end

function RetractableTextWidget:get_retractable()
  if self.retractable == RetractableTextWidget.RETRACT_NONE then
    return false
  end

  return self.retractable
end

function RetractableTextWidget:set_retractable(retractable)
  if retractable == false then
    self.retractable = RetractableTextWidget.RETRACT_NONE
  else
    self.retractable = retractable
  end
end

function RetractableTextWidget:get_retraction_time()
  return self.retraction_time
end

function RetractableTextWidget:set_retraction_time(retraction_time)
  self.retraction_time = retraction_time
end

function RetractableTextWidget:get_retraction_timeout()
  return self.retraction_timeout
end

function RetractableTextWidget:set_retraction_timeout(retraction_timeout)
  self.retraction_timeout = retraction_timeout
end

function RetractableTextWidget:get_retracting()
  return self.retracting
end

function RetractableTextWidget:set_retracting(retracting)
  self.retracting = retracting
end

function RetractableTextWidget:get_retracting_line_number()
  return self.retracting_line_number
end

function RetractableTextWidget:set_retracting_line_number(
  retracting_line_number
)
  self.retracting_line_number = retracting_line_number
end

function RetractableTextWidget:get_last_retracted_line_number()
  return self.last_retracted_line_number
end

function RetractableTextWidget:set_last_retracted_line_number(
  last_retracted_line_number
)
  self.last_retracted_line_number = last_retracted_line_number
end

function RetractableTextWidget:get_retraction_target()
  return self.retraction_target
end

function RetractableTextWidget:set_retraction_target(retraction_target)
  self.retraction_target = retraction_target
end

function RetractableTextWidget:get_retraction_start_time()
  return self.retraction_start_time
end

function RetractableTextWidget:set_retraction_start_time(retraction_start_time)
  self.retraction_start_time = retraction_start_time
end

function RetractableTextWidget:get_text_context()
  return self.text_context
end

function RetractableTextWidget:get_layout_line_count()
  if #self:get_text() == 0 then
    return 0
  end

  return self:get_layout():get_line_count()
end

function RetractableTextWidget:get_min_max_line_numbers(line_count)
  local line_height = self:get_line_height()
  local last_retracted_line_number = self:get_last_retracted_line_number()
  local line_count = line_count or self:get_layout_line_count()
  local min_line = 1
  local max_line = line_count
  
  if line_height == 0 or line_count <= line_height then
    if last_retracted_line_number == 0 then
      return min_line, max_line
    end

    min_line = last_retracted_line_number + 1

    if min_line > line_count then
      return 0, 0
    end

    return min_line, max_line
  end

  min_line = line_count - (line_height - 1)

  if min_line <= last_retracted_line_number then
    min_line = last_retracted_line_number + 1
  end

  max_line = math.min(min_line + (line_height - 1), line_count)

  return min_line, max_line
end

function RetractableTextWidget:update_layout_if_needed()
  if not self.needs_updating then
    return
  end

  local text = self:get_text()
  local layout = self:get_layout()

  if self:get_strip_ending_newline() then
    while true do
      local text_length = #text

      if text:sub(text_length, text_length) ~= '\n' then
        break
      end

      text = text:sub(1, text_length - 1)
    end
  end

  if self:get_use_markup() then
    self:get_layout():set_markup(text, -1)
  else
    self:get_layout():set_text(text, -1)
  end

  PangoCairo.update_context(
    d2k.overlay.render_context, d2k.overlay.text_context
  )
  PangoCairo.update_layout(d2k.overlay.render_context, layout)
  self:check_offsets()

  if self.get_external_text then
    local line_count = self:get_layout_line_count()
    local start_line, end_line = self:get_min_max_line_numbers(line_count)
    local lines = self:get_visible_line_dimensions(start_line, end_line)
    local visible_line_count = #lines

    if visible_line_count == 0 then
      self:set_height_in_pixels(0)
      return
    end

    self:set_height_in_pixels(
      self:get_top_margin() +
      lines[1].y +
      lines[visible_line_count].y +
      lines[visible_line_count].height +
      self:get_bottom_margin()
    )
  end

  self.needs_updating = false
end

function RetractableTextWidget:get_visible_lines(start_line, end_line)
  local line_height = self:get_line_height()
  local layout = self:get_layout()
  local line_number = 1
  local lines = {}

  if start_line == 0 or end_line == 0 then
    return lines
  end

  local line_count = self:get_layout_line_count()
  local iter = layout:get_iter()

  if start_line == nil or end_line == nil then
    start_line, end_line = self:get_min_max_line_numbers(line_count)
  end

  while line_number < start_line do
    iter:next_line()
    line_number = line_number + 1
  end

  while line_number <= end_line do
    local ink_rect, logical_rect = iter:get_line_extents()

    table.insert(lines, {
      number     = line_number,
      x          = logical_rect.x      / Pango.SCALE,
      y          = logical_rect.y      / Pango.SCALE,
      width      = logical_rect.width  / Pango.SCALE,
      height     = logical_rect.height / Pango.SCALE,
      baseline   = iter:get_baseline() / Pango.SCALE,
      pango_line = iter:get_line_readonly()
    })

    if not iter:next_line() then
      break
    end

    line_number = line_number + 1
  end

  return lines
end

function RetractableTextWidget:get_visible_line_dimensions(
  start_line, end_line
)
  local line_height = self:get_line_height()
  local layout = self:get_layout()
  local line_number = 1
  local lines = {}

  if start_line == 0 or end_line == 0 then
    return lines
  end

  local line_count = self:get_layout_line_count()
  local iter = layout:get_iter()

  if start_line == nil or end_line == nil then
    start_line, end_line = self:get_min_max_line_numbers(line_count)
  end

  while line_number < start_line do
    iter:next_line()
    line_number = line_number + 1
  end

  while line_number <= end_line do
    local ink_rect, logical_rect = iter:get_line_extents()

    table.insert(lines, {
      number = line_number,
      x      = logical_rect.x      / Pango.SCALE,
      y      = logical_rect.y      / Pango.SCALE,
      width  = logical_rect.width  / Pango.SCALE,
      height = logical_rect.height / Pango.SCALE
    })

    if not iter:next_line() then
      break
    end

    line_number = line_number + 1
  end

  return lines
end

function RetractableTextWidget:reset()
  local layout = self:get_layout()
  local layout_width, layout_height = layout:get_pixel_size()
  local line_count = self:get_layout_line_count()

  self:set_retracting(false)
  if self:get_retracting_line_number() == -1 then
    self:set_retracting_line_number(-1)
    self:set_last_retracted_line_number(0)
    self:set_vertical_offset(0)
  else
    self:set_retracting_line_number(0)
    self:set_last_retracted_line_number(line_count)
    self:set_vertical_offset(layout_height)
  end
  self:set_retraction_target(0)
  self:set_retraction_start_time(0)
end

function RetractableTextWidget:tick()
  local current_ms = d2k.System.get_ticks()

  if self.get_external_text and self.external_text_updated() then
    self.text = self.get_external_text()
    self.needs_updating = true
    self.clear_external_text_updated()
  end

  if not self:get_retractable() then
    return
  end

  if #self.text == 0 then
    return
  end

  local start_line, end_line = self:get_min_max_line_numbers()

  if start_line == 0 or end_line == 0 then
    return
  end

  local visible_lines = self:get_visible_line_dimensions(start_line, end_line)
  local visible_line_count = #visible_lines

  if visible_line_count == 0 then
    return
  end

  local layout_width, layout_height = self:get_layout():get_pixel_size()

  self:set_height_in_pixels(
    (layout_height - visible_lines[1].y) +
    self:get_top_margin() +
    self:get_bottom_margin()
  )

  if self:get_retracting() then
    local retracting_line_number = self:get_retracting_line_number()

    if retracting_line_number < start_line or
       retracting_line_number > end_line or
       not self:retract(current_ms, visible_lines[1].y) then
      self:set_vertical_offset(self:get_retraction_target())
      self:set_retracting(false)
      self:set_last_retracted_line_number(retracting_line_number)
      self:set_retracting_line_number(0)
      self:set_retraction_target(0)
      self:set_retraction_start_time(0)
    end

    return
  end

  local retraction_start_time = self:get_retraction_start_time()

  if retraction_start_time == 0 then
    self:set_retraction_start_time(current_ms + self:get_retraction_timeout())
    return
  end

  if current_ms > self:get_retraction_time() then
    self:set_retracting_line_number(visible_lines[1].number)
    if visible_line_count > 1 then
      self:set_retraction_target(visible_lines[2].y)
    else
      self:set_retraction_target(layout_height)
    end
    self:set_retracting(true)
  end
end

function RetractableTextWidget:retract(current_ms, top)
  local current_ms = current_ms or d2k.System.get_ticks()
  local retraction_start_time = self:get_retraction_start_time()
  local retraction_ms_elapsed = current_ms - retraction_start_time
  local ms_retracted = retraction_ms_elapsed - self:get_retraction_timeout()

  if ms_retracted <= 0 then
    return true
  end

  local retraction_time = self:get_retraction_time()
  local height_fraction = ms_retracted / retraction_time
  local target = self:get_retraction_target()
  local retraction_distance = target - top
  local y_delta = (ms_retracted / retraction_time) * retraction_distance

  if y_delta < retraction_distance then
    self:set_vertical_offset(top + y_delta)
    return true
  end

  return false
end

function RetractableTextWidget:draw()
  local cr = d2k.overlay.render_context
  local line_count = 0
  local fg_color = self:get_fg_color()
  local bg_color = self:get_bg_color()
  local outline_color = self:get_outline_color()
  local current_render_context = self:get_current_render_context()

  if not current_render_context or current_render_context ~= cr then
    self:build_layout()
  end

  self:update_layout_if_needed()

  local lw, lh = self:get_layout():get_pixel_size()

  line_count = self:get_layout_line_count()

  cr:save()

  cr:set_operator(Cairo.Operator.OVER)

  cr:reset_clip()
  cr:new_path()
  cr:rectangle(
    self:get_x(),
    self:get_y(),
    self:get_width_in_pixels(),
    self:get_height_in_pixels()
  )
  cr:clip()

  cr:set_source_rgba(bg_color[1], bg_color[2], bg_color[3], bg_color[4])
  cr:paint()

  if line_count <= 0 then
    return
  end

  cr:set_source_rgba(fg_color[1], fg_color[2], fg_color[3], fg_color[4])

  local lx = self:get_x() + self:get_left_margin()
  local ly = self:get_y() + self:get_top_margin()
  local text_width = self:get_width_in_pixels() - (
    self:get_left_margin() + self:get_right_margin()
  )
  local text_height = self:get_height_in_pixels() - (
    self:get_top_margin() + self:get_bottom_margin()
  )
  local layout_width, layout_height = self:get_layout():get_pixel_size()
  local layout_ink_extents, layout_logical_extents =
    self:get_layout():get_pixel_extents()
  local retractable = self:get_retractable()

  if self.vertical_alignment == TextWidget.ALIGN_BOTTOM then
    ly = ly + text_height - layout_height
  end

  if self.horizontal_alignment == TextWidget.ALIGN_CENTER then
    lx = (text_width / 2) - (layout_width / 2)
  elseif self.horizontal_alignment == TextWidget.ALIGN_RIGHT then
    lx = lx + text_width - layout_width
  end

  if retractable or layout_height > text_height then
    ly = ly - self:get_vertical_offset()
  end

  local min_line, max_line = self:get_min_max_line_numbers(line_count)
  local visible_lines = self:get_visible_lines(min_line, max_line)
  local start_y_offset = 0
  local rendered_at_least_one_line = false
  local line_height = self:get_line_height()

  for i, line in ipairs(visible_lines) do
    local line_start_x = line.x + lx
    local line_start_y = line.y + ly

    if line_height > 0 and i == 1 and line.y > 0 then
      start_y_offset = -line.y
    end

    cr:move_to(line_start_x, line.baseline + ly + start_y_offset)

    if self:get_outline_text() then
      cr:save()

      PangoCairo.layout_line_path(cr, line.pango_line)

      cr:save()
      cr:set_line_width(self:get_outline_width())
      cr:set_source_rgba(
        outline_color[1],
        outline_color[2],
        outline_color[3],
        outline_color[4]
      )
      cr:stroke_preserve()
      cr:restore()

      cr:set_source_rgba(fg_color[1], fg_color[2], fg_color[3], fg_color[4])
      cr:fill_preserve()

      cr:restore()
    else
      PangoCairo.show_layout_line(cr, line.pango_line)
    end
  end

  cr:restore()
end

function RetractableTextWidget:check_offsets()
end

return {
  RetractableTextWidget = RetractableTextWidget,
  RETRACT_NONE          = RetractableTextWidget.RETRACT_NONE,
  RETRACT_UP            = RetractableTextWidget.RETRACT_UP,
  RETRACT_DOWN          = RetractableTextWidget.RETRACT_DOWN,
  RETRACTION_TIME       = RetractableTextWidget.RETRACTION_TIME,
  RETRACTION_TIMEOUT    = RetractableTextWidget.RETRACTION_TIMEOUT
}

-- vi: et ts=2 sw=2
