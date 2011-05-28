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

#include "Device/Driver/PSMove.hpp"
#include "Device/Internal.hpp"
#include "Device/Driver.hpp"
#include "Message.hpp"
#include "Profile/Profile.hpp"
#include "DeviceBlackboard.hpp"
#include "InputEvents.hpp"
#include "LogFile.hpp"
#include "NMEA/InputLine.hpp"
#include "Units/Units.hpp"
#include "Compiler.h"

#include <tchar.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

using std::max;

class PSMoveDevice : public AbstractDevice {
private:
  Port *port;
public:
  PSMoveDevice(Port *_port):port(_port) {}
protected:
  bool PAHRS(NMEAInputLine& line, struct NMEA_INFO *info);

public:
  virtual bool ParseNMEA(const char *line, struct NMEA_INFO *info);
  virtual void OnSysTicker(const NMEA_INFO &basic,
                           const DERIVED_INFO &calculated);
};

bool
PSMoveDevice::ParseNMEA(const char *String, struct NMEA_INFO *info)
{
  NMEAInputLine line(String);
  char type[16];
  line.read(type, 16);

  if (strcmp(type, "$PAHRS") == 0)
    return PAHRS(line, info);
  return false;
}

bool
PSMoveDevice::PAHRS(NMEAInputLine& line, struct NMEA_INFO *info)
{
  fixed value;
  Angle yaw;
  if (line.read_checked(value))
    info->acceleration.BankAngle = Angle::degrees(value);
  if (line.read_checked(value))
    info->acceleration.PitchAngle = Angle::degrees(value);
  if (line.read_checked(value))
    yaw = Angle::degrees(value);

  info->acceleration.AttitudeAvailable = true;
  info->acceleration.Counter++;

  return true;
}

void
PSMoveDevice::OnSysTicker(const NMEA_INFO &basic,
                          const DERIVED_INFO &calculated)
{
  if (basic.LocationAvailable) {
    char tbuf[100];
    double Vx = basic.GroundSpeed*basic.track.cos();
    double Vy = basic.GroundSpeed*basic.track.sin();
    double Vz = -calculated.GPSVario;

    sprintf(tbuf, "PAHRR,%f,%f,%f,%f,%f,%f",
            (double)basic.Location.Longitude.value_degrees(),
            (double)basic.Location.Latitude.value_degrees(),
            (double)basic.GPSAltitude,
            Vx,
            Vy,
            Vz);

    PortWriteNMEA(port, tbuf);
  }
}

static Device *
PSMoveCreateOnPort(Port *com_port)
{
  return new PSMoveDevice(com_port);
}

const struct DeviceRegister psmove_device_driver = {
  _T("PSMove"),
  0,
  PSMoveCreateOnPort,
};
