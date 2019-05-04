#pragma once
#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "Buffer.h"
#include <filesystem>
#include <string>
#include <vector>


/** Add directory-related classes to the nSuite namespace. */
namespace NST {
	/***/
	class Directory {
	public:
		// Public (de)Constructors
		/***/
		~Directory();
		/**
		@param	directoryPath		the absolute path to a desired directory, or (embedded) package file.
		@param	exclusions			(optional) list of filenames/types to skip. "string" match relative path, ".ext" match extension.*/
		Directory(const std::string & directoryPath = std::string(""), const std::vector<std::string> & exclusions = std::vector<std::string>());
		

		// Public Methods
		/***/
		void changeDirectory(const std::string & directoryPath);
		/***/
		size_t file_count() const;
		/***/
		size_t space_used() const;
		
		
		// Public Derivation Methods
		/** Compresses all disk contents found within this directory, into an .npack - package formatted buffer.		
		Buffer format:
		-----------------------------------------------------------------------------------------------------
		| header: identifier title, package name size, package name  | remaining compressed directory data  |
		-----------------------------------------------------------------------------------------------------	
		@return						a buffer pointer, containing the compressed directory contents on package success, empty otherwise. */
		std::optional<Buffer> package();
		/***/
		bool unpackage(Directory & destinationDirectory);


		// Public Header Structs
		struct PackageHeader : NST::Buffer::Header {
			// Attributes
			static constexpr const char TITLE[] = "nSuite package";
			size_t m_charCount = 0ull;
			std::string m_folderName = "";


			// (de)Constructors
			PackageHeader() = default;
			PackageHeader(const size_t & folderNameSize, const char * folderName) : Header(TITLE), m_charCount(folderNameSize) {
				m_folderName = std::string(folderName, folderNameSize);
			}


			// Interface Implementation
			virtual size_t size() const override {
				return size_t(sizeof(size_t) + (sizeof(char) * m_charCount));
			}
			virtual std::byte * operator << (std::byte * ptr) override {
				ptr = Header::operator<<(ptr);
				std::copy(ptr, &ptr[size_t(sizeof(size_t))], reinterpret_cast<std::byte*>(&m_charCount));
				ptr = &ptr[size_t(sizeof(size_t))];
				char * folderArray = new char[m_charCount];
				std::copy(ptr, &ptr[size_t(sizeof(char) * m_charCount)], reinterpret_cast<std::byte*>(&folderArray[0]));
				m_folderName = std::string(folderArray, m_charCount);
				delete[] folderArray;
				return &ptr[size_t(sizeof(char) * m_charCount)];
			}
			virtual std::byte *operator >> (std::byte * ptr) const override {
				ptr = Header::operator>>(ptr);
				*reinterpret_cast<size_t*>(ptr) = m_charCount;
				ptr = &ptr[size_t(sizeof(size_t))];
				std::copy(m_folderName.begin(), m_folderName.end(), reinterpret_cast<char*>(ptr));
				return &ptr[size_t(sizeof(char) * m_charCount)];
			}
		};
		/** Holds and performs Patch I/O operations on buffers. */
		struct PatchHeader : NST::Buffer::Header {
			// Attributes
			static constexpr const char TITLE[] = "nSuite patch";
			size_t m_fileCount = 0ull;


			// (de)Constructors
			PatchHeader(const size_t size = 0ull) : Header(TITLE), m_fileCount(size) {}


			// Interface Implementation
			virtual size_t size() const override {
				return size_t(sizeof(size_t));
			}
			virtual std::byte * operator << (std::byte * ptr) override {
				ptr = Header::operator<<(ptr);
				std::copy(ptr, &ptr[size()], reinterpret_cast<std::byte*>(&m_fileCount));
				return &ptr[size()];
			}
			virtual std::byte *operator >> (std::byte * ptr) const override {
				ptr = Header::operator>>(ptr);
				*reinterpret_cast<size_t*>(ptr) = m_fileCount;
				return &ptr[size()];
			}
		};


	private:
		// Public Data Structs
		struct DirFile {
			std::string fullpath = "", trunc_path = "";
			size_t size = 0ull, unitSize = 0ull;
			std::byte * data = nullptr;
		};


		// Private Methods/
		/***/
		void parse();
		/***/
		void loadIntoMemory();
		/***/
		void unloadFromMemory();


		// Private Attributes
		std::string m_directoryPath = "";
		std::vector<std::string> m_exclusions;
		std::vector<DirFile> m_files;
	};
};

#endif // DIRECTORY_H