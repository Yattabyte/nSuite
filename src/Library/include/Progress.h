#pragma once
#ifndef PROGRESS_H
#define PROGRESS_H

#include <functional>
#include <string>
#include <vector>


namespace NST {
	/** A Singleton progress tracker, used for globally manipulating and viewing the progress of a task.
	Can accept observers to listen in on when the progress changes.
	Hides singleton pattern from user, exposes functionality through static methods. */
	class Progress {
	public:
		// Public Static Methods
		/** Add an observer method which will be triggered when the task progress changes.
		@param	func		the callback function. (params: progress, range)
		@return				handle for this observer, used to remove it. */
		inline static size_t AddObserver(std::function<void(const size_t&, const size_t & size_t)>&& func) {
			auto& instance = GetInstance();
			auto index = instance.m_observers.size();
			instance.m_observers.emplace_back(std::move(func));
			return index;
		}
		/** Remove an observer method used for when the log progress changes.
		@param	index		the handle given for the observer, when added. */
		inline static void RemoveObserver(const size_t& index) {
			auto& instance = GetInstance();
			instance.m_observers.erase(instance.m_observers.begin() + index);
		}
		/** Set the progress upper range.
		@param	value		the upper range value. */
		inline static void SetRange(const size_t& value) {
			GetInstance().m_range = value;
		}
		/** Get the progress upper range.
		@return				the upper range value. */
		inline static size_t GetRange() {
			return GetInstance().m_range;
		}
		/** Set the current progress value, clamped to the upper range.
		@param	amount		the progress value to use. */
		inline static void SetProgress(const size_t& amount) {
			auto& instance = GetInstance();
			instance.m_pos = amount > instance.m_range ? instance.m_range : amount;

			// Notify all observers that the task has updated
			for each (const auto & callback in instance.m_observers)
				callback(instance.m_pos, instance.m_range);
		}
		/** Increment the progress by 1. */
		inline static void IncrementProgress() {
			auto& instance = GetInstance();
			instance.SetProgress(instance.m_pos + 1ull);
		}


	private:
		// Private (de)constructors, assignment operators, accessors
		inline ~Progress() = default;
		inline Progress() = default;
		inline Progress(Progress const&) = delete;
		inline void operator=(Progress const&) = delete;
		inline static Progress& GetInstance() {
			static Progress instance;
			return instance;
		}


		// Private Attributes
		size_t m_range = 0ull, m_pos = 0ull;
		std::vector<std::function<void(const size_t&, const size_t & size_t)>> m_observers;
	};
};
#endif // PROGRESS_H