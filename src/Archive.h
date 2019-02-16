#include <string>
#include "Windows.h"


class Archive {
public:
	// Public Constructor
	~Archive();
	Archive(int resource_id, const std::string &resource_class);


	// Public Methods
	/** Return the compressed size of the archive file. */
	size_t getCompressedSize() const;
	/** Return the uncompressed size of the content within the archive file. */
	size_t getUncompressedSize() const;
	/***/
	bool uncompressArchive(void ** ptr);


	// Public Attributes
	size_t m_compressedSize = 0, m_uncompressedSize = 0;
	void * m_ptr = nullptr, * m_decompressedPtr = nullptr;


private:
	// Private Attributes
	HRSRC hResource = nullptr;
	HGLOBAL hMemory = nullptr;
};