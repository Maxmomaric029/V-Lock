#define NOMINMAX
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <cstdio>
#include "kill_crash_handler.h"

void rbx::bypass::kill_other_crash_handlers()
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		return;
	}

	PROCESSENTRY32 process_entry{};
	process_entry.dwSize = sizeof(PROCESSENTRY32);

	DWORD current_process_id = GetCurrentProcessId();

	if (Process32First(snapshot, &process_entry))
	{
		do
		{
			if (!_stricmp("RobloxCrashHandler.exe", process_entry.szExeFile))
			{
				if (process_entry.th32ProcessID == current_process_id)
					continue;

				HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, process_entry.th32ProcessID);
				if (process_handle != NULL)
				{
					TerminateProcess(process_handle, 0);
					CloseHandle(process_handle);
				}
			}
		} while (Process32Next(snapshot, &process_entry));
	}

	CloseHandle(snapshot);
}

void rbx::bypass::run()
{
	for (;;)
	{
		__try
		{
			kill_other_crash_handlers();
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			printf("\x1b[38;5;214m   [!] Bypass thread recovered from exception\x1b[0m\n");
		}
		Sleep(1000);
	}
}

void rbx::bypass::initialize()
{
}

