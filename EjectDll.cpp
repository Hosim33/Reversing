//?????? ?ٽ? ????å DLL Ejection ?ǽ?

#include "windows.h"
#include "tlhelp32.h"
#include "tchar.h"

#define DEF_PROC_NAME (L"notepad.exe")
#define DEF_DLL_NAME (L"myhack.dll")

DWORD FindProcessID(LPCTSTR szProcessName)
{
    DWORD dwPID = 0xFFFFFFFF;
    HANDLE hSnapShot = INVALID_HANDLE_VALUE;
    PROCESSENTRY32 pe;

    pe.dwSize = sizeof(PROCESSENTRY32);
    hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);

    Process32First(hSnapShot, &pe);
    do
    {
        if (!_tcsicmp(szProcessName, (LPCTSTR)pe.szExeFile))
        {
            dwPID = pe.th32ProcessID;
            break;
        }
    } while (Process32Next(hSnapShot, &pe));

    CloseHandle(hSnapShot);

    return dwPID;
}

BOOL SetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
    TOKEN_PRIVILEGES tp;
    HANDLE hToken;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        _tprintf(L"OpenProcessToken error: %u\n", GetLastError());
        return FALSE;
    }
    if (!LookupPrivilegeValue(NULL, lpszPrivilege,&luid)) 
    {
        _tprintf(L"LookupPrivilegeValue error: %u\n", GetLastError());
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD) NULL))
    {
        _tprintf(L"AdjustTokenPrivileges error: %u\n", GetLastError());
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        _tprintf(L"The token does not have the specified privilege. \n");
        return FALSE;
    }

    return TRUE;

}

BOOL EjectDll(DWORD dwPID, LPCTSTR szDllName)
{
    BOOL bMore = FALSE, bFound = FALSE;
    HANDLE hSnapshot, hProcess, hThread;
    HMODULE hModule = NULL;
    MODULEENTRY32 me = { sizeof(me) };
    LPTHREAD_START_ROUTINE pThreadProc;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);

    bMore = Module32First(hSnapshot, &me);
    for (; bMore; bMore = Module32Next(hSnapshot, &me))
    {
        if (!_tcsicmp((LPCTSTR)me.szModule, szDllName) ||
            !_tcsicmp((LPCTSTR)me.szExePath, szDllName))
        {
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        CloseHandle(hSnapshot);
        return FALSE;
    }

    if (!(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)))
    {
        _tprintf(L"OpenProcess(%d) failed!!! [%d]\n", dwPID, GetLastError());
        return FALSE;
    }

    hModule = GetModuleHandle(L"kernel32.dll");
    pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(hModule, "FreeLibrary");
    hThread = CreateRemoteThread(hProcess, NULL, 0, pThreadProc, me.modBaseAddr, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    CloseHandle(hProcess);
    CloseHandle(hSnapshot);

    return TRUE;
}

int _tmain(int argc, TCHAR* argv[])
{
    DWORD dwPID = 0xFFFFFFFF;

    dwPID = FindProcessID(DEF_PROC_NAME);
    if (dwPID == 0xFFFFFFFF)
    {
        _tprintf(L"There is no %s process!\n", DEF_PROC_NAME);
        return 1;
    }

    _tprintf(L"PID of \"%s\" is %d\n", DEF_PROC_NAME, dwPID);

    if (!SetPrivilege(SE_DEBUG_NAME, TRUE))
        return 1;

    if (EjectDll(dwPID, DEF_DLL_NAME))
        _tprintf(L"EjectDll(%d,\"%s\") sucess!\n", dwPID, DEF_DLL_NAME);
    else
        _tprintf(L"EjectDll(%d, \"%s\") fail!\n", dwPID, DEF_DLL_NAME);

    return 0;

}