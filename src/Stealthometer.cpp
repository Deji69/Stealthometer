#include <Windows.h>
#include <functional>
#include <ranges>
#include <thread>
#include <Hooks.h>
#include <Logging.h>
#include <Functions.h>
#include <Glacier/EntityFactory.h>
#include <Glacier/SGameUpdateEvent.h>
#include <Glacier/SOnlineEvent.h>
#include <Glacier/ZAIGameState.h>
#include <Glacier/ZActor.h>
#include <Glacier/ZGameLoopManager.h>
#include <Glacier/ZKnowledge.h>
#include <Glacier/ZModule.h>
#include <Glacier/ZSpatialEntity.h>
#include <Glacier/ZScene.h>
#include <IconsMaterialDesign.h>
#include <cmrc/cmrc.hpp>
#include <imgui.h>
#include "deps/imgui/imgui_stdlib.h"

#include "Stealthometer.h"
#include "Enums.h"
#include "Events.h"
#include "Rating.h"
#include "Stats.h"
#include "json.hpp"
#include "FixMinMax.h"

using namespace std::string_literals;

#pragma comment(lib, "Ws2_32.lib")

CMRC_DECLARE(stealthometer);

HINSTANCE hInstance = nullptr;
HWND hWnd = nullptr;
ATOM wclAtom = NULL;

