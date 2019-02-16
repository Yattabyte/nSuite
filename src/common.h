#include <deque>
#include <direct.h>
#include <thread>
#include <shared_mutex>
#include <memory>
#include "Windows.h"


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

class Archive {
public:
	// Public constructor
	Archive(int resource_id, const std::string &resource_class) {
		hResource = FindResourceA(nullptr, MAKEINTRESOURCEA(resource_id), resource_class.c_str());
		hMemory = LoadResource(nullptr, hResource);

		m_size = SizeofResource(nullptr, hResource);
		m_ptr = LockResource(hMemory);
	}

	size_t m_size = 0;
	void * m_ptr = nullptr;


private:
	HRSRC hResource = nullptr;
	HGLOBAL hMemory = nullptr;
};

/** Get the current directory for this executable. */
static std::string get_current_dir()
{
	char cCurrentPath[FILENAME_MAX];
	if (_getcwd(cCurrentPath, sizeof(cCurrentPath)))
		cCurrentPath[sizeof(cCurrentPath) - 1l] = char('/0');
	return std::string(cCurrentPath);
}

/** Return info for all files within this working directory. */
static std::vector<std::filesystem::directory_entry> get_file_paths()
{
	std::vector<std::filesystem::directory_entry> paths;
	for (const auto & entry : std::filesystem::recursive_directory_iterator(get_current_dir()))
		if (entry.is_regular_file())
			paths.push_back(entry);
	return paths;
}