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

#include "Gauge/GaugeVario.hpp"
#include "LogFile.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Math/FastRotation.hpp"
#include "Appearance.hpp"
#include "resource.h"
#include "LogFile.hpp"
#include "Profile/Profile.hpp"

#include <assert.h>
#include <stdio.h>

#include <algorithm>

using std::min;
using std::max;

#define DeltaVstep fixed_four
#define DeltaVlimit fixed(16)
#define TextBug _T("Bug")
#define TextBal _T("Bal")

GaugeVario::GaugeVario(ContainerWindow &parent,
                       int left, int top, unsigned width, unsigned height,
                       const WindowStyle style) :
   ShowAvgText(false),
   ShowMc(false),
   ShowSpeedToFly(false),
   ShowBallast(false),
   ShowBugs(false),
   ShowGross(true),
   ShowAveNeedle(false),
   nlength0(Layout::Scale(16)),
   nlength1(Layout::Scale(4)),
   nlength2(Layout::Scale(6)),
   nlength3(Layout::Scale(12)),
   nwidth(Layout::Scale(3)),
   dirty(true), layout_initialised(false), needle_initialised(false),
   ballast_initialised(false), bugs_initialised(false)
{
  Profile::Get(szProfileAppGaugeVarioSpeedToFly, ShowSpeedToFly);
  Profile::Get(szProfileAppGaugeVarioAvgText, ShowAvgText);
  Profile::Get(szProfileAppGaugeVarioMc, ShowMc);
  Profile::Get(szProfileAppGaugeVarioBugs, ShowBugs);
  Profile::Get(szProfileAppGaugeVarioBallast, ShowBallast);
  Profile::Get(szProfileAppGaugeVarioGross, ShowGross);
  Profile::Get(szProfileAppAveNeedle, ShowAveNeedle);

  diValueTop.InitDone = false;
  diValueMiddle.InitDone = false;
  diValueBottom.InitDone = false;
  diLabelTop.InitDone = false;
  diLabelMiddle.InitDone = false;
  diLabelBottom.InitDone = false;

  LogStartUp(_T("Create Vario"));

  set(parent, left, top, width, height, style);

  // load vario scale
  hDrawBitMap.load(Layout::ScaleEnabled()?
                   (Appearance.InverseInfoBox ?
                   IDB_VARIOSCALEA_HD : IDB_VARIOSCALEC_HD):
                   (Appearance.InverseInfoBox ?
                    IDB_VARIOSCALEA : IDB_VARIOSCALEC));

  Color thesinkColor;
  Color theliftColor;

  if (Appearance.InverseInfoBox) {
    thesinkColor = Graphics::inv_sinkColor;
    theliftColor = Graphics::inv_liftColor;
    colSurround = dialog_prefs.infobox_dark_shade;
    colText = COLOR_WHITE;
    colTextBackgnd = COLOR_BLACK;
    colTextLabel = dialog_prefs.infobox_dark_text;
    hBitmapClimb.load(IDB_CLIMBSMALLINV);
  } else {
    thesinkColor = Graphics::sinkColor;
    theliftColor = Graphics::liftColor;
    colSurround = dialog_prefs.infobox_light_shade;
    colText = COLOR_BLACK;
    colTextBackgnd = COLOR_WHITE;
    colTextLabel = dialog_prefs.infobox_light_text;
    hBitmapClimb.load(IDB_CLIMBSMALL);
  }
  needlePen.set(1, colTextLabel);
  needleBrush.set(colText);
  surroundBrush.set(colSurround);
  sinkBrush.set(thesinkColor);
  liftBrush.set(theliftColor);

  unit_symbol = GetUnitSymbol(Units::Current.VerticalSpeedUnit);

  if (Units::Current.VerticalSpeedUnit == unKnots) {
    degrees_per_unit = fixed(GAUGEVARIOSWEEP)*Units::ToUserVSpeed(fixed_half)/ GAUGEVARIORANGE;
    // 90 * 0.97 / 5
  } else {
    degrees_per_unit = fixed(GAUGEVARIOSWEEP) / GAUGEVARIORANGE;
  }

  xoffset = get_right();
  yoffset = get_height() / 2 + get_top();

  hide();
}

