#include"vm.h"
#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<string.h>
#include<fstream>
using namespace std;

typedef struct instruction
{
	uint8_t opcode;
	size_t size;
	bool   (*execute)(MY_VM*);
}instruction;

/**************************************
*return 'true' if the stack is empty
**************************************/
static bool StackIsEmpty(MY_VM* vm)
{
	return vm->cpu.stack_pointer >= vm->memory_size;
}

/****************************************************
*Return 'true' if the stack is full
****************************************************/

static bool StackIsFull(MY_VM * vm)
{
	return vm->cpu.stack_pointer <= vm->stack_limit;
}

/*returns the amount of free space in the stack in bytes*/

static int64_t GetAvailableStackSize(MY_VM * vm)
{
	return vm->cpu.stack_pointer - vm->stack_limit;
}

/*returns the number of bytes occupied by the stack*/
static int64_t GetOccupiedStackSize(MY_VM * vm)
{
	return vm->memory_size - vm->cpu.stack_pointer;
}

/*returns 'true' if the stack can provide data for all registers*/

static bool CanPerformMultipush(MY_VM * vm)
{
	return GetAvailableStackSize(vm) >= sizeof(int64_t) * N_REGISTERS;
}

/*Returns 'true' if the stack can provide data for all registers*/

static bool CanPerformMultipop(MY_VM * vm)
{
	return GetOccupiedStackSize(vm)  >= sizeof(int64_t) * N_REGISTERS;
}

/*return true if the instruction does not run over the memory*/

static bool InstructionFitsInMemory(MY_VM * vm, uint8_t opcode);

/*return the length of the instruction with opcode 'opcode'*/
static size_t GetInstructionLength(MY_VM *vm, uint8_t opcode);

void InitializeVM(MY_VM * vm, int64_t memory_size, int64_t stack_limit)
{
	/*Make sure both 'memory_size' and 'stack_limit' are divisible by 4*/
	memory_size += sizeof(int64_t) - (memory_size % sizeof(int64_t));

	stack_limit += sizeof(int64_t) - (stack_limit % sizeof(int64_t));

	vm->memory = (uint8_t*)calloc(memory_size, sizeof(uint8_t));
	vm->memory_size = memory_size;
	vm->stack_limit = stack_limit;
	vm->cpu.program_counter = 0;
	vm->cpu.stack_pointer = (int64_t)memory_size;
	/*zero out all status flags*/
	vm->cpu.status.ILLEGAL_ACCESS = 0;
	vm->cpu.status.COMPARISON_ABOVE = 0;
	vm->cpu.status.COMPARISON_BELOW = 0;
	vm->cpu.status.COMPARISON_EQUAL = 0;
	vm->cpu.status.ILLEGAL_INSTRUCTION = 0;
	vm->cpu.status.INVALID_REGISTER_INDEX = 0;
	vm->cpu.status.STACK_OVERFLOW = 0;
	vm->cpu.status.STACK_UNDERFLOW = 0;
    vm->step=0;

	/*zero out the register and the map mapping opcodes to their respectove
	*instruction descriptor*/

	memset(vm->cpu.registers, 0, sizeof(int64_t)*N_REGISTERS);
	memset(vm->opcode_map, 0, sizeof(vm->opcode_map));

	/*build the opcode map*/
	vm->opcode_map[ADD] = 1;
	vm->opcode_map[NEG] = 2;
	vm->opcode_map[MUL] = 3;
	vm->opcode_map[DIV] = 4;
	vm->opcode_map[MOD] = 5;
	vm->opcode_map[CMP] = 6;
	vm->opcode_map[JA] = 7;
	vm->opcode_map[JE] = 8;
	vm->opcode_map[JB] = 9;
	vm->opcode_map[JMP] = 10;
	vm->opcode_map[CALL] = 11;
	vm->opcode_map[RET] = 12;
	vm->opcode_map[LOAD] = 13;
	vm->opcode_map[STORE] = 14;
	vm->opcode_map[CONST] = 15;
	vm->opcode_map[RLOAD] = 16;
    vm->opcode_map[RSTORE] = 17;
	vm->opcode_map[HALT] = 18;
    vm->opcode_map[OUT] = 19;
	vm->opcode_map[NOP] = 20;
	vm->opcode_map[PUSH] = 21;
	vm->opcode_map[PUSH_ALL] = 22;
	vm->opcode_map[POP] = 23;
	vm->opcode_map[POP_ALL] = 24;
	vm->opcode_map[LSP] = 25;

}

void WriteVMMemory(MY_VM * vm, uint8_t * mem, size_t size)
{
	memcpy(mem, vm->memory, size);
}

