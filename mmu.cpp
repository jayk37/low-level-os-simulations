#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <limits>
#include <climits>
#include <queue>
#include <deque>
#include <iomanip>
#include <numeric>
#include <algorithm>
#include <getopt.h>
#define endl "\n"
using namespace std;

int ofs;
int randvals_length;
vector<int> randvals;

int myrandom(int burst)
{
    if(ofs == randvals_length)
    {
        ofs = 0;
    }
    int random_number = randvals[ofs];
    ofs += 1;
    return random_number % burst;
}

void readRandFile(const string& randfile)
{
    ifstream randfile_stream(randfile);
    if(!randfile_stream)
    {
        cerr << "Error Opening randfile." << endl;
        exit(1);
    }
    randfile_stream >> randvals_length;
    int value;
    while(randfile_stream >> value)
    {
        randvals.push_back(value);
    }
    randfile_stream.close();
}

int MAX_FRAMES = 128;
const int MAX_VPAGES = 64;
string input_file, random_file;

bool is_option_O = false;
bool is_option_P = false;
bool is_option_F = false;
bool is_option_S = false;
bool is_option_x = false;
bool is_option_y = false;
bool is_option_f = false;
bool is_option_a = false;

unsigned long COUNT_INSTRUCTIONS = 0;
unsigned long COUNT_PROCESS_EXITS = 0;
unsigned long COUNT_CONTEXT_SWITCHES = 0;
unsigned long long COST = 0;

struct VMA {
    int start_vpage;
    int end_vpage;
    int write_protected;
    int file_mapped;
};

struct PTE {
    unsigned int VALID : 1;
    unsigned int REFERENCED : 1;
    unsigned int MODIFIED : 1;
    unsigned int WRITE_PROTECT : 1;
    unsigned int PAGEDOUT : 1;
    unsigned int FRAMEMAP : 26;
    unsigned int FILEMAP : 1;
};

struct Process {
    int PID;
    unsigned long COUNT_SEGV;
    unsigned long COUNT_SEGPROT;
    unsigned long COUNT_UNMAP;
    unsigned long COUNT_MAP;
    unsigned long COUNT_IN;
    unsigned long COUNT_FIN;
    unsigned long COUNT_OUT;
    unsigned long COUNT_FOUT;
    unsigned long COUNT_ZERO;
    vector<VMA> VMA_List;
    vector<PTE> Page_Table;
    Process()
    {
        COUNT_SEGV = 0;
        COUNT_SEGPROT = 0;
        COUNT_UNMAP = 0;
        COUNT_MAP = 0;
        COUNT_IN = 0;
        COUNT_FIN = 0;
        COUNT_OUT = 0;
        COUNT_FOUT = 0;
        COUNT_ZERO = 0;
        Page_Table.resize(MAX_VPAGES);
        fill(Page_Table.begin(), Page_Table.end(), PTE{0});
    }
};

vector<Process> processes;
Process *current_process;

void reset_PAGE_TABLE_ENTRY(PTE& table_entry)
{
    table_entry.REFERENCED = 0;
    table_entry.MODIFIED = 0;
    table_entry.PAGEDOUT = 0;
}

struct FTE {
    unsigned int TIMER:32;
    unsigned int LASTUSE:32;
    int PROCESS_ID_MAP;
    int VPAGE_MAP;
    FTE()
    {
        TIMER = 0;
        LASTUSE = 0;
        PROCESS_ID_MAP = 1000;
        VPAGE_MAP = 1000;
    }
};

vector<FTE> frame_table;
deque<int> memory_frames;

void setup_HARDWARE_FRAMES()
{
    FTE default_value = FTE();
    frame_table.resize(MAX_FRAMES, default_value);
    memory_frames.resize(MAX_FRAMES);
    iota(memory_frames.begin(), memory_frames.end(), 0);
}

void reset_FRAME_TABLE_ENTRY(int index)
{
    frame_table[index].TIMER = 0;
    frame_table[index].LASTUSE = 0;
    frame_table[index].PROCESS_ID_MAP = 1000;
    frame_table[index].VPAGE_MAP = 1000;
}

class Pager {
public:
    virtual int select_victim_frame() = 0;
    int COUNTER;
    Pager () : COUNTER(0) {}
    void INCREMENT_COUNTER()
    {
        COUNTER += 1;
    }
};

