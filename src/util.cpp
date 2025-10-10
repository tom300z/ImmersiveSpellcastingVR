#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

namespace util
{
    void ExecuteConsoleCommand(std::string_view command)
    {
        auto* scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
        auto script = scriptFactory->Create();
        if (!script) {
            SKSE::log::warn("Failed to create Script object.");
            return;
        }

        script->SetCommand(command.data());
        script->CompileAndRun(nullptr);
        script->~Script();  // Free object memory manually
        SKSE::log::info("Executed script command: {}", command);
    }
}