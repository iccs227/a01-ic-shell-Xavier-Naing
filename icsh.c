/* ICCS227: Project 1: icsh
 * Name: Min Thu Ta Naing
 * StudentID: 6680713
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CMD_BUFFER 255
 
void processCommand(char *cmd){
    int ec= -1;
    while(*cmd == ' ')cmd++;
    if (strncmp(cmd,"echo ",5)== 0){
        char *temp = cmd +5;
        while(*temp == ' '){
            temp++;
        }
        printf("%s\n",temp);
    }
    else if(strncmp(cmd,"exit", 4) == 0 && (cmd[4] == '\0' || cmd[4] == ' ')){
        char *temp = cmd+4;
        while(*temp == ' ')temp++;
        if(*temp == '\0'){
            printf("No exit code found!\n");
        }
        int check = 1;
        for (char *p = temp; *p !='\0' ; p++){
            if (*p < '0' || *p > '9'){
                check = 0;
                break;
            }
        }
        if(check){
            ec = atoi(temp);
            ec &= 0xFF;
            printf("echo $?\n%d\n",ec);
            exit(ec);
        }
        else{
            printf("Invalid Exit Code!\n");
        }
    }
    else{
        pid_t pid = fork();
        if(pid == 0){
            char cpy[MAX_CMD_BUFFER];
            strncpy(cpy,cmd,MAX_CMD_BUFFER);
            char *args[10];
            int i = 0;
            char *token = strtok(cpy," ");
            while(token != NULL && i<9){
                args[i] = token;
                i++;
                token = strtok(NULL," ");
            }
            args[i] = NULL;
            execvp(args[0],args);
            printf("Unknown Command!\n");
            exit(127);
        }
        else if (pid > 0){
            int temp;
            waitpid(pid,&temp,0);
            if (WIFEXITED(temp)){
                ec = WEXITSTATUS(temp);
                if(ec == 127){
                    ec = -1;
                }
            }
        }
        else{
            perror("fork");
        }
    }
}
void runScriptMode(char *path){
    char line[MAX_CMD_BUFFER];
    char last[MAX_CMD_BUFFER]="";
    FILE* file = fopen(path,"r");
    if (file == NULL){
        fprintf(stderr,"Unable to open file!\n");
        return;
    }
    while(fgets(line,sizeof(line),file)){
        line[strcspn(line, "\n")] = '\0';
        char *temp = line; 
        if(strcmp(temp,"!!")==0){
            if(strlen(last)==0){
                printf("No previous command found!\n");
                continue;
            }
            else{
                //printf("Shebang:%s\n",last);
                processCommand(last);
            }
        }
        else{
            printf("Script Mode :%s\n",temp);
            processCommand(temp);
            strcpy(last,line);
        }
    }
    fclose(file);
    exit(0);
}
int main(int argc,char *argv[]) {
    if (argc == 2){
        runScriptMode(argv[1]);
        return 0;
    }
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER]="";
    printf("Starting IC shell\nicsh $ <waiting for command>\n");
    while (1) {
        printf("icsh $ ");
        fgets(buffer, MAX_CMD_BUFFER, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        char *temp = buffer;
        while(*temp == ' '){
            temp++;
        }
        if (*temp == '\0'){
            continue;
        } 
        if (strcmp(buffer,"!!") == 0){
            if(strlen(last_command) == 0){
                printf("No previous command found!\n");
                continue;
            }
            else{
                printf("%s\n", last_command);
                processCommand(last_command);
            }
        }  
        else{
            processCommand(temp);
            strcpy(last_command,buffer);
        }
    }
    return 0;
 }
 