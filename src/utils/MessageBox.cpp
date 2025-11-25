#include "utils/MessageBox.h"

#include "RE/Skyrim.h"

namespace Utils
{
	namespace
	{
		class MessageBoxResultCallback final : public RE::IMessageBoxCallback
		{
		public:
			explicit MessageBoxResultCallback(MessageBoxResultCallbackFunc callback) :
				callback(std::move(callback))
			{}

			void Run(Message message) override
			{
				if (callback) {
					callback(static_cast<unsigned int>(message));
				}
			}

		private:
			MessageBoxResultCallbackFunc callback;
		};
	}

	void ShowMessageBox(const std::string& bodyText, const std::vector<std::string>& buttonTextValues, MessageBoxResultCallbackFunc callback)
	{
		const auto* factoryManager = RE::MessageDataFactoryManager::GetSingleton();
		const auto* uiStringHolder = RE::InterfaceStrings::GetSingleton();
		if (!factoryManager || !uiStringHolder) {
			return;
		}

		auto* factory = factoryManager->GetCreator<RE::MessageBoxData>(uiStringHolder->messageBoxData);
		auto* messageBox = factory ? factory->Create() : nullptr;
		if (!messageBox) {
			return;
		}

		const RE::BSTSmartPointer<RE::IMessageBoxCallback> messageCallback = RE::make_smart<MessageBoxResultCallback>(callback);
		messageBox->callback = messageCallback;
		messageBox->bodyText = bodyText;
		for (const auto& text : buttonTextValues) {
			messageBox->buttonText.emplace_back(text.c_str());
		}
		messageBox->QueueMessage();
	}
}
