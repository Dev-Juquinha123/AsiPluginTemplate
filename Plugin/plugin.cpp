#include "plugin.h"

#include <RakNet/PacketEnumerations.h>
#include <RakNet/StringCompressor.h>

#include <samp.h>

void c_plugin::game_loop() {
	static bool initialized = false;

	if (initialized || !rakhook::initialize() || c_chat::get()->ref() == nullptr)
		return game_loop_hook.call_original();

	initialized = true;
	StringCompressor::AddReference();

	c_chat::get()->ref()->add_message(-1, "samp plugin template");

	c_input::get()->ref()->add_command("plugin_samp", [](const char* param) {
		c_chat::get()->ref()->add_message(-1, "hello from cmd plugin template");
		});

	rakhook::on_send_rpc +=
		[](int& id, RakNet::BitStream* bs, PacketPriority& priority, PacketReliability& reliability, char& ord_channel, bool& sh_timestamp) -> bool {
		c_chat::get()->ref()->add_message(0x03fcdfff, "[asi_plugin] send_rpc -> " + std::to_string(id));
		return true;
		};

	rakhook::on_receive_packet += [](Packet* p) -> bool {
		RakNet::BitStream bs(p->data, p->length, false);
		uint8_t id;
		bs.Read(id);
		c_chat::get()->ref()->add_message(0x03fcdfff, "[asi_plugin] receive packet -> " + std::to_string(id));
		return true;
		};

	rakhook::on_send_packet += [](RakNet::BitStream* bs, PacketPriority& priority, PacketReliability& reliability, char& ord_channel) -> bool {
		uint8_t id;
		bs->Read(id);
		c_chat::get()->ref()->add_message(0x03fcdfff, "[asi_plugin] send packet -> " + std::to_string(id));
		return true;
		};

	rakhook::on_receive_rpc += [](unsigned char& id, RakNet::BitStream* bs) -> bool {
		c_chat::get()->ref()->add_message(0x03fcdfff, "[asi_plugin] receive rpc -> " + std::to_string(id));
		if (id == 61) { // onShowDialog
			printf("dialog\n");
			unsigned short did;
			unsigned char style;
			std::string title, but1, but2, text(4096, 0);

			// read
			bs->Read(did);
			bs->Read(style);
			title = rakhook::detail::read_with_size<unsigned char>(bs);
			but1 = rakhook::detail::read_with_size<unsigned char>(bs);
			but2 = rakhook::detail::read_with_size<unsigned char>(bs);
			StringCompressor::Instance()->DecodeString(text.data(), 4096, bs);

			title = std::to_string(id) + " | " + title;
			text = "[HOOKED] " + text;
			size_t pos = text.find('\0');
			if (pos != std::string::npos)
				text.insert(pos, " [HOOKED]");
			text.resize(4096);

			// write
			bs->Reset();
			bs->Write(did);
			bs->Write(style);
			rakhook::detail::write_with_size<unsigned char>(bs, title);
			rakhook::detail::write_with_size<unsigned char>(bs, but1);
			rakhook::detail::write_with_size<unsigned char>(bs, but2);
			StringCompressor::Instance()->EncodeString(text.data(), 4096, bs);
		}
		if (id == 93) { // onClientMessage
			unsigned long color;
			std::string msg;
			bs->Read(color);

			msg = rakhook::detail::read_with_size<unsigned int>(bs);
			msg = "[HOOKED] " + msg;

			// write
			bs->Reset();
			bs->Write(color);
			rakhook::detail::write_with_size<unsigned int>(bs, msg);
		}
		return true;
		};

	return game_loop_hook.call_original();
}

void c_plugin::attach_console() {
	if (!AllocConsole())
		return;

	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
	freopen_s(&f, "CONOUT$", "w", stderr);
	freopen_s(&f, "CONIN$", "r", stdin);
}

c_plugin::c_plugin(HMODULE hmodule) : hmodule(hmodule)
{
	attach_console();
	game_loop_hook.add(&c_plugin::game_loop);
}


c_plugin::~c_plugin()
{
	rakhook::destroy();
}