void
GaugeVario::on_paint_buffer(Canvas &canvas)
{
  if (!is_persistent() || !layout_initialised) {
    unsigned ValueHeight = 4 + Fonts::CDI.get_capital_height()
                    + Fonts::Title.get_capital_height();

    orgMiddle.y = yoffset - ValueHeight / 2;
    orgMiddle.x = get_right();
    orgTop.y = orgMiddle.y - ValueHeight;
    orgTop.x = get_right();
    orgBottom.y = orgMiddle.y + ValueHeight;
    orgBottom.x = get_right();

    // copy scale bitmap to memory DC
    canvas.clear_white();
    canvas.stretch(0, 0, canvas.get_width(), canvas.get_height(),
                   hDrawBitMap, 0, 0, hDrawBitMap.get_size().cx,
                   hDrawBitMap.get_size().cy);
    layout_initialised = true;
  }

  if (ShowAvgText) {
    // JMW averager now displays netto average if not circling
    if (!Calculated().Circling) {
      RenderValue(canvas, orgTop.x, orgTop.y, &diValueTop, &diLabelTop,
                  Units::ToUserVSpeed(Calculated().NettoAverage30s),
                  _T("NetAvg"));
    } else {
      RenderValue(canvas, orgTop.x, orgTop.y, &diValueTop, &diLabelTop,
                  Units::ToUserVSpeed(Calculated().Average30s), _T("Avg"));
    }
  }

  if (ShowMc) {
    fixed mc = Units::ToUserVSpeed(Calculated().common_stats.current_mc);
    RenderValue(canvas, orgBottom.x, orgBottom.y, &diValueBottom, &diLabelBottom,
                mc, SettingsComputer().auto_mc ? _T("Auto MC") : _T("MC"));
  }

  if (ShowSpeedToFly)
    RenderSpeedToFly(canvas, get_right() - 11, get_height() / 2);
  else
    RenderClimb(canvas);

  if (ShowBallast)
    RenderBallast(canvas);

  if (ShowBugs)
    RenderBugs(canvas);

  dirty = false;
  int ival, sval, ival_av = 0;
  static int vval_last = 0;
  static int sval_last = 0;
  static int ival_last = 0;

  fixed vval = Calculated().BruttoVario;
  ival = ValueToNeedlePos(fixed(vval));
  sval = ValueToNeedlePos(Calculated().GliderSinkRate);
  if (ShowAveNeedle) {
    if (!Calculated().Circling)
      ival_av = ValueToNeedlePos(Calculated().NettoAverage30s);
    else
      ival_av = ValueToNeedlePos(Calculated().Average30s);
  }

  // clear items first

  if (ShowAveNeedle) {
    if (!is_persistent() || ival_av != ival_last)
      RenderNeedle(canvas, ival_last, true, true);

    ival_last = ival_av;
  }

  if (!is_persistent() || (sval != sval_last) || (ival != vval_last))
    RenderVarioLine(canvas, vval_last, sval_last, true);

  sval_last = sval;

  if (!is_persistent() || ival != vval_last)
    RenderNeedle(canvas, vval_last, false, true);

  vval_last = ival;

  // now draw items
  RenderVarioLine(canvas, ival, sval, false);
  if (ShowAveNeedle)
    RenderNeedle(canvas, ival_av, true, false);

  RenderNeedle(canvas, ival, false, false);

  if (ShowGross) {
    fixed vvaldisplay = min(fixed(99.9), max(fixed(-99.9), Units::ToUserVSpeed(vval)));

    RenderValue(canvas, orgMiddle.x, orgMiddle.y,
                &diValueMiddle, &diLabelMiddle,
                vvaldisplay,
                _T("Gross"));
  }

  RenderZero(canvas);
}

void
GaugeVario::MakePolygon(const int i)
{
  RasterPoint *bit = getPolygon(i);
  const int width = get_width()*2;
  const int height = get_height();

  const FastRotation r = FastRotation(Angle::degrees(fixed(i)));
  FastRotation::Pair p;

  p = r.Rotate(fixed(-xoffset + nlength0), fixed(nwidth));
  bit[0].x = (int)p.first + xoffset;
  bit[0].y = (int)(p.second * height / width) + yoffset;

  p = r.Rotate(fixed(-xoffset + nlength0), fixed(-nwidth));
  bit[2].x = (int)p.first + xoffset;
  bit[2].y = (int)(p.second * height / width) + yoffset;

  p = r.Rotate(fixed(-xoffset + nlength1), fixed_zero);
  bit[1].x = (int)p.first + xoffset;
  bit[1].y = (int)(p.second * height / width) + yoffset;

  p = r.Rotate(fixed(-xoffset + nlength3), fixed_zero);
  bit[3].x = (int)p.first + xoffset;
  bit[3].y = (int)(p.second * height / width) + yoffset;

  p = r.Rotate(fixed(-xoffset + nlength2), fixed_zero);
  bit[4].x = (int)p.first + xoffset;
  bit[4].y = (int)(p.second * height / width) + yoffset;
}

