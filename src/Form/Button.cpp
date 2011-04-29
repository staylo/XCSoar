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

#include "Form/Button.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Key.h"
#include "Screen/Color.hpp"
#include "Asset.hpp"

ButtonWindowStyle
WndButton::custom_painting(ButtonWindowStyle style) const {
  if (!has_pointer()) {
    style.enable_custom_painting();
  }
  return style;
}

WndButton::WndButton(ContainerWindow &parent,
                     const TCHAR *Caption, int X, int Y, int Width, int Height,
                     const ButtonWindowStyle style,
                     Color _background_color,
    ClickNotifyCallback_t Function,
    LeftRightNotifyCallback_t LeftFunction,
    LeftRightNotifyCallback_t RightFunction) :
    mOnClickNotify(Function),
    mOnLeftNotify(LeftFunction),
    mOnRightNotify(RightFunction),
    background_color(_background_color)
{
  set(parent, Caption, X, Y, Width, Height, custom_painting(style));
  set_font(Fonts::MapBold);
}

bool
WndButton::on_clicked()
{
  // Call the OnClick function
  if (mOnClickNotify != NULL) {
    mOnClickNotify(*this);
    return true;
  }

  return false;
}

bool
WndButton::on_left()
{
  // call on Left key function
  if (mOnLeftNotify != NULL) {
    mOnLeftNotify(*this);
    return true;
  }
  return false;
}

bool
WndButton::on_right()
{
  // call on Left key function
  if (mOnRightNotify != NULL) {
    mOnRightNotify(*this);
    return true;
  }
  return false;
}

bool
WndButton::on_key_check(unsigned key_code) const
{
  switch (key_code) {
  case VK_RETURN:
    return true;

  case VK_LEFT:
    if (mOnLeftNotify)
      return true;

  case VK_RIGHT:
    if (mOnRightNotify)
      return true;

  default:
    return false;
  }
}

bool
WndButton::on_key_down(unsigned key_code)
{
  switch (key_code) {
#ifdef GNAV
  // JMW added this to make data entry easier
  case VK_F4:
#endif
  case VK_RETURN:
    return on_clicked();

  case VK_LEFT:
    return on_left();

  case VK_RIGHT:
    return on_right();
  }

  return ButtonWindow::on_key_down(key_code);
}

void
WndButton::on_paint(Canvas &canvas)
{
  // Get button PixelRect and shrink it to make room for the selector/focus
  PixelRect rc = get_client_rect();

  // Draw button to the background
  canvas.draw_button(rc, is_down());

  // If button has text on it
  tstring caption = get_text();
  if (caption.empty())
    return;

  canvas.null_pen();
  canvas.black_brush();

  // Setup drawing of text
  canvas.set_text_color(is_enabled() ? Color::BLACK : Color::GRAY);
  canvas.background_transparent();
  canvas.select(Fonts::MapBold);

  PixelRect focus_rc = rc;
  InflateRect(&focus_rc, -3, -3);

  // Draw focus rectangle
  if (has_focus()) {
    canvas.fill_focus(focus_rc, background_color);
    canvas.draw_focus(focus_rc);
  }

  // If button is pressed, offset the text for 3D effect
  if (is_down())
    OffsetRect(&rc, 1, 1);

  // Draw text centered
  PixelSize tsize = canvas.text_size(caption.c_str());
  RasterPoint org;
  org.x = (rc.right+rc.left-tsize.cx)/2;
  org.y = (rc.top+rc.bottom-tsize.cy)/2;
  canvas.text(org.x, org.y, caption.c_str());
}
