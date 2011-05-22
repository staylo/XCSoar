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

#include "Airspace/AirspaceRendererSettings.hpp"

#include <algorithm>

void
AirspaceRendererSettings::SetDefaults()
{
  enable = true;
  black_outline = false;
  altitude_mode = ALLON;
  clip_altitude = 1000;

#ifndef ENABLE_OPENGL
  transparency = false;
  fill_mode = AS_FILL_DEFAULT;
#endif

  SetDefaultModes();
  SetDefaultColors();
}

void
AirspaceRendererSettings::SetDefaultModes()
{
  std::fill(display, display + AIRSPACECLASSCOUNT, true);
  display[CLASSG] = false;
}

void
AirspaceRendererSettings::SetDefaultColors()
{
  brushes[OTHER] = 7;
  brushes[RESTRICT] = 5;
  brushes[PROHIBITED] = 6;
  brushes[DANGER] = 6;
  brushes[CLASSA] = 1;
  brushes[CLASSB] = 1;
  brushes[CLASSC] = 2;
  brushes[CLASSD] = 2;
  brushes[CLASSE] = 3;
  brushes[CLASSF] = 3;
  brushes[CLASSG] = 4;
  brushes[CTR] = 7;
  brushes[TMZ] = 7;
  brushes[WAVE] = 2;
  brushes[NOGLIDER] = 6;
  brushes[AATASK] = 2;

  colours[OTHER] = 2;
  colours[RESTRICT] = 5;
  colours[PROHIBITED] = 3;
  colours[DANGER] = 3;
  colours[CLASSA] = 15;
  colours[CLASSB] = 9;
  colours[CLASSC] = 14;
  colours[CLASSD] = 8;
  colours[CLASSE] = 13;
  colours[CLASSF] = 7;
  colours[CLASSG] = 2;
  colours[CTR] = 10;
  colours[TMZ] = 11;
  colours[NOGLIDER] = 2;
  colours[WAVE] = 12;
  colours[AATASK] = 8;
}