RasterPoint *
GaugeVario::getPolygon(int i)
{
  return polys + (i + gmax) * 5;
}

void
GaugeVario::MakeAllPolygons()
{
  if (polys)
    for (int i = -gmax; i <= gmax; i++)
      MakePolygon(i);
}

void
GaugeVario::RenderClimb(Canvas &canvas)
{
  int x = get_right() - Layout::Scale(14);
  int y = get_bottom() - Layout::Scale(24);

  if (!dirty)
    return;

  if (Basic().SwitchState.FlightMode == SWITCH_INFO::MODE_CIRCLING)
    canvas.scale_copy(x, y, hBitmapClimb, 12, 0, 12, 12);
  else if (is_persistent())
    canvas.fill_rectangle(x, y, x + Layout::Scale(12), y + Layout::Scale(12),
                          Appearance.InverseInfoBox
                          ? COLOR_BLACK : COLOR_WHITE);
}

void
GaugeVario::RenderZero(Canvas &canvas)
{
  if (Appearance.InverseInfoBox)
    canvas.white_pen();
  else
    canvas.black_pen();

  canvas.line(0, yoffset, Layout::Scale(17), yoffset);
  canvas.line(0, yoffset + 1, Layout::Scale(17), yoffset + 1);
}

int
GaugeVario::ValueToNeedlePos(fixed Value)
{
  int i;

  if (!needle_initialised){
    MakeAllPolygons();
    needle_initialised = true;
  }
  i = iround(Value * degrees_per_unit);
  i = min((int)gmax, max(-gmax, i));
  return i;
}

void
GaugeVario::RenderVarioLine(Canvas &canvas, int i, int sink, bool clear)
{
  dirty = true;
  if (i == sink)
    return;

  if (clear) {
    if (Appearance.InverseInfoBox) {
      canvas.black_brush();
      canvas.black_pen();
    } else {
      canvas.white_brush();
      canvas.white_pen();
    }
  } else {
    if (i>sink) {
      canvas.select(liftBrush);
    } else {
      canvas.select(sinkBrush);
    }
    if (Appearance.InverseInfoBox) {
      canvas.black_pen();
    } else {
      canvas.white_pen();
    }
  }

  RasterPoint p[4];
  const int i_min = (i>sink)? sink: i;
  const int i_max = (i>sink)? i: sink;
  const RasterPoint* p_last = getPolygon(i_min);
  p[0] = p_last[3];
  p[1] = p_last[4];
  for (int j= i_min+1; j<= i_max; ++j) {
    if ((j % 9 == 0) || (j== i_max)) {
      p_last = getPolygon(j);
      p[2] = p_last[4];
      p[3] = p_last[3];
      canvas.polygon(p, 4);
      p[0] = p[3];
      p[1] = p[2];
    }
  }
}

void
GaugeVario::RenderNeedle(Canvas &canvas, int i, bool average, bool clear)
{
  dirty = true;

  // legacy behaviour
  if (clear ^ Appearance.InverseInfoBox) {
    canvas.white_brush();
    canvas.white_pen();
  } else {
    canvas.black_brush();
    canvas.black_pen();
  }
  if (!clear && !average) {
    canvas.select(needlePen);
    canvas.select(needleBrush);
  }

  if (average)
    canvas.polyline(getPolygon(i), 3);
  else
    canvas.polygon(getPolygon(i), 3);
}

