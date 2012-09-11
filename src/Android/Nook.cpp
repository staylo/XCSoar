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

#include "Android/Nook.hpp"
#include <stdlib.h>

static char cmd_sleep[] = "sleep 1";
static char cmd_host[] = "su -c 'echo host > /sys/devices/platform/musb_hdrc/mode'";
static char cmd_peripheral[] = "su -c 'echo peripheral > /sys/devices/platform/musb_hdrc/mode'";
static char cmd_usb_rw[] = "su -c 'chmod 666 /dev/ttyUSB0'";
static char cmd_usb_ro[] = "su -c 'chmod 500 /dev/ttyUSB0'";
static char cmd_init_date[] = "date > /sdcard/initusbdate";
static char cmd_deinit_date[] = "date > /sdcard/deinitusbdate";
static char cmd_wifi_settings[] = "am start -n com.android.settings/.wifi.Settings_Wifi_Dialog";

void
Nook::InitUsb()
{
    system(cmd_host);
    system(cmd_sleep);

    system(cmd_host);
    system(cmd_sleep);

    system(cmd_usb_rw);
    system(cmd_init_date);
}

void
Nook::DeinitUsb()
{
    system(cmd_peripheral);
    system(cmd_sleep);

    system(cmd_peripheral);
    system(cmd_sleep);

    system(cmd_usb_ro);
    system(cmd_deinit_date);
}

void
Nook::WifiSettings()
{
    system(cmd_wifi_settings);
}
