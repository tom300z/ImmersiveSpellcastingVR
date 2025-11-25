#include "utils/Animation.h"

#include "RE/Skyrim.h"

namespace Utils::Animation
{
	void DumpPlayerGraphVariables()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			return;
		}

		RE::BSTSmartPointer<RE::BSAnimationGraphManager> manager;
		if (!player->GetAnimationGraphManager(manager) || !manager) {
			logger::warn("No animation graph manager yet");
			return;
		}

		const auto& cache = manager->variableCache.variableCache;
		logger::info("Graph cache has {} entries", cache.size());

		std::uint32_t index = 0;
		for (auto& graphPtr : manager->graphs) {
			auto* graph = graphPtr.get();
			if (!graph) {
				continue;
			}

			auto* behavior = graph->behaviorGraph;
			auto* data = behavior ? behavior->data.get() : nullptr;
			auto* strings = data ? data->stringData.get() : nullptr;
			logger::info("Graph[{}]: {} has {} variables", index++, graph->projectName.c_str(), strings ? strings->variableNames.size() : 0);
			if (!strings) {
				continue;
			}

			for (const auto& hkName : strings->variableNames) {
				const auto name = hkName.c_str();
				float f;
				std::int32_t i;
				bool b;

				if (graph->GetGraphVariableFloat(name, f)) {
					logger::info("  {} = {} (float)", name, f);
				} else if (graph->GetGraphVariableInt(name, i)) {
					logger::info("  {} = {} (int)", name, i);
				} else if (graph->GetGraphVariableBool(name, b)) {
					logger::info("  {} = {} (bool)", name, b);
				}
			}
		}
	}
}
