#include "Archive.h"
#include "lz4.h"


Archive::~Archive()
{
	if (m_decompressedPtr)
		delete[] m_decompressedPtr;
}

Archive::Archive(int resource_id, const std::string & resource_class)
{
	hResource = FindResourceA(nullptr, MAKEINTRESOURCEA(resource_id), resource_class.c_str());
	hMemory = LoadResource(nullptr, hResource);

	m_ptr = LockResource(hMemory);
	m_compressedSize = SizeofResource(nullptr, hResource);
	m_uncompressedSize = *reinterpret_cast<size_t*>(m_ptr);
}

size_t Archive::getCompressedSize() const
{
	return m_compressedSize;
}

size_t Archive::getUncompressedSize() const
{
	return m_uncompressedSize;
}

bool Archive::uncompressArchive(void ** ptr)
{
	m_decompressedPtr = new char[m_uncompressedSize];
	*ptr = m_decompressedPtr;
	auto result = LZ4_decompress_safe(
		reinterpret_cast<char*>(reinterpret_cast<unsigned char*>(m_ptr) + size_t(sizeof(size_t))),
		reinterpret_cast<char*>(m_decompressedPtr),
		int(m_compressedSize - size_t(sizeof(size_t))),
		int(m_uncompressedSize)
	);

	return (result > 0);
}
