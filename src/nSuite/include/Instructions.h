#pragma once
#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "Buffer.h"
#include <variant>
#include <vector>


/** Specifies a region in the 'old' file to read from, and where to put it in the 'new' file. */
class Copy_Instruction {
public:
	// Public Methods
	/** Retrieve the bytesize of this instruction. */
	size_t SIZE() const;
	/** Exectute this instruction. */
	void DO(NST::Buffer & bufferNew, const NST::Buffer & bufferOld) const;
	/** Write-out this instruction to a buffer. */
	void WRITE(NST::Buffer & outputBuffer, size_t & byteIndex) const;
	/** Read-in this instruction from a buffer. */
	static Copy_Instruction READ(NST::Buffer & outputBuffer, size_t & byteIndex);


	// Public Attributes
	static constexpr char TYPE = 'C';
	size_t index = 0ull, beginRead = 0ull, endRead = 0ull;
};

/** Contains a block of data to insert into the 'new' file, at a given point. */
class Insert_Instruction {
public:
	// Public Methods
	/** Retrieve the bytesize of this instruction. */
	size_t SIZE() const;
	/** Exectute this instruction. */
	void DO(NST::Buffer & bufferNew, const NST::Buffer & bufferOld) const;
	/** Write-out this instruction to a buffer. */
	void WRITE(NST::Buffer & outputBuffer, size_t & byteIndex) const;
	/** Read-in this instruction from a buffer. */
	static Insert_Instruction READ(NST::Buffer & outputBuffer, size_t & byteIndex);


	// Public Attributes
	static constexpr char TYPE = 'I';
	size_t index = 0ull;
	std::vector<char> newData;
};

/** Contains a single value a to insert into the 'new' file, at a given point, repeating multiple times. */
class Repeat_Instruction {
public:
	// Public Methods
	/** Retrieve the bytesize of this instruction. */
	size_t SIZE() const;
	/** Exectute this instruction. */
	void DO(NST::Buffer & bufferNew, const NST::Buffer & bufferOld) const;
	/** Write-out this instruction to a buffer. */
	void WRITE(NST::Buffer & outputBuffer, size_t & byteIndex) const;
	/** Read-in this instruction from a buffer. */
	static Repeat_Instruction READ(NST::Buffer & outputBuffer, size_t & byteIndex);


	// Public Attributes
	static constexpr char TYPE = 'R';
	size_t index = 0ull, amount = 0ull;
	char value = 0;
};

/** All the types of instructions currently supported. */
using InstructionTypes = std::variant<Copy_Instruction, Insert_Instruction, Repeat_Instruction>;

/** Creates an instruction and returns it after reading it in from a buffer. */
struct Instruction_Maker {
	/** Make an instruction, reading it in from a buffer. */
	static InstructionTypes Make(NST::Buffer & outputBuffer, size_t & byteIndex);
};

#endif // INSTRUCTIONS_H