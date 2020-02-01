#include "threader.hpp"


// Convenience Definitions
using yatta::Threader;


// Public (de)constructors

Threader::~Threader()
{
    shutdown();
}

Threader::Threader(const size_t& maxThreads)
{
    m_maxThreads = std::clamp<size_t>(maxThreads, 1ULL, static_cast<size_t>(std::thread::hardware_concurrency()));
    m_threads.resize(m_maxThreads);
    for (auto& thread : m_threads) {
        thread = std::thread(
            [&]() {
                while (m_alive) {
                    // Check if there is a job to do
                    if (std::unique_lock<std::shared_mutex> writeGuard(m_mutex, std::try_to_lock); writeGuard.owns_lock()) {
                        if (!m_jobs.empty()) {
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
            }
        );
    }
}


// Public Methods

void Threader::addJob(const std::function<void()>&& func)
{
    std::unique_lock<std::shared_mutex> writeGuard(m_mutex);
    m_jobs.emplace_back(func);
    m_jobsStarted++;
}

bool Threader::isFinished() const noexcept
{
    return m_jobsStarted == m_jobsFinished;
}

void Threader::shutdown()
{
    m_alive = false;
    for (auto& thread : m_threads)
        if (thread.joinable())
            thread.join();
    m_threads.clear();
}