#pragma once

#include <unordered_map>
#include <vector>
#include <functional>
#include <string>

#define BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

using EventReciever = std::function<bool(const std::string&, void*)>;

class EventManager
{
public:
	EventManager();
	~EventManager();
	void Subscribe(const std::string& eventName, EventReciever callback);
	void UnsubscribeAll(const std::string& recieverName);
	void RaiseEvent(const std::string& eventName, const std::string& params = "", void* paramsPtr = nullptr);
private:
	std::unordered_map<std::string, std::vector<EventReciever>> m_EventSubscribers;
};