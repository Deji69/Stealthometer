#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include "json.hpp"

enum class Events;
template<Events>
struct Event;

template<Events T>
class ServerEvent
{
public:
	nlohmann::json json;
	typename Event<T>::EventValue Value;
	std::string ContractSessionId;
	std::string ContractId;
	std::string Name;
	double Timestamp = 0;

public:
	ServerEvent(typename Event<T>::EventValue&& value) : Value(std::forward<typename Event<T>::EventValue>(value))
	{ }
};

extern std::unordered_set<std::string> eventNameBlacklist;

class EventListenersBase
{
protected:
	auto virtual call(const nlohmann::json& ev) const -> bool = 0;

public:
	auto operator()(const nlohmann::json& ev) const -> bool {
		return this->call(ev);
	}

	auto handle(const nlohmann::json& ev) const -> bool {
		return this->call(ev);
	}
};

template<Events TEvent>
class EventListeners : public EventListenersBase
{
public:
	using HandlerFunc = void(const ServerEvent<TEvent>&);

	EventListeners()
	{ }

	template<typename TFunc>
	auto add(TFunc&& func) -> void {
		this->handlers.emplace_back(std::forward<TFunc>(func));
	}

protected:
	auto call(const nlohmann::json& ev) const -> bool {
		auto it = ev.find("Value");
		if (it == ev.end()) return false;

		ServerEvent<TEvent> serverEvent{typename Event<TEvent>::EventValue(*it)};
		serverEvent.json  = ev;
		serverEvent.Name = ev.value("Name", "");
		serverEvent.ContractId = ev.value("ContractId", "");
		serverEvent.ContractSessionId = ev.value("ContractSessionId", "");
		serverEvent.Timestamp = ev.value("Timestamp", 0);

		for (auto& handler : this->handlers)
			handler(serverEvent);

		return this->handlers.size() > 0;
	}

private:
	std::vector<std::function<HandlerFunc>> handlers;
};

class EventSystem {
public:
	template<Events TEvent>
	auto listen(std::function<void(const ServerEvent<TEvent>&)> handler) {
		auto eventNameIt = this->eventNames.find(TEvent);
		if (eventNameIt == this->eventNames.end())
			eventNameIt = this->eventNames.emplace(TEvent, Event<TEvent>::Name).first;
		
		auto it = this->listeners.find(eventNameIt->second);
		if (it == this->listeners.end())
			it = this->listeners.emplace(eventNameIt->second, std::make_unique<EventListeners<TEvent>>()).first;
		static_cast<EventListeners<TEvent>*>(it->second.get())->add(handler);
	}

	auto handle(const std::string& str, const nlohmann::json& json) -> bool {
		auto listeners = this->findListeners(str);
		if (listeners) return listeners->handle(json);
		return false;
	}

	auto handle(Events ev, const nlohmann::json& json) {
		auto listeners = this->findListeners(ev);
		if (listeners) return listeners->handle(json);
		return false;
	}

	auto getEventName(Events ev) -> const std::string* {
		auto it = this->eventNames.find(ev);
		if (it != this->eventNames.end())
			return &it->second;
		return nullptr;
	}

private:
	auto findListeners(Events ev) -> EventListenersBase* {
		auto name = this->getEventName(ev);
		if (name) return this->findListeners(*name);
		return nullptr;
	}

	auto findListeners(const std::string& name) -> EventListenersBase* {
		auto it = this->listeners.find(name);
		if (it != this->listeners.end())
			return it->second.get();
		return nullptr;
	}

private:
	std::unordered_map<Events, std::string> eventNames;
	std::unordered_map<std::string, std::unique_ptr<EventListenersBase>> listeners;
};
