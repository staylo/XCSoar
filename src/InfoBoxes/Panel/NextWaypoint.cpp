/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "NextWaypoint.hpp"
#include "Base.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Look/MapLook.hpp"
#include "Form/List.hpp"
#include "Form/WindowWidget.hpp"
#include "Screen/Layout.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Renderer/OZPreviewRenderer.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Dialogs/dlgTaskHelpers.hpp"
#include "Formatter/UserUnits.hpp"
#include "Look/TaskLook.hpp"
#include "Form/ActionListener.hpp"
#include "Form/Button.hpp"
#include "Form/SymbolButton.hpp"
#include "Util/Macros.hpp"
#include "Math/fixed.hpp"
#include "Task/Points/TaskWaypoint.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Input/InputEvents.hpp"
#include "Util/StaticString.hpp"


enum ControlIndex {
  NextWaypoint,
  PreviousWaypoint,
  CloseDialog,
  TaskResume,
};

enum TaskDialogStates {
  TaskEmpty,
  TaskAbortOrGoto,
  TaskOrderedValid,
};

/**
 * Returns state of the task as it effects the widget.  This is based on the
 * mode of the task and the size of the task
 */
TaskDialogStates GetTaskDialogState();

class NextWaypointWidget : public BaseAccessPanel, RatchetListLayout,
  private ListControl::Handler
{
protected:
  /**
   * the list and buttons use the rectangles calculated in the RatchetListLayout
   * class
   */
  ListControl *list;
  WndButton *close, *task_resume;
  WndSymbolButton *prev, *next;
  const OrderedTask *task;
  unsigned waypoint_index;
  TaskDialogStates prepared_state;

public:
  NextWaypointWidget(unsigned id)
    :BaseAccessPanel(id), task(NULL) {}

protected:
  void RefreshList();
  void RatchetWaypoint(int offset);
  void InitTask();
  void CreateList(ContainerWindow &parent, const DialogLook &look,
                  const PixelRect &rc, UPixelScalar row_height);
  void CreateButtons(ContainerWindow &parent, const PixelRect &rc,
                     const DialogLook &dialog_look);
  const Waypoint& GetCurrentWaypoint();

  /**
   * prepare the task point selector list and buttons
   */
  void PrepareOrderedValid(ContainerWindow &parent, const PixelRect &rc);

  /**
   * prepare only a TaskResume button centered in screen
   */
  void PrepareAbortOrGoto(ContainerWindow &parent, const PixelRect &rc);

public:
  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual void Unprepare();

  /* virtual methods from class List::Handler */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx);
  virtual bool CanActivateItem(unsigned index) const;
  virtual void OnActivateItem(unsigned index);
  virtual void OnCursorMoved(unsigned index);

  /* virtual methods from class ActionListener (for buttons) */
  virtual void OnAction(int id);
};

void
NextWaypointWidget::InitTask()
{
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
  if (task_manager->GetMode() != TaskManager::MODE_ORDERED)
    return; // this is not impossible, but extremely unlikely

  const TaskBehaviour &tb = task_manager->GetTaskBehaviour();
  task = task_manager->Clone(tb);
  waypoint_index = task_manager->GetActiveTaskPointIndex();
}

void
NextWaypointWidget::Unprepare()
{
  switch (prepared_state) {
  case TaskEmpty:
    assert(false);
    break;
  case TaskAbortOrGoto:
    delete task_resume;
    break;
  case TaskOrderedValid:
    delete list;
    delete prev;
    delete next;
    delete close;
    if (task != NULL)
      delete task;
    break;
  }
  BaseAccessPanel::Unprepare();
}

void
NextWaypointWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  BaseAccessPanel::Prepare(parent, rc);
  RatchetListLayout::Prepare(parent, content_rc);
  prepared_state = GetTaskDialogState();
  switch (prepared_state) {
  case TaskEmpty:
    assert(false);
    break;
  case TaskAbortOrGoto:
    PrepareAbortOrGoto(parent, content_rc);
    break;
  case TaskOrderedValid:
    PrepareOrderedValid(parent, content_rc);
    break;
  }
}