class FIFO_Pager : public Pager {
private:
    int hand_index;
public:
    FIFO_Pager() : hand_index(0) {}
    int select_victim_frame() override 
    {
        int victim_frame;
        victim_frame = hand_index;
        hand_index = (hand_index + 1) % frame_table.size();
        return victim_frame;
    }
};

class Random_Pager : public Pager {
public:
    int select_victim_frame() override 
    {
        int victim_frame = myrandom(frame_table.size());
        return victim_frame;
    }
};

class CLOCK_Pager : public Pager {
private:
    int hand_index;
public:
    CLOCK_Pager() : hand_index(0) {}
    int select_victim_frame() override 
    {
        do
        {
            FTE& current_frame = frame_table[hand_index];
            int PROCESS_ID_MAP = current_frame.PROCESS_ID_MAP;
            int VPAGE_MAP = current_frame.VPAGE_MAP;
            PTE& table_entry = processes[PROCESS_ID_MAP].Page_Table[VPAGE_MAP];
            if(!table_entry.REFERENCED)
            {
                int victim_frame = hand_index;
                hand_index = (hand_index + 1) % frame_table.size();
                return victim_frame;
            }
            else
            {
                table_entry.REFERENCED &= ~1;
                hand_index = (hand_index + 1) % frame_table.size();
            }
        } while(true);
        return 0;
    }
};

class ESC_Pager : public Pager {
private:
    int hand_index;
public:
    ESC_Pager () : hand_index(0) {}
    int select_victim_frame() override 
    {
        int class_0 = -1, class_1 = -1, class_2 = -1, class_3 = -1;
        int* class_frames[] = { &class_0, &class_1, &class_2, &class_3 };
        bool check_threshold = (COUNTER >= 48);
        int frame_count = frame_table.size();
        for(int i = 0; i < frame_count; i++)
        {
            int current_frame = (hand_index + i) % frame_count;
            FTE& frame = frame_table[current_frame];
            int PROCESS_ID_MAP = frame.PROCESS_ID_MAP;
            int VPAGE_MAP = frame.VPAGE_MAP;
            if(PROCESS_ID_MAP == 1000)
            {
                continue;
            }
            PTE& table_entry = processes[PROCESS_ID_MAP].Page_Table[VPAGE_MAP];
            int class_index = 2 * table_entry.REFERENCED + table_entry.MODIFIED;
            if (class_index >= 0 && class_index <= 3 && *class_frames[class_index] == -1)
            {
                *class_frames[class_index] = current_frame;
            }
            if(check_threshold)
            {
                table_entry.REFERENCED = 0;
            }
            if(!check_threshold && class_0 != -1)
            {
                break;
            }
        }
        if(check_threshold)
        {
            COUNTER = 0;
        }
        int victim_frame = (class_0 != -1) ? class_0
                         : (class_1 != -1) ? class_1
                         : (class_2 != -1) ? class_2
                         : class_3;
        hand_index = (victim_frame + 1) % frame_count;
        return victim_frame;
    }
};

class Aging_Pager : public Pager {
private:
    int hand_index;
public:
    Aging_Pager() : hand_index(0) {}
    int select_victim_frame() override 
    {
        int victim_frame = 1000;
        int inspected_frames = 0;
        int frame_count = frame_table.size();
        unsigned int least_time = numeric_limits<unsigned int>::max();
        while(inspected_frames < frame_count)
        {
            int current_frame = (hand_index + inspected_frames) % frame_count;
            FTE& frame = frame_table[current_frame];
            int PROCESS_ID_MAP = frame.PROCESS_ID_MAP;
            int VPAGE_MAP = frame.VPAGE_MAP;
            if(PROCESS_ID_MAP == 1000)
            {
                inspected_frames += 1;
                continue;
            }
            PTE& table_entry = processes[PROCESS_ID_MAP].Page_Table[VPAGE_MAP];
            frame.TIMER = (frame.TIMER >> 1);
            if(table_entry.REFERENCED)
            {
                table_entry.REFERENCED &= ~1;
                frame.TIMER |= 0x80000000;
            }
            unsigned int check = least_time;
            least_time = min(least_time, frame.TIMER);
            victim_frame = (check > frame.TIMER) ? current_frame : victim_frame;
            inspected_frames += 1;
        }
        frame_table[victim_frame].TIMER = 0;
        hand_index = (victim_frame + 1) % frame_count;
        return victim_frame;
    }
};