// TODO code: Optimise vario rendering, this is slow
void
GaugeVario::RenderValue(Canvas &canvas, int x, int y, DrawInfo_t *diValue,
                        DrawInfo_t *diLabel, fixed Value, const TCHAR *Label)
{
  PixelSize tsize;

#ifndef FIXED_MATH
  Value = (double)iround(Value * 10) / 10; // prevent the -0.0 case
#endif

  if (!diValue->InitDone) {

    diValue->recBkg.right = x - Layout::Scale(5);
    diValue->recBkg.top = y + Layout::Scale(3)
                            + Fonts::Title.get_capital_height();

    diValue->recBkg.left = diValue->recBkg.right;
    // update back rect with max label size
    diValue->recBkg.bottom = diValue->recBkg.top
                             + Fonts::CDI.get_capital_height();

    diValue->orgText.x = diValue->recBkg.left;
    diValue->orgText.y = diValue->recBkg.top
                         + Fonts::CDI.get_capital_height()
                         - Fonts::CDI.get_ascent_height();

    diValue->lastValue = fixed(-9999);
    diValue->lastText[0] = '\0';
    diValue->last_unit_symbol = NULL;
    diValue->InitDone = true;
  }

  if (!diLabel->InitDone) {

    diLabel->recBkg.right = x;
    diLabel->recBkg.top = y + Layout::Scale(1);

    diLabel->recBkg.left = diLabel->recBkg.right;
    // update back rect with max label size
    diLabel->recBkg.bottom = diLabel->recBkg.top
                             + Fonts::Title.get_capital_height();

    diLabel->orgText.x = diLabel->recBkg.left;
    diLabel->orgText.y = diLabel->recBkg.top
                         + Fonts::Title.get_capital_height()
                         - Fonts::Title.get_ascent_height();

    diLabel->lastValue = fixed(-9999);
    diLabel->lastText[0] = '\0';
    diLabel->last_unit_symbol = NULL;
    diLabel->InitDone = true;
  }

  canvas.background_transparent();

  if (!is_persistent() || (dirty && _tcscmp(diLabel->lastText, Label) != 0)) {
    canvas.set_background_color(colTextBackgnd);
    canvas.set_text_color(colTextLabel);
    canvas.select(Fonts::Title);
    tsize = canvas.text_size(Label);
    diLabel->orgText.x = diLabel->recBkg.right - tsize.cx;
    canvas.text_opaque(diLabel->orgText.x, diLabel->orgText.y,
                       diLabel->recBkg, Label);
    diLabel->recBkg.left = diLabel->orgText.x;
    _tcscpy(diLabel->lastText, Label);
  }

  if (!is_persistent() || (dirty && diValue->lastValue != Value)) {
    TCHAR Temp[18];
    canvas.set_background_color(colTextBackgnd);
    canvas.set_text_color(colText);
    _stprintf(Temp, _T("%.1f"), (double)Value);
    canvas.select(Fonts::CDI);
    tsize = canvas.text_size(Temp);
    diValue->orgText.x = diValue->recBkg.right - tsize.cx;

    canvas.text_opaque(diValue->orgText.x, diValue->orgText.y,
                       diValue->recBkg, Temp);

    diValue->recBkg.left = diValue->orgText.x;
    diValue->lastValue = Value;
  }

  if (!is_persistent() || (dirty && unit_symbol != NULL &&
                           diLabel->last_unit_symbol != unit_symbol)) {
    RasterPoint BitmapUnitPos = unit_symbol->get_origin(Appearance.InverseInfoBox
                                                  ? UnitSymbol::INVERSE_GRAY
                                                  : UnitSymbol::GRAY);
    PixelSize BitmapUnitSize = unit_symbol->get_size();

    canvas.scale_copy(x - Layout::Scale(5), diValue->recBkg.top,
                      *unit_symbol,
                      BitmapUnitPos.x, BitmapUnitPos.y,
                      BitmapUnitSize.cx, BitmapUnitSize.cy);

    diLabel->last_unit_symbol = unit_symbol;
  }
}

