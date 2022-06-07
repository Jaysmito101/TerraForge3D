#pragma once
#include "Base/Core/Core.hpp"


namespace TerraForge3D
{
	class ApplicationState;
	

	namespace UI
	{

		enum ModalResult
		{
			ModalResult_Closed = 0,
			ModalResult_Done
		};

		enum MessageType
		{
			MessageType_Info = 0,
			MessageType_Warning,
			MessageType_Error
		};

		enum FileDialogMode
		{
			FileDialogMode_Open = 0,
			FileDialogMode_Save
		};

		enum FileDialogSelection
		{
			FileDialogSelection_FilesOnly = 0,
			FileDialogSelection_DirectoriesOnly,
		};
		
		struct FileDialogInfo
		{
			FileDialogMode mode = FileDialogMode_Open;
			FileDialogSelection selection = FileDialogSelection_FilesOnly;
			std::string selectedFilePath = "";
			std::string selectedFileName = "";
			std::string selectedFileExtension = "";
			std::string initialDir = "";
			std::vector<std::string> allowedExtensions = {".*"};
			std::function<void(FileDialogInfo*)> onSelect = nullptr;
		};

		struct Modal
		{
			std::string name = "Modal";
			std::function<bool(void*)> uiFunction = nullptr;
			std::function<void(void*, ModalResult)> onEnd = nullptr;
			std::function<void(void*)> onBegin = nullptr;
			void* userData;
			bool finished = true;
			bool isOpened = true;
			bool isClosable = true;
			float sizeScale = 0.7f; // Must be within 0.1 - 0.9 range
		};

		class ModalManager
		{
		public:
			ModalManager(ApplicationState* appState);
			virtual ~ModalManager();

			void Update();
			void Show();

			Modal* MessageBox(std::string title, std::string message, MessageType messageType = MessageType_Info, float scale = 0.2f);

			Modal* LoadingBox(std::string title, float* progress, std::function<bool(float)> onCancel = nullptr, std::function<std::string(float)> getMessage = nullptr, float scale = 0.3f);

			Modal* FileDialog(std::string title, FileDialogInfo fileDialogInfo, float scale = 0.7f);
			
			inline Modal* AddModal(Modal& modal) { this->modalsQueue.push(modal); return &this->modalsQueue.back(); }

			inline bool IsActive() { return !currentModal.finished; }

		private:
			Modal currentModal;

			std::queue<Modal> modalsQueue;
			UUID uid;
			std::string uidStr = "";
			ApplicationState* appState = nullptr;
		};

	}

}