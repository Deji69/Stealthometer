#pragma once
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <IPluginInterface.h>
#include <Glacier/ZEntity.h>
#include <Glacier/ZInput.h>
#include "json.hpp"
#include "Config.h"
#include "Events.h"
#include "LiveSplitClient.h"
#include "RunData.h"
#include "Stats.h"
#include "StatWindow.h"
#include "util.h"

struct ActorData
{
	const TEntityRef<ZActor>* ref = nullptr;
	bool isTarget = false;
	int highestTensionLevel = 0;
	ECompiledBehaviorType lastFrameBehaviour = ECompiledBehaviorType::BT_Invalid;
	std::string repoId;
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
	auto OnFrameUpdateAlways(const SGameUpdateEvent&) -> void;
	auto OnFrameUpdatePlayMode(const SGameUpdateEvent&) -> void;

	auto NewContract() -> void;
	auto UpdateDisplayStats() -> void;
	auto CalculateStealthRating() -> double;
	auto GetSilentAssassinStatus() const -> SilentAssassinStatus;
	auto ProcessLoadRemoval() -> void;

	auto InstallHooks() -> void;
	auto UninstallHooks() -> void;

private:
	auto SetupEvents() -> void;
	auto DrawSettingsUI(bool focused) -> void;
	auto DrawExpandedStatsUI(bool focused) -> void;
	auto DrawLiveSplitUI(bool focused) -> void;
	auto DrawOverlayUI(bool focused) -> void;
	auto IsContractEnded() const -> bool;
	auto IsRepoIdTargetNPC(const std::string& id) const -> bool;
	auto GetRepoEntry(const std::string& id) -> const nlohmann::json*;
	auto CreateItemInfo(const std::string& repoId) -> ItemInfo;
	auto AddObtainedItem(const std::string& id, ItemInfo item) -> void;
	auto RemoveObtainedItem(const std::string& id) -> int;
	auto AddDisposedItem(const std::string& id, ItemInfo item) -> void;
	auto GetNPCName(const std::string& id) -> const std::string*;

private:
	//DEFINE_PLUGIN_DETOUR(Stealthometer, void, ZGameStatsManager_SendAISignals, ZGameStatsManager* th);
	DECLARE_PLUGIN_DETOUR(Stealthometer, void, ZAchievementManagerSimple_OnEventSent, ZAchievementManagerSimple* th, uint32_t eventIndex, const ZDynamicObject& ev);
	DECLARE_PLUGIN_DETOUR(Stealthometer, void*, OnLoadingScreenActivated, void* th, void* a1);

private:
	SRWLOCK eventLock = {};
	Stats stats;
	DisplayStats displayStats;
	StatWindow window;
	EventSystem events;
	Config config;
	LiveSplitClient liveSplitClient;
	std::unordered_set<std::string, StringHashLowercase, InsensitiveCompare> freelanceTargets;
	std::array<ActorData, 1000> actorData;
	std::vector<std::string> eventHistory;
	std::mt19937 randomGenerator;
	std::unordered_map<std::string, nlohmann::json, StringHashLowercase, InsensitiveCompare> repo;
	std::unordered_map<std::string, std::string, StringHashLowercase, InsensitiveCompare> npcNames;

	RunData runData;
	FreelancerRunData freelancer;

	int npcCount = 0;
	double cutsceneEndTime = 0;
	double missionEndTime = 0;
	double lastEventTimestamp = 0;
	bool hooksInstalled = false;
	bool statVisibleUI = false;
	bool externalWindowEnabled = true;
	bool externalWindowDarkMode = true;
	bool externalWindowOnTop = false;
	bool showAllStats = false;
	bool liveSplitWindowOpen = false;
	bool inGameOverlayOpen = false;
	bool killsWindowOpen = false;
	bool pacifiesWindowOpen = false;
	bool miscWindowOpen = false;
	ImVec2 overlaySize = {};

	bool loadRemovalActive = false;
	bool isLoadingScreenCheckHasBeenTrue = false;
	bool loadingScreenActivated = false;
	bool startAfterLoad = false;
};

DEFINE_ZHM_PLUGIN(Stealthometer)
