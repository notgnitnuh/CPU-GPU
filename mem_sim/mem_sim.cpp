#include "mem_sim.h"

int main()
{
    size_t num_procs = 0;
    size_t mem_size, mm_policy, mm_param;
    string file_name;
    vector<process> procs; 

    if(!OutputPrompt(mem_size, mm_policy, mm_param, file_name))
    {
        cout << "Error with input" << endl;
        return 1;
    }
    if(!ReadWorkloadFile(file_name, num_procs, procs))
    {
        cout << "Error reading workload file" << endl;
        return 1;
    }

    RunProcesses(mem_size, mm_param, num_procs, mm_policy, procs);
    PrintTurnaround(procs);

    return 0;
}

bool RunProcesses(const size_t& mem_size, const size_t &mm_param, const size_t& num_procs, const size_t& mm_policy, vector<process>& procs)
{
    vector<mem_block> memory;
    vector<process> input_queue; 
    bool all_procs_done = false;
    size_t next_event, sys_clock = 0;
    string output_str; 
    mem_block trash;

    // If Paging, set up page table. 
    if(mm_policy == 2)
    {
        for(int i=0; i<mem_size/mm_param; i++)
        {
            trash.start = i*mm_param; 
            trash.end = (i+1)*mm_param;
            trash.ID = 0;
            memory.push_back(trash);
        }
    }

    while(sys_clock < 100000 && !all_procs_done)
    {
        next_event = 100000;
        cout << "t = " << sys_clock << ": ";

        // Check for arrivals
        CheckArrivals(num_procs, sys_clock, procs, input_queue);

        // check events and Run according to policy
        switch (mm_policy)
        {
        case 1:
            cout << "VSP" << endl;
            break;
        case 2:
            while(StorePAG(memory, mm_param, input_queue, procs, sys_clock));
            break;
        case 3:
            cout << "SEG" << endl;
            break;
        default:
            break;
        }

        // Check for completions
        CheckCompletion(procs, sys_clock, num_procs, memory);

        // Update sys clock to next event
        for(int i=0; i<num_procs; i++)
        {
            if(procs[i].next_event > sys_clock && procs[i].next_event < next_event)
                next_event = procs[i].next_event;
        }
        sys_clock = next_event;
        cout << endl;
    }
    return true;
}

bool RunVSP(const size_t& mem_size, const size_t &mm_param, const size_t& num_procs, vector<process> procs)
{
    return true;
}


bool StorePAG(vector<mem_block>& memory, const size_t &page_size, vector<process>& queue, vector<process>& procs, const size_t& sys_clock)
{ 
    size_t page_num=0, free_space=0;
    size_t pages_needed = ceil((float)queue[0].addrs_total/page_size);

    if(queue.empty())
        return false;
    // check amount of free space
    for(int i=0; i<memory.size(); i++)
    {
        if(memory[i].ID == 0)
            free_space++;
    }

    // store
    if(free_space >= queue[0].addrs_total/page_size)
    {
        // Fill empty pages
        for(int i=0; i<memory.size() && page_num < pages_needed; i++)
        {
            if(memory[i].ID == 0)
            {
                memory[i].ID = queue[0].ID;
                memory[i].part = page_num+1;
                page_num++;
            }
        }
        // Update events
        for(int i=0; i<procs.size(); i++)
        {
            if(procs[i].ID == queue[0].ID)
            {
                procs[i].end_t = sys_clock+procs[i].lifetime;
                procs[i].next_event = procs[i].end_t;
            }
        }
        cout << "MM moves Process " << queue[0].ID << " to memory" << endl;
        queue.erase(queue.begin());
        PrintQueue(queue);
        PrintMemMap(memory, 2);
    }
    else
        return false;

    return true;
}

bool RunSEG()
{
    return true;
}

// Prompt the user to get info 
bool OutputPrompt(size_t& mem_size, size_t& mm_policy, size_t& mm_param, string& file_name)
{
    string input_str = "";

    cout << "Memory size: ";
    getline(cin, input_str, '\n');
    if(!StringToInt(input_str, mem_size)){return false;}

    cout << "Memory management policy (1 - VSP, 2 - PAG, 3 - SEG): ";
    getline(cin, input_str, '\n');
    if(!StringToInt(input_str, mm_policy)){return false;}

    if(mm_policy == 2)
        cout << "Page/Frame size: ";
    else
        cout << "Fit algorithm (1 - first-fit, 2 - best-fit, 3 - worst-fit): ";
    getline(cin, input_str, '\n');
    if(!StringToInt(input_str, mm_param)){return false;}

    cout << "Workload file name: "; 
    getline(cin, file_name, '\n');

    return true;
}

