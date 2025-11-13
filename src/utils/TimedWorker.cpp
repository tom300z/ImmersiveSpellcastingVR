#include "utils/TimedWorker.h"

namespace Utils
{
	TimedWorker::TimedWorker() :
		minInterval(std::chrono::milliseconds(5)),
		running(false)
	{}

	TimedWorker::~TimedWorker()
	{
		Stop();
	}

	void TimedWorker::Start()
	{
		bool expected = false;
		if (!running.compare_exchange_strong(expected, true, std::memory_order::relaxed, std::memory_order::relaxed)) {
			return;
		}

		if (worker.joinable()) {
			worker.join();
		}

		worker = std::thread(&TimedWorker::Run, this);
	}

	void TimedWorker::Stop()
	{
		const bool wasRunning = running.exchange(false, std::memory_order::relaxed);
		if (!wasRunning && !worker.joinable()) {
			return;
		}

		Notify();

		if (worker.joinable()) {
			worker.join();
			worker = std::thread();
		}
	}

	void TimedWorker::Notify()
	{
		cv.notify_one();
	}

	void TimedWorker::Run()
	{
		while (running.load(std::memory_order::relaxed)) {
			Work();

			auto interval = minInterval.load(std::memory_order::relaxed);
			auto now = std::chrono::steady_clock::now();
			bool waitForNotifyOnly = interval.count() <= 0;
			std::chrono::steady_clock::time_point nextLoop = now;

			if (!waitForNotifyOnly) {
				if (interval.count() < 5) {
					interval = std::chrono::milliseconds(5);
				}
				nextLoop = now + interval;
			}

			if (!running.load(std::memory_order::relaxed)) {
				break;
			}

			std::unique_lock<std::mutex> lock(mutex);
			if (!running.load(std::memory_order::relaxed)) {
				break;
			}

			auto updatedInterval = minInterval.load(std::memory_order::relaxed);
			const bool updatedWaitForNotifyOnly = updatedInterval.count() <= 0;

			if (!waitForNotifyOnly && !updatedWaitForNotifyOnly) {
				cv.wait_until(lock, nextLoop);
				continue;
			}

			if (!waitForNotifyOnly && updatedWaitForNotifyOnly) {
				cv.wait(lock);
				continue;
			}

			if (waitForNotifyOnly && !updatedWaitForNotifyOnly) {
				if (updatedInterval.count() < 5) {
					updatedInterval = std::chrono::milliseconds(5);
				}
				const auto adjustedLoop = std::chrono::steady_clock::now() + updatedInterval;
				cv.wait_until(lock, adjustedLoop);
				continue;
			}

			cv.wait(lock);
		}
	}
}
