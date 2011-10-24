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

#include "Dialogs/Waypoint.hpp"
#include "Dialogs/Internal.hpp"
#include "Engine/Task/TaskEvents.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Protection.hpp"
#include "LocalTime.hpp"
#include "LocalPath.hpp"
#include "Blackboard.hpp"
#include "SettingsComputer.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Math/Earth.hpp"
#include "Math/SunEphemeris.hpp"
#include "Math/FastMath.h"
#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Components.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Task/TaskManager.hpp"
#include "Task/MapTaskManager.hpp"
#include "Task/Tasks/TaskSolvers/TaskSolution.hpp"
#include "Task/Tasks/BaseTask/UnorderedTaskPoint.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Waypoint/WaypointGlue.hpp"
#include "Compiler.h"
#include "Compatibility/path.h"
#include "InputEvents.hpp"
#include "NMEA/Aircraft.hpp"
#include "Units/UnitsFormatter.hpp"
#ifdef ANDROID
#include "Android/NativeView.hpp"
#include "Android/Main.hpp"
#endif

#include <assert.h>
#include <stdio.h>


static int page = 0;
static WndForm *wf = NULL;
static WndFrame *wDetails = NULL;
static WndFrame *wInfo = NULL;
static WndFrame *wCommand = NULL;
static WndOwnerDrawFrame *wImage = NULL;
static WndButton *wMagnify = NULL;
static WndButton *wShrink = NULL;
static const Waypoint *selected_waypoint = NULL;

static StaticArray<Bitmap, 5> images;
static int zoom = 0;

static void
NextPage(int Step)
{
  assert(selected_waypoint);
  int last_page = 2 + images.size();

  do {
    page += Step;
    if (page < 0)
      page = last_page;
    else if (page > last_page)
      page = 0;
  } while (page == 1 &&
           selected_waypoint->details.empty() &&
           selected_waypoint->files_external.empty());

  wInfo->set_visible(page == 0);
  wDetails->set_visible(page == 1);
  wCommand->set_visible(page == 2);
  wImage->set_visible(page >= 3);
  zoom = 0;
  wMagnify->set_visible(page >= 3);
  wMagnify->set_enabled(true);
  wShrink->set_visible(page >= 3);
  wShrink->set_enabled(false);
}

static void
OnMagnifyClicked(gcc_unused WndButton &button)
{
  if (zoom >= 5)
    return;
  zoom++;

  wMagnify->set_enabled(zoom < 5);
  wShrink->set_enabled(zoom > 0);
  wImage->invalidate();
}

static void
OnShrinkClicked(gcc_unused WndButton &button)
{
  if (zoom <= 0)
    return;
  zoom--;

  wMagnify->set_enabled(zoom < 5);
  wShrink->set_enabled(zoom > 0);
  wImage->invalidate();
}

static void
OnNextClicked(gcc_unused WndButton &button)
{
  NextPage(+1);
}

static void
OnPrevClicked(gcc_unused WndButton &button)
{
  NextPage(-1);
}

static void
OnCloseClicked(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static bool
FormKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  switch (key_code) {
  case VK_LEFT:
#ifdef GNAV
  case '6':
#endif
    ((WndButton *)wf->FindByName(_T("cmdPrev")))->set_focus();
    NextPage(-1);
    return true;

  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    ((WndButton *)wf->FindByName(_T("cmdNext")))->set_focus();
    NextPage(+1);
    return true;

  default:
    return false;
  }
}

static void
OnGotoClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  assert(selected_waypoint != NULL);

  protected_task_manager->DoGoto(*selected_waypoint);
  wf->SetModalResult(mrOK);

  CommonInterface::main_window.full_redraw();
}

static void
OnReplaceClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::replace_in_task(*selected_waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
    MessageBoxX(_("No active task point."), _("Replace in task"),
                MB_OK | MB_ICONINFORMATION);
    break;

  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_TO_GOTO:
  case MapTaskManager::MUTATED_FROM_GOTO:
    break;
  }
}

static void 
OnNewHomeClicked(gcc_unused WndButton &button)
{
  assert(selected_waypoint != NULL);

  SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SetSettingsComputer();

  settings_computer.SetHome(*selected_waypoint);

  {
    ScopeSuspendAllThreads suspend;
    WaypointGlue::SetHome(way_points, terrain,
                          settings_computer,
                          false);
  }

  wf->SetModalResult(mrOK);
}

