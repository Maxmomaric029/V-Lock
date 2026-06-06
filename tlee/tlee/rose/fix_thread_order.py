import os

path = r"C:\Users\Mxzzy\Downloads\vertex up no weird shit\tlee\tlee\rose\src\main.cpp"

with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# The block to move (init threads + silent init - BEFORE the wait loop)
old_before_wait = '''\tstd::thread(cache::run).detach();
\tstd::thread(jailbreak::run).detach();

\tif (!InitializeStorage())
\t{
\t\tprintf(\"\\n\\x1b[38;5;214m   [!] WARNING: Initial memory scan failed. Retrying in background...\\x1b[0m\\n\");
\t}

\tstd::thread(AutoRescanHandler).detach();

\trbx::silent::initialize();

\t// Wait for Roblox window and DataModel to be ready'''

new_before_wait = '''\t// Wait for Roblox window and DataModel to be ready'''

if old_before_wait in content:
    content = content.replace(old_before_wait, new_before_wait)
    print("main.cpp: Removed init threads from BEFORE the wait loop")
else:
    print("main.cpp: BLOCK BEFORE WAIT NOT FOUND!")
    idx = content.find('cache::run')
    if idx >= 0:
        print("Found 'cache::run' at idx", idx)
        print(repr(content[idx:idx+300]))
    else:
        print("'cache::run' not found!")

# The marker to insert AFTER the wait loop (after '[ SUCCESS ]\n' and '\n')
old_after_wait = '''\tprintf(\"\\n\");

\tif (!render->create_window())'''

new_after_wait = '''\tprintf(\"\\n\");

\t// Now all game pointers are valid - start subsystems that depend on them
\tstd::thread(cache::run).detach();
\tstd::thread(jailbreak::run).detach();

\tif (!InitializeStorage())
\t{
\t\tprintf(\"\\n\\x1b[38;5;214m   [!] WARNING: Initial memory scan failed. Retrying in background...\\x1b[0m\\n\");
\t}

\tstd::thread(AutoRescanHandler).detach();

\trbx::silent::initialize();

\tif (!render->create_window())'''

if old_after_wait in content:
    content = content.replace(old_after_wait, new_after_wait)
    print("main.cpp: Inserted init threads AFTER the wait loop")
else:
    print("main.cpp: AFTER WAIT BLOCK NOT FOUND!")
    idx = content.find('create_window')
    if idx >= 0:
        # Find the block around it
        before = content.rfind('\n', 0, idx-100)
        print("Found 'create_window' at idx", idx)
        print(repr(content[before:idx+50]))
    else:
        print("'create_window' not found!")

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("\nmain.cpp thread ordering: Done!")
