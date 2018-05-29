#ifndef MYVM_H
#define MYVM_H

#include<stdint.h>
#include<stdbool.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<inttypes.h>
#include<fstream>
#include<iostream>



enum
{
	/*Arithmetic*/
	ADD = 0x01,
	NEG = 0x02,
	MUL = 0x03,
	DIV = 0x04,
	MOD = 0x05,

	/*Conditionals*/
	CMP = 0x10,
	JA = 0x11,
	JE = 0x12,
	JB = 0x13,
	JMP = 0x14,

	/*Subroutines*/
	CALL = 0x20,
	RET = 0x21,

	/*Moving data*/
	LOAD = 0x30,
	STORE = 0x31,
	CONST = 0x32,
	RLOAD = 0x33,
	RSTORE  = 0x34,

	/*Auxiliary*/
	HALT = 0x40,
    OUT = 0x41,
	NOP = 0x42,

	/*Stack*/
	PUSH  = 0x50,
	PUSH_ALL = 0X51,
	POP = 0X52,
	POP_ALL = 0X53,
	LSP = 0X54,

	/*Register*/
	RA = 0X00,
	RB = 0X01,
	RC = 0X02,
	RD = 0X03,

	/*Iterupts*/
	INTERRUPT_PRINT_INTEGER = 0X01,
	INTERRUPT_PRINT_STRING = 0X02,

	/*Miscellaneous*/
	N_REGISTERS = 4,

	OPCODE_MAP_SIZE = 256,
};

typedef struct VM_CPU
{
	int64_t registers[N_REGISTERS];
	int64_t program_counter;
	int64_t stack_pointer;

	struct
	{
		uint8_t ILLEGAL_INSTRUCTION : 1;
		uint8_t STACK_UNDERFLOW : 1;
		uint8_t STACK_OVERFLOW : 1;
		uint8_t INVALID_REGISTER_INDEX : 1;
		uint8_t ILLEGAL_ACCESS : 1;
		uint8_t COMPARISON_BELOW : 1;
		uint8_t COMPARISON_EQUAL : 1;
		uint8_t COMPARISON_ABOVE : 1;
	}status;
}VM_CPU;

typedef struct MY_VM
{
	uint8_t* memory;
	int64_t memory_size;
	int64_t stack_limit;
	VM_CPU cpu;
    bool step;
	size_t opcode_map[OPCODE_MAP_SIZE];
}MY_VM;

/*****************************************************************************
* Inintializes the virtual machine with RAM memory of length'memory_size' and*
* the stack fence at 'stack_limit'
*****************************************************************************/

void InitializeVM(MY_VM* vm,int64_t memory_size,int64_t stack_limit);

/****************************************************************************
*Writes 'size' bytes to the memory of the machine. The write begins from the*
*beginning of the memory tape.
****************************************************************************/

void WriteWord(MY_VM* vm, int64_t address, int64_t value);

/****************************************************************************
*Prints the status of the machine to stdout.
****************************************************************************/

void PrintStatus(MY_VM* vm);

/****************************************************************************
*Runs the virtual machine.
****************************************************************************/

void RunVM(MY_VM* vm);

void Assemble();

void Assemble1();

void memory();


#endif /*MY_VM_H*/
