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
#include "Screen/Graphics.hpp"
#include "Screen/Point.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Ramp.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Icon.hpp"
#include "Screen/Brush.hpp"
#include "Screen/Color.hpp"
#include "Screen/Pen.hpp"
#include "Screen/Canvas.hpp"
#include "Math/Screen.hpp"
#include "Appearance.hpp"
#include "SettingsMap.hpp"
#include "resource.h"
#include "Asset.hpp"
#include "LogFile.hpp"

Pen Graphics::hAirspacePens[AIRSPACECLASSCOUNT];

#ifndef ENABLE_SDL
Brush Graphics::hAirspaceBrushes[NUMAIRSPACEBRUSHES];
Bitmap Graphics::hAirspaceBitmap[NUMAIRSPACEBRUSHES];
#endif

#if defined(HAVE_ALPHA_BLEND) || defined(ENABLE_SDL)
Brush Graphics::solid_airspace_brushes[NUMAIRSPACECOLORS];
#endif

Pen Graphics::hpSnail[NUMSNAILCOLORS];
Pen Graphics::hpSnailVario[NUMSNAILCOLORS];

#ifndef ENABLE_SDL
Bitmap Graphics::hAboveTerrainBitmap;
Brush Graphics::hAboveTerrainBrush;
#endif

MaskedIcon Graphics::hAirspaceInterceptBitmap;
MaskedIcon Graphics::hTerrainWarning;
MaskedIcon Graphics::hFLARMTraffic;
MaskedIcon Graphics::hLogger, Graphics::hLoggerOff;
MaskedIcon Graphics::hCruise, Graphics::hClimb,
           Graphics::hFinalGlide, Graphics::hAbort;
MaskedIcon Graphics::hGPSStatus1, Graphics::hGPSStatus2;
MaskedIcon Graphics::hBmpTrafficSafe, Graphics::hBmpTrafficWarning, Graphics::hBmpTrafficAlarm;

Pen Graphics::hpAircraft;
Pen Graphics::hpAircraftSimple1;
Pen Graphics::hpAircraftSimple2;
Pen Graphics::hpCanopy;
Pen Graphics::hpTaskActive;
Pen Graphics::hpTaskInactive;
Pen Graphics::hpTaskArrow;
Pen Graphics::hpWind;
Pen Graphics::hpIsoline;
Pen Graphics::hpBearing;
Pen Graphics::hpBestCruiseTrack;
Pen Graphics::hpCompass1;
Pen Graphics::hpCompass2;
Pen Graphics::hpFinalGlideAbove;
Pen Graphics::hpFinalGlideBelow;
Pen Graphics::hpFinalGlideBelowLandable;
Pen Graphics::hpMapScale;
Pen Graphics::hpTerrainLine;
Pen Graphics::hpTerrainLineThick;
Pen Graphics::hpTrackBearingLine;
Pen Graphics::TracePen;
Pen Graphics::ContestPen[3];

Brush Graphics::hbCanopy;
Brush Graphics::hbCompass1;
Brush Graphics::hbCompass2;
Brush Graphics::hbThermalBand;
Brush Graphics::hbBestCruiseTrack;
Brush Graphics::hbFinalGlideBelow;
Brush Graphics::hbFinalGlideBelowLandable;
Brush Graphics::hbFinalGlideAbove;
Brush Graphics::hbWind;

MaskedIcon Graphics::SmallIcon, Graphics::TurnPointIcon, Graphics::TaskTurnPointIcon;
MaskedIcon Graphics::MountainTopIcon, Graphics::BridgeIcon, Graphics::TunnelIcon;
MaskedIcon Graphics::TowerIcon, Graphics::PowerPlantIcon;
MaskedIcon Graphics::AirportReachableIcon, Graphics::AirportUnreachableIcon;
MaskedIcon Graphics::AirportMarginalIcon, Graphics::FieldMarginalIcon;
MaskedIcon Graphics::FieldReachableIcon, Graphics::FieldUnreachableIcon;
MaskedIcon Graphics::hBmpThermalSource;
MaskedIcon Graphics::hBmpTarget;
MaskedIcon Graphics::hBmpTeammatePosition;

MaskedIcon Graphics::hBmpMapScaleLeft;
MaskedIcon Graphics::hBmpMapScaleRight;

Bitmap Graphics::hBmpTabTask;
Bitmap Graphics::hBmpTabWrench;
Bitmap Graphics::hBmpTabSettings;
Bitmap Graphics::hBmpTabCalculator;

