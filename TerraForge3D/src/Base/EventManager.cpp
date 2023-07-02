#include "Base/EventManager.h"

EventManager::EventManager()
{
}

EventManager::~EventManager()
{
}

void EventManager::Subscribe(const std::string& eventName, EventReciever callback, const std::string& callbackIDIn)
{
	// if the reciever chanel doesn't exist, create it
	if (m_EventSubscribers.find(eventName) == m_EventSubscribers.end())
		m_EventSubscribers[eventName] = std::unordered_map<std::string, EventReciever>();

	std::string callbackID = callbackIDIn;

	// if the callbackID is empty, generate one
	if (callbackID == "")
		callbackID = "callback" + std::to_string(m_EventSubscribers[eventName].size());

	// attach the callback to the reciever chanel
	m_EventSubscribers[eventName][callbackID] = (callback);
}

void EventManager::Unsubscribe(const std::string& recieverName, const std::string& callbackName)
{
	// If the reciever chanel doesn't exist, return
	if (m_EventSubscribers.find(recieverName) == m_EventSubscribers.end())
		return;

	// if the callback doesn't exist, return
	if (m_EventSubscribers[recieverName].find(callbackName) == m_EventSubscribers[recieverName].end())
		return;

	// remove the callback from the reciever chanel
	m_EventSubscribers[recieverName].erase(callbackName);
}

void EventManager::RaiseEvent(const std::string& eventName, const std::string& params, void* paramsPtr)
{
	if (m_EventSubscribers.find(eventName) == m_EventSubscribers.end())
		return;
	for (auto& [callbackID, callback] : m_EventSubscribers[eventName])
		if (callback(params, paramsPtr))
			break;
}
