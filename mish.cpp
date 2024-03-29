#include <iostream>
#include <fstream>
#include <fcntl.h>
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

string AdjustWhitespace(string command, int spacing);
void BuiltInCommand(string command);
string IORedirect(string &command, size_t pos);
void ParseCommands(string args[], string command, char* arg_list[]);
void RunParallel(string args[], string command, char* arg_list[]);
int ShellParse(string command);
pid_t SpawnChild(char* program, char** arg_list, string f_in, string f_out);
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
    string f_in, f_out;

    // Run any commands before the '&'
    for(; pos < command.size(); pos++)
    {
        if (command[pos] == '&')
        {
            // Parse command, spawn child, peform cleanup
            ParseCommands(args, AdjustWhitespace(command.substr(0,pos),1), arg_list);
            SpawnChild(arg_list[0], arg_list, f_in, f_out);
            command.erase(0,pos+1);
            f_in.clear();
            f_out.clear();
            pos = 0;
        }
        else if (command[pos] == '>')
        {
            // Add input file name
            f_out = IORedirect(command, pos);
            pos--;
        }
        else if (command[pos] == '<')
        {
            // Add output file name
            f_in = IORedirect(command, pos);
            pos--;
        }
    }

    // Run if any command after '&'
    if(command.size() > 1)
    {
        ParseCommands(args, AdjustWhitespace(command,1), arg_list);
        SpawnChild(arg_list[0], arg_list, f_in, f_out);
    }

    // Wait for all children to die
    while(wait(NULL)>0);
}

string IORedirect(string &command, size_t pos)
{
    //TODO: throw errors for back to back '>' or '<'
    string filename = AdjustWhitespace(command.substr(pos, command.size()),1);
    filename = filename.substr(1, filename.find_first_of("&|><",pos)-1);
    command.erase(pos,filename.size()+1);

    if(filename[filename.size()-1] == ' ')
        filename.erase(filename.size()-1, 1);
    if(filename[0] == ' ')
        filename.erase(0,1);

    if(filename.find_first_of(' ') < filename.size())
    {
        cout << "Error in number of arguments" << endl;
        filename.clear();
    }
    
    // Cleanup
    return filename; 
}

// Parse command by spaces, put into args[] and point arg_list** toward it
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
// may need to add wait to parent process
pid_t SpawnChild(char* program, char** arg_list, string f_in, string f_out) 
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

        // If filename provided, open it and write to it
        if (!f_in.empty())
        {
            int fi = open(f_in.c_str(), O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
            if(fi == -1)
            {
                cout << "Error opening input file" << endl;
                exit(EXIT_FAILURE);
            }
            dup2(fi, 0);
        }
        if(!f_out.empty())
        {
            int fd = open(f_out.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
            if(fd == -1)
            {
                cout << "Error opening output file" << endl;
                exit(EXIT_FAILURE);
            }
            dup2(fd,1);
        }
        execvpe(program, arg_list, envp);
        cout << program << endl;
        perror("execvpe");
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