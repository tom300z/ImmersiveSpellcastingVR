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

using namespace RE;
using namespace SKSE;

namespace
{
	constexpr std::string_view ToString(RE::MagicSystem::CastingType a_type)
	{
		using RE::MagicSystem::CastingType;
		switch (a_type) {
		case CastingType::kConstantEffect:
			return "Constant Effect";
		case CastingType::kFireAndForget:
			return "Fire and Forget";
		case CastingType::kConcentration:
			return "Concentration";
		case CastingType::kScroll:
			return "Scroll";
		default:
			return "Unknown";
		}
	}

	void LogHandItem(const std::string_view a_handLabel, bool a_isLeftHand, RE::PlayerCharacter* a_player)
	{
		if (auto* form = a_player->GetEquippedObject(a_isLeftHand)) {
			logger::info("{} Hand: {} ({:08X})", a_handLabel, form->GetName(), form->GetFormID());

			if (auto* spell = form->As<RE::SpellItem>()) {
				const auto castingType = spell->GetCastingType();
				logger::info("  Spell Casting Type: {}", ToString(castingType));
				logger::info("  Spell Charge Time: {:.2f}s", spell->GetChargeTime());
			}
		} else {
			logger::info("{} Hand: empty", a_handLabel);
		}
	}

	// --- Our callback: log equipped item in each hand ---
	void LogEquippedItems()
	{
		auto* player = PlayerCharacter::GetSingleton();
		if (!player) {
			logger::info("Player not found.");
			return;
		}

		LogHandItem("Left", true, player);
		LogHandItem("Right", false, player);
		return;
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
					LogEquippedItems();
					break;
				}
		}
	}
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
		logger::info("Event handlers registered successfully.");
	}

	logger::info("{} v{} loaded"sv, Plugin::NAME, Plugin::VERSION.string());

	return true;
}

