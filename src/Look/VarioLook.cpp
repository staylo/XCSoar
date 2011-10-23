/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "VarioLook.hpp"
#include "Screen/Layout.hpp"
#include "Units/Units.hpp"
#include "resource.h"

void
VarioLook::Initialise(bool _inverse, bool _colors,
                      const Font &_text_font, const Font &_value_font)
{
  inverse = _inverse;
  colors = _colors;

  if (inverse) {
    background_color = COLOR_BLACK;
    text_color = COLOR_WHITE;
    dimmed_text_color = COLOR_WHITE;
    sink_color = COLOR_WHITE;
    lift_color = COLOR_WHITE;
  } else {
    background_color = COLOR_WHITE;
    text_color = COLOR_BLACK;
    dimmed_text_color = COLOR_BLACK;
    sink_color = COLOR_BLACK;
    lift_color = COLOR_BLACK;
  }

  sink_brush.Set(sink_color);
  lift_brush.Set(lift_color);

  thick_background_pen.Set(Layout::Scale(5), background_color);
  thick_sink_pen.Set(Layout::Scale(5), sink_color);
  thick_lift_pen.Set(Layout::Scale(5), lift_color);

  background_bitmap.load(Units::GetUserVerticalSpeedUnit() == unKnots
                         ? IDB_VARIOSCALEC : IDB_VARIOSCALEA);
  background_x = inverse ? 58 : 0;

  climb_bitmap.load(inverse ? IDB_CLIMBSMALLINV : IDB_CLIMBSMALL);

  text_font = &_text_font;
  value_font = &_value_font;
}
