#include<iostream>
#include<string>
#include<vector>
#include<map>
#include<fstream>
#include<cstring>
#include<cctype>
#include<cstdlib>
using namespace std;

// Global Variables for Tokenizer
ifstream input_data;
int linenum = 0;
int lineoffset = 0;
char line_buffer[4096];
const char* delimiters = " \t\n";
bool read_line = true;
int eof_offset = 0;

// Global Variables for Symbol Table and Module Base Table
vector<int> module_base_table;
struct Symbol {
    string name;
    int absolute_address;
    bool redefined;
    int module_num;
};
vector<Symbol> symbol_table;

// Helper Function to Add Symbol to Symbol Table
void createsymbol(const string& symbol, int relative_address, int module_base, int module_num)
{
    int absolute_address = module_base + relative_address;
    for(auto& sym:symbol_table)
    {
        if(sym.name == symbol)
        {
            sym.redefined = true;
            return;
        }
    }
    symbol_table.push_back({symbol, absolute_address, false, module_num});
}

// Parse Error Function as Indicated by Professor
void __parseerror(int errcode)
{
    const char* errstr[] = {
        "TOO_MANY_DEF_IN_MODULE",
        "TOO_MANY_USE_IN_MODULE",
        "TOO_MANY_INSTR",
        "NUM_EXPECTED",
        "SYM_EXPECTED",
        "MARIE_EXPECTED",
        "SYM_TOO_LONG",
    };
    printf("Parse Error line %d offset %d: %s\n", linenum, lineoffset, errstr[errcode]);
    exit(1);
}

// Main Tokenizer Function
char* getToken()
{
    char* result = nullptr;

    // Reading Tokens
    if(!read_line)
    {
        result = strtok(NULL, delimiters);
        read_line = (result == NULL);
        // Set Read Line if Line Ending Reached
    }

    // Reading Line
    while(read_line)
    {
        input_data.getline(line_buffer, sizeof(line_buffer));

        if(!input_data.fail())
        {
            linenum += 1;
            eof_offset = strlen(line_buffer) + 1;
        }
        result = strtok(line_buffer, delimiters);
        read_line = (result != NULL) ? 0 : 1;
        // Unset Read Line if Valid Line Found

        if(input_data.eof())
        {
            break;
        }
    }

    // Setting Offset for Token, EOF & EOF with Delimiters
    if(input_data.eof() && strlen(line_buffer) == 0)
    {
        lineoffset = eof_offset;
    }
    else if(input_data.eof() && strspn(line_buffer, delimiters) == strlen(line_buffer))
    {
        lineoffset = eof_offset - 1;
    }
    else
    {
        lineoffset = result - line_buffer + 1;
    }

    return result;
}

// Integer Layer Function over Tokenizer
int readInt()
{
    char* token = getToken();

    if(token == NULL)
    {
        return -1;
    }
    if(strlen(token) > 30)
    {
        __parseerror(3);
    }
    for(int i = 0; token[i] != '\0'; i++)
    {
        if(!isdigit(token[i]))
        {
            __parseerror(3);
        }
    }
    int value = atoi(token);
    return value;
}

// Symbol Layer Function over Tokenizer
string readSymbol()
{

    char* token = getToken();
    if(token == NULL)
    {
        __parseerror(4);
    }
    if(!isalpha(token[0]))
    {
        __parseerror(4);
    }
    if(strlen(token) > 16)
    {
        __parseerror(6);
    }
    for(int i = 1; token[i] != '\0'; i++)
    {
        if(!isalnum(token[i]))
        {
            __parseerror(4);
        }
    }
    return string(token);
}

// Instructions Layer Function over Tokenizer
char readMARIE()
{
    char* token = getToken();
    if(token == NULL)
    {
        __parseerror(5);
    }
    if (strlen(token) != 1 || (token[0] != 'M' && token[0] != 'A' && token[0] != 'R' && token[0] != 'I' && token[0] != 'E'))
    {
        __parseerror(5);
    }
    return token[0];
}

