/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

/**
 * @file
 * The FLARM Traffic dialog displaying a radar screen with the moving
 * FLARM targets in track up orientation. The target can be selected and basic
 * information is given on the selected target. When a warning/alarm is present
 * the target with the highest alarm level is automatically selected and
 * highlighted in orange or red (depending on the level)
 * @todo make targets selectable via mouse
 * @todo details dialog
 * @todo team mates
 */

#include "StdAfx.h"
#include <aygshell.h>

#include "XCSoar.h"

#include "externs.h"
#include "dlgTools.h"
#include "InfoBoxLayout.h"
#include "Utils.h"

#define hcWarning RGB(0xFF, 0xA2, 0x00)
#define hcAlarm RGB(0xFF, 0x00, 0x00)
#define hcStandard RGB(0x00, 0x00, 0x00)
#define hcPassive RGB(0x99, 0x99, 0x99)
#define hcSelection RGB(0x00, 0x00, 0xFF)
#define hcTeam RGB(0x74, 0xFF, 0x00)
#define hcBackground RGB(0xFF, 0xFF, 0xFF)
#define hcRadar RGB(0x55, 0x55, 0x55)

static WndForm *wf = NULL;
static WndOwnerDrawFrame *wdf = NULL;
static unsigned zoom = 2;
static int selection = -1;
static int warning = -1;
static POINT radar_mid;
static SIZE radar_size;

extern HFONT  TitleWindowFont;
extern HFONT  TitleSmallWindowFont;
extern HFONT  MapWindowFont;
extern HFONT  MapWindowBoldFont;
extern HFONT  InfoWindowFont;
extern HFONT  CDIWindowFont;
extern HFONT  StatisticsFont;

/**
 * Tries to select the next target, if impossible selection = -1
 */
static void
NextTarget()
{
  for (int i = selection + 1; i < FLARM_MAX_TRAFFIC; i++) {
    if (GPS_INFO.FLARM_Traffic[i].ID > 0) {
      selection = i;
      return;
    }
  }
  for (int i = 0; i < selection; i++) {
    if (GPS_INFO.FLARM_Traffic[i].ID > 0) {
      selection = i;
      return;
    }
  }
  selection = -1;
}

/**
 * Tries to select the previous target, if impossible selection = -1
 */
static void
PrevTarget()
{
  for (int i = selection - 1; i >= 0; i--) {
    if (GPS_INFO.FLARM_Traffic[i].ID > 0) {
      selection = i;
      return;
    }
  }
  for (int i = FLARM_MAX_TRAFFIC - 1; i > selection; i--) {
    if (GPS_INFO.FLARM_Traffic[i].ID >0) {
      selection = i;
      return;
    }
  }
  selection = -1;
}

/**
 * Checks whether the selection is still on the valid target and if not tries
 * to select the next one
 */
static void
UpdateSelector()
{
  if (!GPS_INFO.FLARM_Available || GPS_INFO.FLARM_RX == 0) {
    selection = -1;
    return;
  }

  if (selection == -1 || GPS_INFO.FLARM_Traffic[selection].ID <= 0)
    NextTarget();
}

/**
 * Iterates through the traffic array, finds the target with the highest
 * alarm level and saves it to "warning".
 */
