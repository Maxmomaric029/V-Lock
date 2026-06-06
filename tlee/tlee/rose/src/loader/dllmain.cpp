#include <windows.h>
#include <iostream>
#include <string>
#include <thread>

// Pipeline name must match the external cheat
#define PIPE_NAME "\\\\.\\pipe\\VertexExecutor"

void ExecuteLua(const std::string& script) {
    // TODO: Connect this to Luau Bytecode Compiler and game's execution pipeline
    std::cout << "[Vertex Internal] Received Script to Execute:\n" << script << "\n" << std::endl;
}

void PipeServerThread() {
    std::cout << "[Vertex Internal] Setting up execution bridge..." << std::endl;

    while (true) {
        HANDLE hPipe = CreateNamedPipeA(
            PIPE_NAME,
            PIPE_ACCESS_INBOUND,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            1, // Max instances
            8192, // Out buffer size
            8192, // In buffer size
            0, // Default timeout
            NULL // Default security attributes
        );

        if (hPipe != INVALID_HANDLE_VALUE) {
            std::cout << "[Vertex Internal] Waiting for external UI connection..." << std::endl;
            
            if (ConnectNamedPipe(hPipe, NULL) != FALSE || GetLastError() == ERROR_PIPE_CONNECTED) {
                std::cout << "[Vertex Internal] Connected to Vertex UI!" << std::endl;
                
                char buffer[8192];
                DWORD bytesRead;
                
                // Read the script from the external cheat
                while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) != FALSE) {
                    buffer[bytesRead] = '\0'; // Null terminate
                    ExecuteLua(std::string(buffer));
                }
            }
            
            DisconnectNamedPipe(hPipe);
            CloseHandle(hPipe);
        } else {
            std::cout << "[Vertex Internal] Failed to create Named Pipe. Error: " << GetLastError() << std::endl;
            Sleep(1000); // Wait before retrying
        }
    }
}

void Initialize() {
    // Allocate a debug console
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONIN$", "r", stdin);
    
    std::cout << "=====================================" << std::endl;
    std::cout << "  VERTEX INTERNAL ENGINE INITIALIZED " << std::endl;
    std::cout << "=====================================" << std::endl;
    
    // Start the named pipe server in the background
    std::thread(PipeServerThread).detach();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        std::thread(Initialize).detach();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
