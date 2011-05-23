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

#include "MapWindow.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/TextInBox.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Layout.hpp"
#include "Math/Screen.hpp"
#include "Appearance.hpp"
#include "Units/Units.hpp"

#include <stdlib.h>
#include <stdio.h>


void
MapWindow::DrawWind(Canvas &canvas, const RasterPoint &Start,
                    const PixelRect &rc) const
{
  if (SettingsMap().EnablePan && !SettingsMap().TargetPan)
    return;

  TCHAR sTmp[12];
  static PixelSize tsize = { 0, 0 };

  if (!Calculated().wind_available)
    return;

  const SpeedVector wind = Calculated().wind;

  if (wind.norm < fixed_one)
    // JMW don't bother drawing it if not significant
    return;

  if (tsize.cx == 0) {
    canvas.select(Fonts::MapBold);
    tsize = canvas.text_size(_T("99"));
    tsize.cx = tsize.cx / 2;
  }

  Angle angle = (wind.bearing - render_projection.GetScreenAngle()).as_bearing();

  int kx = tsize.cx / Layout::FastScale(1) / 2;

  RasterPoint value_location;

  const WindArrowStyle_t& style = SettingsMap().WindArrowStyle;

  if ((style == waTail) || (style == waArrow)) {

    int wmag = iround(4 * wind.norm);

    RasterPoint Arrow[7] = {
      { 0, -20 },
      { -6, -26 },
      { 0, -20 },
      { 6, -26 },
      { 0, -20 },
      { 8 + kx, -24 },
      { -8 - kx, -24 }
    };

    for (int i = 1; i < 4; i++)
      Arrow[i].y -= wmag;

    PolygonRotateShift(Arrow, 7, Start.x, Start.y, angle);
    canvas.select(Graphics::hpWind);
    canvas.select(Graphics::hbWind);
    canvas.polygon(Arrow, 5);

    if (SettingsMap().WindArrowStyle == waTail) {
      // optionally draw dashed line

      RasterPoint Tail[2] = {
        { 0, Layout::FastScale(-20) },
        { 0, Layout::FastScale(-26 - min(20, wmag) * 3) },
      };

      PolygonRotateShift(Tail, 2, Start.x, Start.y, angle);

      Pen dash_pen(Pen::DASH, 1, COLOR_BLACK);
      canvas.select(dash_pen);
      canvas.line(Tail[0], Tail[1]);
    }
    if (Arrow[5].y >= Arrow[6].y) {
      value_location = Arrow[5];
    } else {
      value_location = Arrow[6];
    }
  } else if (style == waStriped) {

    int wmag = iround(3 * wind.norm);

    RasterPoint Arrow[5] = {
      { 0, -20 },
      { -wmag/5-4, -26 -wmag },
      { wmag/5+4, -26 -wmag},
      { 6 + kx, -24 },
      { -6 - kx, -24 }
    };

    PolygonRotateShift(Arrow, 5, Start.x, Start.y, angle);

    // draw background triangle
    canvas.null_pen();
    canvas.select(Graphics::hbWind);
    canvas.polygon(Arrow, 3);

    // draw background triangle
    canvas.white_pen();
    canvas.hollow_brush();
    for (int i=1; i+1<wmag; i+= 4) {
      RasterPoint Line[2] = {{ -i/5-4, -26-i }, { i/5+4, -26-i}};
      PolygonRotateShift(Line, 2, Start.x, Start.y, angle);
      canvas.line(Line[0], Line[1]);
    }

    // draw outline
    canvas.select(Graphics::hpWind);
    canvas.polygon(Arrow, 3);

    if (Arrow[3].y >= Arrow[4].y) {
      value_location = Arrow[3];
    } else {
      value_location = Arrow[4];
    }

  } else assert(0);

  // draw value
  _stprintf(sTmp, _T("%i"), iround(Units::ToUserWindSpeed(wind.norm)));

  canvas.set_text_color(COLOR_BLACK);

  TextInBoxMode_t TextInBoxMode;
  TextInBoxMode.Align = Center;
  TextInBoxMode.Mode = Outlined;
  TextInBox(canvas, sTmp, value_location.x - kx, value_location.y, TextInBoxMode, rc);
}

void
MapWindow::DrawCompass(Canvas &canvas, const PixelRect &rc) const
{
  if (!SettingsMap().NorthArrow)
    return;

  RasterPoint Start;
  Start.y = IBLSCALE(19) + rc.top;
  Start.x = rc.right - IBLSCALE(19);

  RasterPoint Arrow[5] = { { 0, -13 }, { -6, 10 }, { 0, 8 }, { 6, 10 }, { 0, -13 } };

  // North arrow
  PolygonRotateShift(Arrow, 5, Start.x, Start.y,
                     Angle::native(fixed_zero) - render_projection.GetScreenAngle());

  canvas.select(Graphics::hpCompass1);
  canvas.select(Graphics::hbCompass1);
  canvas.polygon(Arrow, 3);
  canvas.select(Graphics::hpCompass2);
  canvas.select(Graphics::hbCompass2);
  canvas.polygon(Arrow+2, 3);
}

void
MapWindow::DrawBestCruiseTrack(Canvas &canvas, const RasterPoint aircraft_pos) const
{
  if (!Basic().LocationAvailable ||
      !Calculated().task_stats.task_valid ||
      !Calculated().task_stats.current_leg.solution_remaining.defined() ||
      Calculated().task_stats.current_leg.solution_remaining.Vector.Distance
      < fixed(0.010))
    return;

  if (Calculated().TurnMode == CLIMB)
    return;

  canvas.select(Graphics::hpBestCruiseTrack);
  canvas.select(Graphics::hbBestCruiseTrack);

  const Angle angle = Calculated().task_stats.current_leg.solution_remaining.CruiseTrackBearing
                    - render_projection.GetScreenAngle();

  RasterPoint Arrow[] = { { -1, -40 }, { -1, -62 }, { -6, -62 }, {  0, -70 },
                    {  6, -62 }, {  1, -62 }, {  1, -40 }, { -1, -40 } };

  PolygonRotateShift(Arrow, sizeof(Arrow) / sizeof(Arrow[0]),
                     aircraft_pos.x, aircraft_pos.y, angle);

  canvas.polygon(Arrow, sizeof(Arrow) / sizeof(Arrow[0]));
}

void
MapWindow::DrawTrackBearing(Canvas &canvas, const RasterPoint aircraft_pos) const
{
  if (!Basic().LocationAvailable ||
      SettingsMap().DisplayTrackBearing == dtbOff ||
      Calculated().Circling)
    return;

  if (SettingsMap().DisplayTrackBearing == dtbAuto &&
      (Basic().track - Calculated().Heading).as_delta().magnitude_degrees() < fixed(5))
    return;

  RasterPoint end;
  fixed x,y;
  (Basic().track - render_projection.GetScreenAngle()).sin_cos(x, y);
  end.x = aircraft_pos.x + iround(x * fixed_int_constant(400));
  end.y = aircraft_pos.y - iround(y * fixed_int_constant(400));

  canvas.select(Graphics::hpTrackBearingLine);
  canvas.line(aircraft_pos, end);
}