static void
UpdateWarnings()
{
  bool found = false;

  for (unsigned i = 0; i < FLARM_MAX_TRAFFIC; ++i) {
    // if Traffic[i] not defined -> goto next one
    if (GPS_INFO.FLARM_Traffic[i].ID <= 0)
      continue;

    // if current target has no alarm -> goto next one
    if (GPS_INFO.FLARM_Traffic[i].AlarmLevel == 0)
      continue;

    // remember that a warning exists
    found = true;
    // if it did not before -> save the id and goto next one
    if (GPS_INFO.FLARM_Traffic[warning].ID <= 0) {
      warning = i;
      continue;
    }

    // if it did before and the other level was higher -> just goto next one
    if (GPS_INFO.FLARM_Traffic[warning].AlarmLevel >
        GPS_INFO.FLARM_Traffic[i].AlarmLevel) {
      continue;
    }

    // if the other level was lower -> save the id and goto next one
    if (GPS_INFO.FLARM_Traffic[warning].AlarmLevel <
        GPS_INFO.FLARM_Traffic[i].AlarmLevel) {
      warning = i;
      continue;
    }

    // if the levels match -> let the distance decide (smaller distance wins)
    double dist_w = sqrt(
        GPS_INFO.FLARM_Traffic[warning].RelativeAltitude *
        GPS_INFO.FLARM_Traffic[warning].RelativeAltitude +
        GPS_INFO.FLARM_Traffic[warning].RelativeEast *
        GPS_INFO.FLARM_Traffic[warning].RelativeEast +
        GPS_INFO.FLARM_Traffic[warning].RelativeNorth *
        GPS_INFO.FLARM_Traffic[warning].RelativeNorth);
    double dist_i = sqrt(
        GPS_INFO.FLARM_Traffic[i].RelativeAltitude *
        GPS_INFO.FLARM_Traffic[i].RelativeAltitude +
        GPS_INFO.FLARM_Traffic[i].RelativeEast *
        GPS_INFO.FLARM_Traffic[i].RelativeEast +
        GPS_INFO.FLARM_Traffic[i].RelativeNorth *
        GPS_INFO.FLARM_Traffic[i].RelativeNorth);

    if (dist_w > dist_i)
      warning = i;
  }

  // If no warning was found -> set warning to -1
  if (!found)
    warning = -1;
}

/**
 * This should be called when the radar needs to be repainted
 */
static void
Update()
{
  UpdateSelector();
  UpdateWarnings();
  wdf->Redraw();
}

/**
 * Zoom out one step
 */
static void
ZoomOut()
{
  if (zoom < 4)
    zoom++;

  Update();
}

/**
 * Zoom in one step
 */
static void
ZoomIn()
{
  if (zoom > 0)
    zoom--;

  Update();
}

/**
 * This event handler is called when the "ZoomIn (+)" button is pressed
 */
static void
OnZoomInClicked(WindowControl * Sender)
{
  ZoomIn();
}

/**
 * This event handler is called when the "ZoomOut (-)" button is pressed
 */
static void
OnZoomOutClicked(WindowControl * Sender)
{
  ZoomOut();
}

/**
 * This event handler is called when the "Prev (<)" button is pressed
 */
static void
OnPrevClicked(WindowControl * Sender)
{
  // If warning is displayed -> prevent selector movement
  if (warning >= 0)
    return;

  PrevTarget();
  Update();
}

/**
 * This event handler is called when the "Next (>)" button is pressed
 */
static void
OnNextClicked(WindowControl * Sender)
{
  // If warning is displayed -> prevent selector movement
  if (warning >= 0)
    return;

  NextTarget();
  Update();
}

/**
 * This event handler is called when the "Close" button is pressed
 */
static void
OnCloseClicked(WindowControl * Sender)
{
  wf->SetModalResult(mrOK);
}

/**
 * This event handler is called when a key is pressed
 * @param key_code The key code of the pressed key
 * @return True if the event was handled, False otherwise
 */
static int
FormKeyDown(WindowControl * Sender, WPARAM wParam, LPARAM lParam)
{
  (void)Sender;

  switch (wParam & 0xffff) {
  case VK_LEFT:
  case '6':
    PrevTarget();
    Update();
    return 0;
  case VK_RIGHT:
  case '7':
    NextTarget();
    Update();
    return 0;

  default:
    return 1;
  }
}

/**
 * This event handler is called when the timer is activated and triggers the
 * repainting of the radar
 */
static int
OnTimerNotify(WindowControl * Sender)
{
  (void)Sender;
  Update();
  return 0;
}

static double
GetZoomDistance() {
  switch (zoom) {
    case 0:
      return 500;
    case 1:
      return 1000;
    case 3:
      return 5000;
    case 4:
      return 10000;
    case 2:
    default:
      return 2000;
  }
}

static void
GetZoomDistanceString(TCHAR* str1, TCHAR* str2, unsigned size) {
  double z = GetZoomDistance();
  double z_half = z * 0.5;

  Units::FormatUserDistance(z, str1, size);
  Units::FormatUserDistance(z_half, str2, size);
}

