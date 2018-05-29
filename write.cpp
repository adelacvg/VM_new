#include<string>
#include<sstream>
#include<iostream>
#include<fstream>
#include<memory>


#include"vm.h"
using namespace std;
unsigned char buffer[100000];
int i = 0;
int reg_id(string s)
{
	if (s == "RA")
		return 0;
	else if (s == "RB")
		return 1;
	else if (s == "RC")
		return 2;
	else if (s == "RD")
        return 3;
}
void writeword(string s)
{
    std::stringstream st;
	int64_t a;
	st << s;
	st >> a;
	st.clear();
	st << hex << a;
	st >> s;
	//cout<<s<<endl;
	while (s.length() < 16)
		s = "0" + s;
	for (int j = 14; j >= 0; j -= 2, st.clear())
	{
		st << string(1, s[j]) + string(1, s[j + 1]);
		st >> hex >> a;
        buffer[i++] = (uint8_t)a;
		//cout<<s[j]<<' '<<s[j+1]<<' '<<a<<endl;
	}
}
int string_to_hex_to_dec(string s)
{
	stringstream st;
	int a;
	st << s;
	st >> hex >> a;
	return a;
}
void Assemble()
{
    i = 0;
    memset(buffer,0,sizeof(buffer));

    ifstream in("C:/VM/in.txt");

    string s;
    string tmp1, tmp2;
    while (in >> s)
    {
        if (s == "ADD")
        {
            buffer[i++] = string_to_hex_to_dec("0x01");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
            in >> tmp2;
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "NEG")
        {
            buffer[i++] = string_to_hex_to_dec("0x02");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
        }
        else if (s == "MUL")
        {
            buffer[i++] = string_to_hex_to_dec("0x03");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
            in >> tmp2;
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "DIV")
        {
            buffer[i++] = string_to_hex_to_dec("0x04");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
            in >> tmp2;
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "MOD")
        {
            buffer[i++] = string_to_hex_to_dec("0x05");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
            in >> tmp2;
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "CMP")
        {
            buffer[i++] = string_to_hex_to_dec("0x10");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "JA")
        {
            buffer[i++] = string_to_hex_to_dec("0x11");
            in >> tmp1;
            writeword(tmp1);
        }
        else if (s == "JE")
        {
            buffer[i++] = string_to_hex_to_dec("0x12");
            in >> tmp1;
            writeword(tmp1);
        }
        else if (s == "JB")
        {
            buffer[i++] = string_to_hex_to_dec("0x13");
            in >> tmp1;
            writeword(tmp1);
        }
        else if (s == "JMP")
        {
            buffer[i++] = string_to_hex_to_dec("0x14");
            in >> tmp1;
            writeword(tmp1);
        }
        else if (s == "CALL")
        {
            buffer[i++] = string_to_hex_to_dec("0x20");
            in >> tmp1;
            writeword(tmp1);
        }
        else if (s == "RET")
        {
            buffer[i++] = string_to_hex_to_dec("0x21");
        }
        else if (s == "LOAD")
        {
            buffer[i++] = string_to_hex_to_dec("0x30");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            writeword(tmp2);
        }
        else if (s == "STORE")
        {
            buffer[i++] = string_to_hex_to_dec("0x31");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            writeword(tmp2);
        }
        else if (s == "CONST")
        {
            buffer[i++] = string_to_hex_to_dec("0x32");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            writeword(tmp2);
        }
        else if (s == "RLOAD")
        {
            buffer[i++] = string_to_hex_to_dec("0x33");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "RSTORE")
        {
            buffer[i++] = string_to_hex_to_dec("0x34");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "HALT")
        {
            buffer[i++] = string_to_hex_to_dec("0x40");
        }
        else if (s == "OUT")
        {
            buffer[i++] = string_to_hex_to_dec("0x41");
            in >> tmp1;
            buffer[i++] = string_to_hex_to_dec(tmp1);
        }
        else if (s == "NOP")
        {
            buffer[i++] = string_to_hex_to_dec("0x42");
        }
        else if (s == "PUSH")
        {
            buffer[i++] = string_to_hex_to_dec("0x50");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
        }
        else if (s == "PUSH_ALL")
        {
            buffer[i++] = string_to_hex_to_dec("0x51");
        }
        else if (s == "POP")
        {
            buffer[i++] = string_to_hex_to_dec("0x52");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
        }
        else if (s == "POP_ALL")
        {
            buffer[i++] = string_to_hex_to_dec("0x53");
        }
        else if (s == "LSP")
        {
            buffer[i++] = string_to_hex_to_dec("0x54");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
        }
    }

    FILE * write_ptr;

    write_ptr = fopen("C:/VM/out.bin", "wb");


    fwrite(buffer, sizeof(buffer), 1, write_ptr);

    fclose(write_ptr);

    in.close();
    in.clear();
    return ;
}
void Assemble1()
{
    i = 0;
    memset(buffer,0,sizeof(buffer));

    ifstream in("C:/VM/stop.txt");

    string s;
    string tmp1, tmp2;
    while (in >> s)
    {
        if (s == "ADD")
        {
            buffer[i++] = string_to_hex_to_dec("0x01");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
            in >> tmp2;
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "NEG")
        {
            buffer[i++] = string_to_hex_to_dec("0x02");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
        }
        else if (s == "MUL")
        {
            buffer[i++] = string_to_hex_to_dec("0x03");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
            in >> tmp2;
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "DIV")
        {
            buffer[i++] = string_to_hex_to_dec("0x04");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
            in >> tmp2;
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "MOD")
        {
            buffer[i++] = string_to_hex_to_dec("0x05");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
            in >> tmp2;
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "CMP")
        {
            buffer[i++] = string_to_hex_to_dec("0x10");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "JA")
        {
            buffer[i++] = string_to_hex_to_dec("0x11");
            in >> tmp1;
            writeword(tmp1);
        }
        else if (s == "JE")
        {
            buffer[i++] = string_to_hex_to_dec("0x12");
            in >> tmp1;
            writeword(tmp1);
        }
        else if (s == "JB")
        {
            buffer[i++] = string_to_hex_to_dec("0x13");
            in >> tmp1;
            writeword(tmp1);
        }
        else if (s == "JMP")
        {
            buffer[i++] = string_to_hex_to_dec("0x14");
            in >> tmp1;
            writeword(tmp1);
        }
        else if (s == "CALL")
        {
            buffer[i++] = string_to_hex_to_dec("0x20");
            in >> tmp1;
            writeword(tmp1);
        }
        else if (s == "RET")
        {
            buffer[i++] = string_to_hex_to_dec("0x21");
        }
        else if (s == "LOAD")
        {
            buffer[i++] = string_to_hex_to_dec("0x30");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            writeword(tmp2);
        }
        else if (s == "STORE")
        {
            buffer[i++] = string_to_hex_to_dec("0x31");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            writeword(tmp2);
        }
        else if (s == "CONST")
        {
            buffer[i++] = string_to_hex_to_dec("0x32");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            writeword(tmp2);
        }
        else if (s == "RLOAD")
        {
            buffer[i++] = string_to_hex_to_dec("0x33");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "RSTORE")
        {
            buffer[i++] = string_to_hex_to_dec("0x34");
            in >> tmp1;
            in >> tmp2;
            buffer[i++] = reg_id(tmp1);
            buffer[i++] = reg_id(tmp2);
        }
        else if (s == "HALT")
        {
            buffer[i++] = string_to_hex_to_dec("0x40");
        }
        else if (s == "OUT")
        {
            buffer[i++] = string_to_hex_to_dec("0x41");
            in >> tmp1;
            buffer[i++] = string_to_hex_to_dec(tmp1);
        }
        else if (s == "NOP")
        {
            buffer[i++] = string_to_hex_to_dec("0x42");
        }
        else if (s == "PUSH")
        {
            buffer[i++] = string_to_hex_to_dec("0x50");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
        }
        else if (s == "PUSH_ALL")
        {
            buffer[i++] = string_to_hex_to_dec("0x51");
        }
        else if (s == "POP")
        {
            buffer[i++] = string_to_hex_to_dec("0x52");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
        }
        else if (s == "POP_ALL")
        {
            buffer[i++] = string_to_hex_to_dec("0x53");
        }
        else if (s == "LSP")
        {
            buffer[i++] = string_to_hex_to_dec("0x54");
            in >> tmp1;
            buffer[i++] = reg_id(tmp1);
        }
    }

    FILE * write_ptr;

    write_ptr = fopen("C:/VM/out_stop.bin", "wb");


    fwrite(buffer, sizeof(buffer), 1, write_ptr);

    fclose(write_ptr);

    in.close();
    in.clear();
    return ;
}