Bitmap Graphics::hBmpTabFlight;
Bitmap Graphics::hBmpTabSystem;
Bitmap Graphics::hBmpTabRules;
Bitmap Graphics::hBmpTabTimes;

// used for flarm
Brush Graphics::AlarmBrush;
Brush Graphics::WarningBrush;
Brush Graphics::TrafficBrush;

// used for landable rendering
Brush Graphics::hbGreen;
Brush Graphics::hbWhite;
Brush Graphics::hbMagenta;
Brush Graphics::hbOrange;
Brush Graphics::hbRed;
Brush Graphics::hbLightGray;
Brush Graphics::hbNotReachableTerrain;
Brush Graphics::hbGround;

ChartLook Graphics::chart;
ThermalBandLook Graphics::thermal_band;
TraceHistoryLook Graphics::trace_history;
CrossSectionLook Graphics::cross_section;
FlarmTrafficLook Graphics::flarm_dialog;
FlarmTrafficLook Graphics::flarm_gauge;

// airspace brushes/colours
const Color
Graphics::GetAirspaceColour(const int i)
{
  return Colours[i];
}

#ifndef ENABLE_SDL
const Brush &
Graphics::GetAirspaceBrush(const int i)
{
  return hAirspaceBrushes[i];
}
#endif

const Color
Graphics::GetAirspaceColourByClass(const int i,
                                   const AirspaceRendererSettings &settings)
{
  return GetAirspaceColour(settings.colours[i]);
}

#ifndef ENABLE_SDL
const Brush &
Graphics::GetAirspaceBrushByClass(const int i,
                                  const AirspaceRendererSettings &settings)
{
  return GetAirspaceBrush(settings.brushes[i]);
}
#endif

#define COLOR_alertSafe Color(0x31, 0x68, 0xe8) // lightblue1 lt 55 #3168e8
#define COLOR_alertWarning Color(0xeb, 0x68, 0x2e) // orange1 lt 55 #eb682e
#define COLOR_alertAlarm Color(0xe6, 0x38, 0x00) // red1 lt 45 #e63800
#define COLOR_task Color(0x86, 0x46, 0xa0) // pink0 lt 45 #8646a0
#define COLOR_bearing Color(0x18, 0x30, 0x51) // lightblue0 lt 20 #183051
#define COLOR_bearing_d Color(0x0f, 0x20, 0x39) // lightblue0 lt 13 #0f2039
#define COLOR_wind Color(0x27, 0x47, 0x74) // lightblue0 lt 30 #274774
#define COLOR_wind_l Color(0x4b, 0x82, 0xce) // lightblue0 lt 55 #4b82ce
#define COLOR_fgabove Color(0x4d, 0x99, 0x00) // green0 lt 30 #4d9900
#define COLOR_fgbelow Color(0xc0, 0x3a, 0x0c) // red0 lt 40 #c03a0c
#define COLOR_fgbelowlandable Color(0xd5, 0x80, 0x5e) // orange0 lt 60 #d5805e
#define COLOR_fgabove_d Color(0x24, 0x4d, 0x00) // green0 lt 15 #244d00
#define COLOR_fgbelow_d Color(0x7b, 0x22, 0x05) // red0 lt 25 #7b2205
#define COLOR_fgbelowlandable_d Color(0x8f, 0x54, 0x3d) // orange0 lt 40 #8f543d
#define COLOR_ground Color(0x7f, 0x4a, 0x35) // orange0 lt 35 #7f4a35
#define COLOR_ground_d Color(0x5c, 0x34, 0x24) // orange0 lt 25 #5c3424
#define COLOR_sky Color(0x67, 0xad, 0xff) // lightblue0 lt 70 #67adff
#define COLOR_sky_d Color(0x44, 0x76, 0xbc) // lightblue0 lt 50 #4476bc
#define COLOR_landable_g Color(0x5b, 0xb3, 0x00) // green0 lt 35 #5bb300
#define COLOR_landable_m Color(0xb8, 0x39, 0xf9) // pink1 lt 60 #b839f9
#define COLOR_landable_o Color(0xc2, 0x55, 0x24) // orange1 lt 45 #c25524
#define COLOR_landable_r Color(0xcc, 0x30, 0x00) // red1 lt 40 #cc3000
#define COLOR_landable_n Color(0x99, 0x99, 0x99) // white1 lt 60 #999999

