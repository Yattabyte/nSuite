#include <string>
#include <memory>
#include "Windows.h"



class Archive {
public:
	// Public Constructor
	~Archive();
	Archive(int resource_id, const std::string & resource_class);


	// Public Methods
	/** Pack the archive directory into installer, overwriting its contents. */
	static bool pack(size_t & fileCount, size_t & byteCount);
	/** Unpack the installer's contents, overwriting the archive directory. */
	bool unpack(size_t & fileCount, size_t & byteCount);


	// Public Attributes
	size_t m_size = 0, m_decompressedSize = 0;
	void * m_ptr = nullptr, * m_decompressedPtr = nullptr;


private:
	// Private Methods
	/** Decompress the archive .*/
	bool decompressArchive();


	// Private Attributes
	HRSRC m_hResource = nullptr;
	HGLOBAL m_hMemory = nullptr;
};