// Main Pass One Function
void pass1()
{
    int module_number = 0;
    int current_instruction_count = 0;
    int current_base_address = 0;
    vector<pair<string, int>> deflist;
    map<string, int> deflist_first_occurence;

    while(true)
    {
        deflist.clear();
        deflist_first_occurence.clear();

        int defcount = readInt();
        if(defcount < 0)
        {
            break;
        }
        if(defcount > 16)
        {
            __parseerror(0);
        }
        for(int i = 0; i < defcount; i++)
        {
            string sym = readSymbol();
            int val = readInt();
            deflist.push_back({sym, val});
            deflist_first_occurence[sym] = -1;
            createsymbol(sym, val, current_base_address, module_number);
        }

        int usecount = readInt();
        if(usecount > 16)
        {
            __parseerror(1);
        }
        for(int i = 0; i < usecount; i++)
        {
            string sym = readSymbol();
        }

        int instcount = readInt();
        current_instruction_count += instcount;
        if(current_instruction_count > 512)
        {
            __parseerror(2);
        }
        
        module_base_table.push_back(current_base_address);

        for(int i = 0; i < instcount; i++)
        {
            char addressmode = readMARIE();
            int operand = readInt();
        }

        for(int i = 0; i < (int) deflist.size(); i++)
        {
            if(deflist_first_occurence[deflist[i].first] == -1)
            {
                deflist_first_occurence[deflist[i].first] = i;
            }
        }

        int counter_deflist = 0;
        for(const auto& entry: deflist)
        {
            const string& sym = entry.first;
            int relative_address = entry.second;
            bool check_if_symbol_og = false;

            for(auto &sym_entry: symbol_table)
            {
                if(sym_entry.name == sym && sym_entry.module_num == module_number)
                {
                    int correct_relative_address = sym_entry.absolute_address - module_base_table[sym_entry.module_num];
                    if(relative_address == correct_relative_address && deflist_first_occurence[sym] == counter_deflist)
                    {
                        check_if_symbol_og = true;
                        break;
                    }
                }
            }
            if (relative_address >= instcount && check_if_symbol_og == true)
            {
                printf("Warning: Module %d: %s=%d valid=[0..%d] assume zero relative\n", module_number, sym.c_str(), relative_address, instcount - 1);
                relative_address = 0;
                for (auto &sym_entry: symbol_table)
                {
                    if(sym_entry.name == sym)
                    {
                        sym_entry.absolute_address = current_base_address + relative_address;
                        break;
                    }
                }
            }
            if(check_if_symbol_og == false)
            {
                printf("Warning: Module %d: %s redefinition ignored\n", module_number, sym.c_str());
            }
            counter_deflist += 1;
        }
        module_number += 1;
        current_base_address += instcount;
    }
}

// Helper Function to Print Symbol Table
void printSymbolTable()
{
    cout << "Symbol Table" << endl;
    for(auto& sym_entry : symbol_table)
    {
        cout << sym_entry.name << "=" << sym_entry.absolute_address;
        if(sym_entry.redefined)
        {
            cout << " Error: This variable is multiple times defined; first value used";
        }
        cout << endl;
    }
    cout << endl;
}

// Helper Function to Reinitialize Variables for Pass Two
void reinitialize()
{
    linenum = 0;
    lineoffset = 0;
    read_line = true;
    eof_offset = 0;
}

// Pass Two Error Function (Similar to Professor's Advice for Parse Errors Helper)
void __pass2error(int errcode, string sym = "")
{
    const char* errstr_2[] = {
        "is not defined; zero used",
        "Error: External operand exceeds length of uselist; treated as relative=0",
        "Error: Absolute address exceeds machine size; zero used",
        "Error: Relative address exceeds module size; relative zero used",
        "Error: Illegal immediate operand; treated as 999",
        "Error: Illegal opcode; treated as 9999",
        "Error: Illegal module operand ; treated as module=0",
    };
    if(errcode == 0)
    {
        printf("Error: %s %s\n", sym.c_str(), errstr_2[errcode]);
        return;
    }
    printf("%s\n", errstr_2[errcode]);
}

