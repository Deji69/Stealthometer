#pragma once
#include <string>
#include <IPluginInterface.h>

enum class DockMode
{
	None,
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight,
	AboveMinimap,
	BelowMinimap,
	LeftBelowMinimap,
};

enum class OverlayDetailMode
{
	Normal,
	WithNames,
	WithCounts,
};

struct ConfigData
{
	bool externalWindow = true;
	bool externalWindowDark = true;
	bool externalWindowOnTop = false;
	bool inGameOverlay = false;
	bool inGameOverlayDetailed = false;
	bool useExtendedShorthand = false;
	DockMode overlayDockMode = DockMode::None;
	OverlayDetailMode overlayDetail = OverlayDetailMode::WithNames;
	bool overlayTransparency = true;
	bool liveSplitEnabled = false;
	std::string liveSplitIP = "127.0.0.1";
	uint16_t liveSplitPort = 16834;
};

class Config
{
public:
	Config(IPluginInterface& plugin) : plugin(plugin)
	{ }

	void Load() {
		data.externalWindow = plugin.GetSettingBool("general", "external_window", data.externalWindow);
		data.externalWindowDark = plugin.GetSettingBool("general", "external_window_dark", data.externalWindowDark);
		data.externalWindowOnTop = plugin.GetSettingBool("general", "external_window_on_top", data.externalWindowOnTop);
		data.inGameOverlay = plugin.GetSettingBool("general", "overlay", data.inGameOverlay);
		data.inGameOverlayDetailed = plugin.GetSettingBool("general", "overlay_detailed", data.inGameOverlayDetailed);
		data.liveSplitEnabled = plugin.GetSettingBool("livesplit", "enabled", data.liveSplitEnabled);
		data.liveSplitIP = plugin.GetSetting("livesplit", "ip", data.liveSplitIP);
		data.liveSplitPort = plugin.GetSettingInt("livesplit", "port", data.liveSplitPort);

		auto overlayDock = plugin.GetSetting("general", "overlay_dock", "");

		if (overlayDock == "topleft") data.overlayDockMode = DockMode::TopLeft;
		else if (overlayDock == "topright") data.overlayDockMode = DockMode::TopRight;
		else if (overlayDock == "bottomleft") data.overlayDockMode = DockMode::BottomLeft;
		else if (overlayDock == "bottomright") data.overlayDockMode = DockMode::BottomRight;
		else if (overlayDock == "abovemap") data.overlayDockMode = DockMode::AboveMinimap;
		else if (overlayDock == "belowmap") data.overlayDockMode = DockMode::BelowMinimap;
		else if (overlayDock == "leftbelowmap") data.overlayDockMode = DockMode::LeftBelowMinimap;
		else data.overlayDockMode = DockMode::None;

		auto overlayDetail = plugin.GetSetting("general", "overlay_detail", "");
		if (overlayDetail == "normal") data.overlayDetail = OverlayDetailMode::Normal;
		else if (overlayDetail == "names") data.overlayDetail = OverlayDetailMode::WithNames;

		data.useExtendedShorthand = plugin.GetSettingBool("general", "use_extended_shorthand", data.useExtendedShorthand);
		data.overlayTransparency = plugin.GetSettingBool("general", "overlay_transparency", true);
	}

	void Save() {
		if (data.liveSplitIP.empty())
			data.liveSplitIP = "127.0.0.1";
		if (!data.liveSplitPort)
			data.liveSplitPort = 16834;

		plugin.SetSettingBool("general", "external_window", data.externalWindow);
		plugin.SetSettingBool("general", "external_window_dark", data.externalWindowDark);
		plugin.SetSettingBool("general", "external_window_on_top", data.externalWindowOnTop);
		plugin.SetSettingBool("general", "overlay", data.inGameOverlay);
		plugin.SetSettingBool("general", "overlay_detailed", data.inGameOverlayDetailed);
		plugin.SetSettingBool("general", "overlay_transparency", data.overlayTransparency);
		plugin.SetSettingBool("general", "use_extended_shorthand", data.useExtendedShorthand);
		plugin.SetSettingBool("livesplit", "enabled", data.liveSplitEnabled);
		plugin.SetSetting("livesplit", "ip", data.liveSplitIP);
		plugin.SetSettingInt("livesplit", "port", data.liveSplitPort);
		
		auto spinOverlayDock = "none";
		switch (data.overlayDockMode) {
		case DockMode::TopLeft:
			spinOverlayDock = "topleft";
			break;
		case DockMode::TopRight:
			spinOverlayDock = "topright";
			break;
		case DockMode::BottomLeft:
			spinOverlayDock = "bottomleft";
			break;
		case DockMode::BottomRight:
			spinOverlayDock = "bottomright";
			break;
		case DockMode::AboveMinimap:
			spinOverlayDock = "abovemap";
			break;
		case DockMode::BelowMinimap:
			spinOverlayDock = "belowmap";
			break;
		case DockMode::LeftBelowMinimap:
			spinOverlayDock = "leftbelowmap";
			break;
		}
		plugin.SetSetting("general", "overlay_dock", spinOverlayDock);

		auto overlayDetail = "normal";
		switch (data.overlayDetail) {
			case OverlayDetailMode::WithNames:
				overlayDetail = "names";
				break;
		}
		plugin.SetSetting("general", "overlay_detail", overlayDetail);
	}

	inline ConfigData& Get() { return data; }
	inline const ConfigData& Get() const { return data; }

private:
	IPluginInterface& plugin;
	ConfigData data;
};