const Color Graphics::inv_redColor = Color(0xff, 0x70, 0x70);
const Color Graphics::inv_blueColor = Color(0x90, 0x90, 0xff);
const Color Graphics::inv_yellowColor = COLOR_YELLOW;
const Color Graphics::inv_greenColor = COLOR_GREEN;
const Color Graphics::inv_magentaColor = COLOR_MAGENTA;

const Color Graphics::cAlertSafe = COLOR_alertSafe;
const Color Graphics::cAlertWarning = COLOR_alertWarning;
const Color Graphics::cAlertAlarm = COLOR_alertAlarm;

const Color Graphics::GroundColor = COLOR_ground;
const Color Graphics::skyColor = COLOR_sky;
const Color Graphics::seaColor = Color(0xbd,0xc5,0xd5); // ICAO open water area

static Color burntOrange(0xff,0x5a,0x00);
// Magenta ICAO color is 0x65,0x23,0x1c
const Color Graphics::TaskColor = COLOR_task;
const Color Graphics::BearingColor = COLOR_bearing;
const Color Graphics::IsolineColor = COLOR_bearing;

const Color Graphics::sinkColor = Color(0xc5,0x57,0x25);
const Color Graphics::liftColor = Color(0x57,0xab,0x00);
const Color Graphics::inv_sinkColor = Color(0xc5,0x57,0x25);
const Color Graphics::inv_liftColor = Color(0x57,0xab,0x00);

const Color Graphics::Colours[] = {
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BLUE,
  COLOR_YELLOW,
  COLOR_MAGENTA,
  COLOR_CYAN,
  dark_color(COLOR_RED),
  dark_color(COLOR_GREEN),
  dark_color(COLOR_BLUE),
  dark_color(COLOR_YELLOW),
  dark_color(COLOR_MAGENTA),
  dark_color(COLOR_CYAN),
  COLOR_WHITE,
  COLOR_LIGHT_GRAY,
  COLOR_GRAY,
  COLOR_BLACK,
};

