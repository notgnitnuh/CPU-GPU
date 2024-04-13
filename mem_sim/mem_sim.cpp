#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdexcept>
#include <vector>

using namespace std;

// Custom Types
enum policy {VSP=1, PAG=2, SEG=3};
enum fit_type{ff=1, bf=2, wf=3};
struct process{
    size_t ID;
    size_t arrival_t;
    size_t lifetime;
    size_t num_addrs; 
    vector<size_t> addrs;
};

// Function Definitions
bool OutputPrompt(size_t& mem_size, size_t& mm_policy, size_t& mm_param, string& file_name);
bool StringToInt(const string& str, size_t& int_val);
bool ReadWorkloadFile(const string& filename, size_t& num_proccess, vector<process>& processes);

int main()
{
    // int sys_clock = 0;
    size_t num_process = 0;
    size_t mem_size, mm_policy, mm_param;
    string file_name;
    vector<process> processes; 

    if(!OutputPrompt(mem_size, mm_policy, mm_param, file_name))
    {
        cout << "Error with input" << endl;
        return 1;
    }
    if(!ReadWorkloadFile(file_name, num_process, processes))
    {
        cout << "Error reading workload file" << endl;
        return 1;
    }

    for(size_t i=0; i<num_process; i++)
    {
        cout << "ID: " << processes[i].ID << ", arrival time: " << processes[i].arrival_t << ", lifetime: " << processes[i].lifetime << ", address space: ";
        for(size_t j=0; j<processes[i].addrs.size(); j++)
            cout << processes[i].addrs[j] << " ";
        cout << endl;
    }

    cout << "program end" << endl;

    return 0;
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
        fin >> holding.ID; 
        fin >> holding.arrival_t;
        fin >> holding.lifetime;
        fin >> holding.num_addrs;
        for(size_t j=0; j< holding.num_addrs; j++)
        {
            fin >> addr; 
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