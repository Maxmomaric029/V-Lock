import os

path = r"C:\Users\Mxzzy\Downloads\vertex up no weird shit\tlee\tlee\rose\src\main.cpp"

with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# Step 1: Remove the initial (premature) pointer chain reads at lines 107-112
old_initial_reads = '''\tstd::uint64_t fake_datamodel{ memory->read<std::uint64_t>(memory->get_module_address() + Offsets::FakeDataModel::Pointer) };
\tgame::datamodel = rbx::instance_t(memory->read<std::uint64_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel));
\tgame::workspace = { memory->read<std::uint64_t>(game::datamodel.address + Offsets::DataModel::Workspace) };
\tgame::visengine = { memory->read<std::uint64_t>(memory->get_module_address() + Offsets::VisualEngine::Pointer) };
\tgame::players = { game::datamodel.find_first_child_by_class(\"Players\") };
\tgame::local_player = { memory->read<std::uint64_t>(game::players.address + Offsets::Player::LocalPlayer) };
\trbx::player_t local_player_obj = { game::local_player.address };
\tgame::local_character = { local_player_obj.get_model_instance().address };'''

if old_initial_reads in content:
    content = content.replace(old_initial_reads, '')
    print("main.cpp: Removed premature pointer chain reads")
else:
    print("main.cpp: PREMATURE READS BLOCK NOT FOUND!")
    idx = content.find('fake_datamodel')
    if idx >= 0:
        print("Found 'fake_datamodel' at index", idx)
        print(repr(content[idx:idx+300]))
    else:
        print("'fake_datamodel' not found!")

# Step 2: Replace the wait loop with validated pointer chain
old_wait_loop = '''\twhile (true)
\t{
\t\tif (!robloxWnd)
\t\t{
\t\t\trobloxWnd = FindWindowA(nullptr, \"Roblox\");
\t\t\tif (!robloxWnd)
\t\t\t{
\t\t\t\tprintf(\"\\x1b[38;5;45m.\\x1b[0m\");
\t\t\t\tstd::this_thread::sleep_for(std::chrono::seconds(2));
\t\t\t\tcontinue;
\t\t\t}
\t\t}

\t\tstd::uint64_t fake_datamodel{ memory->read<std::uint64_t>(memory->get_module_address() + Offsets::FakeDataModel::Pointer) };
\t\tstd::uint64_t real_datamodel = memory->read<std::uint64_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel);

\t\tif (real_datamodel && real_datamodel != 0xCCCCCCCCCCCCCCCC && real_datamodel < 0x7FFFFFFFFFFFFFFF)
\t\t{
\t\t\tprintf(\"\\x1b[38;5;118m [ SUCCESS ]\\x1b[0m\\n\");
\t\t\tbreak;
\t\t}

\t\tprintf(\"\\x1b[38;5;45m.\\x1b[0m\");
\t\tstd::this_thread::sleep_for(std::chrono::seconds(1));
\t}
\tprintf(\"\\n\");'''

