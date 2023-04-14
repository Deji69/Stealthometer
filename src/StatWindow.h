#pragma once
#include <string>
#include <thread>
#include <Windows.h>
#include "Stats.h"

#define STEALTHOMETER_CLOSE_EXTERNAL_WINDOW (WM_USER + 0x8008)

class StatWindow
{
	enum class Stat
	{
		None,
		Tension,
		Pacifications,
		Spotted,
		BodiesFound,
		DisguisesTaken,
		Recorded,
		GuardKills,
		CivKills,
		Witnesses,
		BodiesHidden,
		DisguisesBlown,
		TargetsFound,
		PlayStyle,
		StealthRating,
		SilentAssassin,
	};

public:
	StatWindow(const DisplayStats&);
	~StatWindow();

	auto create(HINSTANCE instance) -> void;
	auto update() -> void;
	auto destroy() -> void;

	auto setDarkMode(bool enable) -> void;

	auto paint(HWND wnd) -> void;
	auto paintBg(HWND wnd, HDC hdc) -> void;

protected:
	auto formatStat(Stat stat) -> std::string;
	auto paintCell(HDC hdc, std::string header, Stat stat, int col, int row) -> void;

private:
	const DisplayStats& stats;
	HWND hWnd = nullptr;
	HWND column1 = nullptr;
	HWND column2 = nullptr;
	ATOM wclAtom = NULL;
	bool darkMode = true;

	std::thread windowThread;

	volatile bool runningWindow = false;
};
