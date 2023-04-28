#pragma once
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <IPluginInterface.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZInput.h>
#include "json.hpp"
#include "Stats.h"
#include "StatWindow.h"
#include "util.h"

struct ActorData
{
	const TEntityRef<ZActor>* ref = nullptr;
	bool isTarget = false;
	int highestTensionLevel = 0;
	ECompiledBehaviorType lastFrameBehaviour;
	std::string repoId;
};

enum class StealthometerPlaystyle
{
};

class Stealthometer : public IPluginInterface
{
public:
	Stealthometer();
	~Stealthometer();

	auto Init() -> void override;
	auto OnEngineInitialized() -> void override;
	auto OnDrawUI(bool hasFocus) -> void override;
	auto OnDrawMenu() -> void override;
	auto OnFrameUpdate(const SGameUpdateEvent&) -> void;

	auto NewContract() -> void;
	auto UpdateDisplayStats() -> void;
	auto CalculateStealthRating() -> double;

private:
	auto DrawSettingsUI(bool focused) -> void;
	auto DrawExpandedStatsUI(bool focused) -> void;
	auto IsRepoIdTargetNPC(const std::string& id) -> bool;
	auto GetRepoEntry(const std::string& id) -> const nlohmann::json*;
	auto CreateItemInfo(const std::string& repoId) -> ItemInfo;
	auto AddObtainedItem(const std::string& id, ItemInfo item) -> void;
	auto RemoveObtainedItem(const std::string& id) -> int;
	auto AddDisposedItem(const std::string& id, ItemInfo item) -> void;

private:
	//DEFINE_PLUGIN_DETOUR(Stealthometer, void, ZGameStatsManager_SendAISignals, ZGameStatsManager* th);
	//DEFINE_PLUGIN_DETOUR(Stealthometer, void, ZKnowledge_SetGameTension, ZKnowledge* knowledge, EGameTension tension);
	DECLARE_PLUGIN_DETOUR(Stealthometer, void, ZAchievementManagerSimple_OnEventSent, ZAchievementManagerSimple* th, uint32_t eventIndex, const ZDynamicObject& ev);

private:
	SRWLOCK eventLock = {};
	Stats stats;
	DisplayStats displayStats;
	StatWindow window;
	std::unordered_set<std::string, StringHashLowercase> freelanceTargets;
	std::array<ActorData, 1000> actorData;
	std::vector<std::string> eventHistory;
	std::mt19937 randomGenerator;
	std::unordered_map<std::string, nlohmann::json, StringHashLowercase> repo;
	int npcCount = 0;
	float cutsceneEndTime = 0;
	bool statVisibleUI = false;
	bool externalWindowEnabled = true;
	bool externalWindowDarkMode = true;
	bool externalWindowOnTop = false;
	bool showAllStats = false;
	bool killsWindowOpen = false;
	bool pacifiesWindowOpen = false;
	bool miscWindowOpen = false;
};

DEFINE_ZHM_PLUGIN(Stealthometer)
