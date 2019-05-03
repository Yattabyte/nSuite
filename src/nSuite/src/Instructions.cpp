#include "Instructions.h"

using namespace NST;


size_t Copy_Instruction::SIZE() const 
{
	return sizeof(char) + (sizeof(size_t) * 3ull);
}

void Copy_Instruction::DO(Buffer & bufferNew, const Buffer & bufferOld) const 
{
	for (auto i = index, x = beginRead; i < bufferNew.size() && x < endRead && x < bufferOld.size(); ++i, ++x)
		bufferNew[i] = bufferOld[x];
}

void Copy_Instruction::WRITE(Buffer & outputBuffer, size_t & byteIndex) const 
{
	// Write Type
	byteIndex = outputBuffer.writeData(&TYPE, size_t(sizeof(char)), byteIndex);
	// Write Index
	byteIndex = outputBuffer.writeData(&index, size_t(sizeof(size_t)), byteIndex);
	// Write Begin
	byteIndex = outputBuffer.writeData(&beginRead, size_t(sizeof(size_t)), byteIndex);
	// Write End
	byteIndex = outputBuffer.writeData(&endRead, size_t(sizeof(size_t)), byteIndex);
}

Copy_Instruction Copy_Instruction::READ(Buffer & outputBuffer, size_t & byteIndex)
{
	// Type already read
	Copy_Instruction inst;
	// Read Index
	byteIndex = outputBuffer.readData(&inst.index, size_t(sizeof(size_t)), byteIndex);
	// Read Begin
	byteIndex = outputBuffer.readData(&inst.beginRead, size_t(sizeof(size_t)), byteIndex);
	// Read End
	byteIndex = outputBuffer.readData(&inst.endRead, size_t(sizeof(size_t)), byteIndex);
	return inst;
}

size_t Insert_Instruction::SIZE() const 
{
	return sizeof(char) + (sizeof(size_t) * 2) + (sizeof(char) * newData.size());
}

void Insert_Instruction::DO(Buffer & bufferNew, const Buffer &) const 
{
	for (auto i = index, x = size_t(0ull), length = newData.size(); i < bufferNew.size() && x < length; ++i, ++x)
		bufferNew[i] = newData[x];
}

void Insert_Instruction::WRITE(Buffer & outputBuffer, size_t & byteIndex) const 
{
	// Write Type
	byteIndex = outputBuffer.writeData(&TYPE, size_t(sizeof(char)), byteIndex);
	// Write Index
	byteIndex = outputBuffer.writeData(&index, size_t(sizeof(size_t)), byteIndex);
	// Write Length
	auto length = newData.size();
	byteIndex = outputBuffer.writeData(&length, size_t(sizeof(size_t)), byteIndex);
	if (length) {
		// Write Data
		byteIndex = outputBuffer.writeData(newData.data(), length, byteIndex);
	}
}

Insert_Instruction Insert_Instruction::READ(Buffer & outputBuffer, size_t & byteIndex)
{
	// Type already read
	Insert_Instruction inst;
	// Read Index
	byteIndex = outputBuffer.readData(&inst.index, size_t(sizeof(size_t)), byteIndex);
	// Read Length
	size_t length;
	byteIndex = outputBuffer.readData(&length, size_t(sizeof(size_t)), byteIndex);
	if (length) {
		// Read Data
		inst.newData.resize(length);
		byteIndex = outputBuffer.readData(inst.newData.data(), length, byteIndex);
	}
	return inst;
}

size_t Repeat_Instruction::SIZE() const 
{
	return sizeof(char) + (sizeof(size_t) * 2ull) + sizeof(char);
}

void Repeat_Instruction::DO(Buffer & bufferNew, const Buffer &) const 
{
	for (auto i = index, x = size_t(0ull); i < bufferNew.size() && x < amount; ++i, ++x)
		bufferNew[i] = value;
}

void Repeat_Instruction::WRITE(Buffer & outputBuffer, size_t & byteIndex) const
{
	// Write Type
	byteIndex = outputBuffer.writeData(&TYPE, size_t(sizeof(char)), byteIndex);
	// Write Index
	byteIndex = outputBuffer.writeData(&index, size_t(sizeof(size_t)), byteIndex);
	// Write Amount
	byteIndex = outputBuffer.writeData(&amount, size_t(sizeof(size_t)), byteIndex);
	// Write Value
	byteIndex = outputBuffer.writeData(&value, size_t(sizeof(char)), byteIndex);
}

Repeat_Instruction Repeat_Instruction::READ(Buffer & outputBuffer, size_t & byteIndex)
{
	// Type already read
	Repeat_Instruction inst;
	// Read Index
	byteIndex = outputBuffer.readData(&inst.index, size_t(sizeof(size_t)), byteIndex);
	// Read Amount
	byteIndex = outputBuffer.readData(&inst.amount, size_t(sizeof(size_t)), byteIndex);	
	// Read Value
	byteIndex = outputBuffer.readData(&inst.value, size_t(sizeof(char)), byteIndex);
	return inst;
}

InstructionTypes Instruction_Maker::Make(Buffer & outputBuffer, size_t & byteIndex)
{
	char type;
	byteIndex = outputBuffer.readData(&type, size_t(sizeof(char)), byteIndex);
	switch (type) {
		case Copy_Instruction::TYPE:
			return Copy_Instruction::READ(outputBuffer, byteIndex);
		case Insert_Instruction::TYPE:
			return Insert_Instruction::READ(outputBuffer, byteIndex);
		case Repeat_Instruction::TYPE:
			return Repeat_Instruction::READ(outputBuffer, byteIndex);
	}
	return {};
}