void
NextWaypointWidget::PrepareAbortOrGoto(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  ButtonWindowStyle button_style;
  button_style.TabStop();
  button_style.multiline();

  PixelSize resume_sz;
  resume_sz.cx = Layout::Scale(120);
  resume_sz.cy = Layout::Scale(50);
  PixelRect resume_rc;
  resume_rc.left = (rc.right - rc.left - resume_sz.cx) / 2;
  resume_rc.right = resume_rc.left + resume_sz.cx;
  resume_rc.top = (rc.bottom - rc.top - resume_sz.cy) / 2;
  resume_rc.bottom = resume_rc.top + resume_sz.cy;

  task_resume = new WndButton(GetClientAreaWindow(), look,
                              _T("Resume Task"), resume_rc,
                              button_style,
                              this, TaskResume);
}

void
NextWaypointWidget::PrepareOrderedValid(ContainerWindow &parent, const PixelRect &rc)
{
  InitTask();

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  UPixelScalar margin = Layout::Scale(2);
  UPixelScalar font_height = dialog_look.list.font->GetHeight();
  CreateList(GetClientAreaWindow(), dialog_look, content_rc,
             3 * margin + 2 * font_height);

  RefreshList();

  CreateButtons(GetClientAreaWindow(), rc, dialog_look);
}

void
NextWaypointWidget::CreateList(ContainerWindow &parent, const DialogLook &look,
                               const PixelRect &rc, UPixelScalar row_height)
{
  WindowStyle list_style;
  list_style.TabStop();
  list_style.Border();

  list = new ListControl(parent, look, ratchet_list_rc, list_style, row_height);
  list->SetHandler(this);
  list->SetHasScrollBar(false);
  list->SetLength(task->TaskSize());

  assert(list);
}

void
NextWaypointWidget::CreateButtons(ContainerWindow &parent, const PixelRect &rc,
                                  const DialogLook &dialog_look)
{
  ButtonWindowStyle button_style;
  button_style.TabStop();
  button_style.multiline();

  prev = new WndSymbolButton(parent, dialog_look,
                             _T("^"), ratchet_up_rc,
                             button_style,
                             this, PreviousWaypoint);

  next = new WndSymbolButton(parent, dialog_look,
                             _T("v"), ratchet_down_rc,
                             button_style,
                             this, NextWaypoint);

  close = new WndButton(parent, dialog_look,
                          _("Close"), details_button_rc,
                          button_style,
                          this, CloseDialog);
}

void
NextWaypointWidget::RefreshList()
{
  list->SetCursorIndex(waypoint_index);
  list->Invalidate();
}

void
NextWaypointWidget::RatchetWaypoint(int offset)
{
  if (task->TaskSize() == 0u) {
    waypoint_index = 0;
    return;
  }
  unsigned old_index = list->GetCursorIndex();
  unsigned i = max(0, (int)old_index + offset);
  waypoint_index = min(i, task->TaskSize() - 1u);
  RefreshList();
}

void
NextWaypointWidget::OnActivateItem(unsigned index)
{
  Close();
}

bool
NextWaypointWidget::CanActivateItem(unsigned index) const {
  return true;
}

void
NextWaypointWidget::OnCursorMoved(unsigned index)
{
  assert(index < task->TaskSize());

  ProtectedTaskManager::ExclusiveLease task_manager(*protected_task_manager);
  if (task_manager->GetMode() != TaskManager::MODE_ORDERED)
    return;
  waypoint_index = index;
  task_manager->SetActiveTaskPoint(index);
}

