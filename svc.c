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

#include <Windows.h>
#include <strsafe.h>

#include "core.h"

// canonical service name
const LPCWSTR k_szSvcName = L"SteelSeriesSvc";

// service display name
// TODO: obtain display name from Version Info Block
const LPCWSTR k_szSvcDisplayName = L"SteelSeries Integration Service";

// contains information for the service
SERVICE_STATUS g_svcStatus;
SERVICE_STATUS_HANDLE g_svcStatusHandle;

//
//  installs the service
//
static void
SvcInstall(void)
{
  SC_HANDLE schManager;
  WCHAR szPath[MAX_PATH];
  DWORD dwError;

  if (!GetModuleFileNameW(NULL, szPath, MAX_PATH))
  {
    wprintf(L"Could not install service (0x%08X)\n", dwError = GetLastError());
    ExitProcess(dwError);
  }

  // get SCM database handler
  if (!(schManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS)))
  {
    wprintf(L"OpenSCManager() failed (0x%08X)\n", dwError = GetLastError());
    ExitProcess(dwError);
  }

  // create the service
  SC_HANDLE schService = CreateServiceW(schManager,
                                        k_szSvcName,
                                        k_szSvcDisplayName,
                                        STANDARD_RIGHTS_REQUIRED
                                        | SC_MANAGER_CREATE_SERVICE,
                                        SERVICE_WIN32_OWN_PROCESS,
                                        SERVICE_AUTO_START,
                                        SERVICE_ERROR_NORMAL,
                                        szPath,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL);
  if (!schService)
  {
    wprintf(L"CreateService() failed (0x%08X)\n", dwError = GetLastError());
    CloseServiceHandle(schManager);
    ExitProcess(dwError);
  }

  wprintf(L"Service installed successfully\n");
  CloseServiceHandle(schService);
  CloseServiceHandle(schManager);
}

//
//  report service status to the SCM
//
static void
SvcReportStatus(DWORD dwCurrentState,
                DWORD dwWin32ExitCode,
                DWORD dwWaitHint)
{
  static DWORD dwCheckPoint = 1;

  // fill the SERVICE_STATUS struct
  g_svcStatus.dwCurrentState = dwCurrentState;
  g_svcStatus.dwWin32ExitCode = dwWin32ExitCode;
  g_svcStatus.dwWaitHint = dwWaitHint;
  g_svcStatus.dwControlsAccepted = SERVICE_ACCEPT_SESSIONCHANGE;

  if (dwCurrentState == SERVICE_START_PENDING)
  {
    g_svcStatus.dwControlsAccepted &= ~SERVICE_ACCEPT_STOP;
  }
  else
  {
    g_svcStatus.dwControlsAccepted |= SERVICE_ACCEPT_STOP;
  }

  g_svcStatus.dwCheckPoint = dwCurrentState == SERVICE_RUNNING
                             || dwCurrentState == SERVICE_STOPPED
                               ? 0
                               : dwCheckPoint++;

  SetServiceStatus(g_svcStatusHandle, &g_svcStatus);
}

//
//  handles service control codes
//
static DWORD WINAPI
SvcCtrlHandler(DWORD dwControl, DWORD dwEvtType, LPVOID lpEvtData, LPVOID lpCtx)
{
  if (dwControl == SERVICE_CONTROL_STOP)
  {
    SvcReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

    // release service resources
    CoreCleanup();
    SvcReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
  }
  else if (dwControl == SERVICE_CONTROL_SESSIONCHANGE)
  {
    // handle session changes
    switch (dwEvtType)
    {
    case WTS_SESSION_LOGOFF:
    case WTS_SESSION_LOCK:
      CoreReset();
      break;

    case WTS_SESSION_UNLOCK:
    case WTS_SESSION_LOGON:
      CoreSyncColorization();
      break;
    }
  }

  return 0;
}

//
//  service entry point
//
static void WINAPI
SvcMain(DWORD dwArgc, LPCWSTR *lpArgv)
{
  // register service handler
  if (!(g_svcStatusHandle = RegisterServiceCtrlHandlerExW(k_szSvcName, SvcCtrlHandler, NULL)))
  {
    // TODO: logging?
    return;
  }

  // fill SERVICE_STATUS
  g_svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS;
  g_svcStatus.dwServiceSpecificExitCode = 0;

  // report initial status
  SvcReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

  // initialize service
  DWORD dwRes;
  if ((dwRes = CoreInit()) != ERROR_SUCCESS)
  {
    SvcReportStatus(SERVICE_STOPPED, dwRes, 0);
    return;
  }

  SvcReportStatus(SERVICE_RUNNING, NO_ERROR, 0);
  CoreRun();
}

//
//  handles Ctrl-C and performs cleanup before exiting
//
static BOOL WINAPI
ConsoleCtrlHandler(DWORD dwControl)
{
  // gracefully terminate process
  CoreCleanup();
  ExitProcess(0);
}

//
//  CLI entry point
//
void WINAPI
wmain(const int nArgc, LPCWSTR *lpArgv)
{
  if (nArgc > 1)
  {
    if (!wcscmp(lpArgv[1], L"start"))
    {
      // initialize core functionality
      CoreInit();

      // register console handler to gracefully terminate our "daemon"
      SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
      CoreRun();
      CoreCleanup();
      return;
    }

    if (!wcscmp(lpArgv[1], L"install"))
    {
      // create the service
      SvcInstall();
      return;
    }
  }

  wprintf(
    L"DESCRIPTION:\n"
    L"        SteelSeriesSvc is a service that synchronizes\n"
    L"        SteelSeries keyboards with the current user's\n"
    L"        accent color in real time.\n"
    L"USAGE:\n"
    L"        SteelSeriesSvc [command]\n\n\n"
    L"        Commands:\n"
    L"          start-----------Starts the service as an\n"
    L"                          standalone process.\n"
    L"          install---------(Re)installs the service\n"
    L"                          system-wide.\n"
    L"LICENSE:\n"
    L"        SteelSeriesSvc version 1.0\n"
    L"        Copyright (C) 2018 Angel P.\n"
    L"        SteelSeriesSvc comes with ABSOLUTELY NO WARRANTY.\n"
    L"        This is free software, and you are welcome to\n"
    L"        redistribute it under certain conditions. See\n"
    L"        http://git.io/steelsvc for more details.\n\n"
  );

  SERVICE_TABLE_ENTRYW dispatchtable[] =
  {
    {(LPWSTR) k_szSvcName, (LPSERVICE_MAIN_FUNCTIONW) SvcMain},
    {NULL, NULL}
  };

  StartServiceCtrlDispatcherW(dispatchtable);
}
