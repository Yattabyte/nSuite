#pragma once
#ifndef TASK_LOGGER_H
#define TASK_LOGGER_H

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
	/** Add a callback method which will be triggered when the log receives new text.
	@param	func		the callback function. (only arg is a string)
	@param	pullOld		if set to true, will immediately call the function, dumping all current log data into it. (optional, defaults true)
	@return				index to callback in logger, used to remove callback. */
	inline static size_t AddCallback_TextAdded(std::function<void(const std::string &)> && func, const bool & pullOld = true) {
		auto & instance = GetInstance();

		// Dump old text if this is true
		if (pullOld)
			func(instance.PullText());

		auto index = instance.m_textCallbacks.size();
		instance.m_textCallbacks.emplace_back(std::move(func));
		return index;
	}
	/** Remove a callback method used for when text is added to the log.
	@param	index		the index for the callback within the logger. */
	inline static void RemoveCallback_TextAdded(const size_t & index) {
		auto & instance = GetInstance();
		instance.m_textCallbacks.erase(instance.m_textCallbacks.begin() + index);
	}
	/** Add a callback method which will be triggered when the task progress changes.
	@param	func		the callback function. (args: progress, range)aults true)
	@return				index to callback in logger, used to remove callback. */
	inline static size_t AddCallback_ProgressUpdated(std::function<void(const size_t &, const size_t & size_t)> && func) {
		auto & instance = GetInstance();
		auto index = instance.m_progressCallbacks.size();
		instance.m_progressCallbacks.emplace_back(std::move(func));
		return index;
	}
	/** Remove a callback method used for when the log progress changes.
	@param	index		the index for the callback within the logger. */
	inline static void RemoveCallback_ProgressUpdated(const size_t & index) {
		auto & instance = GetInstance();
		instance.m_progressCallbacks.erase(instance.m_progressCallbacks.begin() + index);
	}
	/** Push new text into the logger.
	@param	text		the new text to add. */
	inline static void PushText(const std::string & text) {
		auto & instance = GetInstance();

		// Add the message to the log
		instance.m_log += text;

		// Notify all observers that the log has been updated
		for each (const auto & callback in instance.m_textCallbacks)
			callback(text);
	}
	/** Retrieve all text from the logger.
	@return				the entire message log. */
	inline static std::string PullText() {
		return GetInstance().m_log;
	}
	/** Set the progress range for the logger. 
	@param	value		the upper range value. */
	inline static void SetRange(const size_t & value) {
		GetInstance().m_range = value;
	}
	/** Get the progress range for the logger.
	@return				the upper range value. */
	inline static size_t GetRange() {
		return GetInstance().m_range;
	}
	/** Set the current progress value for the logger, <= the upper range.
	@param	amount		the progress value to use. */
	inline static void SetProgress(const size_t & amount) {
		auto & instance = GetInstance();
		instance.m_pos = amount > instance.m_range ? instance.m_range : amount;

		// Notify all observers that the task has updated
		for each (const auto & callback in instance.m_progressCallbacks)
			callback(instance.m_pos, instance.m_range);
	}


private:
	// Private (de)constructors
	inline ~TaskLogger() = default;
	inline TaskLogger() = default;
	inline TaskLogger(TaskLogger const&) = delete;
	inline void operator=(TaskLogger const&) = delete;
	inline static TaskLogger & GetInstance() {
		static TaskLogger instance;
		return instance;
	}
	

	// Private Attributes
	std::string m_log;
	size_t m_range = 0ull, m_pos = 0ull;
	std::vector<std::function<void(const std::string &)>> m_textCallbacks;
	std::vector<std::function<void(const size_t &, const size_t & size_t)>> m_progressCallbacks;
};

#endif // TASK_LOGGER_H