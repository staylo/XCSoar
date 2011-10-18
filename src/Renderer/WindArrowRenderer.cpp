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

#include "WindArrowRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Math/Angle.hpp"
#include "Math/Screen.hpp"
#include "NMEA/Derived.hpp"
#include "Units/Units.hpp"
#include "SettingsMap.hpp"

#include <tchar.h>
#include <cstdio>

void
WindArrowRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                        const SpeedVector wind, const RasterPoint pos,
                        const PixelRect rc, bool with_tail)
{
  canvas.select(Fonts::MapLabel);

  TCHAR sTmp[12];
  _stprintf(sTmp, _T("%i"), iround(Units::ToUserWindSpeed(wind.norm)));

  unsigned text_width = canvas.text_size(sTmp).cx / 2;
  unsigned text_height = canvas.text_size(sTmp).cy;

  canvas.select(Graphics::hpWind);
  canvas.select(Graphics::hbWind);

  int wmag = iround(4 * wind.norm);

  int kx = text_width / Layout::FastScale(1) / 2;
  int ky = text_height / 2; // / Layout::FastScale(1) / 2;

  RasterPoint arrow[7] = {
      { 0, -16 },
      { -4, -16 },
      { 0, -16 },
      { 4, -16 },
      { 0, -16 },
      { 6 + kx, -20 },
      { -6 - kx, -20 }
  };

  for (int i = 1; i < 4; i++)
    arrow[i].y -= wmag;

  PolygonRotateShift(arrow, 7, pos.x, pos.y, wind.bearing - screen_angle);

  canvas.polygon(arrow, 5);

  if (with_tail) {
    RasterPoint tail[2] = {
      { 0, Layout::FastScale(-20) },
      { 0, Layout::FastScale(-26 - min(20, wmag) * 3) },
    };

    PolygonRotateShift(tail, 2, pos.x, pos.y, wind.bearing - screen_angle);

    // optionally draw dashed line
    canvas.select(Graphics::hpWindTail);
    canvas.line(tail[0], tail[1]);
  }

  canvas.set_text_color(COLOR_BLACK);
  canvas.set_background_color(COLOR_WHITE);

  TextInBoxMode style;
  style.Align = Center;
  style.Mode = RoundedBlack;

  //_stprintf(sTmp, _T("%i"), text_width);

  TextInBox(canvas, sTmp, arrow[2].x, arrow[2].y - ky, style, rc);
  //if (arrow[5].y >= arrow[6].y)
  //  TextInBox(canvas, sTmp, arrow[5].x - kx, arrow[5].y, style, rc);
  //else
  //  TextInBox(canvas, sTmp, arrow[6].x - kx, arrow[6].y, style, rc);
}

void
WindArrowRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                        const RasterPoint pos, const PixelRect rc,
                        const DerivedInfo &calculated,
                        const SETTINGS_MAP &settings)
{
  if (!calculated.wind_available)
    return;

  // don't bother drawing it if not significant
  if (calculated.wind.norm < fixed_one)
    return;

  WindArrowRenderer::Draw(canvas, screen_angle, calculated.wind, pos, rc,
                          settings.WindArrowStyle == 1);
}
