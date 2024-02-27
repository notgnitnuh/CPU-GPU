#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <vector>
#include <atomic>
#include <iterator>
//#include <readline/readline.h>
//#include <readline/history.h>

using namespace std;

void BuiltInCommand(string command);
int ShellParse(string command);
pid_t SpawnChild(char* program, char** arg_list);
string AdjustWhitespace(string command, int spacing);
void ParseCommands(string args[], string command, char* arg_list[]);
void RunParallel(string args[], string command, char* arg_list[]);
void SetOutput(string command);
char *path;

// &| just runs &, |& throws error
// ls & | shuf &    - legit
// ls | & shuf      - syntax error
// ending with | is an error

int main(int argc, char *argv[])
{
    string command;
    ifstream f_in;
    path = getenv("PATH");
    string::iterator it;

    // Run interactive mode
    if (argc == 1)
    {
        while(true)
        {
            // Prompt for input, read it in, and format spacing 
            cout << ">";
            getline(cin, command, '\n');
            command = AdjustWhitespace(command, 1);
            BuiltInCommand(command);
        }
    }
    else if (argc == 2)
    {
        //while(!eof)
        // Open file, readline, parse
        f_in.open(argv[1]);
        if(f_in.is_open())
        {
            while(f_in)
            {
                getline(f_in, command, '\n');
                BuiltInCommand(command);
            }
        }
        else
            cout << "error reading file: \"" << argv[1]  << "\"" << endl;
        f_in.close();
    }
    else
	    cout << "Too many arguments" << endl;
    cout << "Shell exited" << endl;
    return 0;
}

void BuiltInCommand(string command)
{
    // Check for built in commands and run accordingly 
    if(command == "exit")
        exit(0);
    // Check for syntax errors
    else if ((command.find("|&") < command.size()-1) || (command.find("| &") < command.size()-1) || (command[command.size()-1] == '|'))
        cout << "Syntax Error" << endl;
    // Check for "cd" and handle accordingly
    else if (command.find("cd") == 0)
    {
        command = AdjustWhitespace(command.substr(command.find("cd")+2, command.size()),1);
        if(command.find(' ') < command.size())
            cout << "Incorrect number of arguments." << endl;
        else
        {
            if(chdir(command.c_str()) != 0)
                cout << "Error in path" << endl;
        }
    }
    // Check for "PATH" renaming
    else if (command.find("PATH") == 0)
        path = (char*)command.substr(command.find('=')+1, command.size()).c_str();
    // Run normally
    else
        SetOutput(command);
}

void SetOutput(string command)
{
    string filename;

    if ( command.find('>') < command.size())
    {
        // get file and command names
        filename = command.substr(command.find('>')+1, command.size());
        filename = filename.substr(filename.find_first_not_of(' '), filename.size());
        command = command.erase(command.find('>'), command.size());

        // Change output location
        auto actual_stdout = fdopen(dup(fileno(stdout)), "w");
        ShellParse(command);
        if(freopen((char*)filename.c_str(), "w", stdout))
        {
            ShellParse(command.substr(0, command.find('>')));
        }
        fclose(stdout);
        auto stdout = fdopen(dup(fileno(actual_stdout)), "w");
        fclose(actual_stdout);
        // cout.rdbuf(stdout);
    }
    else
        ShellParse(command);
}

int ShellParse(string command)
{
    string args[20]; 
    char* arg_list[20];
    string::iterator it; 
    string sub_string;

    // Parse by pipes
    // Determine output location
    // run
    if (command.find('|') < command.size())
    {
        cout << "TODO: add '|' support" << endl;
    }
    else
        RunParallel(args, command, arg_list);
    
    return 0;
}

void RunParallel(string args[], string command, char* arg_list[])
{
    size_t pos =0;
    // Run any commands before the '&'
    for(; pos < command.size(); pos++)
    {
        if (command[pos] == '&')
        {
            ParseCommands(args, AdjustWhitespace(command.substr(0,pos),1), arg_list);
            SpawnChild(arg_list[0], arg_list);
            command.erase(0,pos+1);
            pos = 0;
        }
    }
    // Run last command after '&', will do nothing if ends with '&'
    ParseCommands(args, AdjustWhitespace(command,1), arg_list);
    SpawnChild(arg_list[0], arg_list);

    // Wait for all children to die
    while(wait(NULL)>0);
}

void ParseCommands(string args[], string command, char* arg_list[])
{
    size_t pos=0;
    int i=0;

    while(!command.empty())
    {
        // parse args by spaces
        if((command[pos] == ' ') || (pos == command.size()))
        {
            args[i] = (command.substr(0,pos));
            arg_list[i] = (char*)args[i].c_str();
            command.erase(0,pos+1);
            i++;
            pos=0;
        }
        else
            pos++;
    }
    arg_list[i] = NULL;
}


// spawn a child and use it
pid_t SpawnChild(char* program, char** arg_list) 
{
    //create child
    pid_t ch_pid = fork();
    if (ch_pid == -1)    // kill if error
    {
        perror("fork");
        exit(EXIT_FAILURE);
    } 
    else if (ch_pid > 0) // return if parent
        return ch_pid;
    else                 // exec process if child
    {
        char *envp[] = {path, NULL};
        execvpe(program, arg_list, envp);
        perror("execve");
        exit(EXIT_FAILURE);
    }
}

// remove excess whitespace
string AdjustWhitespace(string command, int spacing)
{
    int num_spaces = 0;
    size_t i = 0;

    //remove leading and trailing spaces
    command = command.substr(command.find_first_not_of(' '),command.find_last_not_of(' ')+1);

    // Check each index of string
    for(i = 0; i< command.size();)
    {
        // Count spaces
        if(command[i] == ' ')
            num_spaces++;
        else
            num_spaces = 0;
        
        // If too many spaces in row, delete one, only move 'i' if no deletions
        if(num_spaces > spacing)
            command.erase(i, 1);
        else
            i++;
    }
    return command;
}