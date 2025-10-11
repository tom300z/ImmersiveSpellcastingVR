#include "spdlog/spdlog.h"

#include <string_view>

#include "SKSE/API.h"
#include "SKSE/Interfaces.h"
#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "SKSE/API.h"
#include "SKSE/Trampoline.h"
#include "AttackTicker.h"
#include "util.h"
//#include "InstantCharge.h"
//#include "DMMF_API.h"

using namespace RE;
using namespace SKSE;


void OnSKSEMessage(SKSE::MessagingInterface::Message* msg)
{
	logger::info("{}"sv, msg->type);
	switch (msg->type) {
	case SKSE::MessagingInterface::kInputLoaded:
			{
				logger::info(FMT_STRING("kInputLoaded"), Plugin::NAME, Plugin::VERSION);
				break;
			}
		case SKSE::MessagingInterface::kPostLoadGame:
			{
				logger::info(FMT_STRING("kPostLoadGame"), Plugin::NAME, Plugin::VERSION);
				log_utils::LogEquippedItems();
				break;
			}
	}
}

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
	spdlog::set_pattern("%s(%#): [%^%l%$] %v"s);

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

	#ifdef _DEBUGG
		while (!::IsDebuggerPresent()) {
		}
	#endif

	SKSE::Init(a_skse);

    //AttackToggle::Start();
    //InstantCharge::Install();

	auto messaging = SKSE::GetMessagingInterface();

	if (!messaging->RegisterListener("SKSE", OnSKSEMessage)) {
		logger::error("failed to init Message Handler");
		return false;
	}

	auto* sourceHolder = ScriptEventSourceHolder::GetSingleton();
	if (sourceHolder) {
		sourceHolder->AddEventSink(log_utils::EquipEventHandler::GetSingleton());
		logger::info("Event handlers registered successfully.");
	}

	logger::info("{} v{} loaded"sv, Plugin::NAME, Plugin::VERSION.string());

	return true;
}

