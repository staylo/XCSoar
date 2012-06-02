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

#include "Device/Driver/CondorUDP.hpp"
#include "Device/Driver.hpp"
#include "NMEA/Info.hpp"
#include "Units/System.hpp"

#include <stdlib.h>

class CondorUDPDevice : public AbstractDevice {
public:
  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info);

  bool ParseEvario(const char *content, NMEAInfo &info);
  bool ParseVario(const char *content, NMEAInfo &info);
  bool ParseNettovario(const char *content, NMEAInfo &info);
  bool ParseAirspeed(const char *content, NMEAInfo &info);
  bool ParseGforce(const char *content, NMEAInfo &info);
};

bool
CondorUDPDevice::ParseEvario(const char *content, NMEAInfo &info)
{
  // e.g. evario=0.550861179828644

  //Disable
  return true;

  char *endptr;
  float value = strtof(content, &endptr);
  if (endptr != content) {
    fixed te = fixed(value);
    info.ProvideTotalEnergyVario(te);
  }

  return true;
}

bool
CondorUDPDevice::ParseVario(const char *content, NMEAInfo &info)
{
  // e.g. vario=0.550861179828644

  char *endptr;
  float value = strtof(content, &endptr);
  if (endptr != content) {
    fixed te = fixed(value);
    info.ProvideTotalEnergyVario(te);
  }

  return true;
}

bool
CondorUDPDevice::ParseNettovario(const char *content, NMEAInfo &info)
{
  // e.g. nettovario=-0.126450657844543

  char *endptr;
  float value = strtof(content, &endptr);
  if (endptr != content) {
    fixed netto = fixed(value);
    info.ProvideNettoVario(netto);
  }

  return true;
}


bool
CondorUDPDevice::ParseAirspeed(const char *content, NMEAInfo &info)
{
  // e.g. airspeed=23.7545757293701

  char *endptr;
  float value = strtof(content, &endptr);
  if (endptr != content) {
    fixed vias = fixed(value);
    info.ProvideIndicatedAirspeed(vias);
  }

  return true;
}

bool
CondorUDPDevice::ParseGforce(const char *content, NMEAInfo &info)
{
  // e.g. gforce=1.23273896150268

  char *endptr;
  float value = strtof(content, &endptr);
  if (endptr != content) {
    fixed g_load = fixed(value);
    info.acceleration.ProvideGLoad(g_load, true);
  }

  return true;
}

bool
CondorUDPDevice::ParseNMEA(const char *line, NMEAInfo &info)
{
  if (memcmp(line, "vario=", 6) == 0)
    return ParseVario(line + 6, info);
  else if (memcmp(line, "evario=", 7) == 0)
    return ParseEvario(line + 7, info);
  else if (memcmp(line, "nettovario=", 11) == 0)
    return ParseNettovario(line + 11, info);
  else if (memcmp(line, "airspeed=", 9) == 0)
    return ParseAirspeed(line + 9, info);
  else if (memcmp(line, "gforce=", 7) == 0)
    return ParseGforce(line + 7, info);
  else
    return false;
}

static Device *
CondorUDPCreateOnPort(const DeviceConfig &config, Port &com_port)
{
  return new CondorUDPDevice();
}

const struct DeviceRegister condorudp_driver = {
  _T("CondorUDP"),
  _T("Condor UDP Data"),
  0,
  CondorUDPCreateOnPort,
};
