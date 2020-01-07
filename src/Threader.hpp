#pragma once
#ifndef THREADER_H
#define THREADER_H

#include <atomic>
#include <algorithm>
#include <deque>
#include <functional>
#include <thread>
#include <shared_mutex>
#include <vector>


namespace yatta {
    /** Utility class for executing tasks across multiple threads. */
    class Threader {
    public:
        // Public (de)constructors
        /** Destroys this threader object, shutting down all threads it owns. */
        inline ~Threader() {
            shutdown();
        }
        /** Creates a threader object and generates a specified number of worker threads.
        @param	maxThreads		 the number of threads to spawn (up to std::thread::hardware_concurrency). */
        inline explicit Threader(const size_t& maxThreads = std::thread::hardware_concurrency()) noexcept {
            m_maxThreads = std::clamp<size_t>(maxThreads, 1ULL, static_cast<size_t>(std::thread::hardware_concurrency()));
            m_threadsActive = m_maxThreads;
            m_threads.resize(m_maxThreads);
            for (auto& thread : m_threads) {
                thread = std::thread([&]() {
                    while (m_alive && m_keepOpen) {
                        // Check if there is a job to do
                        if (std::unique_lock<std::shared_mutex> writeGuard(m_mutex, std::try_to_lock); writeGuard.owns_lock()) {
                            if (m_jobs.size()) {
                                // Get the first job, remove it from the list
                                auto job = m_jobs.front();
                                m_jobs.pop_front();
                                // Unlock
                                writeGuard.unlock();
                                writeGuard.release();
                                // Do Job
                                job();
                                m_jobsFinished++;
                            }
                        }
                    }
                    m_threadsActive--;
                    });
                thread.detach();
            }
        }
        /** Deleted copy-assignment constructor. */
        inline Threader(const Threader&) = delete;
        /** Deleted move-assignment constructor. */
        inline Threader(Threader&&) = delete;


        // Public Methods
        /** Adds a job/task/function to the queue.
        @param	func	the task to be executed on a separate thread. A function with void return type and no arguments. */
        inline void addJob(const std::function<void()>&& func) {
            std::unique_lock<std::shared_mutex> writeGuard(m_mutex);
            m_jobs.emplace_back(func);
            m_jobsStarted++;
        }
        /** Check if the threader has completed all its jobs.
        @return			true if finished, false otherwise. */
        inline bool isFinished() const noexcept {
            return m_jobsStarted == m_jobsFinished;
        }
        /** Prepare the threader for shutdown, notifying threads to complete early. */
        inline void prepareForShutdown() noexcept {
            m_keepOpen = false;
        }
        /** Shuts down the threader, forcing threads to close. */
        inline void shutdown() {
            m_alive = false;
            m_keepOpen = false;
            for (auto& thread : m_threads)
                if (thread.joinable())
                    thread.join();
            while (m_threadsActive > 0ull)
                continue;
            m_threads.clear();
        }
        /** Deleted copy-assignment operator. */
        Threader& operator=(const Threader& other) = delete;
        /** Deleted move-assignment operator. */
        Threader& operator=(Threader&& other) = delete;


    private:
        // Private Attributes
        std::shared_mutex m_mutex;
        std::atomic_bool m_alive = true, m_keepOpen = true;
        std::vector<std::thread> m_threads;
        std::deque<std::function<void()>> m_jobs;
        std::atomic_size_t m_threadsActive = 0ull, m_jobsStarted = 0ull, m_jobsFinished = 0ull;
        size_t m_maxThreads = 0ull;
    };
};

#endif // THREADER_H