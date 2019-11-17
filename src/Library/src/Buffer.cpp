#include "Buffer.h"
#include "Threader.h"
#include "lz4.h"
#include <algorithm>

using namespace NST;


// Public (de)Constructors

Buffer::~Buffer()
{
	release();
}

Buffer::Buffer(const size_t & size) 
	: m_ownsData(true)
{
	resize(size);
}

Buffer::Buffer(std::byte * pointer, const size_t & range, bool hardCopy)
	: m_ownsData(hardCopy)
{
	if (hardCopy) {
		resize(range);
		std::copy(pointer, pointer + range, m_data);
	}
	else {
		m_data = pointer;
		m_size = range;
		m_capacity = range;
	}
}

Buffer::Buffer(const Buffer & other)
	: m_size(other.m_size), m_capacity(other.m_capacity), m_ownsData(true)
{
	alloc(other.m_capacity);
	std::copy(other.m_data, other.m_data + other.m_size, m_data);
}

Buffer::Buffer(Buffer && other)
{
	m_data = other.m_data;
	m_size = other.m_size;
	m_capacity = other.m_capacity;
	m_ownsData = other.m_ownsData;

	other.m_data = nullptr;
	other.m_size = 0ull;
	other.m_capacity = 0ull;
	other.m_ownsData = false;
}


// Public Operators

Buffer & Buffer::operator=(const Buffer & other)
{
	if (this != &other) {
		release();
		alloc(other.m_capacity);
		m_size = other.m_size;
		m_ownsData = other.m_ownsData;
		std::copy(other.m_data, other.m_data + other.m_size, m_data);
	}
	return *this;
}

Buffer & Buffer::operator=(Buffer && other)
{
	if (this != &other) {
		release();
		m_data = other.m_data;
		m_size = other.m_size;
		m_capacity = other.m_capacity;
		m_ownsData = other.m_ownsData;

		other.m_data = nullptr;
		other.m_size = 0ull;
		other.m_capacity = 0ull;
		other.m_ownsData = false;
	}
	return *this;
}


// Private Methods

void Buffer::alloc(const size_t & capacity)
{
	m_capacity = capacity;
	m_data = new std::byte[m_capacity];
}


// Public Methods

const bool Buffer::hasData() const
{
	return m_size > 0ull;
}

const size_t &Buffer::size() const
{
	return m_size;
}

const size_t Buffer::hash() const
{
	// Use data 8-bytes at a time, until end of data or less than 8 bytes remains
	size_t value(1234567890ull);
	auto pointer = reinterpret_cast<size_t*>(const_cast<std::byte*>(&m_data[0]));
	size_t x(0ull), full(size()), max(full / 8ull);
	for (; x < max; ++x)
		value = ((value << 5) + value) + pointer[x]; // use 8 bytes

	// If any bytes remain, switch technique to work byte-wise instead of 8-byte-wise
	x *= 8ull;
	auto remainderPtr = reinterpret_cast<char*>(const_cast<std::byte*>(&m_data[0]));
	for (; x < full; ++x)
		value = ((value << 5) + value) + remainderPtr[x]; // use remaining bytes

	return value;
}


void Buffer::resize(const size_t & size)
{
	// Ensure this is a valid buffer
	if (m_data != nullptr && m_size > 0ull && m_capacity > 0ull) {
		// Check if we've previously allocated enough memory
		if (size > m_capacity) {
			// Copy old pointer
			auto dataPtrCpy = m_data;
			// Need to expand our container
			alloc(size * 2ull);
			// Copy over old memory
			std::copy(dataPtrCpy, dataPtrCpy + m_size, m_data);
			// Delete old memory
			if (m_ownsData)
				delete[] dataPtrCpy;
			else
				m_ownsData = true;
		}
	}
	// Otherwise allocate new memory
	else
		alloc(size * 2ull);
	m_size = size;
}

void Buffer::release()
{
	if (m_data != nullptr && m_size > 0ull && m_ownsData)
		delete[] m_data;

	m_data = nullptr;
	m_size = 0ull;
	m_capacity = 0ull;
}


// Public Mutable Data Accessors

char * Buffer::cArray() const
{
	return reinterpret_cast<char*>(&m_data[0]);
}

std::byte * Buffer::data() const
{
	return m_data;
}

