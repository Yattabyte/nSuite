#include "Windows.h"
#include <string>

// Used by compressor.rc
#define IDR_INSTALLER                     101

// Used by decompressor.rc
#define IDR_ARCHIVE                       102

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        103
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif


/** Utility class encapsulating resource files embedded in a application. */
class Resource{
public:
	~Resource() {
		UnlockResource(m_hMemory);
		FreeResource(m_hMemory);
	}
	Resource(const int & resourceID, const std::string & resourceClass) {
		m_hResource = FindResource(nullptr, MAKEINTRESOURCE(resourceID), resourceClass.c_str());
		m_hMemory = LoadResource(nullptr, m_hResource);
		m_ptr = LockResource(m_hMemory);
		m_size = SizeofResource(nullptr, m_hResource);
	}
	void * getPtr() const {
		return m_ptr;
	}
	size_t getSize() const {
		return m_size;
	}


private:
	// Private Attributes
	HRSRC m_hResource = nullptr;
	HGLOBAL m_hMemory = nullptr;
	void * m_ptr = nullptr;
	size_t m_size = 0;
};