#include "antidebug.h"
#include <thread>
#include <chrono>

void terminate_process() {
    std::exit(0);
}

void trigger_seh_exception() {
    __try
    {
        RaiseException(DBG_PRINTEXCEPTION_C, 0, 0, 0);
        terminate_process();
    }
    __except (GetExceptionCode() == DBG_PRINTEXCEPTION_C) {}
}

void debugger_detection() {

    while (true) {
        if (IsDebuggerPresent()) {
            terminate_process();
        }

        static _PEB64* p_peb{ reinterpret_cast<_PEB64*>(NtCurrentTeb()->ProcessEnvironmentBlock) };
        if (p_peb->BeingDebugged) {
            terminate_process();
        }

        if (p_peb->NtGlobalFlag & (0x10 | 0x20 | 0x40)) {
            terminate_process();
        }

        BOOL is_debugged{};
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &is_debugged);
        if (is_debugged) {
            terminate_process();
        }

        DWORD_PTR debug_port{};
        if (NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(), static_cast<_PROCESSINFOCLASS>(ProcessDebugPort), &debug_port, sizeof(debug_port), nullptr)) && debug_port != 0) {
            terminate_process();
        }

        DWORD debug_flags{};
        if (NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(), static_cast<_PROCESSINFOCLASS>(31), &debug_flags, sizeof(debug_flags), nullptr)) && debug_flags == 0) {
            terminate_process();
        }

        HANDLE debug_object{};
        if (NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(), static_cast<_PROCESSINFOCLASS>(30), &debug_object, sizeof(debug_object), nullptr)) && debug_object != 0) {
            terminate_process();
        }

        static const char* titles[] = { "Process Monitor", "Cheat Engine", "Process Hacker", "x64dbg", "x32dbg", "IDA - ", "IDA Pro", "IDA Free", "WinDbg", "Ghidra", "Binary Ninja", "System Informer", nullptr };
        
        for (int i{}; titles[i]; i++)
        {
            HWND hwnd{};
        
            constexpr size_t maximumWindowNameSize = 256;
            char windowText[maximumWindowNameSize];
        
            while ((hwnd = FindWindowExA(nullptr, hwnd, nullptr, nullptr)) != nullptr)
            {
                if (GetWindowTextA(hwnd, windowText, sizeof(windowText)) > 0 && strstr(windowText, titles[i]) != nullptr)
                {
                    terminate_process();
                }
            }
        }
        
        static const char* classes[] = { "OLLYDBG", "WinDbgFrameClass", "ProcessHacker", "PROCMON_WINDOW_CLASS", nullptr };
        
        for (int i{}; classes[i]; i++) {
            if (FindWindowA(classes[i], nullptr)) { 
                terminate_process();
            }
        }

        CONTEXT ctx{};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

        GetThreadContext(GetCurrentThread(), &ctx);

        if ((ctx.Dr0 != 0) || (ctx.Dr1 != 0) || (ctx.Dr2 != 0) || (ctx.Dr3 != 0) || (ctx.Dr6 != 0) || (ctx.Dr7 & 0xFF)) {
            terminate_process();
        }

        // trigger_seh_exception();  // Comentado: mata el proceso sin debugger en Windows 10/11

        static const char* commonKernel32Functions[] =
        {
            "IsDebuggerPresent",
            "EnumDeviceDrivers",
            "CloseHandle",
            "CheckRemoteDebuggerPresent",
            "GetThreadContext",
            "RaiseException",
            "OutputDebugStringA",
            "OutputDebugStringW",
            "DebugBreak",
            "CreateToolhelp32Snapshot",
            "EnumWindows",
            "FindWindow",
            "GetTickCount",
            "GetTickCount64",
            "GetSystemTime",
            "GetStartupInfo",
            nullptr
        };

        static const char* commonNtDllFunctions[] =
        {
            "NtQueryInformationProcess",
            "ZwQueryInformationProcess",
            "NtSetInformationThread",
            nullptr
        };

        static const char* commonWs2_32Functions[] =
        {
            "send",
            "recv",
            "connect",
            "accept",
            "bind",
            "closesocket",
            "WSAStartup",
            "WSACleanup",
            nullptr
        };

        static HMODULE kernel32_address{ GetModuleHandleA("kernel32.dll") };
        static HMODULE ntdll_address{ GetModuleHandleA("ntdll.dll") };
        static HMODULE ws2_32_address{ GetModuleHandleA("ws2_32.dll") };

        constexpr std::uint8_t int3opcode = 0xCC;
        constexpr std::uint16_t int3multi_byte_opcode = 0xCD03;
        constexpr std::uint16_t undefined_opcode = 0x0F0B;


        for (int i{}; commonKernel32Functions[i]; i++) {

            void* functionPointer{ reinterpret_cast<void*>(GetProcAddress(kernel32_address, commonKernel32Functions[i])) };

            if (!functionPointer)
                continue;

            if (*(std::uint8_t*)functionPointer == int3opcode ||
                *(std::uint16_t*)functionPointer == int3multi_byte_opcode ||
                *(std::uint16_t*)functionPointer == undefined_opcode)
            {
                terminate_process();
                return;
            }
        }

        for (int i{}; commonNtDllFunctions[i]; i++) {

            void* function_pointer{ reinterpret_cast<void*>(GetProcAddress(ntdll_address, commonNtDllFunctions[i])) };

            if (!function_pointer)
                continue;

            if (*(std::uint8_t*)function_pointer == int3opcode ||
                *(std::uint16_t*)function_pointer == int3multi_byte_opcode ||
                *(std::uint16_t*)function_pointer == undefined_opcode)
            {
                terminate_process();
                return;
            }
        }

        for (int i = 0; commonWs2_32Functions[i]; i++) {

            void* functionPointer = reinterpret_cast<void*>(GetProcAddress(ws2_32_address, commonWs2_32Functions[i]));

            if (!functionPointer)
                continue;

            if (*(std::uint8_t*)functionPointer == int3opcode ||
                *(std::uint16_t*)functionPointer == int3multi_byte_opcode ||
                *(std::uint16_t*)functionPointer == undefined_opcode)
            {
                terminate_process();
                return;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}