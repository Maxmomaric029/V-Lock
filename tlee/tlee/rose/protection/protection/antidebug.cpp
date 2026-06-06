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

        // INT3 hook detection eliminado: causa falsos positivos con Windows Defender,
        // antivirus y EDRs que ponen hooks legítimos en kernel32/ntdll/ws2_32.
        // La detección real de debuggers (IsDebuggerPresent, PEB, NtGlobalFlag,
        // debug registers, window enumeration) se mantiene intacta arriba.

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}