class WS_Clock_Pager : public Pager {
private:
    int hand_index;
    unsigned int TAU;
public:
    WS_Clock_Pager() : hand_index(0), TAU(49) {}
    int select_victim_frame() override 
    {
        int victim_frame = 1000;
        unsigned int least_time = numeric_limits<unsigned int>::max();
        int frame_count = frame_table.size();
        int inspected_frames = 0;
        while(inspected_frames < frame_count)
        {
            int current_frame = (hand_index + inspected_frames) % frame_count;
            FTE& frame = frame_table[current_frame];
            int PROCESS_ID_MAP = frame.PROCESS_ID_MAP;
            int VPAGE_MAP = frame.VPAGE_MAP;
            if(PROCESS_ID_MAP == 1000)
            {
                inspected_frames += 1;
                continue;
            }
            PTE& table_entry = processes[PROCESS_ID_MAP].Page_Table[VPAGE_MAP];
            unsigned int time_since_active = COUNTER - frame.LASTUSE;
            if(table_entry.REFERENCED)
            {
                table_entry.REFERENCED &= ~1;
                frame.LASTUSE = COUNTER;
            }
            else if(TAU < time_since_active)
            {
                victim_frame = current_frame;
                break;
            }
            unsigned int check = least_time;
            least_time = min(least_time, frame.LASTUSE);
            victim_frame = (check > frame.LASTUSE) ? current_frame : victim_frame;
            inspected_frames += 1;
        }
        victim_frame = (victim_frame == 1000) ? hand_index : victim_frame;
        frame_table[victim_frame].LASTUSE = COUNTER;
        hand_index = (victim_frame + 1) % frame_count;
        return victim_frame;
    }
};

Pager* THE_PAGER = nullptr;

int get_frame()
{
    int physical_frame_number;
    if((int) memory_frames.size() == 0)
    {
        physical_frame_number = THE_PAGER->select_victim_frame();
        return physical_frame_number;
    }
    physical_frame_number = memory_frames.front();
    memory_frames.pop_front();
    return physical_frame_number;
}

void parse_arguments(int argc, char* argv[])
{
    char option;
    while ((option = getopt(argc, argv, "f:a:o:")) != -1)
    {
        switch(option)
        {
            case 'f':
                MAX_FRAMES = stoi(optarg);
                break;
            case 'a':
                switch(optarg[0])
                {
                    case 'f':
                        THE_PAGER = new FIFO_Pager();
                        break;
                    case 'r':
                        THE_PAGER = new Random_Pager();
                        break;
                    case 'c':
                        THE_PAGER = new CLOCK_Pager();
                        break;
                    case 'e':
                        THE_PAGER = new ESC_Pager();
                        break;
                    case 'a':
                        THE_PAGER = new Aging_Pager();
                        break;
                    case 'w':
                        THE_PAGER = new WS_Clock_Pager();
                        break;
                    default:
                        cerr << "Invalid Page Algorithm!" << endl;
                        exit(1);
                }
                break;
            case 'o':
                for(size_t i = 0; i < strlen(optarg); i++)
                {
                    switch(optarg[i])
                    {
                        case 'O':
                            is_option_O = true;
                            break;
                        case 'P':
                            is_option_P = true;
                            break;
                        case 'F':
                            is_option_F = true;
                            break;
                        case 'S':
                            is_option_S = true;
                            break;
                        case 'x':
                            is_option_x = true;
                            break;
                        case 'y':
                            is_option_y = true;
                            break;
                        case 'f':
                            is_option_f = true;
                            break;
                        case 'a':
                            is_option_a = true;
                            break;
                        default:
                            cerr << "Invalid Option in -o Argument!" << optarg[i] << endl;
                            exit(1);
                    }
                }
                break;
            default:
                cerr << "Invalid Argument!" << option << endl;
                exit(1);
        }
    }
    if(optind + 2 != argc)
    {
        cerr << "Missing inputfile or randfile!" << endl;
        exit(1);
    }
    input_file = argv[optind];
    random_file = argv[optind + 1];
}

