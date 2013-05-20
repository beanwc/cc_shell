#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "CommandParseDef.h"
#include "GlobalVarDef.h"

using namespace std;


_COMMAND command_list[] = {
    {"cd", cd_command, "     Change to Dir!"},
    {"echo", echo_command, "   Print the string or the value of variable!"},
    {"help", help_command, "   Print help information!"},
    {"history", history_command, "List history command!"},
    {"pwd", pwd_command, "    List files in current Dir!"},
    {"unset", unset_command, "  Reset user's builtin variable!"},
    {(char *) NULL, (rl_i_cp_cpp_func_t *) NULL, (char *) NULL}
};


void readline_init()
{
    rl_readline_name = "ccShell";
    rl_attempted_completion_function = command_complete;
}

char ** command_complete(const char * text, int start, int end)
{
    char ** matches;

    matches = (char **) NULL;
    if (0 == start)
    {
        matches = rl_completion_matches(text, command_produce);
    }

    return matches;
}

char * command_produce(const char * text, int state)
{
    static int list_index, len;
    char * command_name = (char *) malloc(100);

    memset(command_name, '\0', 100);

    if (!state)
    {
        list_index = 0;
        len = strlen(text);
    }

    while (NULL != command_list[list_index].commandName)
    {
        strcpy(command_name, command_list[list_index].commandName);
        list_index++;
        if (strncmp(command_name, text, len) == 0)
        {
            return command_name;
        }
    }

    return NULL;
}

_COMMAND * get_execute_handle(char * command)
{
    int i;

    for (i = 0; command_list[i].commandName; i++)
	{
		if (strcmp(command, command_list[i].commandName) == 0)
			{
                return (&command_list[i]);
			}
    }

    return NULL;
}

void analyse_command(char * command_line)
{
    int i = 0;
	int j = 0;
	int arg_count;
	char * strtmp;
    char ** arg;

    arg = get_command_arg(command_line, &arg_count);
//
//    int k = 0;
//    while (arg[k])
//    {
//        cout<<k<<" :"<<arg[k]<<endl;
//        k++;
//    }

  	while(arg[i])
	{
		if(strcmp(arg[i], ";") == 0)
		{
			strtmp = arg[i];
			arg[i] = 0;
			analyse_pipe_command(i-j, arg+j);
			arg[i] = strtmp;
			j = ++i;
		}
		else
		{
			i++;
		}
	}
	analyse_pipe_command(i-j, arg+j);
//
//    int x = 0;
//    for (; x < variable_count; x++)
//    {
//        cout<<variable[x].variable_name<<" = "<<variable[x].variable_value<<endl;
//    }

	reset_arg();
}

void analyse_pipe_command(int arg_count, char ** arg)
{
    int i = 0, j = 0;
	int prefd[2];
	int postfd[2];
	bool prepipe = false;
	char * strtmp;

	while(arg[i])
	{
		if(strcmp(arg[i], "|") == 0)
		{
			strtmp = arg[i];
			arg[i] = 0;

			pipe(postfd);

			if(prepipe)
            {
                execute_command(i-j, arg+j, prefd, postfd);
            }
			else
			{
            	execute_command(i-j, arg+j, 0, postfd);
			}
			arg[i] = strtmp;
			prepipe = true;
			prefd[0] = postfd[0];
			prefd[1] = postfd[1];
			j = ++i;
		}
		else
		{
            i++;
		}
	}
	if(prepipe)
	{
        execute_command(i-j, arg+j, prefd, 0);
	}
	else
    {
        execute_command(i-j, arg+j, 0, 0);
    }
}

