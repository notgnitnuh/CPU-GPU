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
int OpenFile(string &command, size_t pos, bool input);
void BuiltInCommand(string command);
void MakeArgList(string args[], string command, char* arg_list[]);
void RunSingle(string command, int fi, int fd);
void Pipeing(string command, int &fi, int &fd);
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

    return 0;
}

// Check for built in commands
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

// Parse shell inputs to determine input/output
int ShellParse(string command)
{
    int fi = -1, fd = -1, pos;

    // sets i/o
    for(size_t i=0; i< command.size(); i++)
    {
        if(command[0] == '&')
        {
            command.erase(0,1);
            i--;
        }

        if(command[i] == '|')
        {
            // Set and run pipe
            // FIXME: only runs single pipe, doesn't send output anywhere else
            pos = command.find_first_of("&|<>",i+1);
            Pipeing(command.substr(0,pos), fi, fd);
            command.erase(0,pos);
            i=0;
        }
        else if (command[i] == '<')
        {
            // Open input file and run
            fi = OpenFile(command, i, true);
            pos=command.find_first_not_of(' ',i);

            // Check and send results to different file if desired
            if(command[pos] == '>')
                fd = OpenFile(command, pos, false);
            RunSingle(command.substr(0,i), fi, fd);
            command.erase(0,i+1);
            fi = -1;
            i=0;
        }
        else if (command[i] == '>')
        {   
            // Add output file name and run
            fd = OpenFile(command, i, false);
            RunSingle(command.substr(0,i), fi, fd);
            command.erase(0,i+1);
            fd = -1;
            i=0;
        }
        else if (command[i] == '&')
        {
            // Run command simply
            RunSingle(command.substr(0,i), fi, fd);
            command.erase(0,i+1);
            i=0;
        }
        
    }

    // Run any remaining command
    if(!command.empty())
        RunSingle(command, fi, fd);

    // Wait for all children to die
    while(wait(NULL)>0);
    return 0;
}

void RunSingle(string command, int fi, int fd)
{
    string args[20]; 
    char* arg_list[20];

    // format and exec
    MakeArgList(args, AdjustWhitespace(command,1), arg_list);
    SpawnChild(arg_list[0], arg_list, fi, fd);
}


void Pipeing(string command, int &fi, int &fd)
{
    string args[20]; 
    char* arg_list[20];
    int pipefd[2];
    pid_t cpid;
    char *envp[] = {path, NULL};

    // Parse left and right of '|'
    string command1 = AdjustWhitespace(command.substr(0,command.find('|')),1);
    string command2 = AdjustWhitespace(command.substr(command.find('|')+1, command.size()),1);
    if(pipe(pipefd) == -1){
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork
    cpid = fork();
    if(cpid == 0)
    {
        close(pipefd[0]);       // close read end
        dup2(pipefd[1],1);      // send stdout to pipe
        close(pipefd[1]);       // descriptor no longer needed

        MakeArgList(args, command1, arg_list);
        execvpe(arg_list[0], arg_list, envp);   //call exec
        perror("execvpe");
        exit(EXIT_FAILURE);
    }
    else
    {   // Second Fork
        int cpid2 = fork();
        if(cpid2 == 0)
        {
            close(pipefd[1]);   //close write
            dup2(pipefd[0], 0);
            close(pipefd[0]);
            //dup2(stout, 1);

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

// Remove the file name from the string "command" and return the file name
int OpenFile(string &command, size_t pos, bool input)
{
    int fd;

    //TODO: throw errors for back to back '>' or '<'
    string filename = AdjustWhitespace(command.substr(pos, command.size()),1);
    filename = filename.substr(1, filename.find_first_of("&|><",1)-1);
    command.erase(pos,filename.size()+1);

    // Remove leading and trailing spaces
    if(filename[filename.size()-1] == ' ')
        filename.erase(filename.size()-1, 1);
    if(filename[0] == ' ')
        filename.erase(0,1);

    // Check number of arguments
    if(filename.find_first_of(' ') < filename.size())
    {
        cout << "Error in number of arguments" << endl;
        filename.clear();
    }

    // Open, check, and return
    if(!input)
        fd= open(filename.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_TRUNC, S_IRUSR | S_IWUSR);
    else
        fd= open(filename.c_str(), O_RDWR | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if(fd == -1)
    {
        perror("errno");
        exit(EXIT_FAILURE);
    }
    return fd;
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