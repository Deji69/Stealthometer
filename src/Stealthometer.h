#pragma once
#include <vector>
#include <IPluginInterface.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZInput.h>
#include "Stats.h"
#include "StatWindow.h"

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
	auto DrawSettingsUI(bool hasFocus) -> void;
	auto IsRepoIdTargetNPC(std::string id) -> bool;

private:
	//DEFINE_PLUGIN_DETOUR(Stealthometer, void, ZGameStatsManager_SendAISignals, ZGameStatsManager* th);
	//DEFINE_PLUGIN_DETOUR(Stealthometer, void, ZKnowledge_SetGameTension, ZKnowledge* knowledge, EGameTension tension);
	DECLARE_PLUGIN_DETOUR(Stealthometer, void, ZAchievementManagerSimple_OnEventSent, ZAchievementManagerSimple* th, uint32_t eventIndex, const ZDynamicObject& ev);

private:
	SRWLOCK eventLock = {};
	Stats stats;
	DisplayStats displayStats;
	StatWindow window;
	std::array<ActorData, 1000> actorData;
	std::vector<std::string> eventHistory;
	bool statVisibleUI = false;
	bool externalWindowEnabled = true;
	bool externalWindowDarkMode = true;
	bool externalWindowOnTop = false;
};

DEFINE_ZHM_PLUGIN(Stealthometer)
