#pragma once
#ifndef STRING_CONVERSIONS_H
#define STRING_CONVERSIONS_H

#include <algorithm>
#include <shlobj.h>
#include <string>


/** Add string methods to nSuite NST namespace. */
namespace NST {
	/** Changes an input string to lower case, returning it.
	@param	string		the input string.
	@return				lower case version of the string. */
	inline static std::string string_to_lower(const std::string & string)
	{
		std::string input = string;
		std::transform(input.begin(), input.end(), input.begin(), [](const int & character) { return static_cast<char>(::tolower(character)); });
		return input;
	}
	/** Changes an input string to upper case, returning it.
	@param	string		the input string.
	@return				lower case version of the string. */
	inline static std::string string_to_upper(const std::string & string)
	{
		std::string input = string;
		std::transform(input.begin(), input.end(), input.begin(), [](const int & character) { return static_cast<char>(::toupper(character)); });
		return input;
	}
	/** Converts the input string into a wide string.
	@param	str			the input string.
	@return				wide string version of the string. */
	inline static std::wstring to_wideString(const std::string & str)
	{
		if (!str.empty()) {
			if (const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0)) {
				std::wstring wstr(size_needed, 0);
				MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);
				return wstr;
			}
		}
		return std::wstring();
	}
	/** Converts the input wide string into a string.
	@param	str			the input wide string.
	@return				string version of the wide string. */
	inline static std::string from_wideString(const std::wstring & wstr)
	{
		if (!wstr.empty()) {
			if (const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL)) {
				std::string str(size_needed, 0);
				WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
				return str;
			}
		}
		return std::string();
	}
};

#endif // STRING_CONVERSIONS_H