std::byte & Buffer::operator[](const size_t & byteOffset) const
{
	return m_data[byteOffset];
}

size_t Buffer::readData(void * outputPtr, const size_t & size, const size_t byteOffset) const
{
	if (outputPtr != nullptr && size > 0ull)
		std::copy(&m_data[byteOffset], &m_data[byteOffset + size], reinterpret_cast<std::byte*>(outputPtr));
	return byteOffset + size;
}

size_t Buffer::writeData(const void * inputPtr, const size_t & size, const size_t byteOffset)
{
	auto * ptrCast = reinterpret_cast<std::byte*>(const_cast<void*>(inputPtr));
	if (inputPtr != nullptr && size > 0ull)
		std::copy(ptrCast, ptrCast + size, &m_data[byteOffset]);
	return byteOffset + size;
}


// Public Derivation Methods

std::optional<Buffer> Buffer::compress() const
{
	// Ensure this buffer has some data to compress
	if (hasData()) {
		// Pre-allocate a huge buffer to allow for compression operations
		const auto sourceSize = size();
		const auto workingSize = sourceSize * 2ull;
		Buffer compressedBuffer(workingSize);

		// Try to compress the source buffer
		const auto compressedSize = LZ4_compress_default(
			this->cArray(),
			compressedBuffer.cArray(),
			int(sourceSize),
			int(workingSize)
		);

		// Ensure we have a non-zero sized buffer
		if (compressedSize > 0) {
			// We now know the actual compressed size, downsize our oversized buffer to the compressed size
			compressedBuffer.resize(compressedSize);

			// Prepend header information
			CompressionHeader header(sourceSize);
			compressedBuffer.writeHeader(&header);

			// Success
			return compressedBuffer;
		}
	}

	// Failure
	return {};
}

std::optional<Buffer> Buffer::decompress() const
{
	// Ensure buffer at least *exists*
	if (hasData()) {
		// Read in header
		CompressionHeader header;		
		std::byte * dataPtr(nullptr);
		size_t dataSize(0ull);
		readHeader(&header, &dataPtr, dataSize);

		// Ensure header title matches
		if (header.isValid()) {
			// Uncompress the remaining data
			Buffer uncompressedBuffer(header.m_uncompressedSize);
			const auto decompressionResult = LZ4_decompress_safe(
				reinterpret_cast<char*>(dataPtr),
				uncompressedBuffer.cArray(),
				int(dataSize),
				int(uncompressedBuffer.size())
			);

			// Ensure we have a non-zero sized buffer
			if (decompressionResult > 0) {
				// Success
				return uncompressedBuffer;
			}
		}
	}

	// Failure
	return {};
}

/** Specifies a region in the 'old' file to read from, and where to put it in the 'new' file. */
struct Copy_Instruction : public Buffer::Differential_Instruction {
	// Constructor 
	Copy_Instruction() : Differential_Instruction('C') {}


	// Interface Implementation
	virtual size_t size() const override;
	virtual void execute(NST::Buffer & bufferNew, const NST::Buffer & bufferOld) const override;
	virtual void write(NST::Buffer & outputBuffer, size_t & byteIndex) const override;
	virtual void read(const NST::Buffer & inputBuffer, size_t & byteIndex) override;


	// Public Attributes
	size_t m_beginRead = 0ull, m_endRead = 0ull;
};
/** Contains a block of data to insert into the 'new' file, at a given point. */
struct Insert_Instruction : public Buffer::Differential_Instruction {
	// Constructor 
	Insert_Instruction() : Differential_Instruction('I') {}


	// Interface Implementation
	virtual size_t size() const override;
	virtual void execute(NST::Buffer & bufferNew, const NST::Buffer & bufferOld) const override;
	virtual void write(NST::Buffer & outputBuffer, size_t & byteIndex) const override;
	virtual void read(const NST::Buffer & inputBuffer, size_t & byteIndex) override;


	// Attributes
	std::vector<std::byte> m_newData;
};
/** Contains a single value a to insert into the 'new' file, at a given point, repeating multiple times. */
struct Repeat_Instruction : public Buffer::Differential_Instruction {
	// Constructor 
	Repeat_Instruction() : Differential_Instruction('R') {}


