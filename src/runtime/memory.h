#pragma once
#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <memory>
#include <cstdio>

extern "C" intptr_t
Luck_ReadVirtualMemory
(
	HANDLE ProcessHandle,
	PVOID BaseAddress,
	PVOID Buffer,
	SIZE_T NumberOfBytesToRead,
	PSIZE_T NumberOfBytesRead
);

extern "C" intptr_t
Luck_WriteVirtualMemory
(
	HANDLE Processhandle,
	PVOID BaseAddress,
	PVOID Buffer,
	SIZE_T NumberOfBytesToWrite,
	PSIZE_T NumberOfBytesWritten
);

class memory_t final
{
public:
	memory_t() : process_id(0), base_address(0), process_handle(NULL) {}
	~memory_t() 
	{ 
		if (process_handle && process_handle != INVALID_HANDLE_VALUE)
			CloseHandle(process_handle); 
	}

	std::uint32_t find_process_id(const std::string& process_name);
	std::uint64_t find_module_address(const std::string& module_name);

	bool attach_to_process(const std::string& process_name);

	std::string read_string(std::uint64_t address);

	// Returns true if address looks valid for user-mode reading (not kernel space, not null-ish)
	bool is_valid_address(std::uint64_t address) const
	{
		return address > 0x10000 && address <= 0x7FFFFFFFFFFFFFFF;
	}

	// Stricter check: address must be in the heap range (0x001... to 0x7FE...),
	// excluding module space (0x7FFx...) and kernel space (0xFFFFx...).
	// Use this for filtering potential Roblox instance addresses.
	bool is_valid_instance_address(std::uint64_t address) const
	{
		constexpr std::uint64_t HEAP_MIN = 0x001000000000ULL;
		constexpr std::uint64_t MODULE_BASE = 0x7FF000000000ULL;
		return address >= HEAP_MIN && address < MODULE_BASE;
	}

	template <typename T>
	T read(std::uint64_t address);

	template <typename T>
	bool write(std::uint64_t address, T value);

	// Suppress memory read/write error printing (true = silent, false = print errors)
	void set_silent(bool s) { silent_mode = s; }
	bool is_silent() const { return silent_mode; }

	std::uint32_t get_process_id();
	std::uint64_t get_module_address();
	HANDLE get_process_handle();
	bool is_attached() { return process_handle != NULL && process_handle != INVALID_HANDLE_VALUE; }
private:
	std::uint32_t process_id;
	std::uint64_t base_address;
	HANDLE process_handle;
	bool silent_mode{ true };
};

template <typename T>
T memory_t::read(uint64_t address)
{
	T buffer{};
	if (!process_handle || process_handle == INVALID_HANDLE_VALUE)
	{
		printf("\x1b[38;5;196m[!] memory::read failed: no valid process handle!\x1b[0m\n");
		return buffer;
	}

	// Silently skip kernel-space or clearly-invalid addresses (prevents error 998 spam)
	if (!is_valid_address(address))
	{
		return buffer;
	}

	SIZE_T bytes_read = 0;
	Luck_ReadVirtualMemory(process_handle, reinterpret_cast<void*>(address), &buffer, sizeof(T), &bytes_read);
	
	// Silently return 0 on failure — callers should check addresses before using
	return buffer;
}

template <typename T>
bool memory_t::write(uint64_t address, T value)
{
	if (!process_handle || process_handle == INVALID_HANDLE_VALUE)
	{
		printf("\x1b[38;5;196m[!] memory::write failed: no valid process handle!\x1b[0m\n");
		return false;
	}

	// Silently skip kernel-space or clearly-invalid addresses
	if (!is_valid_address(address))
	{
		return false;
	}

	SIZE_T bytes_written = 0;
	intptr_t result = Luck_WriteVirtualMemory(process_handle, reinterpret_cast<void*>(address), &value, sizeof(T), &bytes_written);
	
	return result != 0 && bytes_written == sizeof(T);
}

inline std::unique_ptr<memory_t> memory = std::make_unique<memory_t>();
