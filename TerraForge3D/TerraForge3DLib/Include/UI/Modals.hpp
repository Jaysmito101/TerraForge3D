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


		struct Modal
		{
			std::string name = "Modal";
			std::function<bool(void*)> uiFunction;
			std::function<void(void*, ModalResult)> onEnd;
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
			~ModalManager();

			void Update();
			void Show();

			Modal* MessageBox(std::string title, std::string message, MessageType messageType = MessageType_Info, float scale = 0.2f);

			Modal* LoadingBox(std::string title, float* progress, std::function<bool(float)> onCancel = nullptr, std::function<std::string(float)> getMessage = nullptr, float scale = 0.3f);

			inline void AddModal(Modal& modal) { this->modalsQueue.push(modal); }
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