static int64_t ReadWord(MY_VM * vm, int64_t address)
{
	uint8_t b1 = vm->memory[address];
	uint8_t b2 = vm->memory[address + 1];
	uint8_t b3 = vm->memory[address + 2];
	uint8_t b4 = vm->memory[address + 3];
	uint8_t b5 = vm->memory[address + 4];
	uint8_t b6 = vm->memory[address + 5];
	uint8_t b7 = vm->memory[address + 6];
	uint8_t b8 = vm->memory[address + 7];

	/*little endian*/
	return (int64_t)((b8 << 56) | (b7 << 48) | (b6 << 40) | (b5 << 32) | (b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}

void WriteWord(MY_VM * vm, int64_t address, int64_t value)
{
	uint8_t b1 =  value & 0xff;
	uint8_t b2 = (value & 0xff00) >> 8;
	uint8_t b3 = (value & 0xff0000) >> 16;
	uint8_t b4 = (value & 0xff000000) >> 24;
	uint8_t b5 = (value & 0xff00000000) >> 32;
	uint8_t b6 = (value & 0xff0000000000) >> 40;
	uint8_t b7 = (value & 0xff000000000000) >> 48;
	uint8_t b8 = (value & 0xff00000000000000) >> 56;

	vm->memory[address] = b1;
	vm->memory[address + 1] = b2;
	vm->memory[address + 2] = b3;
	vm->memory[address + 3] = b4;
	vm->memory[address + 4] = b5;
	vm->memory[address + 5] = b6;
	vm->memory[address + 6] = b7;
	vm->memory[address + 7] = b8;
}

static uint8_t ReadByte(MY_VM * vm, size_t address)
{
	return vm->memory[address];
}

/*pops a single word from the stack. used by some instructions that implicitly
*operate on stack*/

static int64_t PopVM(MY_VM * vm)
{
	if (StackIsEmpty(vm))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return 0;
	}
	int64_t word = ReadWord(vm, vm->cpu.stack_pointer);
	vm->cpu.stack_pointer += 8;
	return word;
}

/*push a single word to the stack. used by some instructions.used implicitly by
*some instructions*/

static void PushVM(MY_VM* vm, uint64_t value)
{
	WriteWord(vm, vm->cpu.stack_pointer -= 8, value);
}

static bool IsValidRegisterIndex(uint8_t byte)
{
	switch (byte)
	{
		case RA:
		case RB:
		case RC:
		case RD:
			return true;
	}
	return false;
}

static int64_t GetProgramCounter(MY_VM* vm)
{
	return  vm->cpu.program_counter;
}

static bool ExecuteAdd(MY_VM* vm)
{
	uint8_t source_register_index;
	uint8_t target_register_index;

	if (!InstructionFitsInMemory(vm, ADD))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
	target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);

	if (!IsValidRegisterIndex(source_register_index) || !IsValidRegisterIndex(target_register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	vm->cpu.registers[target_register_index] += vm->cpu.registers[source_register_index];

	vm->cpu.program_counter += GetInstructionLength(vm, ADD);
	return false;
}

static bool ExecuteNeg(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, NEG))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}
	uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);

	if(!IsValidRegisterIndex(register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	vm->cpu.registers[register_index] = -vm->cpu.registers[register_index];
	vm->cpu.program_counter += GetInstructionLength(vm, NEG);
	return false;
}

static bool ExecuteMul(MY_VM * vm)
{
	uint8_t source_register_index;
	uint8_t target_register_index;

	if (!InstructionFitsInMemory(vm, MUL))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}
	source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
	target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);
	if (!IsValidRegisterIndex(source_register_index) || !IsValidRegisterIndex(target_register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	vm->cpu.registers[target_register_index] *= vm->cpu.registers[source_register_index];
	vm->cpu.program_counter += GetInstructionLength(vm, MUL);
	return false;
}

static bool ExecuteDiv(MY_VM * vm)
{
	uint8_t source_register_index;
	uint8_t target_register_index;

	if (!InstructionFitsInMemory(vm, DIV))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
	target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);

	if (!IsValidRegisterIndex(source_register_index) || !IsValidRegisterIndex(target_register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	vm->cpu.registers[target_register_index] /= vm->cpu.registers[source_register_index];
	vm->cpu.program_counter += GetInstructionLength(vm, DIV);
	return false;
}

static bool ExecuteMod(MY_VM * vm)
{
	uint8_t source_register_index;
	uint8_t target_register_index;

	if (!InstructionFitsInMemory(vm, MOD))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
	target_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);

	if (!IsValidRegisterIndex(source_register_index) || !IsValidRegisterIndex(target_register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	vm->cpu.registers[target_register_index] %= vm->cpu.registers[source_register_index];
	vm->cpu.program_counter += GetInstructionLength(vm, MOD);
	return false;
}

static bool ExecuteCmp(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, CMP))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	uint8_t register_index_1 = ReadByte(vm, GetProgramCounter(vm) + 1);
	uint8_t register_index_2 = ReadByte(vm, GetProgramCounter(vm) + 2);

	if (!IsValidRegisterIndex(register_index_1) || !IsValidRegisterIndex(register_index_2))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	int64_t register_1 = vm->cpu.registers[register_index_1];
	int64_t register_2 = vm->cpu.registers[register_index_2];

	if (register_1 < register_2)
	{
		vm->cpu.status.COMPARISON_ABOVE = 0;
		vm->cpu.status.COMPARISON_BELOW = 1;
		vm->cpu.status.COMPARISON_EQUAL = 0;
	}
	else if (register_1 > register_2)
	{
		vm->cpu.status.COMPARISON_ABOVE = 1;
		vm->cpu.status.COMPARISON_BELOW = 0;
		vm->cpu.status.COMPARISON_EQUAL = 0;
	}
	else
	{
		vm->cpu.status.COMPARISON_ABOVE = 0;
		vm->cpu.status.COMPARISON_BELOW = 0;
		vm->cpu.status.COMPARISON_EQUAL = 1;
	}

	vm->cpu.program_counter += GetInstructionLength(vm, CMP);
	return false;
}

static bool ExecuteJumpIfAbove(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, JA))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	if (vm->cpu.status.COMPARISON_ABOVE)
	{
		vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
	}
	else
	{
		vm->cpu.program_counter += GetInstructionLength(vm, JA);
	}
	return false;
}

