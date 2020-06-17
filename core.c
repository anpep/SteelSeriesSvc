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

#include "core.h"

#include <SetupAPI.h>
#include <hidsdi.h>
#include <dwmapi.h>
#include <WtsApi32.h>
#include <sddl.h>

#include <stdio.h>

typedef enum
{
  RGN_L = 0x01,  // left keyboard region
  RGN_M = 0x02,  // middle keyboard region
  RGN_R = 0x03   // right keyboard region
} SSKBDRGN;

// holds the handle to the HID file
HANDLE g_hHidFile = INVALID_HANDLE_VALUE;

// colorization registry key
HKEY g_hkColorization = NULL;

//
//  initializes keyboard colorization mode
//
inline void
_CoreKbdInit()
{
  BYTE rgbPacket[] =
  {
    0x01, 0x02, 0x41, 0x01,
    0x00, 0x00, 0x00, 0x00
  };

  wprintf(L"sending NORMAL COLORIZATION feature report\n");
  HidD_SetFeature(g_hHidFile, rgbPacket, sizeof(rgbPacket));
}

//
//  sets keyboard region color
//
inline void
_CoreKbdSetColor(SSKBDRGN bRgn, BYTE bR, BYTE bG, BYTE bB)
{
  BYTE rgbPacket[] =
  {
    0x01, 0x02, 0x40, bRgn,
    bR, bG, bB, 0x00
  };

  wprintf(L"setting REGION %u COLOR: %u, %u, %u\n", bRgn, bR, bG, bB);
  HidD_SetFeature(g_hHidFile, rgbPacket, sizeof(rgbPacket));
}

DWORD
CoreInit(void)
{
  // get system-issued HID class GUID
  GUID iidHidClass;
  HidD_GetHidGuid(&iidHidClass);

  // obtain the device class set for the local computer
  HDEVINFO hDevs = SetupDiGetClassDevsW(&iidHidClass,
                                        NULL,
                                        NULL,
                                        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
  WCHAR szDevPath[MAX_PATH] = L"";   // will hold the HID instance path
  SP_DEVICE_INTERFACE_DATA iface;  // current HID interface info
  iface.cbSize = sizeof(iface);

  // enumerate device interfaces until we find the SteelSeries VID/PID sequence
  for (DWORD dwDevIdx = 0;
       SetupDiEnumDeviceInterfaces(hDevs, NULL, &iidHidClass, dwDevIdx, &iface);
       dwDevIdx++)
  {
    DWORD cbReq;

    // query the size required for the device interface buffer
    // this call must fail with ERROR_INSUFFICIENT_BUFFER and store the required
    // size in bytes in cbReq
    if (SetupDiGetDeviceInterfaceDetailW(hDevs, &iface, NULL, 0, &cbReq, NULL)
      || GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
      wprintf(L"BUG: SetupDiGetDeviceInterfaceDetailW() did NOT fail!\n");
      continue;
    }

    // allocate the device interface detail struct
    SP_DEVICE_INTERFACE_DETAIL_DATA_W *ifdet = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbReq);
    ifdet->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    wprintf(L"allocated %u bytes for interface detail data struct\n", cbReq);

    // obtain device interface details so that we can obtain the instance path
    if (!SetupDiGetDeviceInterfaceDetailW(hDevs, &iface, ifdet, cbReq, NULL, NULL))
    {
      // but it failed
      wprintf(L"could not obtain interface details (0x%08x)", GetLastError());
      LocalFree(ifdet);
      continue;
    }

    // HACK: maybe we should verify VID/PID via other means?
    if (wcsstr(ifdet->DevicePath, L"vid_1770&pid_ff00"))
    {
      // anyway, here's the good path - copy to szDevPath
      wprintf(L"found keyboard HID on %ls\n", ifdet->DevicePath);
      wcscpy_s(szDevPath, (sizeof(szDevPath) - 1) / sizeof(WCHAR), ifdet->DevicePath);
      LocalFree(ifdet);
      break;
    }

    LocalFree(ifdet);
  }

  // release resources
  SetupDiDestroyDeviceInfoList(hDevs);

  if (!wcslen(szDevPath))
  {
    // no device found
    wprintf(L"FAIL: no keyboard HID found\n");
    return ERROR_NO_SUCH_DEVICE_INTERFACE;
  }

  // create file for writing feature reports to the HID
  if ((g_hHidFile = CreateFileW(szDevPath,
                                0,
                                FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                0)) == INVALID_HANDLE_VALUE)
  {
    // could not create file
    DWORD dwError = GetLastError();
    wprintf(L"FAIL: could not CreateFile (0x%08x)\n", dwError);
    return dwError;
  }

  // everything's alright
  wprintf(L"initialization complete\n");
  return EXIT_SUCCESS;
}