void CheckArrivals(const size_t& num_procs, const size_t& sys_clock, const vector<process> &procs, vector<process>& queue)
{        
    for(int i=0; i<num_procs; i++)
    {
        // Check for and print arrivals
        if(procs[i].arrival_t == sys_clock)
        {
            cout << "Process " << procs[i].ID << " arrives" << endl;
            queue.push_back(procs[i]);
            PrintQueue(queue);
        }
    }

}

void PrintProcStart(const size_t& ID, vector<process>& queue)
{
    cout << "MM moves Process " << ID << " to memory" << endl;
    queue.erase(queue.begin());
    PrintQueue(queue);
}

void CheckCompletion( const vector<process> &procs, const size_t& sys_clock, const size_t& num_procs, vector<mem_block>& memory)
{
    // Check all processes
    for(int i=0; i<num_procs; i++)
    {
        // Check for completed processes
        if(procs[i].end_t == sys_clock)
        {
            // Output completion and clean memory
            cout << "Process " << procs[i].ID << " completes" << endl << "        ";
            for(int j=0; j<memory.size(); j++)
            {
                if(memory[j].ID == procs[i].ID)
                    memory[j].ID = 0;
            }
            PrintMemMap(memory, 2);
        }
    }
}

void PrintMemMap(const vector<mem_block>& memory, const size_t& mm_policy)
{
    int ff_start, ff_end;
    cout << "Memory Map:" << endl << "        ";
    switch (mm_policy)
    {
    case 1:
        /* code */
        break;
    case 2:
        for(int i=0; i<memory.size(); i++)
        {
            if(memory[i].ID != 0)
                cout << "        " << memory[i].start << "-" << memory[i].end -1 << ": Process " << memory[i].ID
                << " Page " << memory[i].part << endl << "        ";
            if(memory[i].ID == 0)
            {
                ff_start = memory[i].start;
                while(memory[i].ID == 0 && i < memory.size())
                {
                    ff_end = memory[i].end-1;
                    i++;
                }
                cout << "        " << ff_start << "-" << ff_end << ": Free Frame(s)" << endl << "        ";
            }
        }
        break;
    
    default:
        break;
    }
}

void PrintQueue(const vector<process>& queue)
{
    cout << "        Input Queue:[";
    for(int j=0; j<queue.size(); j++)
    {
        if(j>0)
            cout << " ";
        cout << queue[j].ID;
    }
    cout << "]" << endl << "        ";
}

// Read the given workload file and parse its information
bool ReadWorkloadFile(const string& filename, size_t& num_proccess, vector<process>& processes)
{
    ifstream fin;
    string input_str, holding_str; 
    size_t addr; 
    vector<size_t> addrs; 
    process holding; 

    // Open file and ensure open
    fin.open(filename);
    if(!fin.is_open()) return false;

    // Read number of processes
    fin >> input_str;
    StringToInt(input_str, num_proccess);

    // Read each process into process vector
    for(size_t i=0; i<num_proccess; i++)
    {
        holding.addrs.clear();
        holding.addrs_total = 0;
        fin >> holding.ID; 
        fin >> holding.arrival_t;
        holding.next_event = holding.arrival_t;
        fin >> holding.lifetime;
        fin >> holding.num_addrs;
        for(size_t j=0; j< holding.num_addrs; j++)
        {
            fin >> addr; 
            holding.addrs_total += addr;
            holding.addrs.push_back(addr);
        }

        processes.push_back(holding);
    }
    fin.close();

    return true;
}

// Convert string to int, return false if unable to
// stoi will convert as long as first char is number
// any extra characters just get removed
// FIXME: may want to adjust later
bool StringToInt(const string& str, size_t& int_val)
{
    try{
        int_val = stoi(str);
        return true; 
    } catch(...) {
        return false;
    }
}


void PrintTurnaround(const vector<process>& procs)
{
    float avg_trd = 0;
    for(int i=0; i<procs.size(); i++)
    {
        avg_trd += procs[i].end_t - procs[i].arrival_t;
    }
    avg_trd = float(avg_trd/procs.size());
    cout << "Average Turnaround Time: " << fixed << setprecision(1) << avg_trd << endl;
}