static bool ExecuteJumpIfEqual(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, JE))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	if (vm->cpu.status.COMPARISON_EQUAL)
	{
		vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
	}
	else
	{
		vm->cpu.program_counter += GetInstructionLength(vm, JE);
	}
	return false;
}

static bool ExecuteJumpIfBelow(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, JB))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	if (vm->cpu.status.COMPARISON_BELOW)
	{
		vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
	}
	else
	{
		vm->cpu.program_counter += GetInstructionLength(vm, JB);
	}
	return false;
}

static bool ExecuteJump(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, JMP))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	vm->cpu.program_counter = ReadWord(vm, GetProgramCounter(vm) + 1);
	return false;
}

static bool ExecuteCall(MY_VM *vm)
{
	if(!InstructionFitsInMemory(vm,CALL))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	if (GetAvailableStackSize(vm) < 8)
	{
		vm->cpu.status.STACK_OVERFLOW = 1;
		return true;
	}

	/*save the return address on the stack*/

	uint64_t address = ReadWord(vm, GetProgramCounter(vm) + 1);
	PushVM(vm, (uint64_t)(GetProgramCounter(vm) + GetInstructionLength(vm, CALL)));

	/*Actual jump to the subroutine*/
	vm->cpu.program_counter = address;
	return false;
}

static bool ExecuteRet(MY_VM* vm)
{
	if (!InstructionFitsInMemory(vm, RET))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	if (StackIsEmpty(vm))
	{
		vm->cpu.status.STACK_UNDERFLOW = 1;
		return true;
	}
	vm->cpu.program_counter = PopVM(vm);
	return false;
}

static bool ExecuteLoad(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, LOAD))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);

	if (!IsValidRegisterIndex(register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	uint64_t address = ReadWord(vm, GetProgramCounter(vm) + 2);
	vm->cpu.registers[register_index] = ReadWord(vm, address);
	vm->cpu.program_counter += GetInstructionLength(vm, LOAD);
	return false;
}

static bool ExecuteStore(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, STORE))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);

	if (!IsValidRegisterIndex(register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	uint64_t address = ReadWord(vm, GetProgramCounter(vm) + 2);
	WriteWord(vm, address, vm->cpu.registers[register_index]);
	vm->cpu.program_counter += GetInstructionLength(vm, STORE);
	return false;
}

static bool ExecuteConst(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, CONST))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
	int64_t datum = ReadWord(vm, GetProgramCounter(vm) + 2);

	if (!IsValidRegisterIndex(register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	vm->cpu.registers[register_index] = datum;
	vm->cpu.program_counter += GetInstructionLength(vm, CONST);
	return false;
}

static bool ExecuteRload(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, RLOAD))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	uint8_t address_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
	uint8_t data_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);

	if (!IsValidRegisterIndex(address_register_index) || !IsValidRegisterIndex(data_register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
	}
	vm->cpu.registers[data_register_index] = ReadWord(vm, vm->cpu.registers[address_register_index]);
	vm->cpu.program_counter += GetInstructionLength(vm, RLOAD);
	return false;
}

