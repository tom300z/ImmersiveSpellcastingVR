#include "utils/Console.h"

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

namespace Utils
{
	void ExecuteConsoleCommand(std::string_view command)
	{
		auto* scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
		auto script = scriptFactory ? scriptFactory->Create() : nullptr;
		if (!script) {
			SKSE::log::warn("Failed to create Script object.");
			return;
		}

		script->SetCommand(command.data());
		script->CompileAndRun(nullptr);
		script->~Script();
		SKSE::log::info("Executed script command: {}", command);
	}
}