void
Graphics::Initialise()
{
  /// @todo enhancement: support red/green color blind pilots with adjusted colour scheme

  LogStartUp(_T("Initialise graphics"));

  LoadUnitSymbols();

  AlarmBrush.set(cAlertAlarm);
  WarningBrush.set(cAlertWarning);
  TrafficBrush.set(cAlertSafe);

  hFLARMTraffic.load_big(IDB_FLARMTRAFFIC, IDB_FLARMTRAFFIC_HD);
  hTerrainWarning.load_big(IDB_TERRAINWARNING, IDB_TERRAINWARNING_HD);
  hGPSStatus1.load_big(IDB_GPSSTATUS1, IDB_GPSSTATUS1_HD, false);
  hGPSStatus2.load_big(IDB_GPSSTATUS2, IDB_GPSSTATUS2_HD, false);
  hLogger.load_big(IDB_LOGGER, IDB_LOGGER_HD);
  hLoggerOff.load_big(IDB_LOGGEROFF, IDB_LOGGEROFF_HD);
  hBmpTeammatePosition.load_big(IDB_TEAMMATE_POS, IDB_TEAMMATE_POS_HD);

  hCruise.load_big(IDB_CRUISE, IDB_CRUISE_HD, false);
  hClimb.load_big(IDB_CLIMB, IDB_CLIMB_HD, false);
  hFinalGlide.load_big(IDB_FINALGLIDE, IDB_FINALGLIDE_HD, false);
  hAbort.load_big(IDB_ABORT, IDB_ABORT_HD, false);

  // airspace brushes and colors
#ifndef ENABLE_SDL
  hAirspaceBitmap[0].load(IDB_AIRSPACE0);
  hAirspaceBitmap[1].load(IDB_AIRSPACE1);
  hAirspaceBitmap[2].load(IDB_AIRSPACE2);
  hAirspaceBitmap[3].load(IDB_AIRSPACE3);
  hAirspaceBitmap[4].load(IDB_AIRSPACE4);
  hAirspaceBitmap[5].load(IDB_AIRSPACE5);
  hAirspaceBitmap[6].load(IDB_AIRSPACE6);
  hAirspaceBitmap[7].load(IDB_AIRSPACE7);
#endif

  hAirspaceInterceptBitmap.load_big(IDB_AIRSPACEI, IDB_AIRSPACEI_HD);

#ifndef ENABLE_SDL
  hAboveTerrainBitmap.load(IDB_ABOVETERRAIN);

  for (int i = 0; i < NUMAIRSPACEBRUSHES; i++)
    hAirspaceBrushes[i].set(hAirspaceBitmap[i]);

  hAboveTerrainBrush.set(hAboveTerrainBitmap);
#endif

#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
#endif
#if defined(HAVE_ALPHA_BLEND) || defined(ENABLE_SDL)
    for (unsigned i = 0; i < NUMAIRSPACECOLORS; ++i)
      solid_airspace_brushes[i].set(Colours[i]);
#endif

  hpIsoline.set(Pen::DASH, Layout::Scale(1), IsolineColor);

  hbWind.set(COLOR_wind_l);
  hpWind.set(Layout::Scale(1), COLOR_wind);

  hbCompass1.set(COLOR_wind_l);
  hpCompass1.set(Layout::Scale(1), COLOR_wind);
  hbCompass2.set(COLOR_wind);
  hpCompass2.set(Layout::Scale(1), COLOR_wind_l);

  hpTaskActive.set(Pen::DASH, Layout::Scale(2), Graphics::TaskColor);
  hpTaskInactive.set(Pen::DASH, Layout::Scale(1), Graphics::TaskColor);
  hpTaskArrow.set(Layout::Scale(1), Graphics::TaskColor);

  hBmpMapScaleLeft.load_big(IDB_MAPSCALE_LEFT, IDB_MAPSCALE_LEFT_HD, false);
  hBmpMapScaleRight.load_big(IDB_MAPSCALE_RIGHT, IDB_MAPSCALE_RIGHT_HD, false);

  hBmpTabTask.load((Layout::scale > 1) ? IDB_TASK_HD : IDB_TASK);
  hBmpTabWrench.load((Layout::scale > 1) ? IDB_WRENCH_HD : IDB_WRENCH);
  hBmpTabSettings.load((Layout::scale > 1) ? IDB_SETTINGS_HD : IDB_SETTINGS);
  hBmpTabCalculator.load((Layout::scale > 1) ? IDB_CALCULATOR_HD : IDB_CALCULATOR);

  hBmpTabFlight.load((Layout::scale > 1) ? IDB_GLOBE_HD : IDB_GLOBE);
  hBmpTabSystem.load((Layout::scale > 1) ? IDB_DEVICE_HD : IDB_DEVICE);
  hBmpTabRules.load((Layout::scale > 1) ? IDB_RULES_HD : IDB_RULES);
  hBmpTabTimes.load((Layout::scale > 1) ? IDB_CLOCK_HD : IDB_CLOCK);

  hBmpThermalSource.load_big(IDB_THERMALSOURCE, IDB_THERMALSOURCE_HD);
  hBmpTarget.load_big(IDB_TARGET, IDB_TARGET_HD);

  hBmpTrafficSafe.load_big(IDB_TRAFFIC_SAFE, IDB_TRAFFIC_SAFE_HD, false);
  hBmpTrafficWarning.load_big(IDB_TRAFFIC_WARNING, IDB_TRAFFIC_WARNING_HD, false);
  hBmpTrafficAlarm.load_big(IDB_TRAFFIC_ALARM, IDB_TRAFFIC_ALARM_HD, false);

  hbThermalBand.set(COLOR_sky);
  hpThermalBand.set(Layout::Scale(1), COLOR_sky_d);

  hbFinalGlideBelow.set(COLOR_fgbelow);
  hpFinalGlideBelow.set(Layout::Scale(1), COLOR_fgbelow_d);

  hbFinalGlideBelowLandable.set(COLOR_fgbelowlandable);
  hpFinalGlideBelowLandable.set(Layout::Scale(1), COLOR_fgbelowlandable_d);

  hbFinalGlideAbove.set(COLOR_fgabove);
  hpFinalGlideAbove.set(Layout::Scale(1), COLOR_fgabove_d);

  hpBearing.set(Layout::Scale(2), COLOR_bearing);
  hpBestCruiseTrack.set(Layout::Scale(1), COLOR_bearing_d);
  hbBestCruiseTrack.set(COLOR_bearing);

  hpMapScale.set(Layout::Scale(1), COLOR_BLACK);

  hpTerrainLine.set(Pen::DASH, Layout::Scale(1), COLOR_ground_d);
  hpTerrainLineThick.set(Pen::DASH, Layout::Scale(2), COLOR_ground_d);

  TracePen.set(2, Color(50, 243, 45));
  ContestPen[0].set(Layout::Scale(1)+2, COLOR_RED);
  ContestPen[1].set(Layout::Scale(1)+1, COLOR_ORANGE);
  ContestPen[2].set(Layout::Scale(1), COLOR_BLUE);

  SmallIcon.load_big(IDB_SMALL, IDB_SMALL_HD);
  TurnPointIcon.load_big(IDB_TURNPOINT, IDB_TURNPOINT_HD);
  TaskTurnPointIcon.load_big(IDB_TASKTURNPOINT, IDB_TASKTURNPOINT_HD);
  MountainTopIcon.load_big(IDB_MOUNTAIN_TOP, IDB_MOUNTAIN_TOP_HD);
  BridgeIcon.load_big(IDB_BRIDGE, IDB_BRIDGE_HD);
  TunnelIcon.load_big(IDB_TUNNEL, IDB_TUNNEL_HD);
  TowerIcon.load_big(IDB_TOWER, IDB_TOWER_HD);
  PowerPlantIcon.load_big(IDB_POWER_PLANT, IDB_POWER_PLANT_HD);

  hpAircraft.set(1, COLOR_DARK_GRAY);
  hpAircraftSimple1.set(Layout::Scale(1), COLOR_BLACK);
  hpAircraftSimple2.set(Layout::Scale(3), COLOR_WHITE);
  hpCanopy.set(1, dark_color(COLOR_CYAN));
  hbCanopy.set(COLOR_CYAN);

    // used for landable rendering
  hbGreen.set(COLOR_landable_g);
  hbWhite.set(COLOR_WHITE);
  hbMagenta.set(COLOR_landable_m);
  hbOrange.set(COLOR_landable_o);
  hbRed.set(COLOR_landable_r);
  hbLightGray.set(COLOR_landable_n);
  hbNotReachableTerrain.set(light_color(COLOR_RED));

  hbGround.set(GroundColor);

  hpTrackBearingLine.set(3, COLOR_GRAY);

  flarm_dialog.Initialise(false);
  flarm_gauge.Initialise(true);
}