static bool ExecuteRstore(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, RSTORE))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	uint8_t source_register_index = ReadByte(vm, GetProgramCounter(vm) + 1);
	uint8_t address_register_index = ReadByte(vm, GetProgramCounter(vm) + 2);

	if (!IsValidRegisterIndex(source_register_index) || !IsValidRegisterIndex(address_register_index))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	WriteWord(vm, vm->cpu.registers[address_register_index], vm->cpu.registers[source_register_index]);

	return false;
}

static void PrintString(MY_VM * vm, uint64_t address)
{
	ofstream out("C:/VM/out.txt", ios::app);
    out<<(const char *)(&vm->memory[address])<<"\n";
	out.close();
}

static bool ExecuteInterrupt(MY_VM * vm)
{
    if (!InstructionFitsInMemory(vm, OUT))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	uint8_t interrupt_number = ReadByte(vm, GetProgramCounter(vm) + 1);

	if (StackIsEmpty(vm))
	{
		vm->cpu.status.STACK_UNDERFLOW = 1;
		return true;
	}
	ofstream out("C:/VM/out.txt",ios::app);
	switch (interrupt_number)
	{
		case INTERRUPT_PRINT_INTEGER:
            out<<PopVM(vm)<<"\n";
			out.close();
			break;
		case INTERRUPT_PRINT_STRING:
			PrintString(vm, PopVM(vm));
			out.close();
			break;
		default:
			out.close();
			return true;
	}

    vm->cpu.program_counter += GetInstructionLength(vm, OUT);
	return false;
}

static bool ExecutePush(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, PUSH))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	if (StackIsFull(vm))
	{
		return true;
	}

	uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);

	if (!IsValidRegisterIndex(register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	WriteWord(vm, vm->cpu.stack_pointer - 8, vm->cpu.registers[register_index]);

	vm->cpu.stack_pointer -= 8;
	vm->cpu.program_counter += GetInstructionLength(vm, PUSH);
	return false;
}

static bool ExecutePushAll(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, PUSH_ALL))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	if (!CanPerformMultipush(vm))
	{
		vm->cpu.status.STACK_OVERFLOW = 1;
		return true;
	}

	WriteWord(vm, vm->cpu.stack_pointer -= 8, vm->cpu.registers[RA]);
	WriteWord(vm, vm->cpu.stack_pointer -= 8, vm->cpu.registers[RB]);
	WriteWord(vm, vm->cpu.stack_pointer -= 8, vm->cpu.registers[RC]);
	WriteWord(vm, vm->cpu.stack_pointer -= 8, vm->cpu.registers[RD]);

	vm->cpu.program_counter += GetInstructionLength(vm, PUSH_ALL);
	return false;
}

static bool ExecutePop(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, POP))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	if (StackIsEmpty(vm))
	{
		return true;
	}

	uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);

	if (!IsValidRegisterIndex(register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	int64_t datum = ReadWord(vm, GetProgramCounter(vm) + 2);
	vm->cpu.registers[register_index] += 8;
	vm->cpu.program_counter += GetInstructionLength(vm, POP);
	return false;
}

static bool ExecutePopAll(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, POP_ALL))
	{
		vm->cpu.status
			.ILLEGAL_ACCESS = 1;
		return true;
	}

	if (!CanPerformMultipop(vm))
	{
		vm->cpu.status.STACK_UNDERFLOW = 1;
		return true;
	}

	vm->cpu.registers[RD] = ReadWord(vm, vm->cpu.stack_pointer);
	vm->cpu.registers[RC] = ReadWord(vm, vm->cpu.stack_pointer+8);
	vm->cpu.registers[RB] = ReadWord(vm, vm->cpu.stack_pointer+16);
	vm->cpu.registers[RA] = ReadWord(vm, vm->cpu.stack_pointer+24);
	vm->cpu.stack_pointer += 32;
	vm->cpu.program_counter += GetInstructionLength(vm, POP_ALL);
	return false;
}

static bool ExecuteLSP(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, LSP))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	uint8_t register_index = ReadByte(vm, GetProgramCounter(vm) + 1);

	if (!IsValidRegisterIndex(register_index))
	{
		vm->cpu.status.INVALID_REGISTER_INDEX = 1;
		return true;
	}

	vm->cpu.registers[register_index] = vm->cpu.stack_pointer;
	vm->cpu.program_counter += GetInstructionLength(vm, LSP);
	return false;
}