auto APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) -> BOOL {
	switch (reason) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
			hInstance = module;
			break;
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

Stealthometer::Stealthometer() : window(this->displayStats), randomGenerator(std::random_device{}()), config(*this), liveSplitClient(config.Get()) {
	this->SetupEvents();
}

Stealthometer::~Stealthometer() {
	this->UninstallHooks();
}

auto Stealthometer::Init() -> void
{
	InitializeSRWLock(&this->eventLock);

	auto const fs = cmrc::stealthometer::get_filesystem();

	if (!fs.is_file("data/repo.json")
		|| !fs.is_file("data/npc.json"))
		Logger::Error("Stealthometer: repo.json not found in embedded filesystem.");
	else {
		auto file = fs.open("data/repo.json");
		auto repo = nlohmann::json::parse(file.begin(), file.end());

		if (repo.is_array()) {
			for (auto const& entry : repo) {
				if (!entry.is_object()) continue;
				auto id = entry.find("ID_");
				if (id == entry.end()) continue;
				this->repo.emplace(id.value().get<std::string>(), entry);
			}
		}
		else Logger::Error("Stealthometer: repo.json invalid.");
	}
	if (fs.is_file("data/npc.json")) {
		auto file = fs.open("data/npc.json");
		auto repo = nlohmann::json::parse(file.begin(), file.end());

		if (repo.is_array()) {
			for (auto const& entry : repo) {
				if (!entry.is_object()) continue;
				auto id = entry.find("ID_");
				if (id == entry.end()) continue;
				auto name = entry.find("Name");
				if (name == entry.end()) continue;
				this->npcNames.emplace(id.value().get<std::string>(), name.value().get<std::string>());
			}
		}
		else Logger::Error("Stealthometer: npc.json invalid.");
	}
	//this->window.create(hInstance);
}

auto Stealthometer::InstallHooks() -> void {
	if (this->hooksInstalled) return;

	const ZMemberDelegate<Stealthometer, void(const SGameUpdateEvent&)> frameUpdateDelegatePlayMode(this, &Stealthometer::OnFrameUpdatePlayMode);
	Globals::GameLoopManager->RegisterFrameUpdate(frameUpdateDelegatePlayMode, 1, EUpdateMode::eUpdatePlayMode);

	const ZMemberDelegate<Stealthometer, void(const SGameUpdateEvent&)> frameUpdateDelegateAlways(this, &Stealthometer::OnFrameUpdateAlways);
	Globals::GameLoopManager->RegisterFrameUpdate(frameUpdateDelegateAlways, 0, EUpdateMode::eUpdateAlways);

	Hooks::ZLoadingScreenVideo_ActivateLoadingScreen->AddDetour(this, &Stealthometer::OnLoadingScreenActivated);
	Hooks::ZAchievementManagerSimple_OnEventSent->AddDetour(this, &Stealthometer::ZAchievementManagerSimple_OnEventSent);

	this->hooksInstalled = true;
}

auto Stealthometer::UninstallHooks() -> void {
	if (!this->hooksInstalled) return;

	const ZMemberDelegate<Stealthometer, void(const SGameUpdateEvent&)> frameUpdateDelegatePlayMode(this, &Stealthometer::OnFrameUpdatePlayMode);
	Globals::GameLoopManager->UnregisterFrameUpdate(frameUpdateDelegatePlayMode, 1, EUpdateMode::eUpdatePlayMode);

	const ZMemberDelegate<Stealthometer, void(const SGameUpdateEvent&)> frameUpdateDelegateAlways(this, &Stealthometer::OnFrameUpdateAlways);
	Globals::GameLoopManager->UnregisterFrameUpdate(frameUpdateDelegateAlways, 0, EUpdateMode::eUpdateAlways);

	Hooks::ZLoadingScreenVideo_ActivateLoadingScreen->RemoveDetour(&Stealthometer::OnLoadingScreenActivated);
	Hooks::ZAchievementManagerSimple_OnEventSent->RemoveDetour(&Stealthometer::ZAchievementManagerSimple_OnEventSent);

	this->hooksInstalled = false;
}

auto Stealthometer::OnEngineInitialized() -> void
{
	config.Load();
	this->InstallHooks();

	if (config.Get().externalWindow)
		this->window.create(hInstance);

	if (config.Get().liveSplitEnabled)
		this->liveSplitClient.start();
}

auto Stealthometer::IsRepoIdTargetNPC(const std::string& id) const -> bool {
	if (this->freelanceTargets.contains(id))
		return true;

	for (auto const& actor : this->actorData) {
		if (!actor.isTarget) continue;
		if (InsensitiveCompare{}(id, actor.repoId)) return true;
	}
	return false;
}

auto Stealthometer::GetRepoEntry(const std::string& id) -> const nlohmann::json* {
	if (!id.empty()) {
		auto it = this->repo.find(id);
		if (it != this->repo.end()) return &it->second;
	}
	return nullptr;
}

auto Stealthometer::GetNPCName(const std::string& id) -> const std::string* {
	if (!id.empty()) {
		auto it = this->npcNames.find(id);
		if (it != this->npcNames.end()) return &it->second;
	}
	return nullptr;
}

auto Stealthometer::OnFrameUpdateAlways(const SGameUpdateEvent& ev) -> void {
	this->ProcessLoadRemoval();
}

auto Stealthometer::OnFrameUpdatePlayMode(const SGameUpdateEvent& ev) -> void {
	for (int i = 0; i < *Globals::NextActorId; ++i) {
		const auto& actor = Globals::ActorManager->m_aActiveActors[i];
		const auto actorSpatial = actor.m_ref.QueryInterface<ZSpatialEntity>();
		auto& actorData = this->actorData[i];

		if (!actorData.ref) {
			actorData.ref = &actor;
			auto repoEntity = actor.m_ref.QueryInterface<ZRepositoryItemEntity>();
			actorData.repoId = repoEntity->m_sId.ToString();
			actorData.isTarget = actor.m_pInterfaceRef->m_bUnk16;
		}

		if (!actorSpatial)
			continue;

		if (i > this->npcCount)
			this->npcCount = i;

		if (actor.m_pInterfaceRef->m_nCurrentBehaviorIndex >= 0) {
			// (&behaviour + 0xD8) = m_pPreviousBehavior ?

			auto& behaviour = Globals::BehaviorService->m_aKnowledgeData[actor.m_pInterfaceRef->m_nCurrentBehaviorIndex];
			if (!behaviour.m_pCurrentBehavior) continue;
			auto behaviourType = static_cast<ECompiledBehaviorType>(behaviour.m_pCurrentBehavior->m_Type);
			auto lastBehaviourType = actorData.lastFrameBehaviour;
			actorData.lastFrameBehaviour = behaviourType;

			if (lastBehaviourType == behaviourType) continue;

			auto tension = getBehaviourTension(behaviourType);

			if (behaviourType != ECompiledBehaviorType::BT_Act)
				Logger::Debug("{}: {}", behaviourToString(behaviourType), tension);

			if (!tension) continue;

			if (behaviourType == ECompiledBehaviorType::BT_CloseCombat)
				++this->stats.misc.closeCombatEngagements;

			if (tension > actorData.highestTensionLevel) {
				if (actorData.highestTensionLevel) tension -= actorData.highestTensionLevel;
				actorData.highestTensionLevel += tension;
				this->stats.tension.level += tension;
				this->UpdateDisplayStats();
			}
		}
	}
}

auto Stealthometer::ProcessLoadRemoval() -> void {
	class ZRenderManager {
	public:
		virtual ~ZRenderManager() = default;
		virtual bool ZRenderManager_unk1() = 0;
		virtual bool ZRenderManager_unk2() = 0;
		virtual bool ZRenderManager_unk3() = 0;
		virtual bool ZRenderManager_unk4() = 0; //
		virtual bool ZRenderManager_unk5() = 0; // gracefully freezes the game??
		virtual bool IsLoadingScreenActive() = 0;

	public:
		PAD(0x14178);
		ZRenderDevice* m_pDevice; // 0x14180, look for ZRenderDevice constructor
		PAD(0xF8); // 0x14188
		ZRenderContext* m_pRenderContext; // 0x14280, look for "ZRenderManager::RenderThread" string, first thing being constructed and assigned
	};

	static_assert(offsetof(ZRenderManager, m_pDevice) == 0x14180);
	static_assert(offsetof(ZRenderManager, m_pRenderContext) == 0x14280);

	auto ptr = Globals::RenderManager;
	auto renderManager = reinterpret_cast<ZRenderManager*>(Globals::RenderManager);
	if (!renderManager) return;

	auto isLoadingScreenActive = renderManager->IsLoadingScreenActive();

	if ((isLoadingScreenActive || loadingScreenActivated) && !loadRemovalActive) {
		liveSplitClient.pause();
		loadRemovalActive = true;
	}

	if (isLoadingScreenActive)
		isLoadingScreenCheckHasBeenTrue = true;
	else if (isLoadingScreenCheckHasBeenTrue) {
		loadingScreenActivated = false;
		isLoadingScreenCheckHasBeenTrue = false;

		if (loadRemovalActive) {
			if (this->runData.shouldAutoStartLiveSplit) {
				liveSplitClient.send(this->startAfterLoad ? eClientMessage::StartTimer : eClientMessage::Resume);
				this->startAfterLoad = false;
			}
			loadRemovalActive = false;
		}
	}
}

auto Stealthometer::OnDrawMenu() -> void {
	if (ImGui::Button(ICON_MD_PIE_CHART " STEALTHOMETER"))
		this->statVisibleUI = !this->statVisibleUI;
}

auto Stealthometer::DrawSettingsUI(bool focused) -> void {
	if (!focused) return;

	ImGui::PushFont(SDK()->GetImGuiBlackFont());

	ImGui::SetNextWindowSizeConstraints(ImVec2{100, 300}, ImVec2{500, 500});

	if (ImGui::Begin(ICON_MD_SETTINGS " STEALTHOMETER", &this->statVisibleUI, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::PushFont(SDK()->GetImGuiRegularFont());
		auto& cfg = config.Get();

		if (ImGui::Checkbox("Overlay", &cfg.inGameOverlay)) {
			config.Save();
		}

		ImGui::SameLine(120.0);

		auto selectedOverlayDockName = "Undocked";

		switch (cfg.overlayDockMode) {
			case DockMode::TopLeft:
				selectedOverlayDockName = "Top Left";
				break;
			case DockMode::TopRight:
				selectedOverlayDockName = "Top Right";
				break;
			case DockMode::BottomLeft:
				selectedOverlayDockName = "Bottom Left";
				break;
			case DockMode::BottomRight:
				selectedOverlayDockName = "Bottom Right";
				break;
			case DockMode::BelowMinimap:
				selectedOverlayDockName = "Below Minimap";
				break;
		}

		if (ImGui::BeginCombo("##OverlayDock", selectedOverlayDockName, ImGuiComboFlags_HeightLarge)) {
			if (ImGui::Selectable("Undocked", cfg.overlayDockMode == DockMode::None, 0)) {
				cfg.overlayDockMode = DockMode::None;
				this->config.Save();
			}
			if (cfg.overlayDockMode == DockMode::None) ImGui::SetItemDefaultFocus();

			if (ImGui::Selectable("Top Left", cfg.overlayDockMode == DockMode::TopLeft, 0)) {
				cfg.overlayDockMode = DockMode::TopLeft;
				this->config.Save();
			}
			if (cfg.overlayDockMode == DockMode::TopLeft) ImGui::SetItemDefaultFocus();

			if (ImGui::Selectable("Top Right", cfg.overlayDockMode == DockMode::TopRight, 0)) {
				cfg.overlayDockMode = DockMode::TopRight;
				this->config.Save();
			}
			if (cfg.overlayDockMode == DockMode::TopRight) ImGui::SetItemDefaultFocus();

			if (ImGui::Selectable("Bottom Left", cfg.overlayDockMode == DockMode::BottomLeft, 0)) {
				cfg.overlayDockMode = DockMode::BottomLeft;
				this->config.Save();
			}
			if (cfg.overlayDockMode == DockMode::BottomLeft) ImGui::SetItemDefaultFocus();

			if (ImGui::Selectable("Bottom Right", cfg.overlayDockMode == DockMode::BottomRight, 0)) {
				cfg.overlayDockMode = DockMode::BottomRight;
				this->config.Save();
			}
			if (cfg.overlayDockMode == DockMode::BottomRight) ImGui::SetItemDefaultFocus();

			if (ImGui::Selectable("Above Minimap", cfg.overlayDockMode == DockMode::AboveMinimap, 0)) {
				cfg.overlayDockMode = DockMode::AboveMinimap;
				this->config.Save();
			}
			if (cfg.overlayDockMode == DockMode::AboveMinimap) ImGui::SetItemDefaultFocus();

			if (ImGui::Selectable("Below Minimap", cfg.overlayDockMode == DockMode::BelowMinimap, 0)) {
				cfg.overlayDockMode = DockMode::BelowMinimap;
				this->config.Save();
			}
			if (cfg.overlayDockMode == DockMode::BelowMinimap) ImGui::SetItemDefaultFocus();

			ImGui::EndCombo();
		}

		auto selectedDetailName = "Normal";

		switch (cfg.overlayDetail) {
			case OverlayDetailMode::Normal:
				selectedDetailName = "Normal";
				break;
			case OverlayDetailMode::WithNames:
				selectedDetailName = "With NPC Names";
				break;
		}

		if (ImGui::BeginCombo("##OverlayDetail", selectedDetailName, ImGuiComboFlags_HeightLarge)) {
			if (ImGui::Selectable("Normal", cfg.overlayDetail == OverlayDetailMode::Normal, 0)) {
				cfg.overlayDetail = OverlayDetailMode::Normal;
				this->config.Save();
			}
			if (cfg.overlayDetail == OverlayDetailMode::Normal) ImGui::SetItemDefaultFocus();

			if (ImGui::Selectable("With NPC Names", cfg.overlayDetail == OverlayDetailMode::WithNames, 0)) {
				cfg.overlayDetail = OverlayDetailMode::WithNames;
				this->config.Save();
			}
			if (cfg.overlayDetail == OverlayDetailMode::WithNames) ImGui::SetItemDefaultFocus();

			ImGui::EndCombo();
		}

		
		if (ImGui::Checkbox("Transparent Overlay", &cfg.overlayTransparency)) {
			config.Save();
		}
		
		if (ImGui::Checkbox("Use Shorthand", &cfg.useExtendedShorthand)) {
			config.Save();
		}

		if (ImGui::Checkbox("External Window", &cfg.externalWindow)) {
			if (cfg.externalWindow) this->window.create(hInstance);
			else this->window.destroy();
			config.Save();
		}
		if (ImGui::Checkbox("External Window Dark Mode", &cfg.externalWindowDark)) {
			this->window.setDarkMode(cfg.externalWindowDark);
			config.Save();
		}

		if (ImGui::Checkbox("External Window On Top", &cfg.externalWindowOnTop)) {
			this->window.setAlwaysOnTop(cfg.externalWindowOnTop);
			config.Save();
		}

		if (ImGui::Button("LiveSplit")) this->liveSplitWindowOpen = true;

		if (ImGui::Button("Kill Stats")) this->killsWindowOpen = true;
		ImGui::SameLine();
		if (ImGui::Button("KO Stats")) this->pacifiesWindowOpen = true;
		ImGui::SameLine();
		if (ImGui::Button("Misc Stats")) this->miscWindowOpen = true;

		ImGui::PopFont();
	}

	ImGui::End();
	ImGui::PopFont();
}

auto Stealthometer::DrawOverlayUI(bool focused) -> void
{
	auto& cfg = config.Get();
	if (!cfg.inGameOverlay) return;

	auto viewportSize = ImGui::GetMainViewport()->Size;
	auto flags = static_cast<ImGuiWindowFlags>(ImGuiWindowFlags_AlwaysAutoResize);

	if (cfg.overlayDockMode != DockMode::None || !focused)
		flags |= ImGuiWindowFlags_NoTitleBar;

	if (cfg.overlayTransparency)
		flags |= ImGuiWindowFlags_NoBackground;

	switch (cfg.overlayDockMode) {
		case DockMode::TopLeft:
			ImGui::SetNextWindowPos({0, 0});
			break;
		case DockMode::TopRight:
			ImGui::SetNextWindowPos({viewportSize.x - this->overlaySize.x, 0});
			break;
		case DockMode::BottomLeft:
			ImGui::SetNextWindowPos({0, viewportSize.y - this->overlaySize.y});
			break;
		case DockMode::BottomRight:
			ImGui::SetNextWindowPos({viewportSize.x - this->overlaySize.x, viewportSize.y - this->overlaySize.y});
			break;
		case DockMode::AboveMinimap:
			ImGui::SetNextWindowPos({ 34, viewportSize.y - this->overlaySize.y - 240 });
			break;
		case DockMode::BelowMinimap:
			ImGui::SetNextWindowPos({ 34, viewportSize.y - this->overlaySize.y + 11 });
			break;
		case DockMode::LeftBelowMinimap:
			ImGui::SetNextWindowPos({ 0, viewportSize.y - this->overlaySize.y + 11 });
			break;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	if (ImGui::Begin(ICON_MD_PIE_CHART " STATUS", &cfg.inGameOverlay, flags)) {
		this->overlaySize = ImGui::GetWindowSize();

		ImGui::PushFont(SDK()->GetImGuiBoldFont());

		if (this->displayStats.silentAssassin == SilentAssassinStatus::OK) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 255, 0, 255));
			ImGui::Text("Silent Assassin");
			ImGui::PopStyleColor();
		}
		else if (this->displayStats.silentAssassin == SilentAssassinStatus::RedeemableCamera) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(217, 109, 0, 255));
			ImGui::Text("Cams");
			ImGui::PopStyleColor();
		}
		else if (this->displayStats.silentAssassin == SilentAssassinStatus::RedeemableTarget) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(217, 109, 0, 255));
			if (!this->stats.firstSpottedByName.empty()) {
				ImGui::Text(this->stats.firstSpottedByName.c_str());
			}
			else if (!this->stats.firstBodyFoundByName.empty()) {
				ImGui::Text(this->stats.firstBodyFoundByName.c_str());
			}
			else ImGui::Text("Target");
			ImGui::PopStyleColor();
		}
		else if (this->displayStats.silentAssassin == SilentAssassinStatus::RedeemableCameraAndTarget) {
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(217, 109, 0, 255));
			ImGui::Text("Cams | Target");
			ImGui::PopStyleColor();
		}
		else {
			std::string str;
			auto enableNames = cfg.overlayDetail == OverlayDetailMode::WithNames;

			if (this->stats.detection.nonTargetsSpottedBy > 0) {
				str += "Spotted";
				if (enableNames && !this->stats.firstSpottedByName.empty() && this->stats.firstSpottedByName != this->stats.firstBodyFoundByName) {
					str += " by " + this->stats.firstSpottedByName;
				}
			}
			if (this->stats.bodies.foundMurderedByNonTarget > 0) {
				str += (str.empty() ? ""s : " | "s) + "Body Found"s;
				if (enableNames && !this->stats.firstBodyFoundByName.empty()) {
					str += " by " + this->stats.firstBodyFoundByName;
				}
			}
			if (this->displayStats.civilianKills > 0 || this->displayStats.guardKills > 0) {
				str += (str.empty() ? ""s : " | "s) + (cfg.useExtendedShorthand ? "NTK"s : "Non-Target Kill"s);
				if (enableNames && !this->stats.firstNTKName.empty()) {
					str += " of " + this->stats.firstNTKName;
				}
			}

			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
			ImGui::Text(str.c_str());
			ImGui::PopStyleColor();
		}

		ImGui::PopFont();
	}
	ImGui::End();
	ImGui::PopFont();
}