void
Graphics::InitialiseConfigured(const SETTINGS_MAP &settings_map)
{
  InitSnailTrail(settings_map);
  InitLandableIcons();
  InitAirspacePens(settings_map.airspace);

  chart.Initialise();
  thermal_band.Initialise();
  trace_history.Initialise(Appearance.InverseInfoBox);
  cross_section.Initialise();
}

void
Graphics::InitSnailTrail(const SETTINGS_MAP &settings_map)
{
  const ColorRamp snail_colors_vario[] = {
    {0, 0xd6, 0x5e, 0x29},
    {10, 0xcd, 0x5a, 0x27},
    {20, 0xc5, 0x56, 0x25},
    {30, 0xbc, 0x52, 0x23},
    {40, 0xb2, 0x4d, 0x20},
    {50, 0xaa, 0x49, 0x1e},
    {60, 0x9f, 0x44, 0x1c},
    {70, 0x96, 0x40, 0x1a},
    {80, 0x8e, 0x3c, 0x18},
    {90, 0x84, 0x38, 0x15},
    {100, 0x4d, 0x4d, 0x4d},
    {110, 0x40, 0x80, 0x00},
    {120, 0x47, 0x8e, 0x00},
    {130, 0x4f, 0x9c, 0x00},
    {140, 0x57, 0xab, 0x00},
    {150, 0x5f, 0xb9, 0x00},
    {160, 0x66, 0xc7, 0x00},
    {170, 0x6e, 0xd5, 0x00},
    {180, 0x76, 0xe3, 0x00},
    {190, 0x7d, 0xf1, 0x00},
    {200, 0x85, 0xff, 0x00},
  };

  const ColorRamp snail_colors_vario2[] = {
    {0,   0x00, 0x00, 0xff},
    {99,  0x00, 0xff, 0xff},
    {100, 0xff, 0xff, 0x00},
    {200, 0xff, 0x00, 0x00}
  };

  const ColorRamp snail_colors_alt[] = {
    {0,   0xff, 0x00, 0x00},
    {50,  0xff, 0xff, 0x00},
    {100, 0x00, 0xff, 0x00},
    {150, 0x00, 0xff, 0xff},
    {200, 0x00, 0x00, 0xff},
  };

  int iwidth;
  int minwidth = Layout::Scale(2);

  Color color;
  for (int i = 0; i < NUMSNAILCOLORS; i++) {
    short ih = i * 200 / (NUMSNAILCOLORS - 1);
    switch (settings_map.SnailType) {
    case stAltitude:
      color = ColorRampLookup(ih, snail_colors_alt,
                              sizeof(snail_colors_alt)/sizeof(snail_colors_alt[0]));
      break;
    case stSeeYouVario:
      color = ColorRampLookup(ih, snail_colors_vario2,
                              sizeof(snail_colors_vario2)/sizeof(snail_colors_vario2[0]));
      break;
    case stStandardVario:
      color = ColorRampLookup(ih, snail_colors_vario,
                              sizeof(snail_colors_vario)/sizeof(snail_colors_vario[0]));
    }
    if (i < NUMSNAILCOLORS / 2 ||
        !settings_map.SnailScaling)
      iwidth = minwidth;
    else
      iwidth = max(minwidth, (i - NUMSNAILCOLORS / 2) *
                             Layout::Scale(16) / NUMSNAILCOLORS);

    hpSnail[i].set(minwidth, color);
    hpSnailVario[i].set(iwidth, color);
  }
}

