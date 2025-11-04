#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace Utils
{
	class TimedWorker
	{
	public:
		TimedWorker();
		virtual ~TimedWorker();

		void Start();
		void Stop();
		void Notify();

		std::atomic<std::chrono::milliseconds> minInterval;

	protected:
		virtual void Work() = 0;

	private:
		void Run();

		std::mutex mutex;
		std::condition_variable cv;
		std::thread worker;
		std::atomic<bool> running;
	};
}
