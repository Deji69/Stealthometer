#include <WinSock2.h>
#include <chrono>
#include <format>
#include <map>
#include <shared_mutex>
#include <WS2tcpip.h>
#include <Logging.h>
#include "Config.h"
#include "LiveSplitClient.h"
#include "util.h"

#pragma comment(lib, "Ws2_32.lib")

using namespace std::chrono_literals;
using namespace std::string_literals;

std::map<eClientMessage, std::string> clientMessageTypeMap = {
	{eClientMessage::StartOrSplit, "startorsplit"},
	{eClientMessage::Split, "split"},
	{eClientMessage::Unsplit, "unsplit"},
	{eClientMessage::SkipSplit, "skipsplit"},
	{eClientMessage::Pause, "pause"},
	{eClientMessage::Resume, "resume"},
	{eClientMessage::Reset, "reset"},
	{eClientMessage::StartTimer, "starttimer"},
	{eClientMessage::SwitchTo, "switchto"},
	{eClientMessage::SetGameTime, "setgametime"},
	{eClientMessage::GetCurrentTimerPhase, "getcurrenttimerphase"},
};

auto ClientMessage::toString() const -> std::string {
	auto it = clientMessageTypeMap.find(this->type);
	if (it == end(clientMessageTypeMap)) return "";
	return this->args.size() ? std::format("{} {}", it->second, this->args) : it->second;
}

LiveSplitClient::LiveSplitClient(const ConfigData& config) : config(config) {}

auto LiveSplitClient::reconnect() -> bool {
	if (!this->keepOpen) return false;
	if (this->connected) return true;

	// Make this thread-safe (?)
	if (!connectionMutex.try_lock()) return false;

	if (!this->connected) {
		addrinfo* addressInfo = nullptr;
		auto connectInfo = addrinfo {};
		connectInfo.ai_family = AF_INET;
		connectInfo.ai_socktype = SOCK_STREAM;
		connectInfo.ai_protocol = IPPROTO_TCP;

		auto formattedPort = std::format("{}", config.liveSplitPort);
		if (getaddrinfo(config.liveSplitIP.c_str(), formattedPort.c_str(), &connectInfo, &addressInfo) != 0) {
			Logger::Error("getaddrinfo failed");
		}

		auto addrInfo = addressInfo;

		if (this->sock != INVALID_SOCKET) {
			closesocket(this->sock);
			this->sock = INVALID_SOCKET;
		}

		for (; addrInfo != nullptr; addrInfo = addrInfo->ai_next) {
			this->sock = socket(addrInfo->ai_family, addrInfo->ai_socktype, addrInfo->ai_protocol);
			if (this->sock == INVALID_SOCKET) {
				Logger::Error("Error creating socket");
				continue;
			}

			if (connect(this->sock, addrInfo->ai_addr, static_cast<int>(addrInfo->ai_addrlen)) != SOCKET_ERROR)
				break;
			closesocket(this->sock);
			this->sock = INVALID_SOCKET;
			Logger::Error("Error connecting to socket");
		}

		this->connected = this->sock != INVALID_SOCKET;
		freeaddrinfo(addressInfo);
	}

	connectionMutex.unlock();
	return this->connected;
}

auto LiveSplitClient::start() -> bool {
	if (this->keepOpen) return false;

	this->keepOpen = true;
	this->connected = false;

	WSADATA wsaData = {};
	int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (res != 0) {
		Logger::Error("WSAStartup failed {}", res);
		return false;
	}

	writeThread = std::thread([this] {
		std::vector<ClientMessage> writeQueue;

		while (this->keepOpen) {
			if (!this->reconnect()) {
				std::this_thread::sleep_for(3s);
				continue;
			}

			// Process the outward message queue
			if (this->queueMutex.try_lock()) {
				while (!this->queue.empty()) {
					auto& msg = this->queue.back();
					writeQueue.push_back(std::move(msg));
					this->queue.pop_back();
				}
				this->queueMutex.unlock();
			}

			// Send out messages to socket
			while (!writeQueue.empty()) {
				auto& msg = writeQueue.back();
				if (!this->writeMessage(msg)) break;
				writeQueue.pop_back();
			}

			if (this->connected) std::this_thread::sleep_for(100ms);
		}
	});
	return true;
}

auto LiveSplitClient::stop() -> void {
	this->keepOpen = false;
	this->writeThread.join();
	this->connected = false;

	if (this->sock != INVALID_SOCKET) {
		if (shutdown(this->sock, SD_SEND) == SOCKET_ERROR)
			Logger::Error("Shutdown failed");

		closesocket(this->sock);
		this->sock = INVALID_SOCKET;
	}

	WSACleanup();
}

auto LiveSplitClient::abort() -> void {
	this->keepOpen = false;
	this->writeThread.detach();
	this->connected = false;

	if (this->sock != INVALID_SOCKET) {
		if (shutdown(this->sock, SD_SEND) == SOCKET_ERROR)
			Logger::Error("Shutdown failed");

		closesocket(this->sock);
		this->sock = INVALID_SOCKET;
	}

	WSACleanup();
}

LiveSplitClient::~LiveSplitClient() {
	if (this->keepOpen) this->abort();
}

auto LiveSplitClient::writeMessage(const ClientMessage& msg) -> bool {
	auto data = msg.toString() + "\n";
	DWORD written = 0;
	int bytes_sent = ::send(this->sock, data.c_str(), data.size(), 0);
	if (bytes_sent == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAETIMEDOUT)
			this->connected = false;
		return false;
	}
	return true;
}

auto LiveSplitClient::send(eClientMessage type, std::initializer_list<std::string> args) -> void {
	if (!this->connected) return;
	ClientMessage message;
	message.type = type;
	std::string argStr;
	for (auto const& arg : args)
		argStr += (argStr.empty() ? "" : " ") + arg;
	message.args = std::move(argStr);
	if (useQueue)
		this->queue.push_back(std::move(message));
	else
		this->writeMessage(message);
}

auto LiveSplitClient::pause() -> void {
	auto state = this->getTimerPhase();
	if (state == eLiveSplitTimerPhase::Running)
		this->send(eClientMessage::Pause);
}

auto LiveSplitClient::getTimerPhase() -> std::optional<eLiveSplitTimerPhase> {
	if (!this->connected) return std::nullopt;
	this->send(eClientMessage::GetCurrentTimerPhase);
	char buffer[64] = {};
	auto res = ::recv(this->sock, buffer, sizeof(buffer), 0);
	if (res > 0) {
		if ("NotRunning\n"s == buffer)
			return eLiveSplitTimerPhase::NotRunning;
		if ("Running\n"s == buffer)
			return eLiveSplitTimerPhase::Running;
		if ("Ended\n"s == buffer)
			return eLiveSplitTimerPhase::Ended;
		if ("Paused\n"s == buffer)
			return eLiveSplitTimerPhase::Paused;
	}
	return std::nullopt;
}