void
Graphics::InitLandableIcons()
{
  switch (Appearance.IndLandable) {
  case wpLandableWinPilot:
    AirportReachableIcon.load_big(IDB_REACHABLE, IDB_REACHABLE_HD);
    AirportMarginalIcon.load_big(IDB_MARGINAL, IDB_MARGINAL_HD);
    AirportUnreachableIcon.load_big(IDB_LANDABLE, IDB_LANDABLE_HD);
    FieldReachableIcon.load_big(IDB_REACHABLE, IDB_REACHABLE_HD);
    FieldMarginalIcon.load_big(IDB_MARGINAL, IDB_MARGINAL_HD);
    FieldUnreachableIcon.load_big(IDB_LANDABLE, IDB_LANDABLE_HD);
    break;
  case wpLandableAltA:
    AirportReachableIcon.load_big(IDB_AIRPORT_REACHABLE,
                                  IDB_AIRPORT_REACHABLE_HD);
    AirportMarginalIcon.load_big(IDB_AIRPORT_MARGINAL,
                                 IDB_AIRPORT_MARGINAL_HD);
    AirportUnreachableIcon.load_big(IDB_AIRPORT_UNREACHABLE,
                                    IDB_AIRPORT_UNREACHABLE_HD);
    FieldReachableIcon.load_big(IDB_OUTFIELD_REACHABLE,
                                IDB_OUTFIELD_REACHABLE_HD);
    FieldMarginalIcon.load_big(IDB_OUTFIELD_MARGINAL,
                               IDB_OUTFIELD_MARGINAL_HD);
    FieldUnreachableIcon.load_big(IDB_OUTFIELD_UNREACHABLE,
                                  IDB_OUTFIELD_UNREACHABLE_HD);
    break;
  case wpLandableAltB:
    AirportReachableIcon.load_big(IDB_AIRPORT_REACHABLE,
                                  IDB_AIRPORT_REACHABLE_HD);
    AirportMarginalIcon.load_big(IDB_AIRPORT_MARGINAL2,
                                 IDB_AIRPORT_MARGINAL2_HD);
    AirportUnreachableIcon.load_big(IDB_AIRPORT_UNREACHABLE2,
                                    IDB_AIRPORT_UNREACHABLE2_HD);
    FieldReachableIcon.load_big(IDB_OUTFIELD_REACHABLE,
                                IDB_OUTFIELD_REACHABLE_HD);
    FieldMarginalIcon.load_big(IDB_OUTFIELD_MARGINAL2,
                               IDB_OUTFIELD_MARGINAL2_HD);
    FieldUnreachableIcon.load_big(IDB_OUTFIELD_UNREACHABLE2,
                                  IDB_OUTFIELD_UNREACHABLE2_HD);
    break;
  }
}

void
Graphics::InitAirspacePens(const AirspaceRendererSettings &settings)
{
  for (int i = 0; i < AIRSPACECLASSCOUNT; i++)
    hAirspacePens[i].set(Layout::Scale(2),
                         GetAirspaceColourByClass(i, settings));
}

