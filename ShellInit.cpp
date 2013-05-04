#include <iostream>

#include "ShellInit.h"

using namespace std;

void replace_username(char path[], char * username)
{
    char result[1024], tmpstr[1025], tmpusername[strlen(username)+6];
    char *  position = NULL;

    memset(result, '\0', 1024);
    memset(tmpusername, '\0', strlen(username)+6);

    if(0 == strcmp(username, "root"))
    {
        strcpy(tmpusername, "/");
    }
    else
    {
        strcpy(tmpusername, "/home/");
    }
    strcat(tmpusername, username);

    strcpy(tmpstr,path);
    position = strstr(path, tmpusername);
    if(NULL != position)
    {
        while(NULL != path && path != position)
        {
            path++;
        }
        path[0]='\0';
        strcat(path, "~");
        strcat(path, strstr(tmpstr,tmpusername) + strlen(tmpusername));
    }
}

char * get_userinfo()
{
    char * username = NULL;
    char host[100], path[1024], result[1200];
    struct group * groupdata;

    memset(host, '\0', 100);
    memset(path, '\0', 1024);
    memset(result, '\0', 1200);

    strcat(result, "[");

    groupdata = getgrgid(getgid());
    username=groupdata->gr_name;
    strcat(result, username);
    strcat(result, "@");

    if(-1 == gethostname(host, 100))
    {
        strcpy(host, "localhost");
    }
    strcat(result, host);
    strcat(result, " ");

    if(NULL == getcwd(path,1024))
    {
        cout<<"路径获取失败！"<<endl;
    }
    replace_username(path, username);
    strcat(result, path);
    strcat(result, "]");

    if(0 == strcmp(username,"root"))
    {
        strcat(result, "#");
    }
    else
    {
        strcat(result, "$");
    }

    return result;
}

char * get_command()
{
    char *command_line = (char *)NULL;
    char * userinfo = NULL;

    userinfo = get_userinfo();
    if(command_line)
    {
        delete command_line;
        command_line = (char *)NULL;
    }
    command_line = readline(userinfo);

    if(command_line && *command_line)
        add_history(command_line);

    return command_line;
}