void check_PARSING()
{
    for(const auto& process_iter: processes)
    {
        cout << "Process " << process_iter.PID << ":" << endl;
        for(const auto& vma_iter : process_iter.VMA_List)
        {
            cout << "  VMA: start=" << vma_iter.start_vpage << ", end=" << vma_iter.end_vpage << ", write_protected=" << vma_iter.write_protected << ", file_mapped=" << vma_iter.file_mapped << endl;
        }
        for(const auto& pte_iter : process_iter.Page_Table)
        {
            cout << "  PTE: VALID=" << pte_iter.VALID << ", REFERENCED=" << pte_iter.REFERENCED << ", MODIFIED=" << pte_iter.MODIFIED << ", WRITEPROTECT=" << pte_iter.WRITE_PROTECT << ", PAGEDOUT=" << pte_iter.PAGEDOUT << ", FRAMEMAP=" << pte_iter.FRAMEMAP << "\n";
        }
    }
}

void display_PAGE_TABLE()
{
    for(const auto& process : processes)
    {
        cout << "PT[" << process.PID << "]: ";
        int CURRENT_VPAGE = 0;
        while(CURRENT_VPAGE < MAX_VPAGES)
        {
            const auto& table_entry = process.Page_Table[CURRENT_VPAGE];
            bool in_vma = false;
            for(const auto& vma : process.VMA_List)
            {
                if(CURRENT_VPAGE >= vma.start_vpage && CURRENT_VPAGE <= vma.end_vpage)
                {
                    in_vma = true;
                    break;
                }
            }
            if(!in_vma)
            {
                cout << "*";
            }
            else if(!table_entry.VALID)
            {
                cout << (table_entry.PAGEDOUT && !table_entry.FILEMAP ? "#" : "*");
            }
            else
            {
                cout << CURRENT_VPAGE << ":"
                     << (table_entry.REFERENCED ? "R" : "-")
                     << (table_entry.MODIFIED ? "M" : "-")
                     << (table_entry.PAGEDOUT && !table_entry.FILEMAP ? "S" : "-");
            }
            if(CURRENT_VPAGE < MAX_VPAGES - 1)
            {
                cout << " ";
            }
            CURRENT_VPAGE += 1;
        }
        cout << endl;
    }
}

void display_CURRENT_PAGE_TABLE()
{
    if(current_process == nullptr)
    {
        cerr << "Error: No Active Page Table!" << endl;
        return;
    }
    cout << "PT[" << current_process->PID << "]: ";
    int CURRENT_VPAGE = 0;
    while(CURRENT_VPAGE < MAX_VPAGES)
    {
        const auto& table_entry = current_process->Page_Table[CURRENT_VPAGE];
        bool in_vma = false;
        for (const auto& vma : current_process->VMA_List)
        {
            if(CURRENT_VPAGE >= vma.start_vpage && CURRENT_VPAGE <= vma.end_vpage)
            {
                in_vma = true;
                break;
            }
        }
        if(!in_vma)
        {
            cout << "*";
        }
        else if(!table_entry.VALID)
        {
            cout << (table_entry.PAGEDOUT && !table_entry.FILEMAP ? "#" : "*");
        }
        else
        {
            cout << CURRENT_VPAGE << ":"
                 << (table_entry.REFERENCED ? "R" : "-")
                 << (table_entry.MODIFIED ? "M" : "-")
                 << (table_entry.PAGEDOUT && !table_entry.FILEMAP ? "S" : "-");
        }
        if(CURRENT_VPAGE < MAX_VPAGES - 1)
        {
            cout << " ";
        }
        CURRENT_VPAGE += 1;
    }
    cout << endl;
}

void display_FRAME_TABLE()
{
    cout << "FT: ";
    int CURRENT_FRAME = 0;
    int frame_count = frame_table.size();
    while(CURRENT_FRAME < frame_count)
    {
        const FTE& frame = frame_table[CURRENT_FRAME];
        cout << (frame.PROCESS_ID_MAP == 1000 ? "*" : to_string(frame.PROCESS_ID_MAP) + ":" + to_string(frame.VPAGE_MAP));
        if(CURRENT_FRAME + 1 != (int) frame_table.size())
        {
            cout << " ";
        }
        CURRENT_FRAME += 1;
    }
    cout << endl;
}

