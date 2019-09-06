// SteelSeriesSvc -- Tints MSI SteelSeries keyboards with Windows accent color
// Copyright (c) 2018 Angel P.
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#pragma once

#include <Windows.h>

//
//  initializes SteelSeries keyboard device
//  returns 0 on success or an error code
//
DWORD CoreInit(void);

//
//  service loop
//
void CoreRun(void);

//
//  resets the keyboard color (bright red)
//
void CoreReset(void);

//
//  obtains the desktop colorization accent and sets the keyboard color accordingly
//
void CoreSyncColorization(void);

//
//  releases resources used by the service and prepares for shutdown
//
void CoreCleanup(void);