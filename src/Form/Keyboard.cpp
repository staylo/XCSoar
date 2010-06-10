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

#include "Form/Keyboard.hpp"

#include "Form/Form.hpp"
#include "Form/Container.hpp"
#include "Dialogs/Message.hpp"

#include <tchar.h>

static void
OnButtonClicked(WndButton &button)
{
  MessageBoxX(button.GetCaption(), _T("buttonclick"), MB_OK);
}

KeyboardControl::KeyboardControl(WndForm &form, ContainerControl *owner,
                                 int x, int y, unsigned width, unsigned height,
                                 const WindowStyle _style)
  :PanelControl(owner, x, y, width, height, _style)
{
  TCHAR letters[] = _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

  for (unsigned i = 0; i < sizeof(letters) / sizeof(letters[0]) - 1; i++) {
    TCHAR caption[2];
    _tcsncpy(caption, letters + i, 1);
    caption[1] = 0;

    TCHAR name[5] = _T("cmd");
    _tcsncat(name, letters + i, 1);
    name[4] = 0;

    add_button(form, owner, name, caption, OnButtonClicked);
  }

  add_button(form, owner, _T("cmdSpace"), _T(" Space "));
  add_button(form, owner, _T("cmdPeriod"), _T("."));
  add_button(form, owner, _T("cmdComma"), _T(","));
  add_button(form, owner, _T("cmdMinus"), _T("-"));
  add_button(form, owner, _T("cmdBackspace"), _T("<-"));
}

void KeyboardControl::add_button(WndForm &form, ContainerControl *owner,
                                 const TCHAR* name, const TCHAR* caption,
                                 WndButton::ClickNotifyCallback_t Function)
{
  WindowStyle style;
  style.tab_stop();

  WndButton *button = NULL;
  button = new WndButton(this, caption, 0, 0, 50, 50, style, Function);
  button->SetFont(this->GetFont());
  form.AddNamed(name, button);
  form.AddDestruct(button);
}