void display_SUMMARY()
{
    for(const auto& process : processes)
    {
        printf("PROC[%d]: U=%lu M=%lu I=%lu O=%lu FI=%lu FO=%lu Z=%lu SV=%lu SP=%lu\n",
               process.PID,
               process.COUNT_UNMAP,
               process.COUNT_MAP,
               process.COUNT_IN,
               process.COUNT_OUT,
               process.COUNT_FIN,
               process.COUNT_FOUT,
               process.COUNT_ZERO,
               process.COUNT_SEGV,
               process.COUNT_SEGPROT);
    }
    printf("TOTALCOST %lu %lu %lu %llu %lu\n",
           COUNT_INSTRUCTIONS,
           COUNT_CONTEXT_SWITCHES,
           COUNT_PROCESS_EXITS,
           COST,
           sizeof(PTE));
}

void context_switch(int PROCESS_ID)
{
    COUNT_CONTEXT_SWITCHES += 1;
    current_process = &processes[PROCESS_ID];
}

void clear_CACHE_FAULT(int CACHE_FRAME)
{
    int CACHE_PROCESS = frame_table[CACHE_FRAME].PROCESS_ID_MAP;
    int CACHE_VPAGE = frame_table[CACHE_FRAME].VPAGE_MAP;
    PTE& table_entry = processes[CACHE_PROCESS].Page_Table[CACHE_VPAGE];
    if(!table_entry.VALID)
    {
        return;
    }
    table_entry.VALID &= ~1;
    table_entry.REFERENCED = 0;
    processes[CACHE_PROCESS].COUNT_UNMAP += 1;
    COST += 410;
    if(is_option_O)
    {
        cout << " UNMAP " << CACHE_PROCESS << ":" << CACHE_VPAGE << endl;
    }
    if(!table_entry.MODIFIED)
    {
        return;
    }
    table_entry.MODIFIED &= ~1;
    table_entry.PAGEDOUT = 1;
    bool is_filemap = table_entry.FILEMAP;
    processes[CACHE_PROCESS].COUNT_FOUT += is_filemap ? 1 : 0;
    processes[CACHE_PROCESS].COUNT_OUT += is_filemap ? 0 : 1;
    COST += is_filemap ? 2800 : 2750;
    if(is_option_O)
    {
        cout << (is_filemap ? " FOUT" : " OUT") << endl;
    }
}

int clear_CACHE_EXIT(int CACHE_VPAGE)
{
    int CACHE_PROCESS = current_process->PID;
    PTE& table_entry = processes[CACHE_PROCESS].Page_Table[CACHE_VPAGE];
    int index = table_entry.FRAMEMAP;
    if(!table_entry.VALID)
    {
        table_entry.PAGEDOUT = 0;
        return 1000;
    }
    table_entry.VALID &= ~1;
    table_entry.REFERENCED = 0;
    processes[CACHE_PROCESS].COUNT_UNMAP += 1;
    COST += 410;
    if(is_option_O)
    {
        cout << " UNMAP " << CACHE_PROCESS << ":" << CACHE_VPAGE << endl;
    }
    if(!table_entry.MODIFIED)
    {
        reset_PAGE_TABLE_ENTRY(table_entry);
        return index;
    }
    table_entry.MODIFIED &= ~1;
    table_entry.PAGEDOUT = 1;
    bool is_filemap = table_entry.FILEMAP;
    processes[CACHE_PROCESS].COUNT_FOUT += is_filemap ? 1 : 0;
    COST += is_filemap ? 2800 : 0;
    if(is_option_O && is_filemap)
    {
        cout << " FOUT" << endl;
    }
    reset_PAGE_TABLE_ENTRY(table_entry);
    return index;
}