	// Interface Implementation
	virtual size_t size() const override;
	virtual void execute(NST::Buffer & bufferNew, const NST::Buffer & bufferOld) const override;
	virtual void write(NST::Buffer & outputBuffer, size_t & byteIndex) const override;
	virtual void read(const NST::Buffer & inputBuffer, size_t & byteIndex) override;


	// Attributes
	size_t m_amount = 0ull;
	std::byte m_value;
};

std::optional<Buffer> Buffer::diff(const Buffer & target) const
{
	// Ensure that at least ONE of the two source buffers exists
	if (hasData() || target.hasData()) {
		const auto size_old = size();
		const auto size_new = target.size();
		std::vector<Differential_Instruction*> instructions;
		instructions.reserve(std::max<size_t>(size_old, size_new) / 8ull);
		std::mutex instructionMutex;
		Threader threader;
		constexpr size_t m_amount(4096);
		size_t bytesUsed_old(0ull), bytesUsed_new(0ull);
		while (bytesUsed_old < size_old && bytesUsed_new < size_new) {
			// Variables for this current chunk
			auto windowSize = std::min<size_t>(m_amount, std::min<size_t>(size_old - bytesUsed_old, size_new - bytesUsed_new));

			// Find best matches for this chunk
			threader.addJob([&, windowSize, bytesUsed_old, bytesUsed_new]() {
				// Step 1: Find all regions that match
				struct MatchInfo { size_t length = 0ull, start1 = 0ull, start2 = 0ull; };
				size_t bestContinuous(0ull), bestMatchCount(windowSize);
				std::vector<MatchInfo> bestSeries;
				auto buffer_slice_old = &(operator[](bytesUsed_old));
				auto buffer_slice_new = &target[bytesUsed_new];
				for (size_t index1 = 0ull; index1 + 8ull < windowSize; index1 += 8ull) {
					const size_t OLD_FIRST8 = *reinterpret_cast<size_t*>(&buffer_slice_old[index1]);
					std::vector<MatchInfo> matches;
					size_t largestContinuous(0ull);
					for (size_t index2 = 0ull; index2 + 8ull < windowSize; index2 += 8ull) {
						const size_t NEW_FIRST8 = *reinterpret_cast<size_t*>(&buffer_slice_new[index2]);
						// Check if values match						
						if (OLD_FIRST8 == NEW_FIRST8) {
							size_t offset = 8ull;
							// Restrict matches to be at least 32 bytes (minimum byte size of 1 instruction)
							if (buffer_slice_old[index1 + 32ull] == buffer_slice_new[index2 + 32ull]) {
								// Check how long this series of matches continues for
								for (; ((index1 + offset) < windowSize) && ((index2 + offset) < windowSize); ++offset)
									if (buffer_slice_old[index1 + offset] != buffer_slice_new[index2 + offset])
										break;

								// Save series
								if (offset >= 32ull) {
									matches.emplace_back(MatchInfo{ offset, bytesUsed_old + index1, bytesUsed_new + index2 });
									if (offset > largestContinuous)
										largestContinuous = offset;
								}
							}

							index2 += offset;
						}
					}
					// Check if the recently completed series saved the most continuous data
					if (largestContinuous > bestContinuous && matches.size() <= bestMatchCount) {
						// Save the series for later
						bestContinuous = largestContinuous;
						bestMatchCount = matches.size();
						bestSeries = matches;
					}

					// Break if the largest series is as big as the window
					if (bestContinuous >= windowSize)
						break;
				}

				// Step 2: Generate instructions based on the matching-regions found
				// Note: 
				//			data from [start1 -> +length] equals [start2 -> + length]
				//			data before and after these ranges isn't equal
				if (!bestSeries.size()) {
					// NO MATCHES
					// NEW INSERT_INSTRUCTION: use entire window
					Insert_Instruction * inst = new Insert_Instruction();
					inst->m_index = bytesUsed_new;
					inst->m_newData.resize(windowSize);
					std::copy(buffer_slice_new, buffer_slice_new + windowSize, inst->m_newData.data());
					std::unique_lock<std::mutex> writeGuard(instructionMutex);
					instructions.push_back(inst);
				}
				else {
					size_t lastMatchEnd(bytesUsed_new);
					for (size_t mx = 0, ms = bestSeries.size(); mx < ms; ++mx) {
						const auto & match = bestSeries[mx];

						const auto newDataLength = match.start2 - lastMatchEnd;
						if (newDataLength > 0ull) {
							// NEW INSERT_INSTRUCTION: Use data from end of last match until beginning of current match
							Insert_Instruction * inst = new Insert_Instruction();
							inst->m_index = lastMatchEnd;
							inst->m_newData.resize(newDataLength);
							std::copy(&target[lastMatchEnd], &target[lastMatchEnd + newDataLength], inst->m_newData.data());
							std::unique_lock<std::mutex> writeGuard(instructionMutex);
							instructions.push_back(inst);
						}

						// NEW COPY_INSTRUCTION: Use data from beginning of match until end of match
						Copy_Instruction * inst = new Copy_Instruction();
						inst->m_index = match.start2;
						inst->m_beginRead = match.start1;
						inst->m_endRead = match.start1 + match.length;
						lastMatchEnd = match.start2 + match.length;
						std::unique_lock<std::mutex> writeGuard(instructionMutex);
						instructions.push_back(inst);
					}

					const auto newDataLength = (bytesUsed_new + windowSize) - lastMatchEnd;
					if (newDataLength > 0ull) {
						// NEW INSERT_INSTRUCTION: Use data from end of last match until end of window
						Insert_Instruction * inst = new Insert_Instruction();
						inst->m_index = lastMatchEnd;
						inst->m_newData.resize(newDataLength);
						std::copy(&target[lastMatchEnd], &target[lastMatchEnd + newDataLength], inst->m_newData.data());
						std::unique_lock<std::mutex> writeGuard(instructionMutex);
						instructions.push_back(inst);
					}
				}
			});
			// increment
			bytesUsed_old += windowSize;
			bytesUsed_new += windowSize;
		}

		if (bytesUsed_new < size_new) {
			// NEW INSERT_INSTRUCTION: Use data from end of last block until end-of-file
			Insert_Instruction * inst = new Insert_Instruction();
			inst->m_index = bytesUsed_new;
			inst->m_newData.resize(size_new - bytesUsed_new);
			std::copy(&target[bytesUsed_new], &target[bytesUsed_new + (size_new - bytesUsed_new)], inst->m_newData.data());
			std::unique_lock<std::mutex> writeGuard(instructionMutex);
			instructions.push_back(inst);
		}

		// Wait for jobs to finish
		while (!threader.isFinished())
			continue;

		// Replace insertions with some repeat instructions
		for (size_t i = 0, mi = instructions.size(); i < mi; ++i) {
			threader.addJob([i, &instructions, &instructionMutex]() {
				auto instruction = instructions[i];
				if (auto inst = dynamic_cast<Insert_Instruction*>(instruction)) {
					// We only care about repeats larger than 36 bytes.
					if (inst->m_newData.size() > 36ull) {
						// Upper limit (mx and my) reduced by 36, since we only care about matches that exceed 36 bytes
						size_t max = std::min<size_t>(inst->m_newData.size(), inst->m_newData.size() - 37ull);
						for (size_t x = 0ull; x < max; ++x) {
							const auto & value_at_x = inst->m_newData[x];
							if (inst->m_newData[x + 36ull] != value_at_x)
								continue; // quit early if the value 36 units away isn't the same as this index

							size_t y = x + 1;
							while (y < max) {
								if (value_at_x == inst->m_newData[y])
									y++;
								else
									break;
							}

							const auto length = y - x;
							if (length > 36ull) {
								// Worthwhile to insert a new instruction
								// Keep data up-until region where repeats occur
								Insert_Instruction * instBefore = new Insert_Instruction();
								instBefore->m_index = inst->m_index;
								instBefore->m_newData.resize(x);
								std::copy(inst->m_newData.data(), inst->m_newData.data() + x, instBefore->m_newData.data());

								// Generate new Repeat Instruction
								Repeat_Instruction * instRepeat = new Repeat_Instruction();
								instRepeat->m_index = inst->m_index + x;
								instRepeat->m_value = value_at_x;
								instRepeat->m_amount = length;

								// Modifying instructions vector
								std::unique_lock<std::mutex> writeGuard(instructionMutex);
								instructions.push_back(instBefore);
								instructions.push_back(instRepeat);
								// Modify original insert-instruction to contain remainder of the data
								inst->m_index = inst->m_index + x + length;
								std::memmove(&inst->m_newData[0], &inst->m_newData[y], inst->m_newData.size() - y);
								inst->m_newData.resize(inst->m_newData.size() - y);
								writeGuard.unlock();
								writeGuard.release();

								x = ULLONG_MAX; // require overflow, because we want next iteration for x == 0
								max = std::min<size_t>(inst->m_newData.size(), inst->m_newData.size() - 37ull);
								break;
							}
							x = y - 1;
							break;
						}
					}
				}
			});
		}

		// Wait for jobs to finish
		threader.prepareForShutdown();
		while (!threader.isFinished())
			continue;
		threader.shutdown();

		// Create a buffer to contain all the diff instructions
		size_t size_patch(0ull);
		for each (const auto & instruction in instructions)
			size_patch += instruction->size();
		Buffer patchBuffer(size_patch);

		// Write instruction data to the buffer
		size_t m_index(0ull);
		for each (const auto & instruction in instructions)
			instruction->write(patchBuffer, m_index);

		// Free up memory
		for (auto * inst : instructions)
			delete inst;
		instructions.clear();
		instructions.shrink_to_fit();

		// Try to compress the diff buffer
		auto compressedPatchBuffer = patchBuffer.compress();
		patchBuffer.release();
		if (compressedPatchBuffer) {
			// Prepend header information
			DiffHeader header(size_new);
			compressedPatchBuffer->writeHeader(&header);

			// Success
			return compressedPatchBuffer;
		}
	}

	// Failure
	return {};
}