/**
 * Returns the distance to the own plane in pixels
 * @param d Distance in meters to the own plane
 */
static double
RangeScale(double d)
{
  d = d / GetZoomDistance();
  return min(d, 1.0) * radar_size.cx * 0.5;
}

/**
 * Paints the basic info for the selected target on the given canvas
 * @param canvas The canvas to paint on
 */
static void
PaintTrafficInfo(HDC hDC) {
  // Don't paint numbers if no plane selected
  if (selection == -1 || GPS_INFO.FLARM_Traffic[selection].ID <= 0)
    return;

  // Temporary string
  TCHAR tmp[20];
  // Temporary string size
  SIZE sz;
  // Shortcut to the selected traffic
  FLARM_TRAFFIC traffic;
  if (warning >= 0 && GPS_INFO.FLARM_Traffic[warning].ID > 0)
    traffic = GPS_INFO.FLARM_Traffic[warning];
  else
    traffic = GPS_INFO.FLARM_Traffic[selection];

  RECT rc;
  rc.left = min(radar_mid.x - radar_size.cx * 0.5,
                radar_mid.y - radar_size.cy * 0.5);
  rc.top = rc.left;
  rc.right = 2 * radar_mid.x - rc.left;
  rc.bottom = 2 * radar_mid.y - rc.top;

  // Set the text color and background
  switch (traffic.AlarmLevel) {
  case 1:
    SetTextColor(hDC, hcWarning);
    break;
  case 2:
  case 3:
    SetTextColor(hDC, hcAlarm);
    break;
  case 4:
  case 0:
  default:
    SetTextColor(hDC, hcStandard);
    break;
  }
  SelectObject(hDC, TitleSmallWindowFont);
  SetBkMode(hDC, TRANSPARENT);

  // Climb Rate
  if (warning < 0) {
#ifdef FLARM_AVERAGE
    _stprintf(tmp, _T("%+.1f m/s"), traffic.Average30s);
#else
    _stprintf(tmp, _T("%+.1f m/s"), traffic.ClimbRate);
#endif
    GetTextExtentPoint(hDC, tmp, _tcslen(tmp), &sz);
    ExtTextOut(hDC, rc.right - sz.cx, rc.top, 0, NULL, tmp, _tcslen(tmp), NULL);
  }

  // Distance
  Units::FormatUserDistance(sqrt(traffic.RelativeEast * traffic.RelativeEast
      + traffic.RelativeNorth * traffic.RelativeNorth), tmp, 20);
  GetTextExtentPoint(hDC, tmp, _tcslen(tmp), &sz);
  ExtTextOut(hDC, rc.left, rc.bottom - sz.cy, 0, NULL, tmp, _tcslen(tmp), NULL);

  // Relative Height
  Units::FormatUserArrival(traffic.RelativeAltitude, tmp, 20);
  GetTextExtentPoint(hDC, tmp, _tcslen(tmp), &sz);
  ExtTextOut(hDC, rc.right - sz.cx, rc.bottom - sz.cy, 0, NULL, tmp, _tcslen(tmp), NULL);

  // ID / Name
  if (traffic.Name[0] != _T('\0')) {
    SelectObject(hDC, InfoWindowFont);
    if (traffic.AlarmLevel < 1) {
      if (TeamFlarmTracking && traffic.ID == TeamFlarmIdTarget)
        SetTextColor(hDC, hcTeam);
      else
        SetTextColor(hDC, hcSelection);
    }
    _tcscpy(tmp, traffic.Name);
  } else {
    _stprintf(tmp, _T("%lX"), traffic.ID);
  }
  ExtTextOut(hDC, rc.left, rc.top, 0, NULL, tmp, _tcslen(tmp), NULL);
}

/**
 * Paints a "No Traffic" sign on the given canvas
 * @param canvas The canvas to paint on
 */
