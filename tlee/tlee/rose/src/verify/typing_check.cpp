#include "typing_check.h"

#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <session/game.h>
#include <Windows.h>
#include <thread>
#include <chrono>

namespace check
{
	// ChatInputBarConfiguration was removed from the new offset dump.
	// Using its last known Visible offset as a local constant until re-dumped.
	static constexpr uintptr_t CIBC_VISIBLE_OFFSET = 0x156;
	
	static constexpr int UPDATE_INTERVAL = 10;
	static std::uint64_t cibc_address = 0;

	void run()
	{
		int last_update = 0;

		for (;;)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			if (game::datamodel.address == 0)
				continue;

			if (last_update == 0 || (last_update % UPDATE_INTERVAL) == 0)
			{
				rbx::instance_t text_chat_service = game::datamodel.find_first_child_by_class("TextChatService");
				if (text_chat_service.address != 0)
				{
					rbx::instance_t cibc = text_chat_service.find_first_child("ChatInputBarConfiguration");
					if (cibc.address != 0)
					{
						cibc_address = cibc.address;
					}
				}
			}

			if (cibc_address != 0)
			{
				bool textchatopen = memory->read<bool>(cibc_address + CIBC_VISIBLE_OFFSET);
				if (textchatopen && !check::was_typing)
				{
					check::was_typing = true;
				}
				check::textchatopen = textchatopen;
			}
			else
			{
				check::textchatopen = false;
			}

			last_update++;
		}
	}
}

