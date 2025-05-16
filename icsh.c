/* ICCS227: Project 1: icsh
 * Name: Min Thu Ta Naing
 * StudentID: 6680713
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
 
#define MAX_CMD_BUFFER 255
 
int processCommand(char * cmd){
    int ret = 0;
    if (strncmp(cmd,"echo ",5)== 0){
        char *temp = cmd +5;
        while(*temp == ' '){
            temp++;
        }
        printf("%s\n",temp);
    }
    else if(strncmp(cmd,"exit ",5) == 0){
        char *temp = cmd+5;
        if(*temp == '\0'){
            printf("No exit code found!\n");
        }
        int check = 1;
        for (char *p = temp; *p !='\0' ; p++){
            if (*p <'0' || *p > '9'){
                check = 0;
                break;
            }
        }
        if(check){
            ret = atoi(temp);
            ret &= 0xFF;
        }
        else{
            printf("Invalid Command Code!\n");
        }
    }
    else{
        printf("Unknown Command!\n");
    }
    return ret;
}
void runScriptMode(char *path){
    char line[MAX_CMD_BUFFER];
    char last[MAX_CMD_BUFFER]="";
    int ret=0;
    FILE* file = fopen(path,"r");
    if (file == NULL){
        fprintf(stderr,"Unable to open file!\n");
        return;
    }
    while( fgets(line,sizeof(line),file)){
        line[strcspn(line, "\n")] = '\0';
        char *temp = line; 
        if (strcmp(temp,"!!")==0){
            if(strlen(last)==0){
                printf("No previous command found!\n");
                continue;
            }
            else{
                ret=processCommand(last);
            }
        }
        else{
            ret=processCommand(temp);
            strcpy(last,line);
        }
        if( ret != 0){
            printf("echo $?\n");
            printf("%d\n",ret);
            exit(ret);
        }
    }
    exit(0);
}
int main(int argc,char *argv[]) {
    if (argc == 2){
        runScriptMode(argv[1]);
        return 0;
    }
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER]="";
    int ret = 0;
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
                strcpy(buffer,last_command);
            }
        }  
        else{
            ret=processCommand(temp);
            strcpy(last_command,buffer);
        }
        if(ret != 0){
            printf("bye\n");
            exit(ret);
        }
        
    }
    return 0;
 }
 