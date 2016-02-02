#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <sstream>
#include <bitset>
using namespace std;

#define BYTE  1
#define WORD  2
#define DWORD 4

const char* cstr_registers[] = 
	{
		"AL", "AX",
		"CL", "CX",
		"DL", "DX",
		"BL", "BX",
		"AH", "SP",
		"CH", "BP",
		"DH", "SI",
		"BH", "DI"
	};

const char* cstr_values[] = 
	{
		"000",
		"001",
		"010",
		"011",
		"100",
		"101",
		"110",
		"111"
	};
const char* cstr_rm_values[] = 
	{
		"BX+SI",
		"BX+DI",
		"BP+SI",
		"BP+DI",
		"SI",
		"DI",
		"BP",
		"BX"
	};
map<string, string> registers;


vector<string> split(const string& , char );


class Compiler
{
	public: //быстра€ инициализаци€
	Compiler(const string& str)  : opcode("100010"), is_error(false)
	{ 		
		if((str.find("mov") < 0) || (str.find("MOV") < 0))
		{
			cout << "Mov wasn't found." << endl;
			return;
		}
		string operands = str.substr(4);
		vector<string> operands_v = split(operands, ',');//делим все на операнды:)
		leftOp = operands_v[0];
		rightOp = operands_v[1];
		transform(leftOp.begin(), leftOp.end(), leftOp.begin(), toupper);//убеждаемс€ в том, что операнд был написан большими буквами
		transform(rightOp.begin(), rightOp.end(), rightOp.begin(), toupper);//если он написан маленькими - преобразовываем в большие:) 3
		disp.dispH = 0x0000;

/*#ifdef _DEBUG
		cout << leftOp << " " << rightOp << endl; 
#endif*/
	}

	void compile() 
	{
		string reg_name;
		if(is_register(rightOp))
		{
			d = 0;
			reg_name = rightOp;
		}
		else 
		{
			d = 1;
			reg_name = leftOp;
		}

		try
		{
			w = (register_size(reg_name) == WORD) ? 0 : 1;

			if(d)//если не регистр - мен€ем местами
			{
				leftOp = rightOp;
				rightOp = reg_name;
			}
			reg = registers[reg_name];//определ€ем что у нас –≈√

			bool op_is_memory = normalize((reg_name == leftOp) ? rightOp : leftOp);
			if(!op_is_memory)
			{
				mod = "11";
				r_m = registers[op_dispatches[0]];
				if(r_m == "err") throw exception("Wrong operand");
				return;
			}
		
			if(!is_number(op_dispatches[op_dispatches.size()-1]))
				mod = "00";
			else
			{
				short number = atoi((op_dispatches.end()-1)->c_str());
				if(number > 127 || number < -128)
				{
					mod = "10";
					disp.dispH = number;
				}
				else
				{
					mod = "01";
					disp.dispL = number;
				}
			}

			string tmp = "";
			if(op_dispatches.size() > 1)//готовим тмп дл€ поиска и проверки по третьей табл
			{
				for_each(op_dispatches.begin(), op_dispatches.end()-1, [&](const string& str) { tmp += str + '+'; } );
				tmp.resize(tmp.size()-1);
			}
			else tmp = op_dispatches[0];
			r_m = r_m_value(tmp);
			if(r_m == "err") throw exception("Wrong operand");
		}
		catch(exception &ex)
		{
			cout << "Error occurred." << endl;
			cout << ex.what();
		}
	}

	void write_to_text_file(const string& filename)
	{
		string tmp = opcode + ((d) ? "1" : "0") + ((w) ? "1" : "0") + " " + mod + reg + r_m;
		ofstream f(filename, ofstream::app);
		f << tmp;
		//cout<<tmp<<endl;
		if (disp.dispH){
			std::bitset<8> x(disp.dispH);
			f << " " << x;
		}
			
		f << endl;
		f.close();
	}