auto Stealthometer::DrawLiveSplitUI(bool focused) -> void
{
	if (!this->liveSplitWindowOpen) return;
	if (!focused) return;

	auto& cfg = config.Get();

	ImGui::PushFont(SDK()->GetImGuiBlackFont());

	ImGui::SetNextWindowSizeConstraints(ImVec2{450, 400}, ImVec2{600, -1});
	if (ImGui::Begin(ICON_MD_TIMER " LIVESPLIT", &this->liveSplitWindowOpen)) {
		ImGui::PushFont(SDK()->GetImGuiRegularFont());

		auto connected = liveSplitClient.isConnected();
		ImGui::PushStyleColor(ImGuiCol_Text, connected ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 0, 0, 255));
		ImGui::TextUnformatted(connected ? "Connected" : "Disconnected");
		ImGui::PopStyleColor();

		if (ImGui::Checkbox("Enable", &cfg.liveSplitEnabled)) {
			config.Save();

			if (!cfg.liveSplitEnabled && liveSplitClient.isStarted())
				liveSplitClient.stop();
			else if (cfg.liveSplitEnabled && !liveSplitClient.isStarted())
				liveSplitClient.start();
		}
		if (ImGui::InputText("IP", &cfg.liveSplitIP)) {
			auto res = cfg.liveSplitIP.empty() ? INADDR_NONE : inet_addr(cfg.liveSplitIP.c_str());
			if (res == INADDR_NONE)
				cfg.liveSplitIP = "127.0.0.1";
			config.Save();
		}
		static std::string portInput;
		portInput = std::format("{}", cfg.liveSplitPort);
		if (ImGui::InputText("Port", &portInput)) {
			if (portInput.size() > 5) portInput.clear();
			uint16_t port = 16834;
			auto res = !portInput.empty() ? std::from_chars(portInput.c_str(), portInput.c_str() + portInput.size(), port).ec : std::errc{};
			if (res != std::errc{})
				port = 16834;
			cfg.liveSplitPort = port;
			config.Save();
		}

		if (cfg.liveSplitEnabled && connected) {
			if (ImGui::Button("Reset")) {
				liveSplitClient.send(eClientMessage::Reset);
				this->runData.shouldAutoStartLiveSplit = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Split"))
				liveSplitClient.send(eClientMessage::Split);
			ImGui::SameLine();
			if (ImGui::Button("Unsplit"))
				liveSplitClient.send(eClientMessage::Unsplit);
			ImGui::SameLine();
			if (ImGui::Button("Pause/Resume"))
				liveSplitClient.send(eClientMessage::Pause);
		}

		ImGui::PopFont();
	}

	ImGui::End();
	ImGui::PopFont();
}