static void
PaintRadarNoTraffic(HDC hDC) {
  static TCHAR str[] = _T("No Traffic");
  SelectObject(hDC, StatisticsFont);
  SIZE ts;
  GetTextExtentPoint(hDC, str, _tcslen(str), &ts);
  SetTextColor(hDC, hcStandard);
  ExtTextOut(hDC, radar_mid.x - (ts.cx / 2), radar_mid.y - (radar_size.cy / 4),
             0, NULL, str, _tcslen(str), NULL);
}

/**
 * Paints the traffic symbols on the given canvas
 * @param canvas The canvas to paint on
 */
static void
PaintRadarTraffic(HDC hDC) {
  if (!GPS_INFO.FLARM_Available || GPS_INFO.FLARM_RX == 0) {
    PaintRadarNoTraffic(hDC);
    return;
  }

  #define hbWarning CreateSolidBrush(hcWarning)
  #define hbAlarm CreateSolidBrush(hcAlarm)
  #define hbStandard CreateSolidBrush(hcStandard)
  #define hbPassive CreateSolidBrush(hcPassive)
  #define hbSelection CreateSolidBrush(hcSelection)
  #define hbTeam CreateSolidBrush(hcTeam)
  #define hpWarning CreatePen(PS_SOLID, 2 * InfoBoxLayout::scale, hcWarning)
  #define hpAlarm CreatePen(PS_SOLID, 2 * InfoBoxLayout::scale, hcAlarm)
  #define hpStandard CreatePen(PS_SOLID, 2 * InfoBoxLayout::scale, hcStandard)
  #define hpPassive CreatePen(PS_SOLID, 2 * InfoBoxLayout::scale, hcPassive)
  #define hpSelection CreatePen(PS_SOLID, 2 * InfoBoxLayout::scale, hcSelection)

  // Iterate through the traffic
  for (unsigned i = 0; i < FLARM_MAX_TRAFFIC; ++i) {
    const FLARM_TRAFFIC &traffic = GPS_INFO.FLARM_Traffic[i];

    // If FLARM target does not exist -> next one
    if (traffic.ID <= 0)
      continue;

    // Save relative East/North
    double x, y;
    x = traffic.RelativeEast;
    y = -traffic.RelativeNorth;

    // Calculate the distance in meters
    double d = sqrt(x * x + y * y);

    // Calculate the distance in pixels
    double scale = RangeScale(d);

    // x and y are not between 0 and 1 (distance will be handled via scale)
    if (d > 0) {
      x /= d;
      y /= d;
    } else {
      x = 0;
      y = 0;
    }

    // Rotate x and y to have a track up display
    double DisplayAngle = -GPS_INFO.TrackBearing;
    // or use .Heading? (no, because heading is not reliable)
    rotate(x, y, DisplayAngle);   // or use .Heading?

    // Calculate screen coordinates
    POINT sc;
    sc.x = radar_mid.x + iround(x * scale);
    sc.y = radar_mid.y + iround(y * scale);

    // Set the arrow color depending on alarm level
    switch (traffic.AlarmLevel) {
    case 1:
      SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
      SelectObject(hDC, hpWarning);
      Ellipse(hDC, sc.x - 16 * InfoBoxLayout::scale, sc.y - 16 * InfoBoxLayout::scale,
                   sc.x + 16 * InfoBoxLayout::scale, sc.y + 16 * InfoBoxLayout::scale);
      SelectObject(hDC, hbWarning);
      break;
    case 2:
    case 3:
      SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
      SelectObject(hDC, hpAlarm);
      Ellipse(hDC, sc.x - 16 * InfoBoxLayout::scale, sc.y - 16 * InfoBoxLayout::scale,
                   sc.x + 16 * InfoBoxLayout::scale, sc.y + 16 * InfoBoxLayout::scale);
      Ellipse(hDC, sc.x - 19 * InfoBoxLayout::scale, sc.y - 19 * InfoBoxLayout::scale,
                   sc.x + 19 * InfoBoxLayout::scale, sc.y + 19 * InfoBoxLayout::scale);
      SelectObject(hDC, hbAlarm);
      break;
    case 0:
    case 4:
      if (GPS_INFO.FLARM_Traffic[warning].ID > 0) {
        SelectObject(hDC, hbPassive);
        SelectObject(hDC, hpPassive);
      } else {
        if (static_cast<unsigned> (selection) == i) {
          SelectObject(hDC, hpSelection);
          SelectObject(hDC, hbSelection);
        } else {
          SelectObject(hDC, hbStandard);
          SelectObject(hDC, hpStandard);
        }
        if (TeamFlarmTracking && traffic.ID == TeamFlarmIdTarget)
          SelectObject(hDC, hbTeam);
      }
      break;
    }

    // Create an arrow polygon
    POINT Arrow[5];
    Arrow[0].x = -6;
    Arrow[0].y = 8;
    Arrow[1].x = 0;
    Arrow[1].y = -10;
    Arrow[2].x = 6;
    Arrow[2].y = 8;
    Arrow[3].x = 0;
    Arrow[3].y = 5;
    Arrow[4].x = -6;
    Arrow[4].y = 8;

    // Rotate and shift the arrow
    PolygonRotateShift(Arrow, 5, sc.x, sc.y,
                       traffic.TrackBearing + DisplayAngle);

    // Draw the polygon
    Polygon(hDC, Arrow, 5);

#ifdef FLARM_AVERAGE
    // if warning exists -> don't draw vertical speeds
    if (warning >= 0)
      continue;

    // if vertical speed to small or negative -> skip this one
    if (traffic.Average30s < 0.5)
      continue;

    // Select font and color
    SetBkMode(hDC, TRANSPARENT);
    SelectObject(hDC, MapWindowBoldFont);
    if (static_cast<unsigned> (selection) == i)
      SetTextColor(hDC, hcSelection);
    else
      SetTextColor(hDC, hcStandard);

    // Draw vertical speed
    TCHAR tmp[10];
    _stprintf(tmp, _T("%+.1f"), traffic.Average30s);
    SIZE sz;
    GetTextExtentPoint(hDC, tmp, _tcslen(tmp), &sz);
    ExtTextOut(hDC, sc.x + (InfoBoxLayout::scale * 11), sc.y - sz.cy * 0.5,
               0, NULL, tmp, _tcslen(tmp), NULL);

#endif
  }
}

