#pragma once
#include <string>
#include <IPluginInterface.h>

struct ConfigData
{
	bool externalWindow = true;
	bool externalWindowDark = true;
	bool externalWindowOnTop = false;
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
		data.liveSplitEnabled = plugin.GetSettingBool("livesplit", "enabled", data.liveSplitEnabled);
		data.liveSplitIP = plugin.GetSetting("livesplit", "ip", data.liveSplitIP);
		data.liveSplitPort = plugin.GetSettingInt("livesplit", "port", data.liveSplitPort);
	}

	void Save() {
		if (data.liveSplitIP.empty())
			data.liveSplitIP = "127.0.0.1";
		if (!data.liveSplitPort)
			data.liveSplitPort = 16834;

		plugin.SetSettingBool("general", "external_window", data.externalWindow);
		plugin.SetSettingBool("general", "external_window_dark", data.externalWindowDark);
		plugin.SetSettingBool("general", "external_window_on_top", data.externalWindowOnTop);
		plugin.SetSettingBool("livesplit", "enabled", data.liveSplitEnabled);
		plugin.SetSetting("livesplit", "ip", data.liveSplitIP);
		plugin.SetSettingInt("livesplit", "port", data.liveSplitPort);
	}

	inline ConfigData& Get() { return data; }
	inline const ConfigData& Get() const { return data; }

private:
	IPluginInterface& plugin;
	ConfigData data;
};