void
Graphics::Deinitialise()
{
  DeinitialiseUnitSymbols();

  AlarmBrush.reset();
  WarningBrush.reset();
  TrafficBrush.reset();

  hFLARMTraffic.reset();
  hTerrainWarning.reset();
  hGPSStatus1.reset();
  hGPSStatus2.reset();
  hLogger.reset();
  hLoggerOff.reset();
  hBmpTeammatePosition.reset();

  hCruise.reset();
  hClimb.reset();
  hFinalGlide.reset();
  hAbort.reset();

  hAirspaceInterceptBitmap.reset();

#ifndef ENABLE_SDL
  for (unsigned i = 0; i < NUMAIRSPACEBRUSHES; i++) {
    hAirspaceBrushes[i].reset();
    hAirspaceBitmap[i].reset();
  }

  hAboveTerrainBrush.reset();
  hAboveTerrainBitmap.reset();
#endif

#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
#endif
#if defined(HAVE_ALPHA_BLEND) || defined(ENABLE_SDL)
    for (unsigned i = 0; i < NUMAIRSPACECOLORS; ++i)
      solid_airspace_brushes[i].reset();
#endif

  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++)
    hAirspacePens[i].reset();

  hbWind.reset();

  hBmpMapScaleLeft.reset();
  hBmpMapScaleRight.reset();

  hBmpTabTask.reset();
  hBmpTabWrench.reset();
  hBmpTabSettings.reset();
  hBmpTabCalculator.reset();

  hBmpTabFlight.reset();
  hBmpTabSystem.reset();
  hBmpTabRules.reset();
  hBmpTabTimes.reset();

  hBmpThermalSource.reset();
  hBmpTarget.reset();

  hBmpTrafficSafe.reset();
  hBmpTrafficWarning.reset();
  hBmpTrafficAlarm.reset();

  hbCompass1.reset();
  hbCompass2.reset();
  hpCompass1.reset();
  hpCompass2.reset();

  hbBestCruiseTrack.reset();
  hbFinalGlideBelow.reset();
  hbFinalGlideBelowLandable.reset();
  hbFinalGlideAbove.reset();

  hpTaskActive.reset();
  hpTaskInactive.reset();
  hpTaskArrow.reset();

  hpWind.reset();
  hpIsoline.reset();

  hpBearing.reset();
  hpBestCruiseTrack.reset();

  hpFinalGlideBelow.reset();
  hpFinalGlideBelowLandable.reset();
  hpFinalGlideAbove.reset();

  hpMapScale.reset();
  hpTerrainLine.reset();
  hpTerrainLineThick.reset();

  TracePen.reset();
  ContestPen[0].reset();
  ContestPen[1].reset();
  ContestPen[2].reset();

  SmallIcon.reset();
  TurnPointIcon.reset();
  TaskTurnPointIcon.reset();
  MountainTopIcon.reset();
  BridgeIcon.reset();
  TunnelIcon.reset();
  TowerIcon.reset();
  PowerPlantIcon.reset();

  hpAircraft.reset();
  hpAircraftSimple1.reset();
  hpAircraftSimple2.reset();
  hpCanopy.reset();
  hbCanopy.reset();

  hbGreen.reset();
  hbWhite.reset();
  hbMagenta.reset();
  hbOrange.reset();
  hbRed.reset();
  hbLightGray.reset();
  hbNotReachableTerrain.reset();

  hbGround.reset();

  hpTrackBearingLine.reset();

  for (unsigned i = 0; i < NUMSNAILCOLORS; i++) {
    hpSnail[i].reset();
    hpSnailVario[i].reset();
  }

  AirportReachableIcon.reset();
  AirportUnreachableIcon.reset();
  AirportMarginalIcon.reset();
  FieldMarginalIcon.reset();
  FieldReachableIcon.reset();
  FieldUnreachableIcon.reset();

  chart.Deinitialise();
  thermal_band.Deinitialise();
  trace_history.Deinitialise();
  cross_section.Deinitialise();
  flarm_dialog.Deinitialise();
  flarm_gauge.Deinitialise();
}

static void
DrawMirroredPolygon(const RasterPoint *src, RasterPoint *dst, unsigned points,
                    Canvas &canvas, const Angle angle,
                    const RasterPoint pos)
{
  std::copy(src, src + points, dst);
  for (unsigned i = 0; i < points; ++i) {
    dst[2 * points - i - 1].x = -dst[i].x;
    dst[2 * points - i - 1].y = dst[i].y;
  }
  PolygonRotateShift(dst, 2 * points, pos.x, pos.y, angle, false);
  canvas.polygon(dst, 2 * points);
}


