#include "Profiler.h"

#include <imgui/imgui.h>

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace
{
	using EventSnapshot = PerformanceMonitor::EventSnapshot;
	using FrameSnapshot = PerformanceMonitor::FrameSnapshot;

	struct HotPath
	{
		double totalMs = 0.0;
		double maximumMs = 0.0;
		uint64_t calls = 0;
	};

	struct CallTreeNode
	{
		std::string key;
		double recordedMs = 0.0;
		uint64_t calls = 0;
		std::vector<CallTreeNode> children;
		std::unordered_set<uint64_t> threadIds;
		uint64_t idSalt = 0;
	};

	CallTreeNode* FindOrAddChild(CallTreeNode& parent, const std::string& key)
	{
		for (CallTreeNode& child : parent.children)
			if (child.key == key) return &child;
		parent.children.push_back(CallTreeNode{ key });
		return &parent.children.back();
	}

	void AddCallTreeEvent(CallTreeNode& root, const EventSnapshot& event)
	{
		if (event.callStack.empty()) return;
		root.recordedMs += event.durationMs;
		root.calls++;
		root.threadIds.insert(event.threadId);
		CallTreeNode* node = &root;
		for (const std::string& key : event.callStack)
		{
			node = FindOrAddChild(*node, key);
			node->recordedMs += event.durationMs;
			node->calls++;
			node->threadIds.insert(event.threadId);
		}
	}

	void SortCallTree(CallTreeNode& node)
	{
		std::sort(node.children.begin(), node.children.end(), [](const CallTreeNode& left, const CallTreeNode& right) {
			return left.recordedMs > right.recordedMs;
		});
		for (CallTreeNode& child : node.children) SortCallTree(child);
	}

	double ChildRecordedMs(const CallTreeNode& node)
	{
		double childTotal = 0.0;
		for (const CallTreeNode& child : node.children) childTotal += child.recordedMs;
		return childTotal;
	}

	std::string ShortTaskName(const std::string& key)
	{
		const std::size_t separator = key.find_last_of('/');
		return separator == std::string::npos ? key : key.substr(separator + 1);
	}

	void DrawCallTreeNode(const CallTreeNode& node, const std::string& path)
	{
		const double selfMs = std::max(0.0, node.recordedMs - ChildRecordedMs(node));
		const bool hasChildren = !node.children.empty();
		const ImGuiTreeNodeFlags flags = hasChildren ? 0 : ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		const std::string displayName = ShortTaskName(node.key);
		const std::string label = displayName + "##" + path;
		const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%.3f ms", node.recordedMs);
		ImGui::TableSetColumnIndex(2);
		ImGui::Text("%.3f ms", selfMs);
		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%llu", static_cast<unsigned long long>(node.calls));
		ImGui::TableSetColumnIndex(4);
		ImGui::Text("%zu", node.threadIds.size());
		if (open && hasChildren)
		{
			for (const CallTreeNode& child : node.children)
				DrawCallTreeNode(child, path + "/" + child.key);
			ImGui::TreePop();
		}
	}

	void DrawHotPathList(const std::vector<PerformanceMonitor::FrameSnapshot>& frames)
	{
		std::unordered_map<std::string, HotPath> paths;
		for (const auto& frame : frames)
		{
			for (const EventSnapshot& event : frame.events)
			{
				HotPath& path = paths[event.key];
				path.totalMs += event.durationMs;
				path.maximumMs = std::max(path.maximumMs, event.durationMs);
				++path.calls;
			}
		}

		std::vector<std::pair<std::string, HotPath>> sortedPaths(paths.begin(), paths.end());
		std::sort(sortedPaths.begin(), sortedPaths.end(), [](const auto& left, const auto& right) {
			if (left.second.totalMs != right.second.totalMs) return left.second.totalMs > right.second.totalMs;
			return left.first < right.first;
		});

		if (sortedPaths.empty())
		{
			ImGui::TextDisabled("No captured events.");
			return;
		}

		if (!ImGui::BeginTable("##PerformanceHotPaths", 5,
			ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
			ImVec2(0.0f, 0.0f))) return;
		ImGui::TableSetupColumn("Task", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed, 90.0f);
		ImGui::TableSetupColumn("Average", ImGuiTableColumnFlags_WidthFixed, 90.0f);
		ImGui::TableSetupColumn("Max", ImGuiTableColumnFlags_WidthFixed, 90.0f);
		ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 70.0f);
		ImGui::TableHeadersRow();
		for (const auto& [key, path] : sortedPaths)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(key.c_str());
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%.3f ms", path.totalMs);
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%.3f ms", path.totalMs / static_cast<double>(path.calls));
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.3f ms", path.maximumMs);
			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%llu", static_cast<unsigned long long>(path.calls));
		}
		ImGui::EndTable();
	}
}