	void write_to_binary_file(const string& filename)
	{
		short binary_bytes = 0x0000;
		string tmp = opcode + ((d) ? "1" : "0") + ((w) ? "1" : "0") + mod + reg + r_m;
		for(auto i = 0; i < tmp.size(); i++)
		{
			if(tmp[i] == '1')
				binary_bytes |= 0x0001 << tmp.size()-1-i;
		}
		short disp_bytes = 0x0000 | disp.dispH;

		FILE* file = fopen(filename.c_str(), "a");
		char secondByte = binary_bytes;
		char firstByte = binary_bytes >> 8;  
		fwrite(&firstByte, 1, 1, file);
		fwrite(&secondByte, 1, 1, file);
		fwrite(&disp_bytes, 1, 2, file);
		fclose(file);
	}

	private:
	template<typename T>
	string to_string(T value)
	{
		stringstream ss;
		ss << value;
		return ss.str();
	}

	bool is_register(const string& str)//проверка на зарезервированный регистр
	{
		if(registers.find(str) != registers.end())
			return true;
		return false;
	}

	short register_size(const string& name)
	{
		auto index = 0;
		auto n = sizeof(cstr_registers) / sizeof(WORD);
		for(auto i = 0; i < n; i++)
		{
			if(cstr_registers[i] == name.c_str())//узнаем индекс регистра нэйм
			{
				index = i;
				break;
			}
		}
		if(index % 2) //если индекс делитс€ без остатка, значит это слово(втора€ колонка в первой таблице) 
			return WORD;
		else
			return BYTE;
	}

	string r_m_value(const string& expr)//проверка на зарезервированные штуки в 3 таблице
	{
		const char* conv = expr.c_str();
		for(auto i = 0; i < 8; i++)
		{
			if(!strcmp(conv, cstr_rm_values[i]))
				return cstr_values[i];
		}
		return "err";
	}

	
	bool normalize(string& str)
	{
		if(!(str.find('[') != string::npos && str.find(']') != string::npos))
		{
			op_dispatches.push_back(str);
			return false;
		}

		str.erase(std::remove(str.begin(), str.end(), '['), str.end());
		str.erase(std::remove(str.begin(), str.end(), ']'), str.end());
		op_dispatches = ::split(str, '+');
		return true;
	}

	bool is_number(const std::string& s)
	{
    return !s.empty() && std::find_if(s.begin(), 
		s.end(), [](char c) { return !isdigit(c); }) == s.end();
	}

private:
	string leftOp;
	string rightOp;
	vector<string> op_dispatches; //
	bool is_error;

	const string opcode;//команды ћ”¬
	
	bool d;		
	bool w;		
	string mod;	
	string reg;	
	string r_m;	
	union
	{
		char dispL;
		short dispH;
	} disp;
};

int main()
{	
	//присваеваем каждому регистру(перва€ таблица) его значение(втора€)
	for(auto i = 0, j = -1; i < sizeof(cstr_registers) / sizeof(WORD); i++)
	{
		if(i % 2 == 0)
			j++;
		registers.insert(pair<string, string>(cstr_registers[i], cstr_values[j]));
	}

	Compiler* compiler = nullptr;

	{

		fstream f("source.txt", fstream::in);
		vector<string> code; 
		if(f.fail())
			throw exception("File not found exception.");
		string tmp = "";
		getline(f, tmp);
		//cout<<tmp<<endl;
		code.push_back(tmp);
		while(!f.eof())
		{
			getline(f, tmp);
			//cout<<tmp<<endl;
			code.push_back(tmp);
		}
		cout << "File was read. Wait until compiles..." << endl;
		for(size_t i = 0; i < code.size(); i++)
		{
			compiler = new Compiler(code[i]);
			compiler->compile();
			compiler->write_to_binary_file("RESULT.com");
			compiler->write_to_text_file("RESULT.txt");
		}
		_sleep(1000);
		cout << "File was written." << endl;
	}


	if(compiler != nullptr)
	{
		delete compiler;
		compiler = nullptr;
	}

	cin.ignore();
	cin.get();
	return 0;
}

vector<string> split(const string& str, char seperator)
{
	vector<string> v;
	int index(0);
	int offset(0);

	while((index = str.find(seperator, offset)) >= 0)
	{
		v.push_back(str.substr(offset, index));
		offset = index+1;
	}
	v.push_back(str.substr(offset));
	return v;
}