void
NextWaypointWidget::OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  assert(i < task->TaskSize());

  const PixelScalar line_height = rc.bottom - rc.top;

  StaticString<120> buffer;

  const MapLook &map_look = UIGlobals::GetMapLook();
  const DialogLook &dialog_look = UIGlobals::GetDialogLook();
  const Font &name_font = *dialog_look.list.font;
  const Font &small_font = *dialog_look.small_font;

  const OrderedTaskPoint &tp = task->GetTaskPoint(i);
  GeoVector leg = tp.GetNominalLegVector();
  bool show_leg_info = leg.distance > fixed(0.01);

  // Draw icon
  const RasterPoint pt = {
    PixelScalar(rc.left + line_height / 2),
    PixelScalar(rc.top + line_height / 2),
  };

  PixelScalar radius = std::min(PixelScalar(line_height / 2
                                            - Layout::FastScale(4)),
                                Layout::FastScale(10));

  OZPreviewRenderer::Draw(canvas, tp.GetObservationZone(),
                          pt, radius, map_look.task,
                          CommonInterface::GetMapSettings().airspace,
                          map_look.airspace);

  // Y-Coordinate of the second row
  PixelScalar top2 = rc.top + name_font.GetHeight() + Layout::FastScale(4);

  // Use small font for details
  canvas.Select(small_font);
  canvas.SetTextColor(COLOR_BLACK);

  UPixelScalar leg_info_width = 0;
  if (show_leg_info) {
    // Draw leg distance
    FormatUserDistanceSmart(leg.distance, buffer.buffer(), true);
    UPixelScalar width = leg_info_width = canvas.CalcTextWidth(buffer.c_str());
    canvas.text(rc.right - Layout::FastScale(2) - width,
                rc.top + Layout::FastScale(2) +
                (name_font.GetHeight() - small_font.GetHeight()) / 2, buffer.c_str());

    // Draw leg bearing
    FormatBearing(buffer.buffer(), buffer.MAX_SIZE, leg.bearing);
    width = canvas.CalcTextWidth(buffer.c_str());
    canvas.text(rc.right - Layout::FastScale(2) - width, top2, buffer.c_str());

    if (width > leg_info_width)
      leg_info_width = width;

    leg_info_width += Layout::FastScale(2);
  }

  // Draw details line
  PixelScalar left = rc.left + line_height + Layout::FastScale(2);
  OrderedTaskPointRadiusLabel(tp.GetObservationZone(), buffer.buffer());
  if (!StringIsEmpty(buffer.c_str()))
    canvas.text_clipped(left, top2, rc.right - leg_info_width - left, buffer.c_str());

  // Draw turnpoint name
  canvas.Select(name_font);
  OrderedTaskPointLabel(tp.GetType(), tp.GetWaypoint().name.c_str(),
                        i, buffer.buffer());
  canvas.text_clipped(left, rc.top + Layout::FastScale(2),
                      rc.right - leg_info_width - left, buffer.c_str());
}

const Waypoint&
NextWaypointWidget::GetCurrentWaypoint()
{
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
  return task_manager->GetActiveTaskPoint()->GetWaypoint();
}

TaskDialogStates
GetTaskDialogState()
{
  ProtectedTaskManager::Lease task_manager(*protected_task_manager);
  switch (task_manager->GetMode()) {
  case TaskManager::MODE_ABORT:
  case TaskManager::MODE_GOTO:
    return TaskAbortOrGoto;
    break;
  case TaskManager::MODE_NULL:
    return TaskEmpty;
    break;
  case TaskManager::MODE_ORDERED:
    if (task_manager->TaskSize() == 0)
      return TaskEmpty;
    else
      return TaskOrderedValid;
     break;
  }
  assert(false);
  return TaskOrderedValid;
}

void
NextWaypointWidget::OnAction(int id)
{
  switch (id) {
  case NextWaypoint:
    RatchetWaypoint(1);
    break;

  case PreviousWaypoint:
    RatchetWaypoint(-1);
    break;

  case CloseDialog:
  {
    Close();
    break;
  }
  case TaskResume:
    InputEvents::eventAbortTask(_T("resume"));
    Close();
    break;

  default:
    BaseAccessPanel::OnAction(id);
  }
}

Widget *
LoadNextWaypointWidget(unsigned id)
{
  if (GetTaskDialogState() == TaskEmpty)
    return NULL;
  else
    return new NextWaypointWidget(id);
}