auto Stealthometer::DrawExpandedStatsUI(bool focused) -> void {
	auto const& stats = this->stats;
	auto printRow = []<typename T>(const char* label, const char* fmt, T arg) {
		if (ImGui::TableNextColumn()) ImGui::Text(fmt, arg);
		if (ImGui::TableNextColumn()) ImGui::TextUnformatted(label);
	};

	ImGui::PushFont(SDK()->GetImGuiBlackFont());

	if (this->killsWindowOpen) {
		ImGui::SetNextWindowSizeConstraints(ImVec2{250, 200}, ImVec2{600, -1});

		if (ImGui::Begin(ICON_MD_PIE_CHART " KILLS", &this->killsWindowOpen)) {
			ImGui::PushFont(SDK()->GetImGuiRegularFont());

			if (ImGui::BeginChild("KillsL", ImVec2{ImGui::GetContentRegionAvail().x * .5f, 115})) {
				if (ImGui::BeginTable("KillStatsTableL", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
					printRow("Guard", "%d", stats.kills.guard);
					printRow("Target", "%d", stats.kills.targets);
					printRow("Unnoticed", "%d", stats.kills.unnoticed);
					printRow("Unnoticed Non-Target", "%d", stats.kills.unnoticedNonTarget);
				}
				ImGui::EndTable();
			}

			ImGui::EndChild();
			ImGui::SameLine();

			if (ImGui::BeginChild("KillsR", ImVec2{0, 110})) {
				if (ImGui::BeginTable("KillStatsTableR", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
					printRow("Civilian", "%d", stats.kills.civilian);
					printRow("Non-Target", "%d", stats.kills.nonTargets);
					printRow("Noticed", "%d", stats.kills.noticed);
				}
				ImGui::EndTable();
			}

			ImGui::EndChild();

			if (ImGui::BeginTable("KillStatsTableT", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
				printRow("Total", "%d", stats.kills.total);
			}

			ImGui::EndTable();
			ImGui::PopFont();
		}
		ImGui::End();
	}

	if (this->pacifiesWindowOpen) {
		ImGui::SetNextWindowSizeConstraints(ImVec2 { 250, 200 }, ImVec2 { 600, -1 });

		if (ImGui::Begin(ICON_MD_PIE_CHART " PACIFICATIONS", &this->pacifiesWindowOpen)) {
			ImGui::PushFont(SDK()->GetImGuiRegularFont());

			if (ImGui::BeginChild("PacificationsL", ImVec2{ImGui::GetContentRegionAvail().x * .5f, 115})) {
				if (ImGui::BeginTable("KOStatsTableL", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
					printRow("Guard", "%d", stats.pacifies.guard);
					printRow("Target", "%d", stats.pacifies.targets);
					printRow("Unnoticed", "%d", stats.pacifies.unnoticed);
					printRow("Unnoticed Non-Target", "%d", stats.pacifies.unnoticedNonTarget);
				}
				ImGui::EndTable();
			}
			ImGui::EndChild();
			ImGui::SameLine();
			if (ImGui::BeginChild("PacificationsR", ImVec2{0, 110})) {
				if (ImGui::BeginTable("KOStatsTableR", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
					printRow("Civilian", "%d", stats.pacifies.civilian);
					printRow("Non-Target", "%d", stats.pacifies.nonTargets);
					printRow("Noticed", "%d", stats.pacifies.noticed);
				}
				ImGui::EndTable();
			}
			ImGui::EndChild();
			if (ImGui::BeginTable("KOStatsTableT", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 30);
				printRow("Total", "%d", stats.pacifies.total);
			}
			ImGui::EndTable();
			ImGui::PopFont();
		}
		ImGui::End();
	}

	if (this->miscWindowOpen) {
		ImGui::SetNextWindowSizeConstraints(ImVec2 { 250, 200 }, ImVec2 { 600, -1 });

		if (ImGui::Begin(ICON_MD_PIE_CHART " MISC STATS", &this->miscWindowOpen)) {
			ImGui::PushFont(SDK()->GetImGuiRegularFont());

			if (ImGui::BeginTable("MiscTableL", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 15);
				printRow("Recorded", "%s", stats.misc.recordedThenErased || stats.detection.onCamera ? "Yes" : "No");
				printRow("Recorder Destroyed", "%s", stats.misc.recorderDestroyed ? "Yes" : "No");
				printRow("Recorder Erased", "%s", stats.misc.recorderErased ? "Yes" : "No");
				printRow("Suit Retrieved", "%s", stats.misc.suitRetrieved ? "Yes" : "No");
				printRow("Agilities", "%d", stats.misc.agilityActions);
				printRow("Cameras Destroyed", "%d", stats.misc.camerasDestroyed);
				printRow("Disguises Blown", "%d", stats.disguisesBlown.size());
				printRow("Disguises Taken", "%d", stats.misc.disguisesTaken);
				printRow("Doors Unlocked", "%d", stats.misc.doorsUnlocked);
				printRow("Items Obtained", "%d", stats.misc.itemsPickedUp);
				printRow("Items Lost", "%d", stats.misc.itemsRemovedFromInventory);
				printRow("Items Thrown", "%d", stats.misc.itemsThrown);
				printRow("Setpieces Destroyed", "%d", stats.misc.setpiecesDestroyed);
				printRow("Targets Made Sick", "%d", stats.misc.targetsMadeSick);
				printRow("Times Trespassed", "%d", stats.misc.timesTrespassed);
			}
			ImGui::EndTable();
			ImGui::PopFont();
		}
		ImGui::End();
	}

	ImGui::PopFont();
}

auto Stealthometer::OnDrawUI(bool focused) -> void {
	this->DrawExpandedStatsUI(focused);
	this->DrawLiveSplitUI(focused);
	this->DrawOverlayUI(focused);

	if (!this->statVisibleUI) return;

	this->DrawSettingsUI(focused);
}

auto Stealthometer::NewContract() -> void {
	for (auto& actorData : this->actorData) {
		actorData = ActorData{};
	}

	this->stats = Stats();
	this->displayStats = DisplayStats();
	this->npcCount = 0;
	this->missionEndTime = 0;
	this->cutsceneEndTime = 0;
	this->freelanceTargets.clear();
	this->eventHistory.clear();
	this->window.update();
}

auto Stealthometer::CreateItemInfo(const std::string& id) -> ItemInfo {
	ItemInfo item;
	item.type = ItemInfoType::None;
	auto entry = this->GetRepoEntry(id);
	if (entry) {
		auto itemType = entry->value("ItemType", "");
		auto inventoryCategoryIcon = entry->value("InventoryCategoryIcon", "");
		auto itemInfoType = ItemInfoType::Other;

		if (itemType == "eOther_Keycard_A") {
			itemInfoType = ItemInfoType::Key;
			++stats.misc.keyItemsPickedUp;
		}
		else if (itemType == "eDetonator" && inventoryCategoryIcon == "remote") {
			itemInfoType = ItemInfoType::Detonator;
		}
		else if (itemType == "eCC_Brick") {
			++stats.misc.itemsPickedUp;
		}
		else {
			++stats.misc.itemsPickedUp;

			if (itemType == "eDetonator" && inventoryCategoryIcon == "distraction")
				itemInfoType = ItemInfoType::Coin;
			else if (itemType == "eItemAmmo")
				itemInfoType = ItemInfoType::AmmoBox;
			else if (inventoryCategoryIcon == "QuestItem" || inventoryCategoryIcon == "questitem") {
				itemInfoType = ItemInfoType::Intel;
				++stats.misc.intelItemsPickedUp;
			}
			else if (inventoryCategoryIcon == "poison")
				itemInfoType = ItemInfoType::Poison;
			else if (inventoryCategoryIcon == "melee") {
				if (itemType == "eCC_Knife") itemInfoType = ItemInfoType::LethalMelee;
				else itemInfoType = ItemInfoType::Melee;
			}
			else if (inventoryCategoryIcon == "explosives") {
				itemInfoType = ItemInfoType::Explosive;
			}
			else if (
				inventoryCategoryIcon == "pistol"
				|| inventoryCategoryIcon == "smg"
				|| inventoryCategoryIcon == "shotgun"
				|| inventoryCategoryIcon == "assaultrifle"
				|| inventoryCategoryIcon == "sniperrifle"
			) {
				itemInfoType = ItemInfoType::Firearm;
			}
		}

		switch (itemInfoType) {
			case ItemInfoType::Detonator: break;
			default:
				if (id.empty()) break;
				item.type = itemInfoType;
				item.name = entry->value("Title", "");
				item.commonName = entry->value("CommonName", "");
				item.itemType = itemType;
				item.inventoryCategoryIcon = inventoryCategoryIcon;
				break;
		}
	}
	return item;
}

auto Stealthometer::AddObtainedItem(const std::string& id, ItemInfo item) -> void {
	if (item.type == ItemInfoType::None) return;
	if (id.empty()) return;
	auto it = this->stats.itemsObtained.find(id);
	if (it != this->stats.itemsObtained.end())
		++it->second.count;
	else
		this->stats.itemsObtained.emplace(id, item);
}

auto Stealthometer::AddDisposedItem(const std::string& id, ItemInfo item) -> void {
	if (item.type == ItemInfoType::None) return;
	if (id.empty()) return;
	auto it = this->stats.itemsDisposed.find(id);
	if (it != this->stats.itemsDisposed.end())
		++it->second.count;
	else
		this->stats.itemsDisposed.emplace(id, item);
}

auto Stealthometer::RemoveObtainedItem(const std::string& id) -> int {
	if (id.empty()) return -1;
	auto it = stats.itemsObtained.find(id);
	if (it != stats.itemsObtained.end()) {
		if (it->second.count > 1) return --it->second.count;
		stats.itemsObtained.erase(it);
		return 0;
	}
	return -1;
}

auto Stealthometer::GetSilentAssassinStatus() const -> SilentAssassinStatus {
	// Non-Target Kills
	auto nonTargetKills = this->stats.kills.nonTargets.size() + this->stats.kills.crowd;
	if (nonTargetKills > 0) return SilentAssassinStatus::Fail;

	// Spotted
	auto isKilled = [this](const std::string& id) {
		return this->stats.kills.targets.contains(id)
			|| this->stats.kills.nonTargets.contains(id);
	};
	auto isTarget = [this](const std::string& id) {
		return this->IsRepoIdTargetNPC(id);
	};
	auto witnessesNotKilled = this->stats.witnesses | std::views::filter(std::not_fn(isKilled));
	auto spottedByNotKilled = this->stats.spottedBy | std::views::filter(std::not_fn(isKilled));
	auto witnessesNonTarget = witnessesNotKilled | std::views::filter(std::not_fn(isTarget));
	auto spottedByNonTarget = spottedByNotKilled | std::views::filter(std::not_fn(isTarget));
	auto numWitnessesNT = std::distance(witnessesNonTarget.begin(), witnessesNonTarget.end());
	auto numSpottedByNT = std::distance(spottedByNonTarget.begin(), spottedByNonTarget.end());

	if (numWitnessesNT > 0 || numSpottedByNT > 0)
		return SilentAssassinStatus::Fail;

	// TODO: Learn if there are any situations that invalidate 'No Noticed Kills' independently from 'Never Spotted'.
	// Otherwise, it's pointless considering this for SA tracking. 'No Noticed Kills' is seemingly not based on noticed kills.
	
	// Noticed Kills
	//if (stats.kills.noticed > 0)
	//	return SilentAssassinStatus::Fail;

	// Bodies Found - if body found by non-target, it's definitely not recoverable.
	if (this->stats.bodies.foundMurderedByNonTarget > 0)
		return SilentAssassinStatus::Fail;

	// We can shortcut the target redeemable SA logic by comparing the number of target witnesses vs. the number of target witnesses killed.
	auto spottedByTarget = this->stats.targetBodyWitnesses.size() > this->stats.bodies.targetBodyWitnessesKilled
		|| this->stats.targetsSpottedBy.size() > this->stats.detection.targetsSpottedByAndKilled;

	// Evidence - also redeemable. Check if we have redeemability from both target and cams.
	if (this->stats.detection.onCamera) {
		if (spottedByTarget)
			return SilentAssassinStatus::RedeemableCameraAndTarget;

		return SilentAssassinStatus::RedeemableCamera;
	}

	return spottedByTarget ? SilentAssassinStatus::RedeemableTarget : SilentAssassinStatus::OK;
}

auto Stealthometer::CalculateStealthRating() -> double {
	auto rating = 100.0;
	rating -= this->stats.kills.civilian * 8;
	rating -= this->stats.kills.guard * 5;
	rating -= this->displayStats.witnesses * 10;
	rating -= this->stats.detection.onCamera * 15;
	rating -= this->displayStats.bodiesFound * 5;
	rating -= this->stats.detection.spotted * 5;
	rating -= std::max(this->stats.pacifies.nonTargets - 3, 0) * 2;
	rating += std::min(this->displayStats.bodiesHidden * 3, 15);
	// TODO: more + adjustments
	return std::min(std::max(rating, 0.0), 100.0);
}

auto Stealthometer::UpdateDisplayStats() -> void {
	this->window.update();

	auto updated = false;

	// Tension
	auto level = this->stats.tension.level;
	auto witness = static_cast<int>(this->stats.witnesses.size());
	auto tension = std::min(level + witness, 470);

	if (tension != this->displayStats.tension) {
		this->displayStats.tension = tension;
		updated = true;
	}

	// Pacifications
	if (this->displayStats.pacifications != this->stats.pacifies.nonTargets) {
		this->displayStats.pacifications = this->stats.pacifies.nonTargets;
		updated = true;
	}

	// Spotted
	if (this->displayStats.spotted != (this->stats.targetsSpottedBy.size() + this->stats.detection.nonTargetsSpottedBy)) {
		this->displayStats.spotted = this->stats.targetsSpottedBy.size() + this->stats.detection.nonTargetsSpottedBy;
		updated = true;
	}

	// Bodies Found
	if (this->displayStats.bodiesFound != this->stats.bodies.found) {
		this->displayStats.bodiesFound = this->stats.bodies.found;
		updated = true;
	}

	// Disguises Taken
	if (this->displayStats.disguisesTaken != this->stats.misc.disguisesTaken) {
		this->displayStats.disguisesTaken = this->stats.misc.disguisesTaken;
		updated = true;
	}

	// Recorded
	if (this->displayStats.recorded != this->stats.detection.onCamera) {
		this->displayStats.recorded = this->stats.detection.onCamera;
		updated = true;
	}

	// Guard Kills
	if (this->displayStats.guardKills != this->stats.kills.guard) {
		this->displayStats.guardKills = this->stats.kills.guard;
		updated = true;
	}

	// Civilian Kills
	if (this->displayStats.civilianKills != this->stats.kills.civilian) {
		this->displayStats.civilianKills = this->stats.kills.civilian;
		updated = true;
	}

	// Witnesses
	if (this->displayStats.witnesses != this->stats.witnesses.size()) {
		this->displayStats.witnesses = static_cast<int>(this->stats.witnesses.size());
		updated = true;
	}

	// Bodies Hidden
	if (this->displayStats.bodiesHidden != this->stats.bodies.hidden) {
		this->displayStats.bodiesHidden = this->stats.bodies.hidden;
		updated = true;
	}

	// Disguises Blown
	if (this->displayStats.disguisesBlown != this->stats.disguisesBlown.size()) {
		this->displayStats.disguisesBlown = this->stats.disguisesBlown.size();
		updated = true;
	}

	// Targets Found
	const auto targetsFound = this->stats.bodies.targetsFound > 0;
	if (this->displayStats.targetsFound != targetsFound) {
		this->displayStats.targetsFound = targetsFound;
		updated = true;
	}

	// Noticed Kills
	if (this->displayStats.noticedKills != this->stats.kills.noticed) {
		this->displayStats.noticedKills = this->stats.kills.noticed;
		updated = true;
	}

	// Silent Assassin Status
	auto sa = this->GetSilentAssassinStatus();

	if (this->displayStats.silentAssassin != sa) {
		this->displayStats.silentAssassin = sa;
		updated = true;
	}

	// Stealth Rating
	auto rating = this->CalculateStealthRating();
	if (static_cast<int>(rating * 100) != static_cast<int>(this->displayStats.stealthRating * 100)) {
		this->displayStats.stealthRating = rating;
		updated = true;
	}

	// Play Style
	auto playStyleRating = getPlayStyleRating(stats);
	if (playStyleRating) {
		if (playStyleRating != this->displayStats.playstyle.rating) {
			std::uniform_int_distribution<size_t> rng(0, playStyleRating->getTitles().size() - 1);
			this->displayStats.playstyle.rating = playStyleRating;
			this->displayStats.playstyle.index = rng(this->randomGenerator);
		}
	}

	if (updated) this->window.update();
}

auto Stealthometer::IsContractEnded() const -> bool {
	return this->missionEndTime > 0;
}

auto Stealthometer::SetupEvents() -> void {
	// Helper to be called when a body found event is sent with a valid repo ID.
	auto onRealBodyFound = [this](const Stats::WitnessEvent& ev) {
		auto const foundMurderedInfoIt = stats.bodies.foundMurderedInfos.find(ev.bodyId);
		auto const bodyAlreadyFound = foundMurderedInfoIt != stats.bodies.foundMurderedInfos.end();

		// If already found, just keep track of target vs. non-target sightings.
		if (bodyAlreadyFound) {
			if (ev.isWitnessTarget) {
				stats.targetBodyWitnesses.emplace(ev.witnessId);
			}
			else if (!foundMurderedInfoIt->second.isSightedByNonTarget) {
				foundMurderedInfoIt->second.isSightedByNonTarget = true;
				++stats.bodies.foundMurderedByNonTarget;

				// Get name of first NPC to find a body, replace target name with non target name
				if (stats.firstBodyFoundByID.empty() && stats.firstBodyFoundWasByTarget) {
					auto name = this->GetNPCName(ev.witnessId);
					if (name) {
						stats.firstBodyFoundByID = ev.witnessId;
						stats.firstBodyFoundByName = *name;
						stats.firstBodyFoundWasByTarget = ev.isWitnessTarget;
					}
				}
			}

			foundMurderedInfoIt->second.sightings.try_emplace(ev.witnessId, ev.isWitnessTarget);
			return;
		}

		// Increment these only when this body was not already found as an 'accident' body.
		if (stats.bodies.uniqueBodiesFound.emplace(ev.bodyId).second) {
			if (this->IsRepoIdTargetNPC(ev.bodyId))
				++this->stats.bodies.targetsFound;
			++stats.bodies.found;
		}

		// Even if body already found, this is the first time it's found 'murdered' (e.g. since dragging an already found accident body).
		++stats.bodies.foundMurdered;

		// Track target body witnesses so they may be compared against the number of target body witnesses killed for redeemable SA tracking.
		if (ev.isWitnessTarget)
			stats.targetBodyWitnesses.emplace(ev.witnessId);
		else
			++stats.bodies.foundMurderedByNonTarget;

		// Get name of first NPC to find a body
		if (stats.firstBodyFoundByID.empty()) {
			auto name = this->GetNPCName(ev.witnessId);
			if (name) {
				stats.firstBodyFoundByID = ev.witnessId;
				stats.firstBodyFoundByName = *name;
				stats.firstBodyFoundWasByTarget = ev.isWitnessTarget;
			}
		}

		BodyStats::MurderedBodyFoundInfo bodyFoundInfo;
		bodyFoundInfo.sightings.emplace(ev.witnessId, ev.isWitnessTarget);
		bodyFoundInfo.isSightedByNonTarget = !ev.isWitnessTarget;
		stats.bodies.foundMurderedInfos.try_emplace(ev.bodyId, std::move(bodyFoundInfo));
	};
	events.listen<Events::EvergreenCampaignActivated>([this](const ServerEvent<Events::EvergreenCampaignActivated>& ev) {
		if (!this->freelancer.campaignInProgress)
			this->liveSplitClient.send(eClientMessage::Reset);

		this->liveSplitClient.send(eClientMessage::StartOrSplit);

		this->freelancer.campaignCompleted = false;
	});
	events.listen<Events::ScoringScreenEndState_CampaignCompleted>([this](const ServerEvent<Events::ScoringScreenEndState_CampaignCompleted>& ev) {
		this->liveSplitClient.send(eClientMessage::Split);
		this->runData.freelancer.campaignCompleted = true;
		this->runData.freelancer.campaignInProgress = false;
	});
	events.listen<Events::NoCampaignActive>([this](const ServerEvent<Events::NoCampaignActive>& ev) {
		this->runData.freelancer.campaignInProgress = false;
		this->runData.freelancer.noSyndicateActive = true;
	});
	events.listen<Events::CampaignInProgress>([this](const ServerEvent<Events::CampaignInProgress>& ev) {
		this->runData.freelancer.campaignInProgress = true;
	});
	events.listen<Events::ContractStart>([this](const ServerEvent<Events::ContractStart>& ev) {
		this->runData.missionType = ev.Value.ContractType;
		this->runData.shouldAutoStartLiveSplit = true;
		this->startAfterLoad = this->loadRemovalActive;
		if (!this->loadRemovalActive)
			this->liveSplitClient.send(eClientMessage::StartTimer);
		Logger::Info("ContractStart: {}", ev.json.dump());
		this->NewContract();
	});
	events.listen<Events::ContractLoad>([this](auto& ev) {
		this->NewContract();
	});
	events.listen<Events::ContractEnd>([this](const ServerEvent<Events::ContractEnd>& ev) {
		if (this->IsContractEnded()) return;
		this->missionEndTime = ev.Timestamp;
	});
	events.listen<Events::ExitGate>([this](const ServerEvent<Events::ExitGate>& ev) {
		if (this->runData.missionType != MissionType::Evergreen) {
			this->liveSplitClient.pause();
			this->liveSplitClient.send(eClientMessage::SetGameTime, {std::to_string(ev.Timestamp)});
			this->liveSplitClient.send(eClientMessage::Split);
			this->runData.shouldAutoStartLiveSplit = false;
		}

		this->missionEndTime = ev.Timestamp;
	});
	events.listen<Events::IntroCutEnd>([this](const ServerEvent<Events::IntroCutEnd>& ev) {
		this->cutsceneEndTime = ev.Timestamp;
	});
	events.listen<Events::AddSyndicateTarget>([this](const ServerEvent<Events::AddSyndicateTarget>& ev) {
		if (!ev.Value.repoID.empty())
			this->freelanceTargets.emplace(ev.Value.repoID);
	});
	events.listen<Events::StartingSuit>([this](const ServerEvent<Events::StartingSuit>& ev) {
		auto entry = this->GetRepoEntry(ev.Value.value);
		if (entry) {
			auto const isSuit = entry->value("IsHitmanSuit", false);
			stats.misc.startedInSuit = isSuit;
			stats.current.inSuit = isSuit;
		}
	});
	events.listen<Events::ItemPickedUp>([this](const ServerEvent<Events::ItemPickedUp>& ev) {
		if (this->IsContractEnded()) return;
		// TODO: bit of a hacky workaround to fix Freelancer loadout items counting as picked up
		// ignore any items picked up in the first 3 seconds (+ compensate for cutscene length)
		auto time = ev.Timestamp;
		time -= this->cutsceneEndTime;

		if (time > 3.0) {
			auto it = stats.itemsObtained.find(ev.Value.RepositoryId);
			if (it != stats.itemsObtained.end()) {
				++it->second.count;
			}
			else {
				auto item = this->CreateItemInfo(ev.Value.RepositoryId);
				if (item.type != ItemInfoType::None)
					stats.itemsObtained.emplace(ev.Value.RepositoryId, item);
			}
		}
	});
	events.listen<Events::ItemDropped>([this](const ServerEvent<Events::ItemDropped>& ev) {
		if (this->IsContractEnded()) return;
		++stats.misc.itemsDropped;

		auto& id = ev.Value.RepositoryId;
		if (!id.empty()) {
			this->RemoveObtainedItem(id);
			auto item = this->CreateItemInfo(id);
			this->AddDisposedItem(id, item);
		}
	});
	events.listen<Events::ItemThrown>([this](const ServerEvent<Events::ItemThrown>& ev) {
		if (this->IsContractEnded()) return;
		// inventory removal handled in ItemRemovedFromInventory
		++stats.misc.itemsThrown;

		auto item = this->CreateItemInfo(ev.Value.RepositoryId);
		this->AddDisposedItem(ev.Value.RepositoryId, item);
	});
	events.listen<Events::ItemRemovedFromInventory>([this](const ServerEvent<Events::ItemRemovedFromInventory>& ev) {
		if (this->IsContractEnded()) return;
		++stats.misc.itemsRemovedFromInventory;
		this->RemoveObtainedItem(ev.Value.RepositoryId);
	});
	events.listen<Events::FirstNonHeadshot>([this](const ServerEvent<Events::FirstNonHeadshot>& ev) {
		// TODO: ?
	});
	events.listen<Events::FirstMissedShot>([this](const ServerEvent<Events::FirstMissedShot>& ev) {
		// TODO: ?
	});
	events.listen<Events::Actorsick>([this](const ServerEvent<Events::Actorsick>& ev) {
		if (this->IsContractEnded()) return;
		if (ev.Value.IsTarget) ++stats.misc.targetsMadeSick;
		Logger::Debug("{} ActorSick: {}", ev.Timestamp, nlohmann::json{
			{"ActorId", ev.Value.ActorId},
			{"ActorName", ev.Value.ActorName},
			{"ActorType", ev.Value.ActorType},
			{"IsTarget", ev.Value.IsTarget},
			{"actor_R_ID", ev.Value.actor_R_ID},
			{"item_R_ID", ev.Value.item_R_ID},
			{"setpiece_R_ID", ev.Value.setpiece_R_ID},
		}.dump());
	});
	events.listen<Events::Trespassing>([this](const ServerEvent<Events::Trespassing>& ev) {
		if (this->IsContractEnded()) return;

		stats.current.trespassing = ev.Value.IsTrespassing;
		if (stats.current.trespassing) {
			stats.trespassStartTime = ev.Timestamp;
			++stats.misc.timesTrespassed;
		} else {
			stats.misc.trespassTime += ev.Timestamp - stats.trespassStartTime;
		}
	});
	events.listen<Events::SecuritySystemRecorder>([this](const ServerEvent<Events::SecuritySystemRecorder>& ev) {
		if (this->IsContractEnded()) return;

		bool destroyed = false;
		switch (ev.Value.event) {
			case SecuritySystemRecorderEvent::Spotted:
				stats.detection.onCamera = true;
				break;
			case SecuritySystemRecorderEvent::Destroyed:
				destroyed = true;
				stats.misc.recorderDestroyed = true;
				[[fallthrough]];
			case SecuritySystemRecorderEvent::Erased:
				if (stats.detection.onCamera) stats.misc.recordedThenErased = true;
				stats.detection.onCamera = false;
				if (!destroyed && !this->stats.misc.recorderDestroyed)
					stats.misc.recorderErased = true;
				break;
			case SecuritySystemRecorderEvent::CameraDestroyed:
				++stats.misc.camerasDestroyed;
				break;
		}
	});
	events.listen<Events::Agility_Start>([this](const ServerEvent<Events::Agility_Start>& ev) {
		++stats.misc.agilityActions;
	});
	events.listen<Events::Drain_Pipe_Climbed>([this](const ServerEvent<Events::Drain_Pipe_Climbed>& ev) {
		++stats.misc.agilityActions;
	});
	events.listen<Events::HoldingIllegalWeapon>([this](const ServerEvent<Events::HoldingIllegalWeapon>& ev) {
		if (this->IsContractEnded()) return;

		if (stats.current.holdingIllegalWeapon != ev.Value.IsHoldingIllegalWeapon) {
			stats.weaponHoldingStartTime = ev.Timestamp;
			++stats.misc.timesTrespassed;
		} else {
			stats.misc.weaponHoldingTime += ev.Timestamp - stats.weaponHoldingStartTime;
		}

		stats.current.holdingIllegalWeapon = ev.Value.IsHoldingIllegalWeapon;
	});
	// TODO: The game can send 0'd repository IDs for dead bodies in certain situations.
	// This makes it difficult to uniquely identify bodies to keep count of bodies found.
	// IsCrowdActor is also usually true when this happens. Seemingly the game always
	// eventually sends other body found events with correct IDs. Need a good solution
	// to link these events to reliably obtain the necessary information.
	events.listen<Events::AccidentBodyFound>([this](const ServerEvent<Events::AccidentBodyFound>& ev) {
		Logger::Debug("{} AccidentBodyFound: {}", ev.Timestamp, ev.json.dump());
		if (this->IsContractEnded()) return;

		const auto& bodyId = ev.Value.DeadBody.RepositoryId;

		// Count only if this body is found for the first time, and ensure we don't double count if the body gets dragged and found again.
		if (stats.bodies.uniqueBodiesFound.emplace(bodyId).second) {
			if (this->IsRepoIdTargetNPC(bodyId))
				++stats.bodies.targetsFound;

			++stats.bodies.found;
			++stats.bodies.foundAccidents;
		}
	});
	events.listen<Events::DeadBodySeen>([this](const ServerEvent<Events::DeadBodySeen>& ev) {
		Logger::Debug("{} DeadBodySeen: {}", ev.Timestamp, ev.json.dump());
		if (this->IsContractEnded()) return;
		++stats.bodies.deadSeen;
	});
	events.listen<Events::MurderedBodySeen>([this, onRealBodyFound](const ServerEvent<Events::MurderedBodySeen>& ev) {
		Logger::Debug("{} MurderedBodySeen: {}", ev.Timestamp, ev.json.dump());
		if (this->IsContractEnded()) return;

		auto const& value = ev.Value;
		auto const& deadBody = value.DeadBody;
		auto const deadBodyId = deadBody.IsCrowdActor ? "" : deadBody.RepositoryId;

		stats.witnessEvents.emplace_back(ev.Timestamp, Events::MurderedBodySeen, value.Witness, value.IsWitnessTarget, deadBodyId);

		if (!deadBodyId.empty()) onRealBodyFound(stats.witnessEvents.back());
	});
	events.listen<Events::BodyFound>([this, onRealBodyFound](const ServerEvent<Events::BodyFound>& ev) {
		Logger::Debug("{} BodyFound: {}", ev.Timestamp, ev.json.dump());

		auto const& id = ev.Value.DeadBody.RepositoryId;

		if (ev.Value.DeadBody.IsCrowdActor) {
			if (this->IsContractEnded()) return;
			++stats.bodies.foundCrowd;
		}
		else {
			for (auto it = stats.witnessEvents.rbegin(); it != stats.witnessEvents.rend(); ++it) {
				if (it->timestamp != ev.Timestamp) break; // floating-point equality comparison - should be low enough precision to be fine?
				if (it->event != Events::MurderedBodySeen) continue;
				if (!it->bodyId.empty()) continue;
				it->bodyId = id;
				onRealBodyFound(*it);
			}
		}
	});
	events.listen<Events::Disguise>([this](const ServerEvent<Events::Disguise>& ev) {
		++stats.misc.disguisesTaken;
		stats.misc.suitRetrieved = false;

		auto entry = this->GetRepoEntry(ev.Value.value);
		if (entry) {
			auto isHitmanSuit = entry->value("IsHitmanSuit", false);
			if (isHitmanSuit) stats.misc.suitRetrieved = true;
		}
	});
	events.listen<Events::SituationContained>([this](const ServerEvent<Events::SituationContained>& ev) {
		++stats.detection.situationsContained;
	});
	events.listen<Events::TargetBodySpotted>([this](const ServerEvent<Events::TargetBodySpotted>& ev) {
		if (this->IsContractEnded()) return;
		++stats.bodies.targetsFound;
	});
	events.listen<Events::BodyHidden>([this](const ServerEvent<Events::BodyHidden>& ev) {
		if (this->IsContractEnded()) return;
		++stats.bodies.hidden;
	});
	events.listen<Events::BodyBagged>([this](const ServerEvent<Events::BodyBagged>& ev) {
		if (this->IsContractEnded()) return;
		++stats.bodies.bagged;
	});
	events.listen<Events::AllBodiesHidden>([this](const ServerEvent<Events::AllBodiesHidden>& ev) {
		if (this->IsContractEnded()) return;
		stats.bodies.allHidden = true;
	});
	events.listen<Events::ShotsFired>([this](const ServerEvent<Events::ShotsFired>& ev) {
		// not much we can do without a live update?
		stats.misc.shotsFired = ev.Value.Total;
	});
	events.listen<Events::Spotted>([this](const ServerEvent<Events::Spotted>& ev) {
		if (this->IsContractEnded()) return;

		for (const auto& name : ev.Value.value) {
			auto isTarget = this->IsRepoIdTargetNPC(name);

			if (!stats.spottedBy.contains(name)) {
				Logger::Info("Stealthometer: spotted by {} - Target: {}", name, isTarget);

				if (stats.firstSpottedByName.empty()) {
					auto realName = this->GetNPCName(name);
					if (realName) {
						stats.firstSpottedByID = name;
						stats.firstSpottedByName = *realName;
					}
				}

				++stats.detection.spotted;
				stats.spottedBy.insert(name);

				if (isTarget) {
					// It's possible for the spotted event to fire right AFTER the target died. Handle this dumb edge case.
					if (stats.kills.targets.contains(name)) continue;

					stats.targetsSpottedBy.insert(name);
				}

				stats.detection.nonTargetsSpottedBy = static_cast<int>(stats.spottedBy.size()) - stats.targetsSpottedBy.size();
			}
		}
	});
	events.listen<Events::Witnesses>([this](const ServerEvent<Events::Witnesses>& ev) {
		if (this->IsContractEnded()) return;

		for (const auto& name : ev.Value.value) {
			// It's possible for the witnesses event to fire right AFTER the NPC died. Handle this dumb edge case.
			if (stats.kills.targets.contains(name) || stats.kills.nonTargets.contains(name)) continue;

			stats.witnesses.insert(name);
		}
	});
	events.listen<Events::DisguiseBlown>([this](const ServerEvent<Events::DisguiseBlown>& ev) {
		if (this->IsContractEnded()) return;

		stats.current.disguiseBlown = true;
		stats.disguisesBlown.insert(ev.Value.value);
	});
	events.listen<Events::BrokenDisguiseCleared>([this](const ServerEvent<Events::BrokenDisguiseCleared>& ev) {
		if (this->IsContractEnded()) return;

		stats.current.disguiseBlown = false;
		stats.disguisesBlown.erase(ev.Value.value);
	});
	events.listen<Events::_47_FoundTrespassing>([this](const ServerEvent<Events::_47_FoundTrespassing>& ev) {
		if (this->IsContractEnded()) return;

		++stats.detection.caughtTrespassing;
	});
	events.listen<Events::TargetEliminated>([this](const ServerEvent<Events::TargetEliminated>& ev) {
		//++stats.kills.targets; // is this event sent in all target kill cases?
	});
	events.listen<Events::Door_Unlocked>([this](const ServerEvent<Events::Door_Unlocked>& ev) {
		if (this->IsContractEnded()) return;

		++stats.misc.doorsUnlocked;
	});
	events.listen<Events::CrowdNPC_Died>([this](const ServerEvent<Events::CrowdNPC_Died>& ev) {
		if (this->IsContractEnded()) return;

		++stats.kills.total;
		++stats.kills.crowd;
		++stats.kills.civilian;
	});
	events.listen<Events::NoticedKill>([this](const ServerEvent<Events::NoticedKill>& ev) {
		if (this->IsContractEnded()) return;

		Logger::Debug("{} NoticedKill: {}", ev.Timestamp, ev.json.dump());

		// TODO:
		//ev.Value.RepositoryId
		//ev.Value.IsTarget
		auto const& value = ev.Value;
		auto const noticedKillInfoIt = stats.kills.noticedKillInfos.find(value.RepositoryId);
		auto const killAlreadyNoticed = noticedKillInfoIt != stats.kills.noticedKillInfos.end();
		auto const killAlreadyNoticedByNonTarget = killAlreadyNoticed && noticedKillInfoIt->second.isSightedByNonTarget;
		auto witnessId = std::string("");

		for (auto it = stats.witnessEvents.crbegin(); it != stats.witnessEvents.crend(); ++it) {
			if (it->timestamp != ev.Timestamp) break;
			if (!it->witnessId.empty()) {
				witnessId = it->witnessId;
				break;
			}
		}

		stats.witnessEvents.emplace_back(ev.Timestamp, Events::NoticedKill, witnessId, false, value.RepositoryId);

		if (killAlreadyNoticed) {
			if (value.IsTarget) {
				//stats.targetKillNoticers.emplace(value.)
			}
		}

		++stats.kills.noticed;
	});
	events.listen<Events::Noticed_Pacified>([this](const ServerEvent<Events::Noticed_Pacified>& ev) {
		if (this->IsContractEnded()) return;

		// TODO:
		//ev.Value.RepositoryId
		//ev.Value.IsTarget
		++stats.pacifies.noticed;
	});
	events.listen<Events::Unnoticed_Kill>([this](const ServerEvent<Events::Unnoticed_Kill>& ev) {
		// TODO: ?
		//ev.Value.RepositoryId
		++stats.kills.unnoticed;
		if (ev.Value.IsTarget)
			++stats.kills.unnoticedTarget;
		else
			++stats.kills.unnoticedNonTarget;
	});
	events.listen<Events::Unnoticed_Pacified>([this](const ServerEvent<Events::Unnoticed_Pacified>& ev) {
		// TODO: ?
		//ev.Value.RepositoryId
		++stats.pacifies.unnoticed;
		if (!ev.Value.IsTarget)
			++stats.pacifies.unnoticedNonTarget;
	});
	events.listen<Events::AmbientChanged>([this](const ServerEvent<Events::AmbientChanged>& ev) {
		if (this->IsContractEnded()) return;

		stats.current.tension = ev.Value.AmbientValue;

		switch (ev.Value.AmbientValue) {
		case EGameTension::EGT_Agitated:
			Logger::Debug("Game tension: agitated - it actually happened!");
			++stats.tension.agitated;
			break;
		case EGameTension::EGT_AlertedHigh:
			++stats.tension.alertedHigh;
			break;
		case EGameTension::EGT_AlertedLow:
			++stats.tension.alertedLow;
			break;
		case EGameTension::EGT_Arrest:
			++stats.tension.arrest;
			break;
		case EGameTension::EGT_Combat:
			++stats.tension.combat;
			break;
		case EGameTension::EGT_Hunting:
			++stats.tension.hunting;
			break;
		case EGameTension::EGT_Searching:
			++stats.tension.searching;
			break;
		}

		if (isTensionHigher(ev.Value.PreviousAmbientValue, ev.Value.AmbientValue))
			stats.tension.level += getTensionValue(ev.Value.AmbientValue) - getTensionValue(ev.Value.PreviousAmbientValue);
	});
	events.listen<Events::Pacify>([this](const ServerEvent<Events::Pacify>& ev) {
		if (this->IsContractEnded()) return;

		stats.bodies.allHidden = false;
		++stats.pacifies.total;

		if (ev.Value.IsTarget) stats.bodies.allTargetsHidden = false;
		else ++stats.pacifies.nonTargets;

		if (ev.Value.Accident) ++stats.pacifyMethods.accident;
		if (ev.Value.KillClass == "melee") ++stats.pacifyMethods.melee;
		if (ev.Value.KillMethodBroad == "throw") ++stats.pacifyMethods.thrown;

		if (!ev.Value.IsTarget) {
			if (ev.Value.ActorType == EActorType::eAT_Civilian) ++stats.pacifies.civilian;
			else if (ev.Value.ActorType == EActorType::eAT_Guard) ++stats.pacifies.guard;
		}
	});
	events.listen<Events::Kill>([this](const ServerEvent<Events::Kill>& ev) {
		if (this->IsContractEnded()) return;

		const auto& repoId = ev.Value.RepositoryId;
		const auto isTarget = ev.Value.IsTarget;

		stats.bodies.allHidden = false;
		++stats.kills.total;

		if (repoId == stats.firstSpottedByID) {
			stats.firstSpottedByID = "";
			stats.firstSpottedByName = "";
		}
		if (repoId == stats.firstBodyFoundByID) {
			stats.firstBodyFoundByID = "";
			stats.firstBodyFoundByName = "";
		}

		if (isTarget) {
			auto res = stats.kills.targets.emplace(repoId);
			if (res.second) {
				if (stats.targetsSpottedBy.contains(repoId)) {
					++stats.detection.targetsSpottedByAndKilled;
					++stats.detection.uniqueNPCsCaughtByAndKilled;
				}
				else if (stats.spottedBy.contains(repoId)) {
					stats.targetsSpottedBy.insert(repoId);
					++stats.detection.targetsSpottedByAndKilled;
					++stats.detection.uniqueNPCsCaughtByAndKilled;
				}

				if (stats.targetBodyWitnesses.contains(repoId))
					++stats.bodies.targetBodyWitnessesKilled;
			}
		}
		else if (ev.Value.KillContext == EDeathContext::eDC_NOT_HERO)
			stats.kills.proxyDeaths.emplace(repoId);
		else {
			auto res = stats.kills.nonTargets.emplace(repoId);
			if (res.second) {
				if (ev.Value.ActorType == EActorType::eAT_Civilian) ++stats.kills.civilian;
				if (ev.Value.ActorType == EActorType::eAT_Guard) ++stats.kills.guard;

				if (stats.spottedBy.count(repoId))
					++stats.detection.uniqueNPCsCaughtByAndKilled;
			}

			if (stats.firstNTKName.empty() && stats.firstNTKID != repoId) {
				stats.firstNTKID = repoId;
				auto name = this->GetNPCName(repoId);
				if (name) {
					stats.firstNTKName = *name;
				}
			}
		}

		if (ev.Value.IsHeadshot) {
			++stats.killMethods.headshot;
			if (isTarget) ++stats.killMethods.headshotTarget;
		}

		if (ev.Value.KillClass == "melee") {
			++stats.killMethods.melee;
			if (isTarget) ++stats.killMethods.meleeTarget;
		}

		if (ev.Value.KillMethodBroad == "throw") {
			++stats.killMethods.thrown;
			if (isTarget) ++stats.killMethods.thrownTarget;
		}
		else if (ev.Value.KillMethodBroad == "unarmed") {
			++stats.killMethods.unarmed;
			if (isTarget) ++stats.killMethods.unarmedTarget;
		}
		else if (ev.Value.KillMethodBroad == "pistol") {
			++stats.killMethods.pistol;
			if (isTarget) ++stats.killMethods.pistolTarget;
		}
		else if (ev.Value.KillMethodBroad == "smg") {
			++stats.killMethods.smg;
			if (isTarget) ++stats.killMethods.smgTarget;
		}
		else if (ev.Value.KillMethodBroad == "shotgun") {
			++stats.killMethods.shotgun;
			if (isTarget) ++stats.killMethods.shotgunTarget;
		}
		else if (ev.Value.KillMethodBroad == "close_combat_pistol_elimination") {
			++stats.killMethods.pistolElim;
			if (isTarget) ++stats.killMethods.pistolElimTarget;
		}

		if (ev.Value.Accident) {
			++stats.killMethods.accident;
			if (isTarget) ++stats.killMethods.accidentTarget;

			if (ev.Value.KillMethodStrict == "accident_drown") {
				++stats.killMethods.drown;
				if (isTarget) ++stats.killMethods.drownTarget;
			}
			else if (ev.Value.KillMethodStrict == "accident_push") {
				++stats.killMethods.push;
				if (isTarget) ++stats.killMethods.pushTarget;
			}
			else if (ev.Value.KillMethodStrict == "accident_burn") {
				++stats.killMethods.burn;
				if (isTarget) ++stats.killMethods.burnTarget;
			}
			else if (ev.Value.KillMethodStrict == "accident_explosion") {
				++stats.killMethods.accidentExplosion;
				if (isTarget) ++stats.killMethods.accidentExplosionTarget;
			}
			else if (ev.Value.KillMethodStrict == "accident_suspended_object") {
				++stats.killMethods.fallingObject;
				if (isTarget) ++stats.killMethods.fallingObjectTarget;
			}
			else if (ev.Value.KillMethodStrict.size()) {
				Logger::Info("Stealthometer: Unhandled KillMethodStrict '{}'", ev.Value.KillMethodStrict);
			}
		}

		if (ev.Value.WeaponSilenced) {
			++stats.killMethods.silencedWeapon;
			if (isTarget) ++stats.killMethods.silencedWeaponTarget;
		}

		auto witnessIt = stats.witnesses.find(repoId);
		if (witnessIt != stats.witnesses.end()) {
			stats.witnesses.erase(witnessIt);
			++stats.detection.witnessesKilled;
		}
	});
	events.listen<Events::setpieces>([this](const ServerEvent<Events::setpieces>& ev) {
		// Photo taken
		// {"Timestamp":14.068766,"Name":"setpieces","ContractSessionId":"2517189686363575049-c894c9d2-b984-4c5b-ad0a-2be27e3b04f5","ContractId":"d2419fe4-ea72-4e61-b91b-bb39706f551d","Value":{"RepositoryId":"6c3fa06e-7478-4484-81e6-f08dba1722eb","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Camera","setpieceType_metricvalue":"picturetaken","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NA","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2846824114","Origin":"gameclient","Id":"971f62e0-e3d9-4ad3-8c16-bfc7fd2fa8a8"}

		// Reporter camera destroyed
		// {"Timestamp":405.325409,"Name":"ItemDestroyed","ContractSessionId":"64e8780e-00bb-45d1-a270-ff1df03082de","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"ItemName":"ActItem_Camera"},"UserId":"00000000-0000-0000-0000-000000000000","SessionId":"","Origin":"gameclient","Id":"9ce9fbf2-e001-42cc-a715-9bae8423285d"}

		// Look at evacuation plan in Paris, basement security room
		// { "Timestamp":1030.322388, "Name" : "setpieces", "ContractSessionId" : "64e8780e-00bb-45d1-a270-ff1df03082de", "ContractId" : "00000000-0000-0000-0000-000000000200", "Value" : {"RepositoryId":"7093201b-ff82-465f-a187-7245d9057954", "name_metricvalue" : "Paris_Evac_plan", "setpieceHelper_metricvalue" : "Activator_NoTool", "setpieceType_metricvalue" : "DefaultActivators", "toolUsed_metricvalue" : "NA", "Item_triggered_metricvalue" : "NotAvailable", "Position" : "ZDynamicObject::ToString() unknown type: SVector3"}, "UserId" : "00000000-0000-0000-0000-000000000000", "SessionId" : "", "Origin" : "gameclient", "Id" : "bfabe0cf-cb92-4918-a716-b708f3fdc615" }
		// { "Timestamp":1030.836670, "Name" : "setpieces", "ContractSessionId" : "64e8780e-00bb-45d1-a270-ff1df03082de", "ContractId" : "00000000-0000-0000-0000-000000000200", "Value" : {"RepositoryId":"7093201b-ff82-465f-a187-7245d9057954", "name_metricvalue" : "Paris_Evac_plan", "setpieceHelper_metricvalue" : "Activator_NoTool", "setpieceType_metricvalue" : "DefaultActivators", "toolUsed_metricvalue" : "NA", "Item_triggered_metricvalue" : "NotAvailable", "Position" : "ZDynamicObject::ToString() unknown type: SVector3"}, "UserId" : "00000000-0000-0000-0000-000000000000", "SessionId" : "", "Origin" : "gameclient", "Id" : "39f70b5d-8689-47bd-a765-a73067bd737d" }
			
		// Piano lid pushed down
		// {"Timestamp":75.528625,"Name":"setpieces","ContractSessionId":"64e8780e-00bb-45d1-a270-ff1df03082de","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"db6a820c-22ea-496e-a0f2-6823b32a911d","name_metricvalue":"Trap_Piano","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"00000000-0000-0000-0000-000000000000","SessionId":"","Origin":"gameclient","Id":"7260ec0d-98da-4aa1-b2e5-e2ad74d18db3"}
		// { "Timestamp":75.629402, "Name" : "setpieces", "ContractSessionId" : "64e8780e-00bb-45d1-a270-ff1df03082de", "ContractId" : "00000000-0000-0000-0000-000000000200", "Value" : {"RepositoryId":"db6a820c-22ea-496e-a0f2-6823b32a911d", "name_metricvalue" : "Trap_Piano", "setpieceHelper_metricvalue" : "Activator_NoTool", "setpieceType_metricvalue" : "DefaultActivators", "toolUsed_metricvalue" : "NA", "Item_triggered_metricvalue" : "NotAvailable", "Position" : "ZDynamicObject::ToString() unknown type: SVector3"}, "UserId" : "00000000-0000-0000-0000-000000000000", "SessionId" : "", "Origin" : "gameclient", "Id" : "0fdcc023-01f6-4a15-b2f5-2c786ddf47fe" }
		// { "Timestamp":76.096695, "Name" : "setpieces", "ContractSessionId" : "64e8780e-00bb-45d1-a270-ff1df03082de", "ContractId" : "00000000-0000-0000-0000-000000000200", "Value" : {"RepositoryId":"ab388850-d6cc-4e5c-a4a2-76bb22ca8f73", "name_metricvalue" : "NotAvailable", "setpieceHelper_metricvalue" : "DistractionLogic", "setpieceType_metricvalue" : "DistractionTriggered", "toolUsed_metricvalue" : "NA", "Item_triggered_metricvalue" : "NotAvailable", "Position" : "ZDynamicObject::ToString() unknown type: SVector3"}, "UserId" : "00000000-0000-0000-0000-000000000000", "SessionId" : "", "Origin" : "gameclient", "Id" : "26758af1-b9f6-4892-b33c-644bf2522a02" }

		// Loudspeaker shot down
		// {"Timestamp":110.426361,"Name":"setpieces","ContractSessionId":"64e8780e-00bb-45d1-a270-ff1df03082de","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"2d7a91b9-1b3a-4db3-a8bf-6249db70c339","name_metricvalue":"Loudspeaker","setpieceHelper_metricvalue":"SuspendedObject","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NA","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"00000000-0000-0000-0000-000000000000","SessionId":"","Origin":"gameclient","Id":"744fbb86-b8d1-47e0-b206-fa79b80b259f"}

		// Car blown up
		// {"Timestamp":340.653961,"Name":"setpieces","ContractSessionId":"64e8780e-00bb-45d1-a270-ff1df03082de","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"2b29d641-2a0d-4781-b2dd-0df02bc2674b","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"PropHelper_Explosion","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"Exploded","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"00000000-0000-0000-0000-000000000000","SessionId":"","Origin":"gameclient","Id":"1233f4a1-fafc-43db-a746-972af85d28dc"}

		// Fuse box turned off
		// {"Timestamp":35.705559,"Name":"setpieces","ContractSessionId":"3b541fce-5498-42bd-b853-b07526a07593","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"e29d8ce5-64d6-4207-a55d-ebe5e84b16b3","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"00000000-0000-0000-0000-000000000000","SessionId":"","Origin":"gameclient","Id":"72ea0e72-4c48-4980-9341-c07c5000b7d4"}
		// { "Timestamp":35.839172, "Name" : "setpieces", "ContractSessionId" : "3b541fce-5498-42bd-b853-b07526a07593", "ContractId" : "00000000-0000-0000-0000-000000000200", "Value" : {"RepositoryId":"e29d8ce5-64d6-4207-a55d-ebe5e84b16b3", "name_metricvalue" : "NotAvailable", "setpieceHelper_metricvalue" : "Activator_NoTool", "setpieceType_metricvalue" : "DefaultActivators", "toolUsed_metricvalue" : "NA", "Item_triggered_metricvalue" : "NotAvailable", "Position" : "ZDynamicObject::ToString() unknown type: SVector3"}, "UserId" : "00000000-0000-0000-0000-000000000000", "SessionId" : "", "Origin" : "gameclient", "Id" : "88fc03bf-2fc4-466d-9356-1c74210cace4" }

		// Blown up propane:
		// {"Timestamp":136.863831,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"2b29d641-2a0d-4781-b2dd-0df02bc2674b","name_metricvalue":"PropaneFlask","setpieceHelper_metricvalue":"PropHelper_Explosion","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"Exploded","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"4dcd73fe-7d2c-443b-aeac-10cd279ec971"}

		// Flooded sink:
		// {"Timestamp":219.718857,"Name":"setpieces","ContractSessionId":"01e7cfb4-0c1d-4d6e-99dc-9a604f9e1be0","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"95e7e530-dbd0-4e1a-95f6-8d5c165a7991","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"00000000-0000-0000-0000-000000000000","SessionId":"","Origin":"gameclient","Id":"c9b351ed-80ac-47c9-a73d-9fcd61c49ca5"}
		// { "Timestamp":220.185089, "Name" : "setpieces", "ContractSessionId" : "01e7cfb4-0c1d-4d6e-99dc-9a604f9e1be0", "ContractId" : "00000000-0000-0000-0000-000000000200", "Value" : {"RepositoryId":"95e7e530-dbd0-4e1a-95f6-8d5c165a7991", "name_metricvalue" : "NotAvailable", "setpieceHelper_metricvalue" : "Activator_NoTool", "setpieceType_metricvalue" : "DefaultActivators", "toolUsed_metricvalue" : "NA", "Item_triggered_metricvalue" : "NotAvailable", "Position" : "ZDynamicObject::ToString() unknown type: SVector3"}, "UserId" : "00000000-0000-0000-0000-000000000000", "SessionId" : "", "Origin" : "gameclient", "Id" : "e40d4a1c-5c02-4431-a784-b0e944cf7ae4" }
		// { "Timestamp":224.197235, "Name" : "setpieces", "ContractSessionId" : "01e7cfb4-0c1d-4d6e-99dc-9a604f9e1be0", "ContractId" : "00000000-0000-0000-0000-000000000200", "Value" : {"RepositoryId":"3678cc55-c327-4e79-ab1b-52553c58ec83", "name_metricvalue" : "NotAvailable", "setpieceHelper_metricvalue" : "DistractionLogic", "setpieceType_metricvalue" : "DistractionTriggered", "toolUsed_metricvalue" : "NA", "Item_triggered_metricvalue" : "NotAvailable", "Position" : "ZDynamicObject::ToString() unknown type: SVector3"}, "UserId" : "00000000-0000-0000-0000-000000000000", "SessionId" : "", "Origin" : "gameclient", "Id" : "97285361-ceb7-4f3c-a9e9-0e4352aa70c8" }
			
		// Shot down Shisha sign:
		// {"Timestamp":6.376209,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"2d7a91b9-1b3a-4db3-a8bf-6249db70c339","name_metricvalue":"\n\n","setpieceHelper_metricvalue":"SuspendedObject","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NA","Position":"ZDynamicObject::ToString() unknown type : SVector3"},"UserId":"b1585b4d - 36f0 - 48a0 - 8ffa - 1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61 - 2714020697","Origin":"gameclient","Id":"46904050 - dd0b - 406f - a8ea - 8d56cbbca556"}
			
		// Oil drum ignited:
		// {"Timestamp":158.045746,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"032151ce-e0be-4847-aab4-1b40fcdc2bc7","name_metricvalue":"Explosive_OilDrum","setpieceHelper_metricvalue":"PropHelper_OilSpill_Flammable","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"Exploded","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"4667bd34-ed17-4eef-a46a-370904d2fe47"}
			
		// Marrakesh Toilet Drop:
		// {"Timestamp":422.665924,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"e29d8ce5-64d6-4207-a55d-ebe5e84b16b3","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"b81c3ad3-b4b6-45db-8f08-75ddeb5d919a"}
		// {"Timestamp":423.392487,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"2d7a91b9-1b3a-4db3-a8bf-6249db70c339","name_metricvalue":"ToiletDrop  ","setpieceHelper_metricvalue":"SuspendedObject","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NA","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"a533b4d4-6cfc-4f35-8f77-42e62ee9a523"}
		// {"Timestamp":423.649689,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"e29d8ce5-64d6-4207-a55d-ebe5e84b16b3","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"4990e672-515a-441a-8f9e-63c57284026c"}
			
		// Gas canister boom:
		// {"Timestamp":468.406860,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"e4bc6f9e-def7-4155-9524-16da8d68d4ad","name_metricvalue":"GasCanister_Large_A","setpieceHelper_metricvalue":"PropHelper_Explosion","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"Exploded","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"d8133825-e9ef-4d0d-b5b3-a23db08758e1"}

		// Small oil lamp (distraction object in Reza's office) destroyed:
		// {"Timestamp":466.942963,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"997fbfe6-ba8b-41a0-91bb-366bef9bef9b","name_metricvalue":"OilLamp","setpieceHelper_metricvalue":"ShotAndImpulseListener","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"OnImpact","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"d3f6d21b-6817-4cdc-b4de-6fecf8982189"}
		// {"Timestamp":466.963257,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"0f0bb2c7-1cb3-4211-87ad-555df894026a","name_metricvalue":"OilLamp","setpieceHelper_metricvalue":"DistractionLogic_Visual","setpieceType_metricvalue":"DistractionTriggered","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"f220d7c8-e0fb-48f1-b4de-6fec5dfc41f3"}
		// {"Timestamp":466.963257,"Name":"setpieces","ContractSessionId":"2517213287667595942-d688bab6-034a-488b-a483-89cfac74656f","ContractId":"00000000-0000-0000-0000-000000000400","Value":{"RepositoryId":"0f0bb2c7-1cb3-4211-87ad-555df894026a","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"DistractionLogic_Visual","setpieceType_metricvalue":"DistractionFixed","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"a718105d-75f9-4d18-b4de-6fec660555f3"}

		// Chandelier winch drop
		// {"Timestamp":10.500926,"Name":"setpieces","ContractSessionId":"2517213274420850852-356e1881-82f8-4c5a-b7f1-63ab8432c042","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"683a099f-5d1b-4800-a781-5d9dfe13b12c","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"9fba84b6-85df-4d69-bd36-4376196d1202"}
		// {"Timestamp":10.607255,"Name":"setpieces","ContractSessionId":"2517213274420850852-356e1881-82f8-4c5a-b7f1-63ab8432c042","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"683a099f-5d1b-4800-a781-5d9dfe13b12c","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"Activator_NoTool","setpieceType_metricvalue":"DefaultActivators","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NotAvailable","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"af42da25-9c48-44d1-bd46-3bb88cd91c88"}
		// {"Timestamp":10.864028,"Name":"setpieces","ContractSessionId":"2517213274420850852-356e1881-82f8-4c5a-b7f1-63ab8432c042","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"RepositoryId":"2d7a91b9-1b3a-4db3-a8bf-6249db70c339","name_metricvalue":"NotAvailable","setpieceHelper_metricvalue":"SuspendedObject","setpieceType_metricvalue":"trap","toolUsed_metricvalue":"NA","Item_triggered_metricvalue":"NA","Position":"ZDynamicObject::ToString() unknown type: SVector3"},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"ad4e6c9d-4438-4fc1-bd6d-9697fe6ad6f0"}
		// {"Timestamp":11.463453,"Name":"Investigate_Curious","ContractSessionId":"2517213274420850852-356e1881-82f8-4c5a-b7f1-63ab8432c042","ContractId":"00000000-0000-0000-0000-000000000200","Value":{"ActorId":2655118168.000000,"RepositoryId":"f9c3905a-ec94-43b6-aae6-8b2f752467f7","SituationType":"AIS_INVESTIGATE_CURIOUS","EventType":"AISE_ActorJoined","JoinReason":"AISJR_Default","InvestigationType":9.000000},"UserId":"b1585b4d-36f0-48a0-8ffa-1b72f01759da","SessionId":"61e82efa0bcb4a3088825dd75e115f61-2714020697","Origin":"gameclient","Id":"8ad219e4-5222-4f7b-bdc8-b6224db5aa6d"}
		Logger::Info("Setpieces: {}", nlohmann::json{
			{"name_metricvalue", ev.Value.name_metricvalue},
			{"setpieceHelper_metricvalue", ev.Value.setpieceHelper_metricvalue},
			{"setpieceType_metricvalue", ev.Value.setpieceType_metricvalue},
			{"setpieceType_metricvalue", ev.Value.setpieceType_metricvalue},
			{"Item_triggered_metricvalue", ev.Value.Item_triggered_metricvalue},
			{"RepositoryId", ev.Value.RepositoryId},
		}.dump());
	});
	//eventName == "ItemDestroyed" // broken camcorder
	//eventName == "TargetEscapeFoiled" // Yuki killed in Gondola
}

DEFINE_PLUGIN_DETOUR(Stealthometer, void*, OnLoadingScreenActivated, void* th, void* a1) {
	loadingScreenActivated = true;
	if (!loadRemovalActive) {
		liveSplitClient.pause();
		loadRemovalActive = true;
	}
	return HookResult<void*>(HookAction::Continue());
}

DEFINE_PLUGIN_DETOUR(Stealthometer, void, ZAchievementManagerSimple_OnEventSent, ZAchievementManagerSimple* th, uint32_t eventId, const ZDynamicObject& ev) {
	ZString eventData;
	Functions::ZDynamicObject_ToString->Call(const_cast<ZDynamicObject*>(&ev), &eventData);

	auto eventDataSV = std::string_view(eventData.c_str(), eventData.size());
	auto fixedEventDataStr = std::string(eventData.size(), '\0');
	std::remove_copy(eventDataSV.cbegin(), eventDataSV.cend(), fixedEventDataStr.begin(), '\n');

	try {
		auto json = nlohmann::json::parse(fixedEventDataStr.c_str(), fixedEventDataStr.c_str() + fixedEventDataStr.size());
		auto const eventName = json.value("Name", "");
		auto const timestamp = json.value("Timestamp", 0.0);

		if (timestamp) lastEventTimestamp = timestamp;

		if (!eventNameBlacklist.contains(eventName)) {
			if (!events.handle(eventName, json))
				Logger::Info("Unhandled Event Sent: {}", eventData);
			else {
				this->UpdateDisplayStats();
				this->eventHistory.push_back(eventName);
			}
		}
	}
	catch (const nlohmann::json::exception& ex) {
		Logger::Error("JSON exception: {}", ex.what());
		Logger::Error("{}", eventData);
	}

	return HookResult<void>(HookAction::Continue());
}

DECLARE_ZHM_PLUGIN(Stealthometer);