void
GaugeVario::RenderSpeedToFly(Canvas &canvas, int x, int y)
{
  if (!Calculated().AirspeedAvailable ||
      !Basic().TotalEnergyVarioAvailable)
    return;

  static fixed lastVdiff;
  fixed vdiff;

  const int ARROWYSIZE = Layout::Scale(3);
  const int ARROWXSIZE = Layout::Scale(7);

  int nary = NARROWS * ARROWYSIZE;
  int ytop = get_top() + YOFFSET + nary; // JMW
  int ybottom = get_bottom() - YOFFSET - nary - Layout::FastScale(1);

  ytop += Layout::Scale(14);
  ybottom -= Layout::Scale(14);

  x = get_right() - 2 * ARROWXSIZE;

  // only draw speed command if flying and vario is not circling
  if ((Calculated().flight.Flying)
      && (!Basic().gps.Simulator || !Calculated().Circling)) {
    vdiff = Calculated().V_stf - Calculated().IndicatedAirspeed;
    vdiff = max(-DeltaVlimit, min(DeltaVlimit, vdiff)); // limit it
    vdiff = iround(vdiff/DeltaVstep) * DeltaVstep;
  } else
    vdiff = fixed_zero;

  if (!is_persistent() || lastVdiff != vdiff || dirty) {
    lastVdiff = vdiff;

    if (is_persistent()) {
      Color background_color = Appearance.InverseInfoBox
        ? COLOR_BLACK : COLOR_WHITE;

      // bottom (too slow)
      canvas.fill_rectangle(x, ybottom + YOFFSET,
                            x + ARROWXSIZE * 2 + 1,
                            ybottom + YOFFSET + nary + ARROWYSIZE +
                            Layout::FastScale(2),
                            background_color);

      // top (too fast)
      canvas.fill_rectangle(x, ytop - YOFFSET + 1,
                            x + ARROWXSIZE * 2  +1,
                            ytop - YOFFSET - nary + 1 - ARROWYSIZE -
                            Layout::FastScale(2),
                            background_color);
    }

    RenderClimb(canvas);

    canvas.null_pen();

    if (Appearance.InfoBoxColors) {
      if (positive(vdiff)) {
        // too slow
        canvas.select(sinkBrush);
      } else {
        canvas.select(liftBrush);
      }
    } else {
      if (Appearance.InverseInfoBox)
        canvas.white_brush();
      else
        canvas.black_brush();
    }

    if (positive(vdiff)) {
      // too slow
      y = ybottom;
      y += YOFFSET;

      while (positive(vdiff)) {
        if (vdiff > DeltaVstep) {
          canvas.rectangle(x, y, x + ARROWXSIZE * 2 + 1, y + ARROWYSIZE - 1);
        } else {
          RasterPoint Arrow[4];
          Arrow[0].x = x;
          Arrow[0].y = y;
          Arrow[1].x = x + ARROWXSIZE;
          Arrow[1].y = y + ARROWYSIZE - 1;
          Arrow[2].x = x + 2 * ARROWXSIZE;
          Arrow[2].y = y;
          Arrow[3].x = x;
          Arrow[3].y = y;
          canvas.polygon(Arrow, 4);
        }
        vdiff -= DeltaVstep;
        y += ARROWYSIZE;
      }
    } else if (negative(vdiff)) {
      // too fast
      y = ytop;
      y -= YOFFSET;

      while (negative(vdiff)) {
        if (vdiff < -DeltaVstep) {
          canvas.rectangle(x, y + 1, x + ARROWXSIZE * 2 + 1, y - ARROWYSIZE + 2);
        } else {
          RasterPoint Arrow[4];
          Arrow[0].x = x;
          Arrow[0].y = y;
          Arrow[1].x = x + ARROWXSIZE;
          Arrow[1].y = y - ARROWYSIZE + 1;
          Arrow[2].x = x + 2 * ARROWXSIZE;
          Arrow[2].y = y;
          Arrow[3].x = x;
          Arrow[3].y = y;
          canvas.polygon(Arrow, 4);
        }
        vdiff += DeltaVstep;
        y -= ARROWYSIZE;
      }
    }
  }
}