static void line(HDC dc, int ax, int ay, int bx, int by) {
#ifndef NOLINETO
  ::MoveToEx(dc, ax, ay, NULL);
  ::LineTo(dc, bx, by);
#else
  POINT p[2] = {{ax, ay}, {bx, by}};
  polyline(p, 2);
#endif
}

/**
 * Paint a plane symbol in the middle of the radar on the given canvas
 * @param canvas The canvas to paint on
 */
static void
PaintRadarPlane(HDC hDC) {
  #define hpPlane CreatePen(PS_SOLID, 2 * InfoBoxLayout::scale, hcRadar)

  SelectObject(hDC, hpPlane);
  line(hDC, radar_mid.x + (InfoBoxLayout::scale * 10),
            radar_mid.y - (InfoBoxLayout::scale * 2),
            radar_mid.x - (InfoBoxLayout::scale * 10),
            radar_mid.y - (InfoBoxLayout::scale * 2));
  line(hDC, radar_mid.x,
            radar_mid.y - (InfoBoxLayout::scale * 6),
            radar_mid.x,
            radar_mid.y + (InfoBoxLayout::scale * 6));
  line(hDC, radar_mid.x + (InfoBoxLayout::scale * 4),
            radar_mid.y + (InfoBoxLayout::scale * 4),
            radar_mid.x - (InfoBoxLayout::scale * 4),
            radar_mid.y + (InfoBoxLayout::scale * 4));
}

/**
 * Paints the radar circle on the given canvas
 * @param canvas The canvas to paint on
 */