int analyse_redirect_command(int arg_count, char ** arg, int * redirect_arg)
{
    int i;
	int redirect = 0;
	for(i = 1; i < arg_count; i++)
	{
		if(strcmp(arg[i], "<") == 0)
		{
			redirect = 1;
			arg[i] = 0;
			break;
		}
        else if(strcmp(arg[i], ">") == 0)
		{
			redirect = 2;
			arg[i] = 0;
			break;
		}
	}
	if(redirect)
    {
		if(arg[i+1])
		{
			int fd;
			if(redirect == 2)
			{
				if((fd = open(arg[i+1], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
				{
					cout<<"ccShell :Open out "<<arg[i+1]<<" failed"<<endl;
					return 1;
				}
				dup2(fd, STDOUT_FILENO);
			}
			else
			{
				if((fd = open(arg[i+1], O_RDONLY, S_IRUSR|S_IWUSR)) == -1)
				{
					cout<<"ccShell :Open in "<<arg[i+1]<<" failed"<<endl;
					return 1;
				}
				dup2(fd, STDIN_FILENO);
			}
		}
		else
		{
			cout<<"ccShell :Bad redirect, need more arg"<<endl;
			return 1;
		}
	}
	if(redirect)
	{
		*redirect_arg = redirect;
	}
	return 0;
}

int execute_command(int arg_count, char ** arg, int prefd[], int postfd[])
{
    char * command_file_path = NULL;
    int pid = 0;
    int status;
	_COMMAND * execute_result;

	command_file_path = (char *)malloc(1024);
    memset(command_file_path, '\0', 1024);

	if(arg_count == 0)
	{
        return 0;
    }

    if(strcmp(arg[0], "unset"))
    {
        analyse_variable_command(arg_count, arg);
    }

	if(prefd == 0 && postfd == 0)
	{
		if((execute_result = get_execute_handle(arg[0])))
		{
			(*(execute_result->commandFunction))(arg_count, arg);
			return 0;
		}
	}

	if((pid = fork()) == 0) {
		int redirect = 0;
		signal(SIGINT, SIG_DFL);

		if(analyse_redirect_command(arg_count, arg, &redirect))
			exit(1);

		if(redirect != 1 && prefd)
		{
			close(prefd[1]);
			if(prefd[0] != STDIN_FILENO) {
				dup2(prefd[0], STDIN_FILENO);
				close(prefd[0]);
			}
		}
		if(redirect != 2 && postfd)
		{
			close(postfd[0]);
			if(postfd[1] != STDOUT_FILENO)
			{
				dup2(postfd[1], STDOUT_FILENO);
				close(postfd[1]);
			}
		}

		if((execute_result = get_execute_handle(arg[0])))
		{
			(*(execute_result->commandFunction))(arg_count, arg);
			return 0;
		}

		if(search_command_file_path(arg[0], command_file_path))
        {
            execv(command_file_path, arg);
        }
        if(assignment(arg))
        {
            return 0;
        }
		else
        {
			cout<<"ccShell: "<<arg[0]<<": Command not found!"<<endl;
            exit(0);
		}
	}
	waitpid(pid, &status, 0);
	if(postfd)
	{
		close(postfd[1]);
	}
	return 0;
}

int analyse_variable_command(int arg_count, char ** arg)
{
    int i = 0, j = 0, k = 0;
    char * value, * buffer, * arg_tmp, * variable_tmp;

    buffer = (char *)malloc(1024);
    arg_tmp = (char *)malloc(1024);
    variable_tmp = (char *)malloc(1024);

    memset(buffer, '\0', 1024);
    memset(arg_tmp, '\0', 1024);
    memset(variable_tmp, '\0', 1024);

	for(i = 1; i < arg_count; i++)
	{
		if(arg[i][0] == '$')
		{
            if(arg[i][1] == 0)                      //arg == "$"
			{
                i++;
				break;
            }
			else if(arg[i][1] == '$')               //arg == "$$"
			{
				int pid = getpid();
				sprintf(buffer, "%s%d", "pid :", pid);
				strcpy(variable_tmp, buffer);
				free(arg[i]);
				arg[i] = variable_tmp;
				i++;
				break;
			}
			else
			{
				for(j = 1; j < arg_count; j++)
				{
					if(arg[i][j] == '$')
						break;
					for(k = 1; arg[i][k] != 0; k++)
					{
                        arg_tmp[k-1] = arg[i][k];
					}
				}
				if((value = get_variable_value(arg_tmp)))
                {
					strcpy(variable_tmp, value);
				}
				else if((value = getenv(arg_tmp)))
				{
                    strcpy(variable_tmp, value);
				}
				free(arg[i]);
				arg[i] = variable_tmp;
				i++;
			}
		}
		else
		{
			i++;
		}
	}

	return 0;
}

