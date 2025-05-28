/* ICCS227: Project 1: icsh
 * Name: Min Thu Ta Naing
 * StudentID: 6680713
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#define MAX_CMD_BUFFER 255

//function to disable displaying ^C and ^Z on shell
void disableEchoctl() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);        
    term.c_lflag &= ~ECHOCTL;              
    tcsetattr(STDIN_FILENO, TCSANOW, &term); 
}
//global variables 
volatile int fg_child_pid = -1;
volatile int last_exit_code = 0;

//to handle SIGSTP and SIGINT
void signalHandler(int sig){
    if (fg_child_pid > 0){
       kill(fg_child_pid,sig);
    }
}

//all command processing goes here
void processCommand(char *cmd){
    if(strcmp(cmd,"echo $?") == 0){
        printf("%d\n",last_exit_code);
        return;
    }
    else if (strncmp(cmd,"echo ",5)== 0 && strchr(cmd,'<') == NULL && strchr(cmd,'>') == NULL){
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
            printf("Bye!");
            exit(0);
        }
        int check = 1;
        for (char *p = temp; *p !='\0' ; p++){
            if (*p < '0' || *p > '9'){
                check = 0;
                break;
            }
        }
        if(check){
            int ec = atoi(temp);
            ec &= 0xFF;
            printf("echo $?\n%d\n",ec);
            printf("Bye!");
            exit(ec);
        }
        else{
            printf("Invalid Exit Code!\n");
            last_exit_code = -1;
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
            char *input_file = NULL,*output_file = NULL;
            while(token != NULL && i<9){
                if(strcmp(token,"<")==0){
                    token = strtok(NULL," ");//get the next character after the <
                    if(token){
                        input_file = token;
                    }
                }
                else if(strcmp(token,">")==0){
                    token = strtok(NULL," ");
                    if(token){
                        output_file = token;
                    }
                }
                else{
                args[i++] = token;}
                token = strtok(NULL," ");
            }
            args[i] = NULL;
            if(input_file){
                int in = open(input_file,O_RDONLY);
                if(in < 0){
                    printf("Input file error!");
                    exit(1);
                }
                dup2(in,STDIN_FILENO);
                close(in);
            }
            if(output_file){
                int out = open(output_file,O_WRONLY | O_CREAT | O_TRUNC,0644);
                if(out < 0){
                    printf("Output file error!");
                    exit(1);
                }
                dup2(out,STDOUT_FILENO);
                close(out);
            }
            execvp(args[0],args);
            printf("Unknown Command!\n");
            exit(127);
        }
        else if (pid > 0){
            fg_child_pid = pid;
            int temp;
            waitpid(pid,&temp,WUNTRACED);
            fg_child_pid = -1;
            if (WIFEXITED(temp)){
                last_exit_code = WEXITSTATUS(temp);
                if(last_exit_code == 127){
                    last_exit_code = -1;
                }
                printf("\n");
            }
        }
        else{
            perror("fork");
            last_exit_code = -1;
        }
    }
}

//m2, script mode
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
                printf("Last command: %s\n",last);
                processCommand(last);
            }
        }
        else{
            char clean[MAX_CMD_BUFFER];
            strncpy(clean, line, MAX_CMD_BUFFER);
            clean[MAX_CMD_BUFFER - 1] = '\0';
            processCommand(clean);
            strncpy(last, clean, MAX_CMD_BUFFER);
            last[MAX_CMD_BUFFER - 1] = '\0';
        }
    }
    fclose(file);
    exit(0);
}

//main logic loop
int main(int argc,char *argv[]) {
    disableEchoctl();
    signal(SIGTSTP,signalHandler);
    signal(SIGINT,signalHandler);
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
 