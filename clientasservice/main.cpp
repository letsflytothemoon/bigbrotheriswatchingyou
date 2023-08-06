#define _WIN32_WINNT 0x0601
#include <Windows.h>
#include <winsvc.h>
#include <strsafe.h>
#include <tchar.h>

TCHAR serviceName[] = TEXT("Big Brother");
SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;

void WINAPI SvcMain(int argc, char* argv[]);
void WINAPI SvcCtrlHandler(DWORD);
void ReportSvcStatus(DWORD, DWORD, DWORD);

int _tmain(int argc, char* argv[])
{
    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { serviceName, (LPSERVICE_MAIN_FUNCTION)SvcMain},
        { NULL, NULL }
    };
    StartServiceCtrlDispatcher(dispatchTable);
    return 0;
}

void WINAPI SvcMain(int argc, char* argv[])
{
    gSvcStatusHandle = RegisterServiceCtrlHandler(
        serviceName,
        SvcCtrlHandler);

    if (!gSvcStatusHandle)
        return;

    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gSvcStatus.dwServiceSpecificExitCode = 0;

    ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    ghSvcStopEvent = CreateEvent(
        NULL,
        TRUE,
        FALSE,
        NULL);

    if (ghSvcStopEvent == NULL)
    {
        ReportSvcStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    ReportSvcStatus(SERVICE_RUNNING, NO_ERROR, 0);

    //

    WaitForSingleObject(ghSvcStopEvent, INFINITE);
    ReportSvcStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

void WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
    switch (dwCtrl)
    {
    case SERVICE_CONTROL_STOP:
        ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        SetEvent(ghSvcStopEvent);
        ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);
        return;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    case SERVICE_ACCEPT_SESSIONCHANGE:
        //do actions here
        break;
    default:
        break;
    }
}

void ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

    // Fill in the SERVICE_STATUS structure.

    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if (dwCurrentState == SERVICE_START_PENDING)
        gSvcStatus.dwControlsAccepted = 0;
    else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;

    if ((dwCurrentState == SERVICE_RUNNING) || (dwCurrentState == SERVICE_STOPPED))
        gSvcStatus.dwCheckPoint = 0;
    else gSvcStatus.dwCheckPoint = dwCheckPoint++;

    // Report the status of the service to the SCM.
    SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}