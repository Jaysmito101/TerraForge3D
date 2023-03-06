#include "Base/EventManager.h"

EventManager::EventManager()
{
}

EventManager::~EventManager()
{
}

void EventManager::Subscribe(const std::string& eventName, EventReciever callback)
{
	if (m_EventSubscribers.find(eventName) == m_EventSubscribers.end())
		m_EventSubscribers[eventName] = std::vector<EventReciever>();
	m_EventSubscribers[eventName].push_back(callback);
}

void EventManager::RaiseEvent(const std::string& eventName, const std::string& params, void* paramsPtr)
{
	if (m_EventSubscribers.find(eventName) == m_EventSubscribers.end())
		return;
	for (auto& callback : m_EventSubscribers[eventName])
		if (callback(params, paramsPtr))
			break;
}