std::optional<Buffer> Buffer::patch(const Buffer & diffBuffer) const
{
	// Ensure diff buffer at least *exists* (ignore old buffer, when empty we treat instruction as a brand new file)
	if (diffBuffer.hasData()) {
		// Read in header
		DiffHeader header;
		std::byte * dataPtr(nullptr);
		size_t dataSize(0ull);
		diffBuffer.readHeader(&header, &dataPtr, dataSize);

		// Ensure header title matches
		if (header.isValid()) {
			// Try to decompress the diff buffer
			auto diffInstructionBuffer = Buffer(dataPtr, dataSize).decompress();
			if (diffInstructionBuffer) {
				// Convert buffer into instructions
				Buffer bufferNew(header.m_targetSize);
				size_t bytesRead(0ull), byteIndex(0ull);
				while (bytesRead < diffInstructionBuffer->size()) {
					// Deduce the instruction type
					char type(0);
					byteIndex = diffInstructionBuffer->readData(&type, size_t(sizeof(char)), byteIndex);

					// Make the instruction, reading it in from memory
					Differential_Instruction * instruction(nullptr);
					switch (type) {
					case 'R':
						instruction = new Repeat_Instruction();
						break;
					case 'I':
						instruction = new Insert_Instruction();
						break;
					case 'C':
					default:
						instruction = new Copy_Instruction();
						break;
					}
					instruction->read(*diffInstructionBuffer, byteIndex);

					// Read the instruction size
					bytesRead += instruction->size();

					// Execute the instruction
					instruction->execute(bufferNew, *this);

					delete instruction;
				}

				// Success
				return bufferNew;
			}
		}
	}

	// Failure
	return {};
}


