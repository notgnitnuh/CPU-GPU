#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <vector>
#include <iterator>
//#include <readline/readline.h>
//#include <readline/history.h>

using namespace std;

string AdjustWhitespace(string command, int spacing);
string GetFileName(string &command, size_t pos);
void BuiltInCommand(string command);
void MakeArgList(string args[], string command, char* arg_list[]);
void RunParallel(string args[], string command, char* arg_list[]);
void pipeing(string command, int &fi, int &fd);
int ShellParse(string command);
pid_t SpawnChild(char* program, char** arg_list, int fi, int fd);
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
    int fi = -1, fd = -1;

    // find '|' or '><' the set fi/fd
    // run commands between
    if (command.find('|') < command.size())
        pipeing(command, fi, fd);
    else
        RunParallel(args, command, arg_list);
    
    return 0;
}

// Parse a command at pipes
// void ParsePipes(string command, char* parsed_pipes[])
// {
//     int i=0;
//     parsed_pipes[i] = " ";
//     while(parsed_pipes[i] != NULL)
//     {
//         parsed_pipes[i] = strsep(&command, "|")
//     }
// }

void pipeing(string command, int &fi, int &fd)
{
    string args[20]; 
    char* arg_list[20];
    int pipefd[2];
    pid_t cpid;
    char *envp[] = {path, NULL};

    // parse int left and right of '|'
    string command1 = AdjustWhitespace(command.substr(0,command.find('|')),1);
    string command2 = AdjustWhitespace(command.substr(command.find('|')+1, command.size()),1);
    if(pipe(pipefd) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    cpid = fork();
    if(cpid == 0)
    {
        close(pipefd[0]);       // close read end
        dup2(pipefd[1],1);      //send stdout to pipe
        close(pipefd[1]);       // descriptor no longer needed

        MakeArgList(args, command1, arg_list);
        execvpe(arg_list[0], arg_list, envp);   //call exec
        perror("execvpe");
        exit(EXIT_FAILURE);
    }
    else
    {
        int cpid2 = fork();
        if(cpid2 == 0)
        {
            close(pipefd[1]);   //close write
            dup2(pipefd[0], 0);
            close(pipefd[0]);
            dup2(fd, 1);

            MakeArgList(args, command2, arg_list);
            execvpe(arg_list[0], arg_list, envp);
            perror("execvpe");
            exit(EXIT_FAILURE);
        }
        else
        {
            close(pipefd[1]); // close write end
            close(pipefd[0]);
            wait(NULL);
            wait(NULL);
        }
    }   
}

void RunParallel(string args[], string command, char* arg_list[])
{
    size_t pos =0;
    string f_in, f_out;
    int fi = -1, fd = -1;

    // Run any commands before the '&'
    for(; pos < command.size(); pos++)
    {
        if (command[pos] == '&')
        {
            // Parse command, spawn child, peform cleanup
            MakeArgList(args, AdjustWhitespace(command.substr(0,pos),1), arg_list);
            SpawnChild(arg_list[0], arg_list, fi, fd);
            command.erase(0,pos+1);
            f_in.clear();
            f_out.clear();
            pos = 0;
        }
        else if (command[pos] == '>')
        {
            // Add input file name
            f_out = GetFileName(command, pos);
            fd = open(f_out.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_TRUNC, S_IRUSR | S_IWUSR);
            if(fd == -1)
            {
                perror("errno");
                exit(EXIT_FAILURE);
            }
            pos--;
        }
        else if (command[pos] == '<')
        {
            // Add output file name
            f_in = GetFileName(command, pos);
            fi = open(f_in.c_str(), O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
            if(fi == -1)
            {
                perror("errno");
                exit(EXIT_FAILURE);
            }
            pos--;
        }
    }

    // Run if any command after '&'
    if(command.size() > 1)
    {
        MakeArgList(args, AdjustWhitespace(command,1), arg_list);
        SpawnChild(arg_list[0], arg_list, fi, fd);
    }

    // Wait for all children to die
    while(wait(NULL)>0);
}

// Remove the file name from the string "command" and return the file name
string GetFileName(string &command, size_t pos)
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
void MakeArgList(string args[], string command, char* arg_list[])
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
pid_t SpawnChild(char* program, char** arg_list, int fi, int fd) 
{
    char *envp[] = {path, NULL};
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
        // If filenames provided open and use
        if (fi != -1)
        {
            dup2(fi, 0);
        }
        if(fd != -1)
        {
            dup2(fd,1);
        }

        // Call syscall and return
        execvpe(program, arg_list, envp);
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