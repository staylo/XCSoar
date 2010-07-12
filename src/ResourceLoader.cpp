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

#include "ResourceLoader.hpp"

#include <assert.h>

#ifdef WIN32

#include <windows.h>

static HINSTANCE ResourceLoaderInstance;

void
ResourceLoader::Init(HINSTANCE hInstance)
{
  assert(ResourceLoaderInstance == NULL);

  ResourceLoaderInstance = hInstance;
}

#else /* !WIN32 */

#include "resource_data.h"

#endif /* !WIN32 */

ResourceLoader::Data
ResourceLoader::Load(unsigned id)
{
#ifdef WIN32
  assert(ResourceLoaderInstance != NULL);

  const TCHAR *name = MAKEINTRESOURCE(id);

  HRSRC resource = ::FindResource(ResourceLoaderInstance, name, RT_BITMAP);
  if (resource == NULL)
    return Data(NULL, 0);

  DWORD size = ::SizeofResource(ResourceLoaderInstance, resource);
  if (size == 0)
    return Data(NULL, 0);

  HGLOBAL handle = ::LoadResource(ResourceLoaderInstance, resource);
  if (handle == NULL)
    return Data(NULL, 0);

  LPVOID data = LockResource(handle);
  if (data == NULL)
    return Data(NULL, 0);

  return std::pair<const void *, size_t>(data, size);
#else

  for (unsigned i = 0; numeric_resources[i].data != NULL; ++i)
    if (numeric_resources[i].id == id)
      return Data(numeric_resources[i].data, numeric_resources[i].size);

  return Data(NULL, 0);
#endif
}

#ifdef WIN32
HBITMAP
ResourceLoader::LoadBitmap2(unsigned id)
{
  return ::LoadBitmap(ResourceLoaderInstance, MAKEINTRESOURCE(id));
}
#endif

#ifdef HAVE_AYGSHELL_DLL
#include "OS/AYGShellDLL.hpp"

HBITMAP
ResourceLoader::SHLoadImageResource(unsigned id)
{
  const AYGShellDLL ayg_shell_dll;
  return ayg_shell_dll.SHLoadImageResource(ResourceLoaderInstance, id);
}
#endif