// Public Header Methods

void Buffer::readHeader(Buffer::Header * header, std::byte ** dataPtr, size_t & dataSize) const
{
	(*header) << (const_cast<std::byte*>(&m_data[0]));

	const size_t full_header_size = Header::TITLE_SIZE + header->size();
	*dataPtr = const_cast<std::byte*>(&m_data[full_header_size]);
	dataSize = size() - full_header_size;
}

void Buffer::writeHeader(const Header * header)
{
	// Make container large enough to fit the old data + header
	const size_t full_header_size = Header::TITLE_SIZE + header->size();
	const size_t oldSize = size();
	const size_t newSize = oldSize + full_header_size;
	resize(newSize);

	// Shift old data down to where the header ends
	std::memmove(&m_data[full_header_size], &m_data[0], oldSize);

	// Copy header data
	(*header) >> (&m_data[0]);
}

size_t Copy_Instruction::size() const
{
	return sizeof(char) + (sizeof(size_t) * 3ull);
}

void Copy_Instruction::execute(Buffer & bufferNew, const Buffer & bufferOld) const
{
	for (auto i = m_index, x = m_beginRead; i < bufferNew.size() && x < m_endRead && x < bufferOld.size(); ++i, ++x)
		bufferNew[i] = bufferOld[x];
}

