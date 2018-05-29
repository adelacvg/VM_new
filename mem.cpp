#include <iostream>
#include <fstream>
#include<iomanip>
#include<sstream>
#include <cstdio>
#include <cstdlib>
#include<inttypes.h>
#include"vm.h"
using namespace std;

static size_t getFileSize(FILE* file)
{
	long int original_cursor = ftell(file);
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	fseek(file, original_cursor, SEEK_SET);
	return size;
}

uint8_t* mem;
void memory()
{

	FILE* file = fopen("C:/VM/out.bin", "r");
	size_t file_size = getFileSize(file);
	mem = (uint8_t*)calloc(file_size + 1, sizeof(uint8_t));
    ofstream out("C:/VM/mem.txt",ios::out);

    out.clear();
	fread(mem, 1, file_size, file);
	fclose(file);
	int address = 0;
	for (size_t i = 0; i<file_size; i++, address++)
	{
		if (address % 16 == 0)
		{
			if (address)
				out << "\n";
			out << setw(8) << setfill('0') << hex << address << " ";
		}
		uint8_t x = mem[i];
		out << setw(2) << setfill('0') << hex << (int16_t)x << " ";
		if (i % 16 == 7)
			out << " ";
    }
    out.clear();
	out.close();

}