void read_FAULT(int VPAGE)
{
    PTE& table_entry = current_process->Page_Table[VPAGE];
    if(table_entry.VALID)
    {
        table_entry.REFERENCED = 1;
        return;
    }
    bool VALID_IN_VMA = false;
    for(const auto& vma : current_process->VMA_List)
    {
        if (VPAGE >= vma.start_vpage && VPAGE <= vma.end_vpage)
        {
            VALID_IN_VMA = true;
            table_entry.WRITE_PROTECT = vma.write_protected;
            table_entry.FILEMAP = vma.file_mapped;
            break;
        }
    }
    if(!VALID_IN_VMA)
    {
        cout << " SEGV\n";
        current_process->COUNT_SEGV += 1;
        COST += 440;
        return;
    }
    int physical_frame_number = get_frame();
    int CACHE_PROCESS = frame_table[physical_frame_number].PROCESS_ID_MAP;
    int CACHE_VPAGE = frame_table[physical_frame_number].VPAGE_MAP;
    if(CACHE_PROCESS != 1000 || CACHE_VPAGE != 1000)
    {
        clear_CACHE_FAULT(physical_frame_number);
    }
    frame_table[physical_frame_number].PROCESS_ID_MAP = current_process->PID;
    frame_table[physical_frame_number].VPAGE_MAP = VPAGE;
    table_entry.FRAMEMAP = physical_frame_number;
    table_entry.VALID = 1;
    bool is_filemap = table_entry.FILEMAP;
    bool is_pagedout = table_entry.PAGEDOUT;
    COST += is_filemap ? 2350 : (is_pagedout ? 3200 : 150);
    if(is_option_O)
    {
        cout << (is_filemap ? " FIN\n" : (is_pagedout ? " IN\n" : " ZERO\n"));
    }
    current_process->COUNT_FIN += is_filemap ? 1 : 0;
    current_process->COUNT_IN += (!is_filemap && is_pagedout) ? 1 : 0;
    current_process->COUNT_ZERO += (!is_filemap && !is_pagedout) ? 1 : 0;
    if(is_option_O)
    {
        cout << " MAP " << physical_frame_number << "\n";
    }
    current_process->COUNT_MAP += 1;
    COST += 350;
    table_entry.REFERENCED = 1;
}

void write_FAULT(int VPAGE)
{
    PTE& table_entry = current_process->Page_Table[VPAGE];
    if(table_entry.VALID)
    {
        table_entry.REFERENCED = 1;
        COST += table_entry.WRITE_PROTECT ? 410 : 0;
        current_process->COUNT_SEGPROT += table_entry.WRITE_PROTECT ? 1 : 0;
        table_entry.MODIFIED = table_entry.WRITE_PROTECT ? table_entry.MODIFIED : 1;
        if(table_entry.WRITE_PROTECT)
        {
            cout << " SEGPROT\n";
        }
        return;
    }
    bool VALID_IN_VMA = false;
    for(const auto& vma : current_process->VMA_List)
    {
        if (VPAGE >= vma.start_vpage && VPAGE <= vma.end_vpage)
        {
            VALID_IN_VMA = true;
            table_entry.WRITE_PROTECT = vma.write_protected;
            table_entry.FILEMAP = vma.file_mapped;
            break;
        }
    }
    if(!VALID_IN_VMA)
    {
        cout << " SEGV\n";
        current_process->COUNT_SEGV += 1;
        COST += 440;
        return;
    }
    int physical_frame_number = get_frame();
    int CACHE_PROCESS = frame_table[physical_frame_number].PROCESS_ID_MAP;
    int CACHE_VPAGE = frame_table[physical_frame_number].VPAGE_MAP;
    if(CACHE_PROCESS != 1000 || CACHE_VPAGE != 1000)
    {
        clear_CACHE_FAULT(physical_frame_number);
    }
    frame_table[physical_frame_number].PROCESS_ID_MAP = current_process->PID;
    frame_table[physical_frame_number].VPAGE_MAP = VPAGE;
    table_entry.FRAMEMAP = physical_frame_number;
    table_entry.VALID = 1;
    bool is_filemap = table_entry.FILEMAP;
    bool is_pagedout = table_entry.PAGEDOUT;
    COST += is_filemap ? 2350 : (is_pagedout ? 3200 : 150);
    if(is_option_O)
    {
        cout << (is_filemap ? " FIN\n" : (is_pagedout ? " IN\n" : " ZERO\n"));
    }
    current_process->COUNT_FIN += is_filemap ? 1 : 0;
    current_process->COUNT_IN += (!is_filemap && is_pagedout) ? 1 : 0;
    current_process->COUNT_ZERO += (!is_filemap && !is_pagedout) ? 1 : 0;
    if(is_option_O)
    {
        cout << " MAP " << physical_frame_number << "\n";
    }
    current_process->COUNT_MAP += 1;
    COST += 350;
    table_entry.REFERENCED = 1;
    COST += table_entry.WRITE_PROTECT ? 410 : 0;
    current_process->COUNT_SEGPROT += table_entry.WRITE_PROTECT ? 1 : 0;
    table_entry.MODIFIED = table_entry.WRITE_PROTECT ? table_entry.MODIFIED : 1;
    if(table_entry.WRITE_PROTECT)
    {
        cout << " SEGPROT\n";
    }
}