// Main Pass Two Function
void pass2()
{
    int current_base_address = 0;
    int module_number = 0;
    int current_instruction_count = 0;
    vector<pair<string, int>> deflist;
    vector<pair<string, bool>> uselist;
    int memory_map_key = 0;
    
    map<string, bool> used_throughout;
    for(auto &sym_entry : symbol_table)
    {
        used_throughout[sym_entry.name] = false;
    }

    cout << "Memory Map" << endl;
    while(true)
    {
        deflist.clear();
        uselist.clear();

        int defcount = readInt();
        if(defcount < 0)
        {
            break;
        }
        for(int i = 0; i < defcount; i++)
        {
            string sym = readSymbol();
            int val = readInt();
            deflist.push_back({sym, val});
        }

        int usecount = readInt();
        for(int i = 0; i < usecount; i++)
        {
            string sym = readSymbol();
            uselist.push_back({sym, false});
        }

        int instcount = readInt();
        current_instruction_count += instcount;
        for(int i = 0; i < instcount; i++)
        {
            char addressmode = readMARIE();
            int instruction = readInt();

            int opcode = instruction / 1000;
            int operand = instruction % 1000;

            if(opcode >= 10)
            {
                opcode = 9;
                operand = 999;
                int answer = (opcode * 1000) + operand;
                printf("%03d: %04d ", memory_map_key, answer);
                __pass2error(5);
                memory_map_key += 1;
                continue;
            }

            if(addressmode == 'M')
            {
                if(operand >= (int) module_base_table.size())
                {
                    operand = module_base_table[0];
                    int answer = (opcode * 1000) + operand;
                    printf("%03d: %04d ", memory_map_key, answer);
                    __pass2error(6);
                }
                else
                {
                    operand = module_base_table[operand];
                    int answer = (opcode * 1000) + operand;
                    printf("%03d: %04d\n", memory_map_key, answer);
                }
                memory_map_key += 1;
                continue;
            }

            else if(addressmode == 'A')
            {
                if(operand >= 512)
                {
                    operand = 0;
                    int answer = (opcode * 1000) + operand;
                    printf("%03d: %04d ", memory_map_key, answer);
                    __pass2error(2);
                }
                else
                {
                    int answer = (opcode * 1000) + operand;
                    printf("%03d: %04d\n", memory_map_key, answer);
                }
                memory_map_key += 1;
                continue;
            }
            
            else if(addressmode == 'R')
            {
                if(operand >= instcount)
                {
                    operand = 0;
                    operand += module_base_table[module_number];
                    int answer = (opcode * 1000) + operand;
                    printf("%03d: %04d ", memory_map_key, answer);
                    __pass2error(3);
                }
                else
                {
                    operand += module_base_table[module_number];
                    int answer = (opcode * 1000) + operand;
                    printf("%03d: %04d\n", memory_map_key, answer);
                }
                memory_map_key += 1;
                continue;
            }

            else if(addressmode == 'I')
            {
                if(operand >= 900)
                {
                    operand = 999;
                    int answer = (opcode * 1000) + operand;
                    printf("%03d: %04d ", memory_map_key, answer);
                    __pass2error(4);
                }
                else
                {
                    int answer = (opcode * 1000) + operand;
                    printf("%03d: %04d\n", memory_map_key, answer);
                }
                memory_map_key += 1;
                continue;
            }

            else if(addressmode == 'E')
            {
                if(operand >= uselist.size())
                {
                    operand = 0;
                    operand += module_base_table[module_number];
                    int answer = (opcode * 1000) + operand;
                    printf("%03d: %04d ", memory_map_key, answer);
                    __pass2error(1);
                    memory_map_key += 1;
                    continue;
                }

                string contention_symbol = uselist[operand].first;
                uselist[operand].second = true;

                bool check_if_symbol_defined = false;
                for(auto &sym_entry : symbol_table)
                {
                    if(sym_entry.name == contention_symbol)
                    {
                        check_if_symbol_defined = true;
                        break;
                    }
                }

                if(check_if_symbol_defined == true)
                {
                    used_throughout[contention_symbol] = true;
                }

                if(check_if_symbol_defined == false)
                {
                    operand = 0;
                    int answer = (opcode * 1000) + operand;
                    printf("%03d: %04d ", memory_map_key, answer);
                    __pass2error(0, contention_symbol);
                    memory_map_key += 1;
                    continue;
                }
                
                int symbol_address = 0;
                for(auto &sym_entry : symbol_table)
                {
                    if(sym_entry.name == contention_symbol)
                    {
                        symbol_address = sym_entry.absolute_address;
                        break;
                    }
                }
                operand = symbol_address;
                int answer = (opcode * 1000) + operand;
                printf("%03d: %04d\n", memory_map_key, answer);
                memory_map_key += 1;
                continue;
            }
        }
        
        for(int iter = 0; iter < (int) uselist.size(); iter++)
        {
            if(uselist[iter].second == false)
            {
                printf("Warning: Module %d: uselist[%d]=%s was not used\n", module_number, iter, uselist[iter].first.c_str());
            }
        }
        module_number += 1;
        current_base_address += instcount;
    }

    cout << endl;
    for(auto &sym_entry : symbol_table)
    {
        if(used_throughout[sym_entry.name] == false)
        {
            printf("Warning: Module %d: %s was defined but never used\n", sym_entry.module_num, sym_entry.name.c_str());
        }
    }
}

int main(int argc, char* argv[])
{

    input_data.open(argv[1]);
    pass1();
    input_data.close();
    printSymbolTable();
    reinitialize();
    input_data.open(argv[1]);
    pass2();
    input_data.close();

    // Below is the Code to Check Tokenizer
    // char* token;
    // while ((token = getToken()) != NULL) {
    //     cout << "Token: " << linenum << ":" << lineoffset << " : " << token << endl;
    // }
    // cout << "Final Spot in File : line=" << linenum << " offset=" << lineoffset << endl;
    
    return 0;
}