static void
DrawDetailedAircraft(Canvas &canvas, const SETTINGS_MAP &settings_map,
                     const Angle angle,
                     const RasterPoint aircraft_pos)
{
  {
    static const RasterPoint Aircraft[] = {
      {0, -10},
      {-2, -7},
      {-2, -2},
      {-16, -2},
      {-32, -1},
      {-32, 2},
      {-1, 3},
      {-1, 15},
      {-3, 15},
      {-5, 17},
      {-5, 18},
      {0, 18},
    };
    const unsigned AIRCRAFT_POINTS = sizeof(Aircraft) / sizeof(Aircraft[0]);
    RasterPoint buffer[2 * AIRCRAFT_POINTS];

    if (settings_map.terrain.enable) {
      canvas.white_brush();
      canvas.select(Graphics::hpAircraft);
    } else {
      canvas.black_brush();
      canvas.white_pen();
    }

    DrawMirroredPolygon(Aircraft, buffer, AIRCRAFT_POINTS,
                        canvas, angle, aircraft_pos);
  }

  {
    static const RasterPoint Canopy[] = {
      {0, -7},
      {-1, -7},
      {-1, -2},
      {0, -1},
    };
    const unsigned CANOPY_POINTS = sizeof(Canopy) / sizeof(Canopy[0]);
    RasterPoint buffer[2 * CANOPY_POINTS];

    canvas.select(Graphics::hpCanopy);
    canvas.select(Graphics::hbCanopy);
    DrawMirroredPolygon(Canopy, buffer, CANOPY_POINTS,
                        canvas, angle, aircraft_pos);
  }
}


static void
DrawSimpleAircraft(Canvas &canvas, const Angle angle,
                   const RasterPoint aircraft_pos, bool large)
{
  static const RasterPoint AircraftLarge[] = {
    {1, -7},
    {1, -1},
    {17, -1},
    {17, 1},
    {1, 1},
    {1, 10},
    {5, 10},
    {5, 12},
    {-5, 12},
    {-5, 10},
    {-1, 10},
    {-1, 1},
    {-17, 1},
    {-17, -1},
    {-1, -1},
    {-1, -7},
  };

  static const RasterPoint AircraftSmall[] = {
    {1, -5},
    {1, 0},
    {14, 0},
    {14, 1},
    {1, 1},
    {1, 8},
    {4, 8},
    {4, 9},
    {-3, 9},
    {-3, 8},
    {0, 8},
    {0, 1},
    {-13, 1},
    {-13, 0},
    {0, 0},
    {0, -5},
   };

  const unsigned AIRCRAFT_POINTS_LARGE =
                            sizeof(AircraftLarge) / sizeof(AircraftLarge[0]);
  const unsigned AIRCRAFT_POINTS_SMALL =
                            sizeof(AircraftSmall) / sizeof(AircraftSmall[0]);

  const RasterPoint *Aircraft = large ? AircraftLarge : AircraftSmall;
  const unsigned AircraftPoints = large ?
                                  AIRCRAFT_POINTS_LARGE : AIRCRAFT_POINTS_SMALL;

  RasterPoint aircraft[std::max(AIRCRAFT_POINTS_LARGE, AIRCRAFT_POINTS_SMALL)];
  std::copy(Aircraft, Aircraft + AircraftPoints, aircraft);
  PolygonRotateShift(aircraft, AircraftPoints,
                     aircraft_pos.x, aircraft_pos.y, angle, true);
  canvas.select(Graphics::hpAircraftSimple2);
  canvas.polygon(aircraft, AircraftPoints);
  canvas.black_brush();
  canvas.select(Graphics::hpAircraftSimple1);
  canvas.polygon(aircraft, AircraftPoints);
}


void
Graphics::DrawAircraft(Canvas &canvas, const SETTINGS_MAP &settings_map,
                       const Angle angle,
                       const RasterPoint aircraft_pos)
{
  switch (Appearance.AircraftSymbol) {
    case acDetailed:
      DrawDetailedAircraft(canvas, settings_map, angle, aircraft_pos);
      break;
    case acSimpleLarge:
      DrawSimpleAircraft(canvas, angle, aircraft_pos, true);
      break;
    case acSimple:
    default:
      DrawSimpleAircraft(canvas, angle, aircraft_pos, false);
      break;
  }
}
