#include "Generators/GenerationWorker.h"

#include "Utils/Utils.h"

#include <GLFW/glfw3.h>

GenerationWorker::GenerationWorker(WorkCallback callback)
	: m_Callback(std::move(callback))
{
	GLFWwindow* renderWindow = glfwGetCurrentContext();
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	m_Window = glfwCreateWindow(1, 1, "TerraForge3D Generation", nullptr, renderWindow);
	glfwMakeContextCurrent(renderWindow);
	if (m_Window != nullptr)
	{
		m_Thread = std::thread(&GenerationWorker::Run, this);
	}
	else
	{
		TF3D_LOG_ERROR("Failed to create shared generation OpenGL context; generation will run on the render thread");
	}
}

GenerationWorker::~GenerationWorker()
{
	{
		std::lock_guard lock(m_Mutex);
		m_StopRequested = true;
	}
	m_Condition.notify_one();

	if (m_Thread.joinable()) m_Thread.join();
	if (m_Window != nullptr)
	{
		glfwDestroyWindow(m_Window);
		m_Window = nullptr;
	}
}

bool GenerationWorker::Request(bool force)
{
	if (!HasContext()) return false;
	{
		std::lock_guard lock(m_Mutex);
		m_RequestPending = true;
		m_ForceRequested = m_ForceRequested || force;
	}
	m_Condition.notify_one();
	return true;
}

void GenerationWorker::WaitForIdle()
{
	std::unique_lock lock(m_Mutex);
	m_Condition.wait(lock, [this] {
		return !m_Running.load(std::memory_order_acquire) &&
			!m_RequestPending.load(std::memory_order_acquire);
	});
}

bool GenerationWorker::ConsumeCompleted()
{
	return m_Completed.exchange(false, std::memory_order_acq_rel);
}

void GenerationWorker::Run()
{
	glfwMakeContextCurrent(m_Window);
	while (true)
	{
		bool force = false;
		{
			std::unique_lock lock(m_Mutex);
			m_Condition.wait(lock, [this] {
				return m_StopRequested.load(std::memory_order_acquire) ||
					m_RequestPending.load(std::memory_order_acquire);
			});
			if (m_StopRequested.load(std::memory_order_acquire)) break;

			force = m_ForceRequested;
			m_RequestPending = false;
			m_ForceRequested = false;
			m_Running = true;
		}

		if (m_Callback) m_Callback(force);

		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		glFinish();

		{
			std::lock_guard lock(m_Mutex);
			m_Running = false;
			m_Completed = true;
		}
		m_Condition.notify_all();
	}
	glfwMakeContextCurrent(nullptr);
}
