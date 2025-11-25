#pragma once

#include <functional>
#include <memory>

#include "RE/Skyrim.h"

namespace Utils
{
	template <class T>
	class EventHandler : public RE::BSTEventSink<T>
	{
	public:
		using Callback = std::function<void(const T&)>;

		explicit EventHandler(Callback a_callback) :
			callback(std::move(a_callback))
		{
			instance = std::make_shared<EventHandler<T>>(*this);
		}

		operator RE::BSTEventSink<T>*()
		{
			return instance.get();
		}

		virtual RE::BSEventNotifyControl ProcessEvent(
			const T* a_event,
			RE::BSTEventSource<T>* a_eventSource) override
		{
			(void)a_eventSource;
			if (a_event && callback) {
				callback(*a_event);
			}
			return RE::BSEventNotifyControl::kContinue;
		}

	private:
		Callback callback;
		inline static std::shared_ptr<EventHandler<T>> instance{ nullptr };
	};
}
