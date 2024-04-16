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
            trash.end = (i+1)*mm_param-1;
            trash.ID = 0;
            memory.push_back(trash);
        }
    }
    else
    {
        // Start with single block for VSP and SEG
        trash.start = 0;
        trash.end = mem_size-1; 
        trash.ID = 0;
        memory.push_back(trash);
    }

    while(sys_clock < 100000 && !all_procs_done)
    {
        next_event = 100000;
        cout << "t = " << sys_clock << ": ";

        // Check for arrivals
        CheckArrivals(num_procs, sys_clock, procs, input_queue);

        // Check for completions
        CheckCompletion(procs, sys_clock, num_procs, memory, mm_policy);

        // check events and run stores according to policy
        switch (mm_policy)
        {
        case VSP:
            StoreVSP(memory, mm_param, input_queue, procs, sys_clock);
            break;
        case PAG:
            StorePAG(memory, mm_param, input_queue, procs, sys_clock);
            break;
        case SEG:
            StoreSEG(memory, mm_param, input_queue, procs, sys_clock);
            break;
        default:
            break;
        }

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

void StoreVSP(vector<mem_block>& memory, const size_t &fit_alg, vector<process>& queue, vector<process>& procs, const size_t& sys_clock)
{
    size_t max_space = 0;
    vector<process>::iterator it;
    bool fit_found = false;

    // Cycle through queue
    for(it=queue.begin(); it!= queue.end();)
    {
        // Call appropriate fit algorithm
        switch (fit_alg)
        {
        case FF:
            if(FirstFit(memory, it->addrs_total, it->ID))
                fit_found = true;
            else 
                it++;
            break;
        case BF:
            if(BestFit(memory, it->addrs_total, it->ID))
                fit_found = true;
            else 
                it++;
        case WF:
            if(WorstFit(memory, it->addrs_total, it->ID))
                fit_found = true;
            else
                it++;
            break;
        default:
            break;
        }
        if (fit_found)
        {
            {
                // Update events
                for(int j=0; j<procs.size(); j++)
                {
                    if(procs[j].ID == it->ID)
                    {
                        procs[j].end_t = sys_clock+procs[j].lifetime;
                        procs[j].next_event = procs[j].end_t;
                    }
                }
                cout << "MM moves Process " << it->ID << " to memory" << endl;
                queue.erase(it);
                PrintQueue(queue);
                PrintMemMap(memory, VSP);
            }
        }
    }
}

bool FirstFit(vector<mem_block>& memory, const size_t& mem, const size_t& ID)
{
    vector<mem_block>::iterator it;
    mem_block trash;

    for(it =memory.begin(); it !=memory.end(); it++)
    {
        // If hole found
        if(it->end - it->start >= mem-1 && it->ID == 0)
        {
            trash.start = it->start;
            trash.end = it->start + mem-1;
            trash.ID = ID;
            it->start = trash.end+1;
            memory.emplace(it, trash);
            return true;
        }
    }
    // If no hole found
    return false;
}

bool BestFit(vector<mem_block>& memory, const size_t& mem, const size_t& ID)
{
    vector<mem_block>::iterator it, best = memory.end();
    mem_block trash;
    size_t block_size = 0, best_block = 30000;

    for(it =memory.begin(); it !=memory.end(); it++)
    {
        block_size = it->end - it->start;

        // check hole size
        if((it->ID == 0) && block_size >= mem-1)
        {
            if(block_size < best_block)
            {
                best_block = block_size;
                best = it;
            }
        }
    }

    // If no hole found
    if(best == memory.end())
        return false;
    
    // If Best == exact size
    if(best_block == mem-1)
    {
        best->ID = ID;
        return true;
    }

    // Place into memory
    trash.start = best->start;
    trash.end = best->start + mem-1;
    trash.ID = ID;
    best->start = trash.end+1;
    memory.emplace(best, trash);
    return true;
}


bool WorstFit(vector<mem_block>& memory, const size_t& mem, const size_t& ID)
{   
    vector<mem_block>::iterator it, worst = memory.end();
    mem_block trash;
    size_t block_size = 0, worst_block = 0;

    for(it =memory.begin(); it !=memory.end(); it++)
    {
        block_size = it->end - it->start;

        // Check hole size
        if((it->ID == 0) && block_size >= mem-1)
        {
            if(block_size > worst_block)
            {
                worst_block = block_size;
                worst = it;
            }
        }
    }

    // If no hole found
    if(worst == memory.end())
        return false;

    // If worst == exact size
    if(worst_block == mem-1)
    {
        worst->ID = ID;
        return true;
    }

    // Place into memory
    trash.start = worst->start;
    trash.end = worst->start + mem-1;
    trash.ID = ID;
    worst->start = trash.end+1;
    memory.emplace(worst, trash);
    return true;
}


bool StorePAG(vector<mem_block>& memory, const size_t &page_size, vector<process>& queue, vector<process>& procs, const size_t& sys_clock)
{ 
    size_t page_num=0, free_space=0;
    size_t pages_needed;
    vector<process>::iterator it=queue.begin();

    if(queue.empty())
        return false;

    // check amount of free space
    for(int i=0; i<memory.size(); i++)
    {
        if(memory[i].ID == 0)
            free_space++;
    }

    while(it!=queue.end())
    {
        // Reset page details
        page_num = 0;
        pages_needed = ceil((float)it->addrs_total/page_size);

        // Store
        if(free_space >= pages_needed)
        {
            // Fill empty pages
            for(int j=0; j<memory.size() && page_num < pages_needed; j++)
            {
                if(memory[j].ID == 0)
                {
                    memory[j].ID = it->ID;
                    memory[j].part = page_num+1;
                    page_num++;
                    free_space--;
                }
            }
            // Update events
            for(int j=0; j<procs.size(); j++)
            {
                if(procs[j].ID == it->ID)
                {
                    procs[j].end_t = sys_clock+procs[j].lifetime;
                    procs[j].next_event = procs[j].end_t;
                }
            }
            // Print and cleanup
            cout << "MM moves Process " << it->ID << " to memory" << endl;
            queue.erase(it);
            PrintQueue(queue);
            PrintMemMap(memory, 2);
        }
        else
            it++;
    }

    return true;
}

bool StoreSEG(vector<mem_block>& memory, const size_t &fit_alg, vector<process>& queue, vector<process>& procs, const size_t& sys_clock)
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


// Check all processes to see if complete, and clean memory
void CheckCompletion( const vector<process> &procs, const size_t& sys_clock, const size_t& num_procs, vector<mem_block>& memory, const size_t& mm_policy)
{
    // Check all processes
    for(int i=0; i<num_procs; i++)
    {
        // Check for completed processes
        if(procs[i].end_t == sys_clock)
        {
            switch (mm_policy)
            {
            case VSP:
                CleanOther(procs[i], memory, mm_policy);
                break;
            case PAG:
                CleanPages(procs[i], memory);
                break;
            case SEG:
                CleanOther(procs[i], memory, mm_policy);
                break;
            default:
                break;
            }
        }
    }
}

// Clean pages of completed processes
void CleanPages(const process& proc, vector<mem_block>& memory)
{            
    // Output completion and clean memory
    cout << "Process " << proc.ID << " completes" << endl << "        ";
    for(int j=0; j<memory.size(); j++)
    {
        if(memory[j].ID == proc.ID)
            memory[j].ID = 0;
    }
    PrintMemMap(memory, 2);
}

// Clean memory of completed processes for VSP and SEG
void CleanOther(const process& proc, vector<mem_block>& memory, size_t mm_policy)
{  
    vector<mem_block>::iterator it;

    // Output completion
    cout << "Process " << proc.ID << " completes" << endl << "        ";
    for(int j=0; j<memory.size(); j++)
    {
        if(memory[j].ID == proc.ID)
            memory[j].ID = 0;
    }

    // Combine empty space
    for(it = memory.begin(); it < memory.end()-1;)
    {
        if(it->ID == 0 && (it+1)->ID == 0)
        {
            it->end = (it+1)->end;
            memory.erase(it+1);
        }
        else
            it++;
    }
    PrintMemMap(memory, mm_policy);
}

void PrintMemMap(const vector<mem_block>& memory, const size_t& mm_policy)
{
    int ff_start, ff_end;
    string free_space, seg_page;

    // Choose output text based off of policy
    cout << "Memory Map:" << endl << "        ";
    switch (mm_policy) 
    {
    case VSP:
        free_space = ": Hole ";
        seg_page = "";
        break;
    case PAG:
        free_space = ": Free Frames(s)";
        seg_page = ", Page ";
        break;
    case SEG: 
        free_space = " Hole ";
        seg_page = ", Segment ";
        break;
    default:
        break;
    }

    for(int i=0; i<memory.size(); i++)
    {
        if(memory[i].ID != 0)
        {
            cout << "        " << memory[i].start << "-" << memory[i].end << ": Process " << memory[i].ID;
            if(seg_page != "")
                cout << seg_page << memory[i].part;
            cout << endl << "        ";

        }
        if(memory[i].ID == 0)
        {
            ff_start = memory[i].start;
            while(memory[i].ID == 0 && i < memory.size())
            {
                ff_end = memory[i].end;
                i++;
            }
            i--;
            cout << "        " << ff_start << "-" << ff_end << free_space << endl << "        ";
        }
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
// FIXME: may want to adjust later for error checking
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