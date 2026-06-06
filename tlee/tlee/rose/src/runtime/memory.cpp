#include "memory.h"

std::uint32_t memory_t::find_process_id(const std::string& process_name)
{
	std::uint32_t local_process_id = 0;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (snapshot == INVALID_HANDLE_VALUE)
	{
		printf("\x1b[38;5;196m[!] Failed to create process snapshot! error=%lu\x1b[0m\n", GetLastError());
		return local_process_id;
	}

	PROCESSENTRY32 process_entry{};
	process_entry.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(snapshot, &process_entry))
	{
		do
		{
			if (!_stricmp(process_name.c_str(), process_entry.szExeFile))
			{
				local_process_id = process_entry.th32ProcessID;
				process_id = local_process_id;
				printf("\x1b[38;5;118m[+] Found process '%s' with PID %lu\x1b[0m\n", process_name.c_str(), (unsigned long)process_id);
				break;
			}
		} while (Process32Next(snapshot, &process_entry));
	}

	CloseHandle(snapshot);
	return local_process_id;
}

std::uint64_t memory_t::find_module_address(const std::string& module_name)
{
	std::uint64_t module_address = 0;

	if (!process_handle)
	{
		printf("\x1b[38;5;196m[!] find_module_address: no process handle!\x1b[0m\n");
		return module_address;
	}

	DWORD pid = GetProcessId(process_handle);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);

	if (snapshot == INVALID_HANDLE_VALUE)
	{
		printf("\x1b[38;5;196m[!] Failed to create module snapshot for PID %lu! error=%lu\x1b[0m\n", (unsigned long)pid, GetLastError());
		return module_address;
	}

	MODULEENTRY32 module_entry{};
	module_entry.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(snapshot, &module_entry))
	{
		do
		{
			if (!_stricmp(module_name.c_str(), module_entry.szModule))
			{
				module_address = reinterpret_cast<uint64_t>(module_entry.modBaseAddr);
				base_address = module_address;
				printf("\x1b[38;5;118m[+] Module '%s' at 0x%llx\x1b[0m\n", module_name.c_str(), (unsigned long long)base_address);
				break;
			}
		} while (Module32Next(snapshot, &module_entry));
	}

	CloseHandle(snapshot);
	return module_address;
}

bool memory_t::attach_to_process(const std::string& process_name)
{
	std::uint32_t pid = find_process_id(process_name);
	if (!pid)
	{
		printf("\x1b[38;5;196m[!] Could not find process '%s'\x1b[0m\n", process_name.c_str());
		return false;
	}

	// Try PROCESS_ALL_ACCESS first (may fail on protected processes)
	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	
	if (process == NULL)
	{
		DWORD error = GetLastError();
		printf("\x1b[38;5;214m[!] OpenProcess(PROCESS_ALL_ACCESS) failed for PID %lu! error=%lu\x1b[0m\n", (unsigned long)pid, error);
		printf("\x1b[38;5;45m[+] Trying with VM_READ | VM_WRITE | VM_OPERATION | QUERY_INFORMATION...\x1b[0m\n");
		
		// Fallback: try extended permissions (PROCESS_QUERY_INFORMATION needed to avoid ERROR_PARTIAL_COPY)
		process = OpenProcess(
			PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME,
			FALSE, pid
		);
		
		if (process == NULL)
		{
			error = GetLastError();
			printf("\x1b[38;5;214m[!] Extended OpenProcess also failed! error=%lu\x1b[0m\n", error);
			printf("\x1b[38;5;45m[+] Trying absolute minimum: VM_READ | VM_OPERATION...\x1b[0m\n");
			
			// Last resort: bare minimum to read memory
			process = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION, FALSE, pid);
			
			if (process == NULL)
			{
				error = GetLastError();
				printf("\x1b[38;5;196m[!] All OpenProcess attempts failed! Last error=%lu\x1b[0m\n", error);
				printf("\x1b[38;5;196m[!] Try running as Administrator!\x1b[0m\n");
				return false;
			}
			
			printf("\x1b[38;5;118m[+] Process attached with minimum permissions (PID %lu)\x1b[0m\n", (unsigned long)pid);
		}
		else
		{
			printf("\x1b[38;5;118m[+] Process attached with extended permissions (PID %lu)\x1b[0m\n", (unsigned long)pid);
		}
	}
	else
	{
		printf("\x1b[38;5;118m[+] Process attached with full access (PID %lu)\x1b[0m\n", (unsigned long)pid);
	}

	process_handle = process;
	return true;
}

std::string memory_t::read_string(std::uint64_t address)
{
	std::int32_t string_length = read<std::int32_t>(address + 0x10);
	std::uint64_t string_address = (string_length >= 16) ? read<std::uint64_t>(address) : address;

	// Negative length = garbage read = bad_alloc crash if passed to vector
	if (string_length <= 0 || string_length > 1024)
	{
		return "Unknown";
	}

	std::vector<char> buffer(static_cast<size_t>(string_length) + 1, 0);
	ULONG bytes_read = 0;
	intptr_t result = Luck_ReadVirtualMemory(process_handle, reinterpret_cast<void*>(string_address), buffer.data(), static_cast<ULONG>(buffer.size()), &bytes_read);
	
	if (result == 0 || bytes_read != static_cast<ULONG>(string_length) + 1)
	{
		return "Unknown";
	}

	return std::string(buffer.data(), string_length);
}

std::uint32_t memory_t::get_process_id()
{
	return process_id;
}

std::uint64_t memory_t::get_module_address()
{
	return base_address;
}

HANDLE memory_t::get_process_handle()
{
	return process_handle;
}
