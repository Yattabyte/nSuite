#pragma once
#ifndef YATTA_THREADER_H
#define YATTA_THREADER_H

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
        /** Destroys this threader and shut down all its threads. */
        ~Threader();
        /** Creates a threader and generates a specified number of worker threads.
        @param  maxThreads       the number of threads to spawn (max std::thread::hardware_concurrency). */
        explicit Threader(const size_t& maxThreads = std::thread::hardware_concurrency()) noexcept;
        /** Deleted copy-assignment constructor. */
        Threader(const Threader&) = delete;
        /** Deleted move-assignment constructor. */
        Threader(Threader&&) = delete;


        // Public Assignment Operators
        /** Deleted copy-assignment operator. */
        Threader& operator=(const Threader& other) = delete;
        /** Deleted move-assignment operator. */
        Threader& operator=(Threader&& other) = delete;


        // Public Methods
        /** Adds the specified function object to the queue.
        @param  func            the task to be executed on a separate thread. */
        void addJob(const std::function<void()>&& func);
        /** Check if the threader has completed all its jobs.
        @return                 true if finished, false otherwise. */
        bool isFinished() const noexcept;
        /** Shuts down the threader, forcing threads to close. */
        void shutdown();


    private:
        // Private Attributes
        std::shared_mutex m_mutex;
        std::atomic_bool m_alive = true;
        std::vector<std::thread> m_threads;
        std::deque<std::function<void()>> m_jobs;
        std::atomic_size_t m_jobsStarted = 0ull, m_jobsFinished = 0ull;
        size_t m_maxThreads = 0ull;
    };
};

#endif // YATTA_THREADER_H