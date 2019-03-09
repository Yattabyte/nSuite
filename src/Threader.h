#pragma once
#ifndef THREADER_H
#define THREADER_H

#include <atomic>
#include <deque>
#include <thread>
#include <shared_mutex>
#include <memory>
#include <vector>


/** Utility class for executing tasks across multiple threads. */
class Threader {
public:
	// Public (de)constructors
	/** Destroys this threader object, shutting down all threads it owns. */
	~Threader() {
		shutdown();
	}
	/** Creates a threader object and generates as many worker threads as the system allows. */
	Threader() {
		for (size_t x = 0; x < std::thread::hardware_concurrency(); ++x) {
			std::thread thread([&]() {
				while (m_alive) {
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


	// Public Methods
	/** Adds a job/task/function to the queue.
	@param	func	the task to be executed on a separate thread. A function with void return type and no arguments. */
	void addJob(const std::function<void()> && func) {
		std::unique_lock<std::shared_mutex> writeGuard(m_mutex);
		m_jobs.emplace_back(func);
	}
	void shutdown() {
		m_alive = false;
		for (size_t x = 0; x < std::thread::hardware_concurrency() && x < m_threads.size(); ++x) {
			if (m_threads[x].joinable())
				m_threads[x].join();
		}
		m_threads.clear();
		m_jobs.clear();
	}


private:
	// Private Attributes
	std::shared_mutex m_mutex;
	std::atomic_bool m_alive = true;
	std::vector<std::thread> m_threads;
	std::deque<std::function<void()>> m_jobs;
};

#endif // THREADER_H