static void
OnInsertInTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::insert_in_task(*selected_waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;

  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_TO_GOTO:
    MessageBoxX(_("Created Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::MUTATED_FROM_GOTO:
    MessageBoxX(_("Created 2-point task from Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  }
}

static void
OnAppendInTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::append_to_task(*selected_waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_TO_GOTO:
    MessageBoxX(_("Created Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::MUTATED_FROM_GOTO:
    MessageBoxX(_("Created 2-point task from Goto Task."), _("Success"),
                MB_OK | MB_ICONEXCLAMATION);
    wf->SetModalResult(mrOK);
    break;
  }
}

#if 0
// JMW disabled until 6.2 work, see #996
static task_edit_result
goto_and_clear_task(const Waypoint &wp)
{
  if (protected_task_manager == NULL)
    return INVALID;

  protected_task_manager->DoGoto(wp);
  TaskEvents task_events;
  const OrderedTask blank(task_events,
                          XCSoarInterface::SettingsComputer(),
                          XCSoarInterface::Calculated().glide_polar_task);
  protected_task_manager->task_commit(blank);

  return SUCCESS;
}

static unsigned
ordered_task_size()
{
  assert(protected_task_manager != NULL);
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
  const OrderedTask &ot = task_manager->get_ordered_task();
  if (ot.check_task())
    return ot.TaskSize();

  return 0;
}

static void
OnGotoAndClearTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  if ((ordered_task_size() > 2) && MessageBoxX(_("Clear current task?"),
                        _("Goto and clear task"), MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  switch (goto_and_clear_task(*selected_waypoint)) {
  case SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case NOTASK:
  case UNMODIFIED:
  case INVALID:
    MessageBoxX(_("Unknown error creating task."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  }
}
#endif

static void
OnRemoveFromTaskClicked(gcc_unused WndButton &button)
{
  if (protected_task_manager == NULL)
    return;

  switch (MapTaskManager::remove_from_task(*selected_waypoint)) {
  case MapTaskManager::SUCCESS:
    protected_task_manager->TaskSaveDefault();
    wf->SetModalResult(mrOK);
    break;
  case MapTaskManager::NOTASK:
    MessageBoxX(_("No task defined."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::UNMODIFIED:
    MessageBoxX(_("Waypoint not in task."), _("Remove from task"),
                MB_OK | MB_ICONINFORMATION);
    break;

  case MapTaskManager::INVALID:
    MessageBoxX(_("Task would not be valid after the change."), _("Error"),
                MB_OK | MB_ICONEXCLAMATION);
    break;
  case MapTaskManager::MUTATED_FROM_GOTO:
  case MapTaskManager::MUTATED_TO_GOTO:
    break;
  }
}

static void
OnActivatePanClicked(gcc_unused WndButton &button)
{
  GlueMapWindow *map_window = CommonInterface::main_window.map;
  if (map_window == NULL)
    return;

  map_window->PanTo(selected_waypoint->location);
  XCSoarInterface::main_window.SetFullScreen(true);
  InputEvents::setMode(InputEvents::MODE_PAN);
  wf->SetModalResult(mrOK);
}

static void
OnImagePaint(gcc_unused WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  canvas.clear_white();
  if (page >= 3 && page < 3 + (int)images.size()) {
    Bitmap &img = images[page-3];
    static const int zoom_factors[] = { 1, 2, 4, 8, 16, 32 };
    RasterPoint img_pos, screen_pos;
    PixelSize screen_size;
    PixelSize img_size = img.get_size();
    fixed scale = std::min((fixed)canvas.get_width() / (fixed)img_size.cx,
                           (fixed)canvas.get_height() / (fixed)img_size.cy) *
                  zoom_factors[zoom];

    // centered image and optionally zoomed into the center of the image
    fixed scaled_size = img_size.cx * scale;
    if (scaled_size <= (fixed)canvas.get_width()) {
      img_pos.x = 0;
      screen_pos.x = (int) (((fixed)canvas.get_width() - scaled_size) / 2);
      screen_size.cx = (int) scaled_size;
    } else {
      scaled_size = (fixed)canvas.get_width() / scale;
      img_pos.x = (int) (((fixed)img_size.cx - scaled_size) / 2);
      img_size.cx = (int) scaled_size;
      screen_pos.x = 0;
      screen_size.cx = canvas.get_width();
    }
    scaled_size = img_size.cy * scale;
    if (scaled_size <= (fixed)canvas.get_height()) {
      img_pos.y = 0;
      screen_pos.y = (int) (((fixed)canvas.get_height() - scaled_size) / 2);
      screen_size.cy = (int) scaled_size;
    } else {
      scaled_size = (fixed)canvas.get_height() / scale;
      img_pos.y = (int) (((fixed)img_size.cy - scaled_size) / 2);
      img_size.cy = (int) scaled_size;
      screen_pos.y = 0;
      screen_size.cy = canvas.get_height();
    }
    canvas.stretch(screen_pos.x, screen_pos.y, screen_size.cx, screen_size.cy,
                   img, img_pos.x, img_pos.y, img_size.cx, img_size.cy);
  }
}

static void
OnFileListEnter(gcc_unused unsigned i)
{
  if (i < selected_waypoint->files_external.size()) {
    TCHAR path[MAX_PATH];
    LocalPath(path, selected_waypoint->files_external[i].c_str());

    // TODO: support other platforms
#ifdef ANDROID
    native_view->openFile(path);
#endif
  }
}

static void
OnFileListItemPaint(Canvas &canvas, const PixelRect paint_rc, unsigned i)
{
  canvas.text(paint_rc.left + Layout::Scale(2),
              paint_rc.top + Layout::Scale(2),
              selected_waypoint->files_external[i].c_str());
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
    DeclareCallBackEntry(OnMagnifyClicked),
    DeclareCallBackEntry(OnShrinkClicked),
    DeclareCallBackEntry(OnNextClicked),
    DeclareCallBackEntry(OnPrevClicked),
    DeclareCallBackEntry(OnGotoClicked),
    DeclareCallBackEntry(OnCloseClicked),
    DeclareCallBackEntry(OnReplaceClicked),
    DeclareCallBackEntry(OnInsertInTaskClicked),
    DeclareCallBackEntry(OnAppendInTaskClicked),
    DeclareCallBackEntry(OnRemoveFromTaskClicked),
    DeclareCallBackEntry(OnNewHomeClicked),
    DeclareCallBackEntry(OnActivatePanClicked),
    DeclareCallBackEntry(OnImagePaint),
    DeclareCallBackEntry(NULL)
};

static void
ShowTaskCommands()
{
  if (protected_task_manager == NULL)
    return;

  WndButton *wb = ((WndButton *)wf->FindByName(_T("cmdRemoveFromTask")));
  if (wb)
    wb->set_visible(MapTaskManager::index_of_point_in_task(*selected_waypoint) >= 0);
}

void 
dlgWaypointDetailsShowModal(SingleWindow &parent, const Waypoint& way_point,
                            bool allow_navigation)
{
  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const SETTINGS_COMPUTER &settings_computer =
    CommonInterface::SettingsComputer();

  selected_waypoint = &way_point;

  wf = LoadDialog(CallBackTable, parent,
                  Layout::landscape ? _T("IDR_XML_WAYPOINTDETAILS_L") :
                                      _T("IDR_XML_WAYPOINTDETAILS"));
  assert(wf != NULL);

  TCHAR sTmp[128];
  _stprintf(sTmp, _T("%s: '%s'"), wf->GetCaption(), selected_waypoint->name.c_str());
  wf->SetCaption(sTmp);

  WndProperty *wp;
  wp = ((WndProperty *)wf->FindByName(_T("prpWpComment")));
  wp->SetText(selected_waypoint->comment.c_str());

  TCHAR *radio_frequency;
  if (selected_waypoint->radio_frequency.IsDefined() &&
      (radio_frequency =
       selected_waypoint->radio_frequency.Format(sTmp, 128)) != NULL)
    ((WndProperty *)wf->FindByName(_T("Radio")))->SetText(radio_frequency);

  const Runway &runway = selected_waypoint->runway;
  if (runway.IsDirectionDefined()) {
    _stprintf(sTmp, _T("%02u"), runway.GetDirectionName());
    if (runway.IsLengthDefined()) {
      _tcscat(sTmp, _T("; "));
      Units::FormatUserDistance(fixed(runway.GetLength()),
                                sTmp + _tcslen(sTmp), 128 - _tcslen(sTmp));
    }

    ((WndProperty *)wf->FindByName(_T("Runway")))->SetText(sTmp);
  }

  TCHAR *location = Units::FormatGeoPoint(selected_waypoint->location,
                                          sTmp, 128);
  if (location != NULL)
    ((WndProperty *)wf->FindByName(_T("Location")))->SetText(location);

  Units::FormatUserAltitude(selected_waypoint->altitude, sTmp, sizeof(sTmp)-1);
  ((WndProperty *)wf->FindByName(_T("prpAltitude")))
    ->SetText(sTmp);

  if (basic.connected) {
    SunEphemeris sun;
    sun.CalcSunTimes(selected_waypoint->location,
                     basic.date_time_utc,
                     fixed(GetUTCOffset()) / 3600);

    int sunsethours = (int)sun.TimeOfSunSet;
    int sunsetmins = (int)((sun.TimeOfSunSet - fixed(sunsethours)) * 60);

    _stprintf(sTmp, _T("%02d:%02d"), sunsethours, sunsetmins);
    ((WndProperty *)wf->FindByName(_T("prpSunset")))->SetText(sTmp);
  }

  if (basic.location_available) {
    GeoVector gv = basic.location.DistanceBearing(selected_waypoint->location);

    TCHAR DistanceText[MAX_PATH];
    Units::FormatUserDistance(gv.distance, DistanceText, 10);

    _sntprintf(sTmp, 128, _T("%d")_T(DEG)_T(" %s"),
               iround(gv.bearing.Degrees()),
               DistanceText);
    ((WndProperty *)wf->FindByName(_T("BearingDistance")))->SetText(sTmp);
  }

  if (protected_task_manager != NULL) {
    GlidePolar glide_polar = settings_computer.glide_polar_task;
    const GlidePolar &safety_polar = calculated.glide_polar_safety;

    UnorderedTaskPoint t(way_point, settings_computer.task);

    // alt reqd at current mc

    const AircraftState aircraft_state = ToAircraftState(basic, calculated);
    GlideResult r = TaskSolution::glide_solution_remaining(t, aircraft_state,
                                                           glide_polar);
    wp = (WndProperty *)wf->FindByName(_T("prpMc2"));
    if (wp) {
      _stprintf(sTmp, _T("%.0f %s"),
                (double)Units::ToUserAltitude(r.altitude_difference),
                Units::GetAltitudeName());
      wp->SetText(sTmp);
    }

    // alt reqd at mc 0

    glide_polar.SetMC(fixed_zero);
    r = TaskSolution::glide_solution_remaining(t, aircraft_state, glide_polar);
    wp = (WndProperty *)wf->FindByName(_T("prpMc0"));
    if (wp) {
      _stprintf(sTmp, _T("%.0f %s"),
                (double)Units::ToUserAltitude(r.altitude_difference),
                Units::GetAltitudeName());
      wp->SetText(sTmp);
    }

    // alt reqd at safety mc

    r = TaskSolution::glide_solution_remaining(t, aircraft_state, safety_polar);
    wp = (WndProperty *)wf->FindByName(_T("prpMc1"));
    if (wp) {
      _stprintf(sTmp, _T("%.0f %s"),
                (double)Units::ToUserAltitude(r.altitude_difference),
                Units::GetAltitudeName());
      wp->SetText(sTmp);
    }
  }

  wf->SetKeyDownNotify(FormKeyDown);

  wInfo = (WndFrame *)wf->FindByName(_T("frmInfos"));
  wCommand = (WndFrame *)wf->FindByName(_T("frmCommands"));
  wDetails = (WndFrame *)wf->FindByName(_T("frmDetails"));
  wImage = (WndOwnerDrawFrame *)wf->FindByName(_T("frmImage"));
  wMagnify = (WndButton *)wf->FindByName(_T("cmdMagnify"));
  wShrink = (WndButton *)wf->FindByName(_T("cmdShrink"));

  WndListFrame *wFilesList = (WndListFrame *)wf->FindByName(_T("Files"));
  WndProperty *wDetailsText = (WndProperty *)wf->FindByName(_T("Details"));

  assert(wInfo != NULL);
  assert(wCommand != NULL);
  assert(wDetails != NULL);
  assert(wImage != NULL);
  assert(wMagnify != NULL);
  assert(wShrink != NULL);

  assert(wFilesList != NULL);
  assert(wDetailsText != NULL);

  int num_files = selected_waypoint->files_external.size();
  if (num_files > 0) {
    wFilesList->SetPaintItemCallback(OnFileListItemPaint);
    wFilesList->SetCursorCallback(OnFileListEnter);
    wFilesList->SetActivateCallback(OnFileListEnter);

    unsigned list_height = wFilesList->GetItemHeight() * std::min(num_files, 5);
    wFilesList->resize(wFilesList->get_width(), list_height);
    wFilesList->SetLength(num_files);

    PixelRect rc = wDetailsText->get_position();
    rc.top += list_height + Layout::Scale(2);
    wDetailsText->move(rc);
  }
  wDetailsText->SetText(selected_waypoint->details.c_str(), true);
  wCommand->hide();

  if (!allow_navigation) {
    WndButton* butnav = NULL;
    butnav = (WndButton *)wf->FindByName(_T("cmdPrev"));
    assert(butnav);
    butnav->hide();
    butnav = (WndButton *)wf->FindByName(_T("cmdNext"));
    assert(butnav);
    butnav->hide();
    butnav = (WndButton *)wf->FindByName(_T("cmdGoto"));
    assert(butnav);
    butnav->hide();
  }

  ShowTaskCommands();

  std::vector<tstring>::const_iterator it;
  for (it = selected_waypoint->files_embed.begin();
       it < selected_waypoint->files_embed.end() && !images.full();
       it++) {
    TCHAR path[MAX_PATH];
    LocalPath(path, it->c_str());
    if (!images.append().load_file(path))
      images.shrink(images.size() - 1);
  }

  page = 0;

  NextPage(0); // JMW just to turn proper pages on/off

  wf->ShowModal();

  delete wf;

  for (Bitmap *bmp = images.begin(); bmp < images.end(); bmp++)
    bmp->reset();
  images.clear();
}
