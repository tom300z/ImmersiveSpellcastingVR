
#include "SKSE/API.h"
#include "SKSE/Interfaces.h"
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "SKSE/API.h"
#include "SKSE/Trampoline.h"
#include "spdlog/spdlog.h"
#include "AttackTicker.h"

using namespace RE;
using namespace SKSE;


void OnSKSEMessage(SKSE::MessagingInterface::Message* msg)
{
	switch (msg->type) {
	case SKSE::MessagingInterface::kInputLoaded:
		{
			logger::info(FMT_STRING("kInputLoaded"), Plugin::NAME, Plugin::VERSION);
			break;
		}
	case SKSE::MessagingInterface::kPostLoadGame:
		{
			logger::info(FMT_STRING("kPostLoadGame"), Plugin::NAME, Plugin::VERSION);
			break;
		}
		break;
	}
}


namespace
{
	// --- Our callback: log equipped item in each hand ---
	void LogEquippedItems()
	{
		auto* player = PlayerCharacter::GetSingleton();
		if (!player) {
			logger::info("Player not found.");
			return;
		}

		// Left hand
		if (auto* left = player->GetEquippedObject(true)) {
			logger::info("Left Hand: {} ({:08X})", left->GetName(), left->GetFormID());
		} else {
			logger::info("Left Hand: empty");
		}

		// Right hand
		if (auto* right = player->GetEquippedObject(false)) {
			logger::info("Right Hand: {} ({:08X})", right->GetName(), right->GetFormID());
		} else {
			logger::info("Right Hand: empty");
		}
	}

	// --- Listener for equip events ---
	struct EquipEventHandler : BSTEventSink<TESEquipEvent>
	{
		BSEventNotifyControl ProcessEvent(const TESEquipEvent* evn, BSTEventSource<TESEquipEvent>*)
		{
			if (evn && evn->actor.get() == PlayerCharacter::GetSingleton()) {
				logger::info("EquipEvent fired → logging equipped items");
				LogEquippedItems();
			}
			return BSEventNotifyControl::kContinue;
		}

		static EquipEventHandler* GetSingleton()
		{
			static EquipEventHandler singleton;
			return std::addressof(singleton);
		}
	};


	// --- Listener for cell load events (scene load) ---
	struct CellLoadHandler : BSTEventSink<TESCellFullyLoadedEvent>
	{
		BSEventNotifyControl ProcessEvent(const TESCellFullyLoadedEvent* evn, BSTEventSource<TESCellFullyLoadedEvent>*)
		{
			if (evn) {
				logger::info("CellFullyLoadedEvent fired → logging equipped items");
				LogEquippedItems();
			}
			return BSEventNotifyControl::kContinue;
		}

		static CellLoadHandler* GetSingleton()
		{
			static CellLoadHandler singleton;
			return std::addressof(singleton);
		}
	};

}  // namespace



extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= Plugin::NAME;
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info(FMT_STRING("{} v{} queried"), Plugin::NAME, Plugin::VERSION);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION.major();

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_SSE_1_4_2) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{

	#ifdef _DEBUG
		while (!::IsDebuggerPresent()) {
		}
	#endif

	SKSE::Init(a_skse);

    AttackToggle::Start();

	auto messaging = SKSE::GetMessagingInterface();

	if (!messaging->RegisterListener("SKSE", OnSKSEMessage)) {
		logger::error("failed to init Message Handler");
		return false;
	}

	auto* sourceHolder = ScriptEventSourceHolder::GetSingleton();
	if (sourceHolder) {
		sourceHolder->AddEventSink(EquipEventHandler::GetSingleton());
		sourceHolder->AddEventSink(CellLoadHandler::GetSingleton());
		logger::info("Event handlers registered successfully.");
	}

	logger::info("{} v{} loaded"sv, Plugin::NAME, Plugin::VERSION.string());

	return true;
}