PerformanceMonitor& PerformanceMonitor::Get()
{
	static PerformanceMonitor monitor;
	return monitor;
}

uint64_t PerformanceMonitor::GetThreadId()
{
	return static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

void PerformanceMonitor::SetCurrentThreadName(std::string_view name)
{
	std::lock_guard lock(m_Mutex);
	const uint64_t threadId = GetThreadId();
	if (name.empty())
		m_ThreadNames.erase(threadId);
	else
		m_ThreadNames[threadId] = std::string(name);
}

void PerformanceMonitor::Scope::End()
{
	if (m_Monitor == nullptr || m_EventId == 0) return;
	m_Monitor->EndEvent(m_EventId);
	m_Monitor = nullptr;
	m_EventId = 0;
}

void PerformanceMonitor::BeginFrame()
{
	std::lock_guard lock(m_Mutex);
	m_CurrentFrame = {};
	m_CurrentFrame.index = ++m_NextFrameId;
	m_FrameStart = Clock::now();
	m_FrameOpen = true;
}

void PerformanceMonitor::EndFrame()
{
	std::lock_guard lock(m_Mutex);
	if (!m_FrameOpen) return;

	const auto now = Clock::now();
	m_CurrentFrame.durationMs = std::chrono::duration<double, std::milli>(now - m_FrameStart).count();
	if (m_IsCapturing) m_CapturedFrames.push_back(std::move(m_CurrentFrame));
	m_CurrentFrame = {};
	m_FrameOpen = false;
}

PerformanceMonitor::Scope PerformanceMonitor::BeginScope(std::string_view key)
{
	return Scope(this, BeginEvent(key));
}

uint64_t PerformanceMonitor::BeginEvent(std::string_view key)
{
	if (key.empty()) return 0;

	std::lock_guard lock(m_Mutex);
	if (!m_IsCapturing) return 0;

	const uint64_t eventId = m_NextEventId++;
	const std::string ownedKey(key);
	const uint64_t threadId = GetThreadId();
	ActiveEvent activeEvent;
	activeEvent.key = ownedKey;
	activeEvent.start = Clock::now();
	activeEvent.threadId = threadId;
	const auto threadNameIterator = m_ThreadNames.find(threadId);
	if (threadNameIterator != m_ThreadNames.end()) activeEvent.threadName = threadNameIterator->second;

	std::vector<uint64_t>& threadStack = m_ThreadStacks[threadId];
	if (!threadStack.empty())
	{
		activeEvent.parentEventId = threadStack.back();
		const auto parentIterator = m_ActiveEvents.find(activeEvent.parentEventId);
		if (parentIterator != m_ActiveEvents.end())
		{
			activeEvent.parentKey = parentIterator->second.key;
			activeEvent.callStack = parentIterator->second.callStack;
			activeEvent.depth = parentIterator->second.depth + 1;
		}
	}
	activeEvent.callStack.push_back(ownedKey);

	m_ActiveEvents.emplace(eventId, std::move(activeEvent));
	threadStack.push_back(eventId);
	return eventId;
}

void PerformanceMonitor::EndEvent(uint64_t eventId)
{
	if (eventId == 0) return;

	std::lock_guard lock(m_Mutex);
	const auto activeIterator = m_ActiveEvents.find(eventId);
	if (activeIterator == m_ActiveEvents.end()) return;

	ActiveEvent activeEvent = std::move(activeIterator->second);
	m_ActiveEvents.erase(activeIterator);
	const auto now = Clock::now();
	const double durationMs = std::chrono::duration<double, std::milli>(now - activeEvent.start).count();

	const auto stackIterator = m_ThreadStacks.find(activeEvent.threadId);
	if (stackIterator != m_ThreadStacks.end())
	{
		auto& stack = stackIterator->second;
		const auto eventIterator = std::find(stack.rbegin(), stack.rend(), eventId);
		if (eventIterator != stack.rend()) stack.erase(std::next(eventIterator).base());
		if (stack.empty()) m_ThreadStacks.erase(stackIterator);
	}

	if (!m_IsCapturing || !m_FrameOpen) return;

	double startOffsetMs = 0.0;
	if (activeEvent.start >= m_FrameStart)
		startOffsetMs = std::chrono::duration<double, std::milli>(activeEvent.start - m_FrameStart).count();
	m_CurrentFrame.events.push_back(EventSnapshot{
		eventId,
		activeEvent.parentEventId,
		activeEvent.key,
		activeEvent.parentKey,
		durationMs,
		std::max(0.0, startOffsetMs),
		activeEvent.threadId,
		activeEvent.threadName,
		activeEvent.depth,
		activeEvent.callStack
	});
}

PerformanceMonitor::Snapshot PerformanceMonitor::CaptureSnapshot() const
{
	Snapshot snapshot;
	std::lock_guard lock(m_Mutex);

	snapshot.isCapturing = m_IsCapturing;
	snapshot.capturedFrames.reserve(m_CapturedFrames.size() + (m_IsCapturing && m_FrameOpen ? 1 : 0));
	for (const FrameData& frame : m_CapturedFrames)
	{
		snapshot.capturedFrames.push_back(FrameSnapshot{ frame.index, frame.durationMs, frame.events });
		snapshot.capturedDurationMs += frame.durationMs;
		 snapshot.capturedEventCount += frame.events.size();
	}

	if (m_IsCapturing && m_FrameOpen)
	{
		const double currentDurationMs = std::chrono::duration<double, std::milli>(Clock::now() - m_FrameStart).count();
		snapshot.capturedFrames.push_back(FrameSnapshot{ m_CurrentFrame.index, currentDurationMs, m_CurrentFrame.events });
		snapshot.capturedDurationMs += currentDurationMs;
		snapshot.capturedEventCount += m_CurrentFrame.events.size();
	}

	return snapshot;
}

void PerformanceMonitor::StartCapture()
{
	std::lock_guard lock(m_Mutex);
	m_CapturedFrames.clear();
	m_ActiveEvents.clear();
	m_ThreadStacks.clear();
	m_IsCapturing = true;
	if (m_FrameOpen)
	{
		m_CurrentFrame.events.clear();
		m_CurrentFrame.durationMs = 0.0;
		m_FrameStart = Clock::now();
	}
}

void PerformanceMonitor::StopCapture()
{
	std::lock_guard lock(m_Mutex);
	if (!m_IsCapturing) return;

	if (m_FrameOpen)
	{
		m_CurrentFrame.durationMs = std::chrono::duration<double, std::milli>(Clock::now() - m_FrameStart).count();
		m_CapturedFrames.push_back(std::move(m_CurrentFrame));
		m_CurrentFrame = {};
		m_FrameOpen = false;
	}

	// Discard all scopes that were still open when capture stopped.
	m_ActiveEvents.clear();
	m_ThreadStacks.clear();
	m_IsCapturing = false;
}

void PerformanceMonitor::ClearCapture()
{
	std::lock_guard lock(m_Mutex);
	m_CapturedFrames.clear();
	m_ActiveEvents.clear();
	m_ThreadStacks.clear();
	m_IsCapturing = false;
}

void PerformanceMonitor::RenderUI()
{
	if (!m_WindowOpen) return;

	Snapshot snapshot = CaptureSnapshot();
	if (!ImGui::Begin("Performance Monitor", &m_WindowOpen))
	{
		ImGui::End();
		return;
	}

	bool snapshotChanged = false;
	if (snapshot.isCapturing)
	{
		if (ImGui::Button("Stop Capture"))
		{
			StopCapture();
			snapshotChanged = true;
		}
	}
	else if (ImGui::Button("Start Capture"))
	{
		StartCapture();
		snapshotChanged = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear Capture"))
	{
		ClearCapture();
		snapshotChanged = true;
	}

	if (snapshotChanged) snapshot = CaptureSnapshot();

	if (snapshot.isCapturing)
		ImGui::Text("Recording | %zu frames | %zu completed events | %.3f ms", snapshot.capturedFrames.size(), snapshot.capturedEventCount, snapshot.capturedDurationMs);
	else if (!snapshot.capturedFrames.empty())
		ImGui::Text("Stopped | %zu frames | %zu completed events | %.3f ms", snapshot.capturedFrames.size(), snapshot.capturedEventCount, snapshot.capturedDurationMs);
	else
		ImGui::TextDisabled("No capture. Press Start Capture to begin.");
	ImGui::Separator();

	if (ImGui::BeginTabBar("##PerformanceMonitorTabs"))
	{
		if (ImGui::BeginTabItem("Hot Paths"))
		{
			ImGui::TextDisabled("Aggregated from completed events in this capture.");
			DrawHotPathList(snapshot.capturedFrames);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Call Tree"))
		{
			ImGui::TextDisabled("Inclusive and self time are totals across the entire capture.");
			ImGui::SameLine();
			ImGui::Checkbox("Group by thread", &m_GroupCallTreeByThread);

			if (ImGui::BeginTable("##PerformanceCallTree", 5,
				ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
				ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
				ImVec2(0.0f, 0.0f)))
			{
				ImGui::TableSetupColumn("Task", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Inclusive", ImGuiTableColumnFlags_WidthFixed, 90.0f);
				ImGui::TableSetupColumn("Self", ImGuiTableColumnFlags_WidthFixed, 90.0f);
				ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed, 65.0f);
				ImGui::TableSetupColumn("Threads", ImGuiTableColumnFlags_WidthFixed, 70.0f);
				ImGui::TableHeadersRow();

				if (!m_GroupCallTreeByThread)
				{
					CallTreeNode root{ "root" };
					for (const FrameSnapshot& frame : snapshot.capturedFrames)
						for (const EventSnapshot& event : frame.events) AddCallTreeEvent(root, event);
					SortCallTree(root);
					for (const CallTreeNode& child : root.children) DrawCallTreeNode(child, child.key);
				}
				else
				{
					std::unordered_map<uint64_t, std::size_t> rootIndices;
					std::vector<CallTreeNode> threadRoots;
					for (const FrameSnapshot& frame : snapshot.capturedFrames)
					{
						for (const EventSnapshot& event : frame.events)
						{
							auto [iterator, inserted] = rootIndices.emplace(event.threadId, threadRoots.size());
							if (inserted)
							{
								const std::size_t threadOrdinal = threadRoots.size() + 1;
								const std::string threadLabel = event.threadName.empty()
									? "Thread " + std::to_string(threadOrdinal)
									: event.threadName + " (T" + std::to_string(threadOrdinal) + ")";
								threadRoots.push_back(CallTreeNode{ threadLabel });
								threadRoots.back().idSalt = event.threadId;
							}
							AddCallTreeEvent(threadRoots[iterator->second], event);
						}
					}
					std::sort(threadRoots.begin(), threadRoots.end(), [](const CallTreeNode& left, const CallTreeNode& right) {
						return left.recordedMs > right.recordedMs;
					});
					for (const CallTreeNode& root : threadRoots)
						DrawCallTreeNode(root, root.key + "##thread-" + std::to_string(root.idSalt));
				}
				ImGui::EndTable();
			}
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}
