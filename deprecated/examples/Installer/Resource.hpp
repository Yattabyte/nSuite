#pragma once
#ifndef RESOURCE_H
#define RESOURCE_H

#define YATTA_VERSION_CSTR				"1.4.5"
#define YATTA_VERSION_VS				1,4,5					

// Used for icons
#define IDI_ICON1						101
// Used by InstallerMaker.rc
#define IDR_InstallER					102
#define IDR_UNPACKER					103
// Used by unInstaller.rc
#define IDR_ARCHIVE						104
#define IDR_MANIFEST					105
#define IDR_UNInstallER					106
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        107
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

#include "Windows.h"


/** Add Resource to yatta namespace. */
namespace yatta {
	/** Utility class encapsulating resource files embedded in a application. */
	class Resource {
	public:
		// Public (de)constructors
		/** Destroys this resource object, unlocking it for future use. */
		inline ~Resource() {
			UnlockResource(m_hMemory);
			FreeResource(m_hMemory);
		}
		/** Creates a resource object, locking it, and gets a pointer to the underlying data. */
		inline Resource(const int & resourceID, const char * resourceClass, const HMODULE & moduleHandle = nullptr) {
			m_hResource = FindResource(moduleHandle, MAKEINTRESOURCE(resourceID), resourceClass);
			if (m_hResource != 0)
				m_hMemory = LoadResource(moduleHandle, m_hResource);
			if (m_hMemory != 0)
				m_ptr = LockResource(m_hMemory);
			if (m_hResource != 0)
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
		/** Check if this resource has successfully completed initialization and has at least 1 byte of data.
		@return		true if this resource exists, false otherwise. */
		inline bool exists() const {
			return (m_hResource && m_hMemory && m_ptr) && (m_size > 0ull);
		}


	private:
		// Private Attributes
		HRSRC m_hResource = nullptr;
		HGLOBAL m_hMemory = nullptr;
		void * m_ptr = nullptr;
		size_t m_size = 0;
	};
};

#endif // RESOURCE_H
// SPECIFIC TO THIS ONE FILE, KEEP 1 EXTRA LINE AFTER, BUG WITH VS RESOURCE COMPILER