static void
PaintRadarBackground(HDC hDC) {
  #define hpRadar CreatePen(PS_SOLID, 1, hcRadar)

  SelectObject(hDC, GetStockObject(HOLLOW_BRUSH));
  SelectObject(hDC, hpRadar);
  SetTextColor(hDC, hcRadar);

  // Paint circles
  Ellipse(hDC, radar_mid.x - radar_size.cx * 0.5, radar_mid.y - radar_size.cx * 0.5,
               radar_mid.x + radar_size.cx * 0.5, radar_mid.y + radar_size.cx * 0.5);
  Ellipse(hDC, radar_mid.x - radar_size.cx * 0.25, radar_mid.y - radar_size.cx * 0.25,
               radar_mid.x + radar_size.cx * 0.25, radar_mid.y + radar_size.cx * 0.25);

  // Paint zoom strings
  static TCHAR str1[10], str2[10];
  GetZoomDistanceString(str1, str2, 10);
  static SIZE sz1, sz2;
  SelectObject(hDC, MapWindowFont);

  SetBkMode(hDC, OPAQUE);
  GetTextExtentPoint(hDC, str1, _tcslen(str1), &sz1);
  ExtTextOut(hDC, radar_mid.x - sz1.cx / 2,
                  radar_mid.y + radar_size.cx * 0.5 - sz1.cy * 0.75,
                  0, NULL, str1, _tcslen(str1), NULL);

  GetTextExtentPoint(hDC, str2, _tcslen(str2), &sz2);
  ExtTextOut(hDC, radar_mid.x - sz2.cx / 2,
                  radar_mid.y + radar_size.cx * 0.25 - sz2.cy * 0.75,
                  0, NULL, str2, _tcslen(str2), NULL);
  SetBkMode(hDC, TRANSPARENT);
}

/**
 * This function is called when the Radar needs repainting.
 * @param Sender WindowControl that send the "repaint" message
 * @param canvas The canvas to paint on
 */
static void
OnRadarPaint(WindowControl* Sender, HDC hDC)
{
  (void)Sender;
  PaintRadarBackground(hDC);
  PaintRadarPlane(hDC);
  PaintTrafficInfo(hDC);
  PaintRadarTraffic(hDC);
}

static CallBackTableEntry_t CallBackTable[] = {
  DeclareCallBackEntry(OnTimerNotify),
  DeclareCallBackEntry(NULL)
};

/**
 * The function opens the FLARM Traffic dialog
 */
void
dlgFlarmTrafficShowModal()
{
  // Load dialog from XML
  if (InfoBoxLayout::landscape) {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgFlarmTraffic_L.xml"));
    wf = dlgLoadFromXML(CallBackTable, filename, hWndMainWindow, TEXT(
        "IDR_XML_FLARMTRAFFIC_L"));
  } else {
    char filename[MAX_PATH];
    LocalPathS(filename, TEXT("dlgFlarmTraffic.xml"));
    wf = dlgLoadFromXML(CallBackTable, filename, hWndMainWindow, TEXT(
        "IDR_XML_FLARMTRAFFIC"));
  }

  if (!wf)
    return;

  // Set dialog events
  wf->SetKeyDownNotify(FormKeyDown);
  wf->SetTimerNotify(OnTimerNotify);

  // Find Radar frame
  wdf = ((WndOwnerDrawFrame *)wf->FindByName(_T("frmRadar")));
  // Set Radar frame event
  wdf->SetOnPaintNotify(OnRadarPaint);

  // Calculate Radar size
  int size = min(wdf->GetHeight(), wdf->GetWidth());
  radar_size.cx = size - (InfoBoxLayout::scale * 20);
  radar_size.cy = size - (InfoBoxLayout::scale * 20);
  radar_mid.x = wdf->GetWidth() / 2;
  radar_mid.y = wdf->GetHeight() / 2;

  // Set button events
  ((WndButton *)wf->FindByName(_T("cmdZoomIn")))->
      SetOnClickNotify(OnZoomInClicked);
  ((WndButton *)wf->FindByName(_T("cmdZoomOut")))->
      SetOnClickNotify(OnZoomOutClicked);
  ((WndButton *)wf->FindByName(_T("cmdPrev")))->
      SetOnClickNotify(OnPrevClicked);
  ((WndButton *)wf->FindByName(_T("cmdNext")))->
      SetOnClickNotify(OnNextClicked);
  ((WndButton *)wf->FindByName(_T("cmdClose")))->
      SetOnClickNotify(OnCloseClicked);

  // Update Radar and Selection for the first time
  Update();

  // Show the dialog
  wf->ShowModal();

  // After dialog closed -> delete it
  delete wf;
}
