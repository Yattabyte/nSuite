#pragma once
#ifndef LOGGER_H
#define LOGGER_H

#include <functional>
#include <string>
#include <vector>

/*********** SINGLETON ***********/
/*								 */		
/*  Performs logging operations  */
/*								 */		
/*********** SINGLETON ***********/
class TaskLogger {
public:
	// Public Methods
	inline static TaskLogger & GetInstance() {
		static TaskLogger instance;
		return instance;
	}
	inline size_t addCallback_Text(std::function<void(const std::string &)> && func) {
		auto index = m_textCallbacks.size();
		m_textCallbacks.emplace_back(std::move(func));
		return index;
	}inline size_t addCallback_Progress(std::function<void(const size_t &, const size_t & size_t)> && func) {
		auto index = m_progressCallbacks.size();
		m_progressCallbacks.emplace_back(std::move(func));
		return index;
	}
	inline void removeCallback_Text(const size_t & index) {
		m_textCallbacks.erase(m_textCallbacks.begin() + index);
	}
	inline void removeCallback_Progress(const size_t & index) {
		m_progressCallbacks.erase(m_progressCallbacks.begin() + index);
	}
	inline TaskLogger& operator <<(const std::string & message) {
		// Add the message to the log
		m_log += message;

		// Notify all observers that the log has been updated
		for each (const auto & callback in m_textCallbacks)
			callback(message);

		// Returning a reference so we can chain values like
		// log << value << value << value
		return *this;
	}
	inline std::string getLog() const {
		return m_log;
	}
	inline void setRange(const size_t & value) {
		m_range = value;
	}
	inline size_t getRange() const {
		return m_range;
	}
	inline void setProgress(const size_t & amount) {
		m_pos = amount;

		// Notify all observers that the task has updated
		for each (const auto & callback in m_progressCallbacks)
			callback(m_pos, m_range);
	}


private:
	// Private (de)constructors
	~TaskLogger() = default;
	TaskLogger() = default;
	TaskLogger(TaskLogger const&) = delete;
	void operator=(TaskLogger const&) = delete;


	// Private Attributes
	std::string m_log;
	size_t m_range = 0ull, m_pos = 0ull;
	std::vector<std::function<void(const std::string &)>> m_textCallbacks;
	std::vector<std::function<void(const size_t &, const size_t & size_t)>> m_progressCallbacks;
};

#endif // LOGGER_H