new_wait_loop = '''\twhile (true)
\t{
\t\tif (!robloxWnd)
\t\t{
\t\t\trobloxWnd = FindWindowA(nullptr, \"Roblox\");
\t\t\tif (!robloxWnd)
\t\t\t{
\t\t\t\tprintf(\"\\x1b[38;5;45m.\\x1b[0m\");
\t\t\t\tstd::this_thread::sleep_for(std::chrono::seconds(2));
\t\t\t\tcontinue;
\t\t\t}
\t\t}

\t\t// Safe pointer chain: validate EACH step before using it as the next address
\t\tstd::uint64_t base = memory->get_module_address();
\t\tif (!base)
\t\t{
\t\t\tprintf(\"\\x1b[38;5;45m.\\x1b[0m\");
\t\t\tstd::this_thread::sleep_for(std::chrono::seconds(1));
\t\t\tcontinue;
\t\t}

\t\tstd::uint64_t fake_datamodel = memory->read<std::uint64_t>(base + Offsets::FakeDataModel::Pointer);
\t\tif (!fake_datamodel || fake_datamodel > 0x7FFFFFFFFFFFFFFF)
\t\t{
\t\t\tprintf(\"\\x1b[38;5;45m.\\x1b[0m\");
\t\t\tstd::this_thread::sleep_for(std::chrono::seconds(1));
\t\t\tcontinue;
\t\t}

\t\tstd::uint64_t real_datamodel = memory->read<std::uint64_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel);
\t\tif (!real_datamodel || real_datamodel == 0xCCCCCCCCCCCCCCCC || real_datamodel > 0x7FFFFFFFFFFFFFFF)
\t\t{
\t\t\tprintf(\"\\x1b[38;5;45m.\\x1b[0m\");
\t\t\tstd::this_thread::sleep_for(std::chrono::seconds(1));
\t\t\tcontinue;
\t\t}

\t\t// DataModel is valid! Now resolve the rest of the pointer chain
\t\tgame::datamodel = rbx::instance_t(real_datamodel);

\t\tstd::uint64_t workspace_addr = memory->read<std::uint64_t>(real_datamodel + Offsets::DataModel::Workspace);
\t\tif (!workspace_addr || workspace_addr > 0x7FFFFFFFFFFFFFFF)
\t\t{
\t\t\tprintf(\"\\x1b[38;5;45m.\\x1b[0m\");
\t\t\tstd::this_thread::sleep_for(std::chrono::seconds(1));
\t\t\tcontinue;
\t\t}
\t\tgame::workspace = { workspace_addr };

\t\tstd::uint64_t visengine_addr = memory->read<std::uint64_t>(base + Offsets::VisualEngine::Pointer);
\t\tif (!visengine_addr || visengine_addr > 0x7FFFFFFFFFFFFFFF)
\t\t{
\t\t\tprintf(\"\\x1b[38;5;45m.\\x1b[0m\");
\t\t\tstd::this_thread::sleep_for(std::chrono::seconds(1));
\t\t\tcontinue;
\t\t}
\t\tgame::visengine = { visengine_addr };

\t\t// Now resolve Players and LocalPlayer
\t\tgame::players = { game::datamodel.find_first_child_by_class(\"Players\") };
\t\tif (!game::players.address)
\t\t{
\t\t\tprintf(\"\\x1b[38;5;45m.\\x1b[0m\");
\t\t\tstd::this_thread::sleep_for(std::chrono::seconds(1));
\t\t\tcontinue;
\t\t}

\t\tstd::uint64_t local_player_addr = memory->read<std::uint64_t>(game::players.address + Offsets::Player::LocalPlayer);
\t\tif (!local_player_addr || local_player_addr > 0x7FFFFFFFFFFFFFFF)
\t\t{
\t\t\tprintf(\"\\x1b[38;5;45m.\\x1b[0m\");
\t\t\tstd::this_thread::sleep_for(std::chrono::seconds(1));
\t\t\tcontinue;
\t\t}
\t\tgame::local_player = { local_player_addr };

\t\t// Resolve local character
\t\trbx::player_t local_player_obj = { game::local_player.address };
\t\tgame::local_character = { local_player_obj.get_model_instance().address };

\t\t// All pointer chain resolved successfully!
\t\tprintf(\"\\x1b[38;5;118m [ SUCCESS ]\\x1b[0m\\n\");
\t\tbreak;
\t}
\tprintf(\"\\n\");'''

if old_wait_loop in content:
    content = content.replace(old_wait_loop, new_wait_loop)
    print("main.cpp: Replaced wait loop with validated pointer chain")
else:
    print("main.cpp: WAIT LOOP NOT FOUND!")
    idx = content.find('Syncing with engine state')
    if idx >= 0:
        print("Found 'Syncing with engine state' at idx", idx)
        print(repr(content[idx:idx+400]))
    else:
        print("'Syncing with engine state' not found!")

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("\nmain.cpp: Done!")
