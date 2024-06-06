#pragma once
#include <atomic>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>
#include <Windows.h>

struct ConfigData;

enum class eClientMessage
{
	StartOrSplit,
	Split,
	Unsplit,
	SkipSplit,
	Pause,
	Resume,
	Reset,
	StartTimer,
	SwitchTo,
	SetGameTime,

	GetCurrentTimerPhase,
};

enum class eLiveSplitTimerPhase
{
	NotRunning,
	Running,
	Ended,
	Paused,
};

class ClientMessage
{
public:
	auto toString() const->std::string;

	eClientMessage type;
	std::string args;
};

class LiveSplitClient
{
public:
	LiveSplitClient(const ConfigData&);
	~LiveSplitClient();

	auto start() -> bool;
	auto stop() -> void;
	auto abort() -> void;
	auto isStarted() const -> bool { return this->keepOpen; }
	auto isConnected() const -> bool { return this->connected; }
	auto send(eClientMessage type, std::initializer_list<std::string> args = {}) -> void;
	auto pause() -> void;
	auto getTimerPhase() -> std::optional<eLiveSplitTimerPhase>;

protected:
	auto reconnect() -> bool;
	auto writeMessage(const ClientMessage&) -> bool;

private:
	const ConfigData& config;
	std::thread clientThread;
	std::thread writeThread;
	mutable std::shared_mutex queueMutex;
	mutable std::shared_mutex connectionMutex;
	std::deque<ClientMessage> queue;
	std::atomic_bool connected = false;
	std::atomic_bool keepOpen = false;
	bool useQueue = false;
	SOCKET sock = INVALID_SOCKET;
	OVERLAPPED overlapped = {};
};