void
GaugeVario::RenderBallast(Canvas &canvas)
{
  static fixed lastBallast = fixed_one;
  static PixelRect recLabelBk = {-1,-1,-1,-1};
  static PixelRect recValueBk = {-1,-1,-1,-1};
  static RasterPoint orgLabel = {-1,-1};
  static RasterPoint orgValue = {-1,-1};

  if (!ballast_initialised) { // ontime init, origin and background rect

    PixelSize tSize;
    // get max label size
    canvas.select(Fonts::Title);
    tSize = canvas.text_size(TextBal);

    // position of ballast label
    orgLabel.x = 0;
    orgLabel.y = get_top() + IBLSCALE(1) + tSize.cy;

    // position of ballast value
    orgValue.x = 0;
    orgValue.y = get_top();

    // set upper left corner
    recLabelBk.left = orgLabel.x;
    recLabelBk.top = orgLabel.y;

    // set upper left corner
    recValueBk.left = orgValue.x;
    recValueBk.top = orgValue.y;

    // update back rect with max label size
    recLabelBk.right = recLabelBk.left + tSize.cx + IBLSCALE(2);
    recLabelBk.bottom = recLabelBk.top + tSize.cy;

    // get max value size
    tSize = canvas.text_size(_T("100%"));

    // update back rect with max label size
    recValueBk.right = recValueBk.left + tSize.cx + IBLSCALE(2);
    recValueBk.bottom = recValueBk.top + tSize.cy;

    orgLabel.x += IBLSCALE(1);
    orgValue.x += IBLSCALE(1);

    ballast_initialised = true;
  }

  fixed BALLAST = Calculated().common_stats.current_ballast;

  if (BALLAST != lastBallast) {
    // ballast hase been changed

    TCHAR Temp[18];

    canvas.select(Fonts::Title);
    canvas.set_background_color(colSurround);

    if (lastBallast < fixed(0.001) || BALLAST < fixed(0.001)) {
      // new ballast is 0, hide label
      if (BALLAST < fixed(0.001)) {
        canvas.select(surroundBrush);
        canvas.null_pen();
        canvas.rectangle(recLabelBk.left, recLabelBk.top, recLabelBk.right, recLabelBk.bottom);
      } else {
        canvas.set_text_color(colTextLabel);
        // ols ballast was 0, show label
        canvas.text_opaque(orgLabel.x, orgLabel.y, recLabelBk, TextBal);
      }
    }

    // new ballast 0, hide value
    if (BALLAST < fixed(0.001)) {
        canvas.rectangle(recValueBk.left, recValueBk.top, recValueBk.right, recValueBk.bottom);
    } else {
      _stprintf(Temp, _T("%d%%"), (int)(BALLAST * 100));

      canvas.set_text_color(colText);
      canvas.text_opaque(orgValue.x, orgValue.y, recValueBk, Temp);
    }

    lastBallast = BALLAST;
  }
}

void
GaugeVario::RenderBugs(Canvas &canvas)
{
  static fixed lastBugs = fixed_one;
  static PixelRect recLabelBk = {-1,-1,-1,-1};
  static PixelRect recValueBk = {-1,-1,-1,-1};
  static RasterPoint orgLabel = {-1,-1};
  static RasterPoint orgValue = {-1,-1};

  if (!bugs_initialised) {
    PixelSize tSize;

    canvas.select(Fonts::Title);
    tSize = canvas.text_size(TextBug);

    orgLabel.x = 0;
    orgLabel.y = get_bottom() - tSize.cy*2 - IBLSCALE(1);

    orgValue.x = 0;
    orgValue.y = get_bottom() - tSize.cy;

    recLabelBk.left = orgLabel.x;
    recLabelBk.top = orgLabel.y;
    recValueBk.left = orgValue.x;
    recValueBk.top = orgValue.y;

    recLabelBk.right = recLabelBk.left + tSize.cx + IBLSCALE(2);
    recLabelBk.bottom = recLabelBk.top + tSize.cy;

    tSize = canvas.text_size(_T("100%"));

    recValueBk.right = recValueBk.left + tSize.cx + IBLSCALE(2);
    recValueBk.bottom = recValueBk.top + tSize.cy;

    orgLabel.x += IBLSCALE(1);
    orgValue.x += IBLSCALE(1);

    bugs_initialised = true;
  }

  fixed BUGS = Calculated().common_stats.current_bugs;
  if (BUGS != lastBugs) {
    TCHAR Temp[18];

    canvas.select(Fonts::Title);
    canvas.set_background_color(colSurround);

    if (lastBugs > fixed(0.999) || BUGS > fixed(0.999)) {
      if (BUGS > fixed(0.999)) {
        canvas.select(surroundBrush);
        canvas.null_pen();
        canvas.rectangle(recLabelBk.left, recLabelBk.top, recLabelBk.right, recLabelBk.bottom);
      } else {
        canvas.set_text_color(colTextLabel);
        canvas.text_opaque(orgLabel.x, orgLabel.y, recLabelBk, TextBug);
      }
    }

    if (BUGS > fixed(0.999)) {
        canvas.rectangle(recValueBk.left, recValueBk.top, recValueBk.right, recValueBk.bottom);
    } else {
      _stprintf(Temp, _T("%d%%"), (int)((fixed_one - BUGS) * 100));

      canvas.set_text_color(colText);
      canvas.text_opaque(orgValue.x, orgValue.y, recValueBk, Temp);
    }

    lastBugs = BUGS;
  }
}

void
GaugeVario::move(int left, int top, unsigned width, unsigned height)
{
  Window::move(left, top, width, height);

  /* trigger reinitialisation */
  xoffset = get_right();
  yoffset = get_height() / 2 + get_top();
  layout_initialised = false;
  needle_initialised = false;
  ballast_initialised = false;
  bugs_initialised = false;
}
