#pragma once
#ifndef RESOURCE_H
#define RESOURCE_H

// Used by installer.rc
#define IDR_ARCHIVE                       101
// Used by installerMaker.rc
#define IDR_INSTALLER                     102

// Next default values for new objects
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        103
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

#include "Windows.h"
#include <string>


/** Utility class encapsulating resource files embedded in a application. */
class Resource {
public:
	// Public (de)constructors
	/** Destroys this resource object, unlocking it for future use. */
	~Resource() {
		UnlockResource(m_hMemory);
		FreeResource(m_hMemory);
	}
	/** Creates a resource object, locking it, and gets a pointer to the underlying data. */
	Resource(const int & resourceID, const std::string & resourceClass, const HMODULE & moduleHandle = nullptr) {
		m_hResource = FindResource(moduleHandle, MAKEINTRESOURCE(resourceID), resourceClass.c_str());
		m_hMemory = LoadResource(moduleHandle, m_hResource);
		m_ptr = LockResource(m_hMemory);
		m_size = SizeofResource(moduleHandle, m_hResource);
	}


	// Public Methods
	/** Retrieve a pointer to this resources' data.
	@return		a pointer to the beginning of this resources' data. */
	inline void * getPtr() const {
		return m_ptr;
	}
	/** Retrieve the size of this resource in bytes.
	@return		the size of this resources' data in bytes. */
	inline size_t getSize() const {
		return m_size;
	}


private:
	// Private Attributes
	HRSRC m_hResource = nullptr;
	HGLOBAL m_hMemory = nullptr;
	void * m_ptr = nullptr;
	size_t m_size = 0;
};

#endif // RESOURCE_H
// SPECIFIC TO THIS ONE FILE, KEEP 1 EXTRA LINE AFTER, BUG WITH VS RESOURCE COMPILER