void Copy_Instruction::write(Buffer & outputBuffer, size_t & byteIndex) const
{
	// Write Type
	byteIndex = outputBuffer.writeData(&m_type, size_t(sizeof(char)), byteIndex);
	// Write Index
	byteIndex = outputBuffer.writeData(&m_index, size_t(sizeof(size_t)), byteIndex);
	// Write Begin
	byteIndex = outputBuffer.writeData(&m_beginRead, size_t(sizeof(size_t)), byteIndex);
	// Write End
	byteIndex = outputBuffer.writeData(&m_endRead, size_t(sizeof(size_t)), byteIndex);
}

void Copy_Instruction::read(const Buffer & inputBuffer, size_t & byteIndex)
{
	// Type already read
	// Read Index
	byteIndex = inputBuffer.readData(&m_index, size_t(sizeof(size_t)), byteIndex);
	// Read Begin
	byteIndex = inputBuffer.readData(&m_beginRead, size_t(sizeof(size_t)), byteIndex);
	// Read End
	byteIndex = inputBuffer.readData(&m_endRead, size_t(sizeof(size_t)), byteIndex);
}

size_t Insert_Instruction::size() const
{
	return sizeof(char) + (sizeof(size_t) * 2) + (sizeof(char) * m_newData.size());
}

void Insert_Instruction::execute(Buffer & bufferNew, const Buffer &) const
{
	for (auto i = m_index, x = size_t(0ull), length = m_newData.size(); i < bufferNew.size() && x < length; ++i, ++x)
		bufferNew[i] = m_newData[x];
}

void Insert_Instruction::write(Buffer & outputBuffer, size_t & byteIndex) const
{
	// Write Type
	byteIndex = outputBuffer.writeData(&m_type, size_t(sizeof(char)), byteIndex);
	// Write Index
	byteIndex = outputBuffer.writeData(&m_index, size_t(sizeof(size_t)), byteIndex);
	// Write Length
	auto length = m_newData.size();
	byteIndex = outputBuffer.writeData(&length, size_t(sizeof(size_t)), byteIndex);
	if (length) {
		// Write Data
		byteIndex = outputBuffer.writeData(m_newData.data(), length, byteIndex);
	}
}

void Insert_Instruction::read(const Buffer & inputBuffer, size_t & byteIndex)
{
	// Type already read
	// Read Index
	byteIndex = inputBuffer.readData(&m_index, size_t(sizeof(size_t)), byteIndex);
	// Read Length
	size_t length;
	byteIndex = inputBuffer.readData(&length, size_t(sizeof(size_t)), byteIndex);
	if (length) {
		// Read Data
		m_newData.resize(length);
		byteIndex = inputBuffer.readData(m_newData.data(), length, byteIndex);
	}
}

size_t Repeat_Instruction::size() const
{
	return sizeof(char) + (sizeof(size_t) * 2ull) + sizeof(char);
}

void Repeat_Instruction::execute(Buffer & bufferNew, const Buffer &) const
{
	for (auto i = m_index, x = size_t(0ull); i < bufferNew.size() && x < m_amount; ++i, ++x)
		bufferNew[i] = m_value;
}

void Repeat_Instruction::write(Buffer & outputBuffer, size_t & byteIndex) const
{
	// Write Type
	byteIndex = outputBuffer.writeData(&m_type, size_t(sizeof(char)), byteIndex);
	// Write Index
	byteIndex = outputBuffer.writeData(&m_index, size_t(sizeof(size_t)), byteIndex);
	// Write Amount
	byteIndex = outputBuffer.writeData(&m_amount, size_t(sizeof(size_t)), byteIndex);
	// Write Value
	byteIndex = outputBuffer.writeData(&m_value, size_t(sizeof(char)), byteIndex);
}

void Repeat_Instruction::read(const Buffer & inputBuffer, size_t & byteIndex)
{
	// Type already read
	// Read Index
	byteIndex = inputBuffer.readData(&m_index, size_t(sizeof(size_t)), byteIndex);
	// Read Amount
	byteIndex = inputBuffer.readData(&m_amount, size_t(sizeof(size_t)), byteIndex);
	// Read Value
	byteIndex = inputBuffer.readData(&m_value, size_t(sizeof(char)), byteIndex);
}