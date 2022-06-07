#include "UI/Modals.hpp"
#include "TerraForge3D.hpp"

#include "imgui/imgui.h"
#include "ImGuiFileDialog/ImGuiFileDialog.h"


namespace TerraForge3D
{

	namespace UI
	{


		static std::string	lastPath = Utils::GetExecetableDirectory();

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
						if(currentModal.onBegin)
							currentModal.onBegin(currentModal.userData);
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
						if (currentModal.onBegin)
							currentModal.onBegin(currentModal.userData);
						modalsQueue.pop();
					}
				}
			}
			else
			{
				if (modalsQueue.size() > 0)
				{
					currentModal = modalsQueue.front();
					if (currentModal.onBegin)
						currentModal.onBegin(currentModal.userData);
					modalsQueue.pop();
				}
			}
		}

		Modal* ModalManager::MessageBox(std::string title, std::string message, MessageType messageType, float scale)
		{
			Modal modal;
			modal.finished = false;
			modal.isOpened = true;
			modal.name = title;
			modal.isClosable = true;
			modal.sizeScale = scale;
			modal.uiFunction = [messageType, message](void*) -> bool
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
			return AddModal(modal);
		}


		Modal* ModalManager::LoadingBox(std::string title, float* progress, std::function<bool(float)> onCancel, std::function<std::string(float)> getMessage, float scale)
		{
			TF3D_ASSERT(progress, "Progress pointer cannot be NULL");
			Modal modal;
			modal.finished = false;
			modal.isOpened = true;
			modal.name = title;
			modal.isClosable = false;
			modal.sizeScale = scale;
			modal.uiFunction = [progress, getMessage, onCancel](void*) -> bool
			{
				TF3D_ASSERT(*progress > 0.0, "Progress cannot be negetive");
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
			return AddModal(modal);
		}

		Modal* ModalManager::FileDialog(std::string title, FileDialogInfo fileDialogInfo, float scale)
		{
			Modal modal;
			modal.finished = false;
			modal.isOpened = true;
			modal.name = title;
			modal.isClosable = false;
			modal.sizeScale = scale;
			modal.onBegin = [fileDialogInfo, title](void*) -> void
			{
				std::string filter = "";
				if (fileDialogInfo.selection == FileDialogSelection_FilesOnly)
				{
					for (int i = 0; i < fileDialogInfo.allowedExtensions.size(); i++)
					{
						filter += fileDialogInfo.allowedExtensions[i];
						if (i != fileDialogInfo.allowedExtensions.size() - 1)
							filter += ",";
					}
				}
				std::string initialDir = "";
				if (fileDialogInfo.initialDir.size() > 3)
					initialDir = fileDialogInfo.initialDir;
				else
					initialDir = lastPath;
				ImGuiFileDialog::Instance()->OpenDialog(title, title, filter.data(), initialDir, 1, nullptr, ImGuiFileDialogFlags_NoDialog);
			};
			modal.uiFunction = [fileDialogInfo, title](void*) -> bool
			{
				FileDialogInfo fdi = fileDialogInfo;
				lastPath = ImGuiFileDialog::Instance()->GetCurrentPath();
				if (ImGuiFileDialog::Instance()->Display(title, ImGuiWindowFlags_NoCollapse, ImVec2(0, 0), ImVec2(0, 350)))
				{
					if (ImGuiFileDialog::Instance()->IsOk())
					{
						if (fileDialogInfo.mode == FileDialogMode_Open)
						{
							if (fileDialogInfo.selection == FileDialogSelection_FilesOnly)
							{
								std::map<std::string, std::string> files = ImGuiFileDialog::Instance()->GetSelection();
								if (files.size() == 0)
									return false;
								fdi.selectedFilePath = files.begin()->second;
								fdi.selectedFileName = files.begin()->first;
							}
							else
							{
								fdi.selectedFilePath  = lastPath;
							}
						}
						else
						{
							fdi.selectedFilePath = ImGuiFileDialog::Instance()->GetFilePathName();
							fdi.selectedFileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
							fdi.selectedFileExtension = ImGuiFileDialog::Instance()->GetCurrentFilter();
						}
						if (fileDialogInfo.onSelect)
							fileDialogInfo.onSelect(&fdi);
					}

					ImGuiFileDialog::Instance()->Close();
					return true;
				}
				return false;
			};

			return AddModal(modal);
		}

	}

}
