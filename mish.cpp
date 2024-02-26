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

int shell_parse(string command);
pid_t spawn_child(const char* program, char** arg_list);
string adjust_whitespace(string command, int spacing);
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
    int i=0;
    string::iterator it;

    // Run interactive mode
    if (argc == 1)
    {
        while(true)
        {
            // Prompt for input, read it in, and format spacing 
            cout << ">";
            getline(cin, command, '\n');
            command = adjust_whitespace(command, 1);

            // Check for built in commands and run accordingly 
            if(command == "exit")
                exit(0);
            else if ((command.find("|&") < command.size()-1) || (command.find("| &") < command.size()-1) || (command[command.size()-1] == '|'))
               cout << "Syntax Error" << endl;
            else if (command.find("cd") == 0)
            {
                i=0;
                string temp = "chdir";
                temp.append(command.substr(2,command.size()));
                for(it=command.begin(); it != command.end(); it++)
                {
                    if(*it == ' ')
                        i++;
                }
                if (i != 1)
                    cout << "Invalid number of arguments, must be 1" << endl;
                else
                    shell_parse(temp);
            }
            else if (command.find("PATH") == 0)
                path = (char*)command.substr(command.find('=')+1, command.size()).c_str();
            else
                shell_parse(command);
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
                shell_parse(command);
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

int shell_parse(string command)
{
    char delimiter = ' ';
    int pos=0, i=0;
    string token;
    string args[50];
    char* arg_list[] = {(char*)command.c_str(), NULL};


    if(command.find('&') < command.size())
    {
        cout << "TODO: add '&' support" << endl;
    }
    else if (command.find('|') < command.size())
    {
        cout << "TODO: add '|' support" << endl;
    }
    else // Run standard
    {    
        while((pos = command.find(delimiter))!= int(string::npos))
        {
            args[i] = command.substr(0, pos).c_str();
            //*arg_list[i] = ' ';
            //strcpy(arg_list[i], token.c_str());
            arg_list[i] = (char*)args[i].c_str();
            command.erase(0, pos+1);
            i++; 
            //cout << "delim" << endl;
        }
        spawn_child(arg_list[0], arg_list);
    }

    for (int j=0; j<=i; j++)
    {
        //arg_list[j] = (char*)args[j].c_str();
        //arg_list[j+1] = NULL;
        //cout << "args: " << args[j] << endl;
        //cout << "list: " << arg_list[j] << endl;
    }

    
    return 0;
}


// spawn a child and use it
pid_t spawn_child(const char* program, char** arg_list) 
{
    //create child
    pid_t ch_pid = fork();
    if (ch_pid == -1)    // kill if error
    {
        perror("fork");
        exit(EXIT_FAILURE);
    } 
    else if (ch_pid > 0) // wait if parent
    {
        wait(nullptr);
        return ch_pid;
    } 
    else                 // exec process if child
    {
        char *envp[] = {path, NULL};
        execvpe(program, arg_list, envp);
        perror("execve");
        //cout << arg_list << endl;
        exit(EXIT_FAILURE);
    }
}

// remove excess whitespace
string adjust_whitespace(string command, int spacing)
{
    int num_spaces = 0;
    size_t i = 0, j = command.size();

    //remove leading and trailing spaces
    while(i != j)
    {
        if(command[i] == ' ')
            i++;
        else if(command[j-1] == ' ')
            j--;
        else
            break;
    }
    command = command.substr(i,j);

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