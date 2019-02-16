#include <deque>
#include <thread>
#include <shared_mutex>
#include <memory>
#include <vector>


class Threader {
public:
	// Public constructor
	Threader() {
		for (size_t x = 0; x < std::thread::hardware_concurrency(); ++x) {
			std::thread thread([&]() {
				while (true) {
					// Check if there is a job to do
					std::unique_lock<std::shared_mutex> writeGuard(m_mutex);
					if (m_jobs.size()) {
						// Get the first job, remove it from the list
						auto job = m_jobs.front();
						m_jobs.pop_front();
						// Unlock
						writeGuard.unlock();
						writeGuard.release();
						// Do Job
						job();
					}
				}
			});
			thread.detach();
			m_threads.emplace_back(std::move(thread));
		}
	}


	void addJob(const std::function<void()> && func) {
		std::unique_lock<std::shared_mutex> writeGuard(m_mutex);
		m_jobs.emplace_back(func);
	}


private:
	std::shared_mutex m_mutex;
	std::deque<std::function<void()>> m_jobs;
	std::vector<std::thread> m_threads;
};