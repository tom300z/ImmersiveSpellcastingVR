#include "spdlog/spdlog.h"

#include <atomic>
#include <chrono>
#include <format>
#include <string>
#include <string_view>
#include <thread>

#include "SKSE/API.h"
#include "SKSE/Interfaces.h"
#include "RE/Skyrim.h"
#include "REL/Module.h"
#include "SKSE/Trampoline.h"
#include "InputDispatcher.h"
#include "ConfigManager.h"
#include "InputInterceptor.h"
#include "Settings.h"
#include "ActionAllowedHook.h"
#include "openvr.h"
#include "utils.h"
#include <windows.h>
#include <haptics.h>
#include "SpellChargeTracker.h"
#include "compat/HapticSkyrimVR.h"
#include "compat/HapticSkyrimVRinterface001.h"
#include "PCH.h"
#include <compat/HIGGS.h>
#include <AllowShoutWhileCasting.h>

using namespace RE;
using namespace SKSE;

SKSE::PluginHandle g_pluginHandle;
std::string g_pluginName;

// Event Callbacks
void OnPlayerAnimationGraphEvent(const RE::BSAnimationGraphEvent& event)
{
	// Only run for player
	const auto* player = RE::PlayerCharacter::GetSingleton();
	if (event.holder != player) {
		return;
	}

	//logger::info("Event: {}", event.tag.c_str());

	// Refresh casting state after spell was equipped so it is immediately fired after draw. Also refresh after a weapon
	if (event.tag == "Magic_Equip_Out" || event.tag == "WeapEquip_Out") {
		InputInterceptor::RefreshCastingState();
	}
}

void OnSaveLoadEvent([[maybe_unused]] RE::TESLoadGameEvent event)
{
	SpellChargeTracker::Install();
	AllowShoutWhileCasting::Install();
	ActionAllowedHook::Install();
	Utils::Setup::PerformInteractiveSetup();

	// Add a listener to player animations. This needs to be done once per save load
	static auto playerAnimationGraphHandler = Utils::EventHandler<RE::BSAnimationGraphEvent>(OnPlayerAnimationGraphEvent);
	if (auto* player = RE::PlayerCharacter::GetSingleton()) {
		player->AddAnimationGraphEventSink(playerAnimationGraphHandler);
	}
}

void OnMenuOpenCloseEvent(const RE::MenuOpenCloseEvent& event)
{
	// Only handle game relevant menus
	if (!std::ranges::contains(Utils::kGameBlockingMenus, event.menuName.c_str())) {
		return;
	}

	// Pause haptics while in menus
	const bool inGame = Utils::InGame();
	Haptics::Pause(!inGame);
	InputDispatcher::Pause(!inGame);

	// Interrupt hand casters
	if (auto player = RE::PlayerCharacter::GetSingleton()) {
		for (auto caster : { player->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand), player->GetMagicCaster(RE::MagicSystem::CastingSource::kRightHand) }) {
			caster->InterruptCast(true);
		}
	}
	if (!event.opening && inGame) {

		if (Config::Manager::GetSingleton().Get<bool>(Settings::kInputCastAfterMenuExit, false)) {
			InputInterceptor::RefreshCastingState();
		} else {
			// Suppress active input until it has been released by the player
			InputDispatcher::leftDisp.SuppressUntilCasterInactive();
			InputDispatcher::rightDisp.SuppressUntilCasterInactive();
		}
	}
}

void OnSKSEMessage(SKSE::MessagingInterface::Message* msg)
{
	switch (msg->type) {
	case SKSE::MessagingInterface::kPostLoad:
		{

			HapticSkyrimVRPluginApi::getHapticSkyrimVRInterface001(
				g_pluginHandle,
				SKSE::GetMessagingInterface()
			);
			Compat::HapticSkyrimVR::DisableMagicHaptics(true);
			Compat::HIGGSUseTouchForGrip::Install();
		}
		break;
	case SKSE::MessagingInterface::kPostPostLoad:
		{
			static auto menuHandler = Utils::EventHandler<RE::MenuOpenCloseEvent>(OnMenuOpenCloseEvent);
			if (auto* ui = RE::UI::GetSingleton()) {
				ui->AddEventSink(&menuHandler);
			}
		}
		break;
	case SKSE::MessagingInterface::kDataLoaded:
		{
			//logger::info(FMT_STRING("kDataLoaded"), Plugin::NAME, Plugin::VERSION);
			Config::Init();

			InputInterceptor::ConnectToConfig();
		}
		break;
	}
}

extern "C" [[maybe_unused]] __declspec(dllexport) bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
#ifndef NDEBUG
		// Wait for debugger
		//while (!::IsDebuggerPresent());
#endif


	REL::Module::get().reset(); // Somehow the module resets itself during init, this works around the issue
	SKSE::Init(a_skse);
	g_pluginHandle = a_skse->GetPluginHandle();
	g_pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
	Utils::Setup::SetupLogger();

	SKSE::AllocTrampoline(1 << 10, false); // Unused for now, might come in handy later when i use write_call/write_branch

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", OnSKSEMessage)) {
		logger::error("failed to init Message Handler");
		return false;
	}

	static auto loadGameHandler = Utils::EventHandler<RE::TESLoadGameEvent>(OnSaveLoadEvent);
	auto* scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
	if (scriptEventSource) {
		scriptEventSource->AddEventSink(&loadGameHandler);
	}

	auto* papyrus = SKSE::GetPapyrusInterface();
	if (!papyrus->Register(Config::RegisterPapyrusFunctions)) {
		logger::error("Failed to register Papyrus interface for configuration");
		return false;
	}

	InputInterceptor::Install(a_skse);

	logger::info("{} loaded!", g_pluginName);
	return true;
}