void process_exit()
{
    COUNT_PROCESS_EXITS += 1;
    int CACHE_PROCESS = current_process->PID;
    cout << "EXIT current process " << CACHE_PROCESS << endl;
    int CURRENT_VPAGE = 0;
    while(CURRENT_VPAGE < MAX_VPAGES)
    {
        int index = clear_CACHE_EXIT(CURRENT_VPAGE);
        if(index != 1000)
        {
            reset_FRAME_TABLE_ENTRY(index);
            memory_frames.push_back(index);
        }
        CURRENT_VPAGE += 1;
    }
}

int main(int argc, char* argv[])
{
    int COUNT_PROCESS = 0;
    bool parsing_VMAs = true;
    parse_arguments(argc, argv);
    readRandFile(random_file);
    string buffer;
    ifstream inputfile_stream(input_file);
    if(!inputfile_stream)
    {
        cerr << "Error Opening File: " << input_file << endl;
        exit(1);
    }
    while(getline(inputfile_stream, buffer))
    {
        if(buffer.empty() || buffer[0] == '#')
        {
            continue;
        }
        if(parsing_VMAs)
        {
            if(COUNT_PROCESS == 0)
            {
                COUNT_PROCESS = stoi(buffer);
                continue;
            }
            if(isdigit(buffer[0]))
            {
                int num_vmas = stoi(buffer);
                Process current_process = Process();
                current_process.PID = (int) processes.size();
                for (int i = 0; i < num_vmas; i++)
                {
                    getline(inputfile_stream, buffer);
                    VMA current_vma;
                    sscanf(buffer.c_str(), "%d %d %d %d", &current_vma.start_vpage, &current_vma.end_vpage, &current_vma.write_protected, &current_vma.file_mapped);
                    current_process.VMA_List.push_back(current_vma);
                }
                processes.push_back(current_process);
                COUNT_PROCESS -= 1;
                if(COUNT_PROCESS == 0)
                {
                    parsing_VMAs = false;
                    break;
                }
                continue;
            }
        }
    }
    COUNT_PROCESS = (int) processes.size();
    setup_HARDWARE_FRAMES();
    while(getline(inputfile_stream, buffer))
    {
        char OPERATION;
        int PAGE_PROCID;
        if(buffer.empty() || buffer[0] == '#')
        {
            continue;
        }
        sscanf(buffer.c_str(), "%c %d", &OPERATION, &PAGE_PROCID);
        if(is_option_O)
        {
            cout << COUNT_INSTRUCTIONS << ": ==> " << OPERATION << " " << PAGE_PROCID << endl;
        }
        COUNT_INSTRUCTIONS += 1;
        THE_PAGER->INCREMENT_COUNTER();
        switch(OPERATION)
        {
            case 'c':
            {
                COST += 130;
                int PROCESS_ID = PAGE_PROCID;
                context_switch(PROCESS_ID);
                break;
            }
            case 'r':
            {
                COST += 1;
                read_FAULT(PAGE_PROCID);
                break;
            }
            case 'w':
            {
                COST += 1;
                write_FAULT(PAGE_PROCID);
                break;
            }
            case 'e':
            {
                COST += 1230;
                process_exit();
                break;
            }
            default:
                cerr << "Invalid Operation: " << OPERATION << endl;
        }
        if(is_option_x)
        {
            display_CURRENT_PAGE_TABLE();
        }
        if(is_option_y)
        {
            display_PAGE_TABLE();
        }
        if(is_option_f)
        {
            display_FRAME_TABLE();
        }
    }
    inputfile_stream.close();
    if(is_option_P)
    {
        display_PAGE_TABLE();
    }
    if(is_option_F)
    {
        display_FRAME_TABLE();
    }
    if(is_option_S)
    {
        display_SUMMARY();
    }
    return 0;
}


