#pragma once
#ifndef LOG_H
#define LOG_H

#include <functional>
#include <string>
#include <vector>


/** Add Log to nSuite NST namespace. */
namespace NST {
	/** A Singleton text log which most of the library uses.
	Accepts observers to listen in on newly logged text. */
	class Log {
	public:
		// Public Methods	
		/** Add an observer method to be called when new text is pushed into the log.
		@param	func		the observer function. (only parameter is a string)
		@param	pullOld		(optional) if true, will immediately call the function with the entire log's contents to this point
		@return				handle for this observer, used to remove it. */
		inline static size_t AddObserver(const std::function<void(const std::string &)> && func, const bool & pullOld = true) {
			auto & instance = GetInstance();

			// Dump old text if this is true
			if (pullOld)
				func(instance.PullText());

			auto index = instance.m_observers.size();
			instance.m_observers.emplace_back(std::move(func));
			return index;
		}
		/** Remove an observer method used for when text is added to the log.
		@param	index		the handle given for the observer, when added. */
		inline static void RemoveObserver(const size_t & index) {
			auto & instance = GetInstance();
			instance.m_observers.erase(instance.m_observers.begin() + index);
		}
		/** Push new text into the log.
		@param	text		the new text to add. */
		inline static void PushText(const std::string & text) {
			auto & instance = GetInstance();

			// Add the message to the log
			instance.m_log += text;

			// Notify all observers that the log has been updated
			for each (const auto & callback in instance.m_observers)
				callback(text);
		}
		/** Retrieve all text from the log.
		@return				the entire message log. */
		inline static std::string PullText() {
			return GetInstance().m_log;
		}


	private:
		// Private (de)constructors, assignment operators, accessors
		inline ~Log() = default;
		inline Log() = default;
		inline Log(Log const&) = delete;
		inline void operator=(Log const&) = delete;
		inline static Log & GetInstance() {
			static Log instance;
			return instance;
		}


		// Private Attributes
		std::string m_log;
		std::vector<std::function<void(const std::string &)>> m_observers;
	};
};

#endif // LOG_H