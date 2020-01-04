#include "yatta.hpp"
#include <algorithm>
#include <iostream>
#include <string>

using namespace yatta;


// Forward Declarations
static bool ConstructionTest();
static bool AssignmentTest();
static bool MethodTest();
static bool IOTest();
static bool CompressionTest();
static bool DiffTest();

int main()
{
	const auto test1 = ConstructionTest();
	const auto test2 = AssignmentTest();
	const auto test3 = MethodTest();
	const auto test4 = IOTest();
	const auto test5 = CompressionTest();
	const auto test6 = DiffTest();

	return (test1 && test2 && test3 && test4 && test5 && test6) ? 0 : 1;
}

static bool ConstructionTest()
{
	// Ensure we can make empty buffers
	Buffer buffer;
	if (buffer.empty()) {
		// Ensure we can make a large buffer
		Buffer largeBuffer(1234ULL);
		if (largeBuffer.hasData()) {
			// Ensure move constructor works
			Buffer moveBuffer(Buffer(123ULL));
			if (moveBuffer.size() == 123ULL) {
				// Ensure copy constructor works
				largeBuffer[0] = static_cast<std::byte>(255U);
				const Buffer& copyBuffer(largeBuffer);
				if (copyBuffer[0] == largeBuffer[0]) {
					std::cout << "Construction Test - Success\n";
					return true; // Success
				}
			}
		}
	}

	std::cout << "Construction Test - Failure\n";
	return false; // Failure
}

static bool AssignmentTest()
{
	Buffer bufferA(1234ULL);
	Buffer bufferB(123456789ULL);
	bufferA[0] = static_cast<std::byte>(255U);
	bufferB[0] = static_cast<std::byte>(126U);
	bufferA = bufferB;

	// Ensure buffers are equal
	// To Do: Add equality operator
	if (bufferA[0] == bufferB[0]) {
		Buffer bufferC(456);
		bufferC[0] = static_cast<std::byte>(64U);
		bufferA = std::move(bufferC);
		// Ensure bufferC is fully moved over to bufferA
		if (bufferA[0] == static_cast<std::byte>(64U)) {
			std::cout << "Assignment Test - Success\n";
			return true; // Success
		}
	}

	std::cout << "Assignment Test - Failure\n";
	return false; // Failure
}

static bool MethodTest()
{
	Buffer buffer;
	if (buffer.empty()) {
		buffer.resize(1234ULL);
		if (buffer.hasData()) {
			if (buffer.size() == 1234ULL) {
				if (buffer.capacity() == 2468ULL) {
					if (const auto hash = buffer.hash(); hash != 0ULL && hash != 1234567890ULL) {
						std::cout << "Method Test - Success\n";
						return true; // Success
					}
				}
			}
		}
	}

	std::cout << "Method Test - Failure\n";
	return false; // Failure
}

static bool IOTest()
{
	// Ensure object IO is correct
	Buffer buffer(sizeof(int));
	constexpr int in_int(64);
	buffer.in_type(in_int);
	int out_int(0);
	buffer.out_type(out_int);
	if (in_int == out_int) {
		// Ensure raw pointer IO is correct
		const char word[28] = ("This is a sample sentence.\0");
		constexpr auto sentenceSize = sizeof(char) * 28;
		buffer.resize(sentenceSize + sizeof(int));
		buffer.in_raw(&word, sentenceSize, sizeof(int));

		struct TestStructure {
			int a;
			char b[28];
		} combinedOutput;
		buffer.out_raw(&combinedOutput, sizeof(TestStructure));

		if (combinedOutput.a == in_int && std::string(combinedOutput.b) == word) {
			std::cout << "IO Test - Success\n";
			return true; // Success
		}
	}

	std::cout << "IO Test - Failure\n";
	return false; // Failure
}

static bool CompressionTest()
{
	// The structure we'll compress and decompress
	struct TestStructure {
		int a = 0;
		float b = 0.F;
		char c[32] = { '\0' };
	};

	// Create a buffer and load it with test data
	Buffer buffer(sizeof(TestStructure));
	constexpr TestStructure testData{ 1234, 567.890F,"QWEQWEQWEQWEQWEQWEQWEQWEQWEQWE\0" };
	buffer.in_type(testData);

	// Attempt to compress the buffer
	if (const auto compressedBuffer = buffer.compress()) {
		// Attempt to decompress the compressed buffer
		if (const auto decompressedBuffer = compressedBuffer->decompress()) {
			// Dump buffer data back into test structure
			TestStructure decompressedData;
			decompressedBuffer->out_type(decompressedData);

			// Ensure data matches
			if (testData.a == decompressedData.a && testData.b == decompressedData.b && std::strcmp(testData.c, decompressedData.c) == 0) {
				std::cout << "(De)Compression Test - Success\n";
				return true; // Success
			}
		}
	}

	std::cout << "(De)Compression Test - Failure\n";
	return false; // Failure
}

static bool DiffTest()
{
	struct Foo {
		int a = 0;
		float b = 0.0F;
		char c[28] = {'\0'};
	} dataA{1234, 567.890F, "This is a sample sentence.\0"};
	struct Bar {
		char a[32] = { '\0' };
		float b = 0.0F;
		int c = 0;
	} dataB{"Not even a sample sentence.\0", 890.567F, 4321};
	Buffer bufferA(sizeof(Foo));
	Buffer bufferB(sizeof(Bar));
	bufferA.in_type(dataA);
	bufferB.in_type(dataB);

	// Ensure we've generated an instruction set
	if (const auto diffBuffer = bufferA.diff(bufferB, 8)) {
		// Check that we've actually converted from A to B
		if (const auto patchedBuffer = bufferA.patch(*diffBuffer)) {
			Bar dataC;
			patchedBuffer->out_type(dataC);
			if (std::strcmp(dataB.a, dataC.a) == 0 && dataB.b == dataC.b && dataB.c == dataC.c) {
				std::cout << "Diff-Patch Test - Success\n";
				return true; // Success
			}
		}
	}

	std::cout << "Diff-Patch Test - Failure\n";
	return false; // Failure
}