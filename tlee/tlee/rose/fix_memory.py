import os

path = r"C:\Users\Mxzzy\Downloads\vertex up no weird shit\tlee\tlee\rose\src\runtime\memory.cpp"

with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

old_block = '''\t// Try PROCESS_ALL_ACCESS first (may fail on protected processes)
\tHANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
\t
\tif (process == NULL)
\t{
\t\tDWORD error = GetLastError();
\t\tprintf("\\x1b[38;5;214m[!] OpenProcess(PROCESS_ALL_ACCESS) failed for PID %lu! error=%lu\\x1b[0m\\n", (unsigned long)pid, error);
\t\tprintf("\\x1b[38;5;45m[+] Trying with PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION...\\x1b[0m\\n");
\t\t
\t\t// Fallback: try minimum required permissions
\t\tprocess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
\t\t
\t\tif (process == NULL)
\t\t{
\t\t\terror = GetLastError();
\t\t\tprintf("\\x1b[38;5;196m[!] Fallback OpenProcess also failed! error=%lu\\x1b[0m\\n", error);
\t\t\tprintf("\\x1b[38;5;196m[!] Try running as Administrator!\\x1b[0m\\n");
\t\t\treturn false;
\t\t}
\t\t
\t\tprintf("\\x1b[38;5;118m[+] Process attached with restricted permissions (PID %lu)\\x1b[0m\\n", (unsigned long)pid);
\t}
\telse
\t{
\t\tprintf("\\x1b[38;5;118m[+] Process attached with full access (PID %lu)\\x1b[0m\\n", (unsigned long)pid);
\t}'''

new_block = '''\t// Try PROCESS_ALL_ACCESS first (may fail on protected processes)
\tHANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
\t
\tif (process == NULL)
\t{
\t\tDWORD error = GetLastError();
\t\tprintf("\\x1b[38;5;214m[!] OpenProcess(PROCESS_ALL_ACCESS) failed for PID %lu! error=%lu\\x1b[0m\\n", (unsigned long)pid, error);
\t\tprintf("\\x1b[38;5;45m[+] Trying with VM_READ | VM_WRITE | VM_OPERATION | QUERY_INFORMATION...\\x1b[0m\\n");
\t\t
\t\t// Fallback: try extended permissions (PROCESS_QUERY_INFORMATION needed to avoid ERROR_PARTIAL_COPY)
\t\tprocess = OpenProcess(
\t\t\tPROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION | PROCESS_SUSPEND_RESUME,
\t\t\tFALSE, pid
\t\t);
\t\t
\t\tif (process == NULL)
\t\t{
\t\t\terror = GetLastError();
\t\t\tprintf("\\x1b[38;5;214m[!] Extended OpenProcess also failed! error=%lu\\x1b[0m\\n", error);
\t\t\tprintf("\\x1b[38;5;45m[+] Trying absolute minimum: VM_READ | VM_OPERATION...\\x1b[0m\\n");
\t\t\t
\t\t\t// Last resort: bare minimum to read memory
\t\t\tprocess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION, FALSE, pid);
\t\t\t
\t\t\tif (process == NULL)
\t\t\t{
\t\t\t\terror = GetLastError();
\t\t\t\tprintf("\\x1b[38;5;196m[!] All OpenProcess attempts failed! Last error=%lu\\x1b[0m\\n", error);
\t\t\t\tprintf("\\x1b[38;5;196m[!] Try running as Administrator!\\x1b[0m\\n");
\t\t\t\treturn false;
\t\t\t}
\t\t\t
\t\t\tprintf("\\x1b[38;5;118m[+] Process attached with minimum permissions (PID %lu)\\x1b[0m\\n", (unsigned long)pid);
\t\t}
\t\telse
\t\t{
\t\t\tprintf("\\x1b[38;5;118m[+] Process attached with extended permissions (PID %lu)\\x1b[0m\\n", (unsigned long)pid);
\t\t}
\t}
\telse
\t{
\t\tprintf("\\x1b[38;5;118m[+] Process attached with full access (PID %lu)\\x1b[0m\\n", (unsigned long)pid);
\t}'''

if old_block in content:
    content = content.replace(old_block, new_block)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)
    print("memory.cpp: Fixed OpenProcess permissions (added PROCESS_QUERY_INFORMATION)")
else:
    print("memory.cpp: BLOCK NOT FOUND!")
    idx = content.find('PROCESS_ALL_ACCESS')
    if idx >= 0:
        print("Found at idx", idx)
        print(repr(content[idx:idx+500]))
    else:
        print("PROCESS_ALL_ACCESS not found in file!")