//
//  obtains the desktop colorization accent and sets the keyboard color accordingly
//
void
CoreSyncColorization(void)
{
  DWORD dwType = REG_DWORD;
  DWORD dwColorization = 0;
  DWORD cbColorization = sizeof(dwColorization);

  if (RegQueryValueExW(g_hkColorization, L"KeyboardColorizationColor", NULL, &dwType, (LPBYTE)&dwColorization, &cbColorization) == ERROR_SUCCESS
      || RegQueryValueExW(g_hkColorization, L"ColorizationColor", NULL, &dwType, (LPBYTE)&dwColorization, &cbColorization) == ERROR_SUCCESS)
  {
    wprintf(L"syncing colorization\n");

    BYTE bR = dwColorization >> 16 & 0xFF,
         bG = dwColorization >> 8 & 0xFF,
         bB = dwColorization & 0xFF;

    const float fV = 1.45f;
    const float fG = 0.2989f * bR + 0.587f * bG + 0.114f * bB;
    const float fD = -fG * fV;

    bR = max(0, min(0xFF, (int) (fD + (float) bR * (1.f + fV))));
    bG = max(0, min(0xFF, (int) (fD + (float) bG * (1.f + fV))));
    bB = max(0, min(0xFF, (int) (fD + (float) bB * (1.f + fV))));

    _CoreKbdInit();
    _CoreKbdSetColor(RGN_L, bR, bG, bB);
    _CoreKbdSetColor(RGN_M, bR, bG, bB);
    _CoreKbdSetColor(RGN_R, bR, bG, bB);
  }
  else
  {
    wprintf(L"FAIL: RegQueryValueW() failed\n");
  }
}

//
//  resets the keyboard color (bright red)
//
void
CoreReset(void)
{
  wprintf(L"restoring original color values\n");
  _CoreKbdInit();
  _CoreKbdSetColor(RGN_L, 0xFF, 0x00, 0x00);
  _CoreKbdSetColor(RGN_M, 0xFF, 0x00, 0x00);
  _CoreKbdSetColor(RGN_R, 0xFF, 0x00, 0x00);
}

void
CoreRun(void)
{
  HANDLE hToken;

  if (WTSQueryUserToken(WTSGetActiveConsoleSessionId(), &hToken))
  {
    // call initially to obtain buffer size
    DWORD cbUser = 0;
    GetTokenInformation(hToken, TokenUser, NULL, 0, &cbUser);

    // allocate buffer for user information
    PTOKEN_USER userinfo = (PTOKEN_USER) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbUser);
    wprintf(L"allocated %u bytes for user information\n", cbUser);

    if (GetTokenInformation(hToken, TokenUser, userinfo, cbUser, &cbUser))
    {
      LPWSTR pSid = NULL;
      if (ConvertSidToStringSidW(userinfo->User.Sid, &pSid))
      {
        wprintf(L"interactive user: %ls\n", pSid);

        static LPCWSTR szSuffix = L"\\Software\\Microsoft\\Windows\\DWM";
        SIZE_T cbWords = wcslen(szSuffix) + wcslen(pSid) + 1;
        LPWSTR szSubkey = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbWords * sizeof(WCHAR));

        wprintf(L"allocated %llu words for subkey buffer\n", cbWords);
        wcscpy_s(szSubkey, cbWords, pSid);
        wcscat_s(szSubkey, cbWords, szSuffix);

        if (RegOpenKeyW(HKEY_USERS, szSubkey, &g_hkColorization) != ERROR_SUCCESS)
        {
          wprintf(L"failed to open subkey (0x%08x)\n", GetLastError());
        }

        wprintf(L"subkey opened successfully: %ls\n", szSubkey);
        LocalFree(szSubkey);
        LocalFree(pSid);
      }
      else
      {
        wprintf(L"FAIL: could not convert SID (0x%08x)\n", GetLastError());
      }
    }
    else
    {
      wprintf(L"FAIL: could not obtain token info (0x%08x)\n", GetLastError());
    }

    LocalFree(userinfo);
    CloseHandle(hToken);
  }
  else
  {
    wprintf(L"FAIL: could not query session token (0x%08x)\n", GetLastError());
  }

  if (!g_hkColorization)
  {
    wprintf(L"FAIL: falling back to HKCU hive\n");
    if (RegOpenKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\DWM", &g_hkColorization)
      != ERROR_SUCCESS)
    {
      wprintf(L"FAIL: could not open registry key (0x%08x)\n", GetLastError());
      return;
    }

    wprintf(L"performing initial sync\n");
    CoreSyncColorization();
  }

  HANDLE hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
  do
  {
    RegNotifyChangeKeyValue(g_hkColorization, TRUE, REG_NOTIFY_CHANGE_LAST_SET, hEvent, TRUE);
    wprintf(L"colorization changed\n");
    CoreSyncColorization();
  }
  while (WaitForSingleObject(hEvent, INFINITE) != WAIT_FAILED && g_hHidFile != INVALID_HANDLE_VALUE);

  wprintf(L"outside registry watch loop\n");
}

void
CoreCleanup(void)
{
  wprintf(L"releasing resources\n");
  CoreReset();
  CloseHandle(g_hHidFile);
  g_hHidFile = INVALID_HANDLE_VALUE;
}