static bool ExecuteNop(MY_VM * vm)
{
	if (!InstructionFitsInMemory(vm, NOP))
	{
		vm->cpu.status.ILLEGAL_ACCESS = 1;
		return true;
	}

	vm->cpu.program_counter += GetInstructionLength(vm,NOP);
	return false;
}

static bool ExecuteHalt(MY_VM * vm)
{
	return true;
}

void PrintStatus(MY_VM * vm)
{
	ofstream out("C:/VM/out.txt", ios::app);

    out << "ILLEGAL_INSTRUCTION     :" << (int)vm->cpu.status.ILLEGAL_INSTRUCTION << "\n";
    out << "STACK_UNDERFLOW         :" << (int)vm->cpu.status.STACK_UNDERFLOW << "\n";
    out << "STACK_OVERFLOW          :" << (int)vm->cpu.status.STACK_OVERFLOW << "\n";
    out << "INVALID_REGISTER_INDEX  :" << (int)vm->cpu.status.INVALID_REGISTER_INDEX << "\n";
    out << "ILLEGAL_ACCESS          :" << (int)vm->cpu.status.ILLEGAL_ACCESS << "\n";
    out << "COMPARISON_ABOVE        :" << (int)vm->cpu.status.COMPARISON_ABOVE << "\n";
    out << "COMPARISON_EQUAL        :" << (int)vm->cpu.status.COMPARISON_EQUAL<<"\n";
    out << "COMPARISON_BELOW        :" << (int)vm->cpu.status.COMPARISON_BELOW<<"\n";
	out.close();
	out.clear();
}

const instruction instructions[] = {
	{0,0,NULL},
	{ADD,3,ExecuteAdd},//加法
	{NEG,2,ExecuteNeg},//取反	
	{MUL,3,ExecuteMul},//乘法
	{DIV,3,ExecuteDiv},//除法
	{MOD,3,ExecuteMod},//取模


	{CMP,3,ExecuteCmp},//比较
	{JA ,9,ExecuteJumpIfAbove},//大于跳传
	{JE ,9,ExecuteJumpIfEqual},//等于跳转
	{JB ,9,ExecuteJumpIfBelow},//小于跳转
	{JMP,9,ExecuteJump},//无条件跳转

	{CALL,9,ExecuteCall},//pc跳至指定的指令
	{RET,1,ExecuteRet},//pc跳至栈顶的指令
	{LOAD,10,ExecuteLoad},//地址取数放入寄存器
	{STORE,10,ExecuteStore},//寄存器数写到地址里
	{CONST,10,ExecuteConst},//立即数到寄存器
	{RLOAD,3,ExecuteRload},//一个寄存器里的值作为地址，将地址中的数存到另一个寄存器中
	{RSTORE,3,ExecuteRstore},//一个寄存器里的值作为地址，将另一个寄存器中的数存入该地址

	{HALT,1,ExecuteHalt},//暂停
    {OUT,2,ExecuteInterrupt},//中断输出栈顶
	{NOP,1,ExecuteNop},//空操作

	{PUSH,2,ExecutePush},//压栈
	{PUSH_ALL,1,ExecutePushAll},//将寄存器全部压入
	{POP,2,ExecutePop},//弹出
	{POP_ALL,1,ExecutePopAll},//全部弹出
	{LSP,2,ExecuteLSP}//将栈指针值赋给某寄存器
};

static size_t GetInstructionLength(MY_VM * vm, uint8_t opcode)
{
	size_t index = vm->opcode_map[opcode];
	return instructions[index].size;
}

/*checks that an instruction fits entirely in the memory*/

static bool InstructionFitsInMemory(MY_VM * vm, uint8_t opcode)
{
	size_t instruction_length = GetInstructionLength(vm, opcode);
	return vm->cpu.program_counter + instruction_length <= vm->memory_size;
}

void RunVM(MY_VM * vm)
{
	int ccnt = 0;
	while (true)
	{
		int64_t program_counter = GetProgramCounter(vm);
		if (program_counter < 0 || program_counter >= vm->memory_size)
		{
			vm->cpu.status.ILLEGAL_ACCESS = 1;
			return;
		}

		uint8_t opcode = vm->memory[program_counter];
		size_t index = vm->opcode_map[opcode];

		if (index == 0)
		{
			vm->cpu.status.ILLEGAL_INSTRUCTION = 1;
			return;
		}

		bool(*opcode_exec)(MY_VM*) = instructions[index].execute;
		
		if (vm->step == 0)
		{
			if (opcode_exec(vm) || ((ccnt++) > 100))
			{
				return;
			}
		}
		else
		{
			opcode_exec(vm);
			return;
		}
	}
}
