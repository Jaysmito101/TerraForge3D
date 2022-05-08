#include "UI/Modals.hpp"
#include "Data/ApplicationState.hpp"
#include "Base/Base.hpp"
#include "Utils/Utils.hpp"

#include "imgui/imgui.h"

namespace TerraForge3D
{

	namespace UI
	{



		ModalManager::ModalManager(ApplicationState* appState)
		{
			this->appState = appState;
			this->uid = UUID::Generate();
			this->uidStr = this->uid.ToString();
		}

		ModalManager::~ModalManager()
		{
		}

		void ModalManager::Update()
		{
			// TODO: For future use
		}

		void ModalManager::Show()
		{
			static ImGuiWindowFlags modalWindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

			std::string modalWindowName = currentModal.name + "##" + uidStr;
			if (!currentModal.finished)
			{
				bool result = false;
				if(currentModal.isClosable)
					ImGui::Begin(modalWindowName.data(), &currentModal.isOpened, modalWindowFlags);
				else
					ImGui::Begin(modalWindowName.data(), nullptr, modalWindowFlags);
				ImGui::SetWindowFocus();
				auto [windowWidth, windowHeight] = appState->core.window->GetSize();
				TF3D_ASSERT(currentModal.sizeScale < 0.9 && currentModal.sizeScale > 0.1, "Invalid Modal Size Scale");
				ImGui::SetWindowSize(ImVec2(windowWidth * currentModal.sizeScale, windowHeight * currentModal.sizeScale), ImGuiCond_Always);
				auto modalPosX = (windowWidth - windowWidth * currentModal.sizeScale) / 2.0f;
				auto modalPosY = (windowHeight- windowHeight* currentModal.sizeScale) / 2.0f;
				ImGui::SetWindowPos(ImVec2(modalPosX, modalPosY), ImGuiCond_Always);
				if (currentModal.uiFunction)
					result = currentModal.uiFunction(currentModal.userData);
				ImGui::End();

				if (result)
				{
					if (currentModal.onEnd)
						currentModal.onEnd(currentModal.userData, ModalResult_Done);
					currentModal.finished = true;
					if (modalsQueue.size() > 0)
					{
						currentModal = modalsQueue.front();
						modalsQueue.pop();
					}
				}

				if (!currentModal.isOpened)
				{
					if (currentModal.onEnd)
						currentModal.onEnd(currentModal.userData, ModalResult_Closed);
					currentModal.finished = true;
					if (modalsQueue.size() > 0)
					{
						currentModal = modalsQueue.front();
						modalsQueue.pop();
					}
				}
			}
			else
			{
				if (modalsQueue.size() > 0)
				{
					currentModal = modalsQueue.front();
					modalsQueue.pop();
				}
			}
		}

		Modal* ModalManager::MessageBox(std::string title, std::string message, MessageType messageType, float scale)
		{
			currentModal.finished = false;
			currentModal.isOpened = true;
			currentModal.name = title;
			currentModal.isClosable = true;
			currentModal.sizeScale = scale;
			currentModal.uiFunction = [messageType, message](void*) -> bool
			{
				switch (messageType)
				{
				case MessageType_Info:
					Utils::ImGuiC::TextIcon(ICON_MD_INFO);
					ImGui::Text(message.data());
					break;
				case MessageType_Warning:
					Utils::ImGuiC::ColoredTextIcon(ICON_MD_WARNING, ImVec4(0.4f, 0.8f, 0.8f, 1.0f));
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.8f, 1.0f), message.data());
					break;
				case MessageType_Error:
					Utils::ImGuiC::ColoredTextIcon(ICON_MD_ERROR, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
					ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), message.data());
					break;
				default:
					break;
				}
				return ImGui::Button("OK");
			};
			return &currentModal;
		}

		Modal* ModalManager::LoadingBox(std::string title, float* progress, std::function<bool(float)> onCancel, std::function<std::string(float)> getMessage, float scale)
		{
			TF3D_ASSERT(progress, "Progress pointer cannot be NULL");
			currentModal.finished = false;
			currentModal.isOpened = true;
			currentModal.name = title;
			currentModal.isClosable = false;
			currentModal.sizeScale = scale;
			currentModal.uiFunction = [progress, getMessage, onCancel](void*) -> bool
			{
				ImGui::Text("Please wait ...");
				ImGui::ProgressBar(*progress);
				if (getMessage)
					ImGui::Text("Info: %s", getMessage(*progress).c_str());

				if (*progress >= 1.0f)
					return true;

				if (onCancel)
				{
					if (ImGui::Button("Cancel"))
					{
						if (onCancel(*progress))
						{
							return true;
						}
					}
				}

				return false;
			};
			return &currentModal;
		}

	}

}
