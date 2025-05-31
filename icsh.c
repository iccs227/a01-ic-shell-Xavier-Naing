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

typedef struct Job{
    int id;
    pid_t pid;
    char command[MAX_CMD_BUFFER];
    char status[8];
    struct Job* next_job;
}Job ;
Job *job_list = NULL;
int next_job_id= 1;

//global variables 
volatile int fg_child_pid = -1;
volatile int last_exit_code = 0;

void updateJob(pid_t pid){
    Job *temp = job_list;
    while(temp!= NULL){
        if(temp->pid == pid){
            strcpy(temp->status,"Stopped");
        }
        temp=temp->next_job;
    }
}
//to handle SIGSTP and SIGINT
void signalHandler(int sig){
    if (fg_child_pid > 0){
       updateJob(fg_child_pid);
       kill(fg_child_pid,sig);
    }
}

pid_t getJob_pid(int id){
    Job *temp = job_list;
    while(temp!= NULL){
        if(temp->id == id){
            return temp->pid;
        }
        temp = temp->next_job;
    }
    return -1;
}

void checkBackground(){
    int status,temp_pid;
    while((temp_pid = waitpid(-1,&status,WNOHANG)) > 0){
        Job *curr = job_list;
        Job *prev = NULL;
        while(curr != NULL){
            if(curr->pid == temp_pid){
                printf("\n[%d]   Done          %s\n",curr->id,curr-> command);
                if(prev == NULL){
                    job_list= curr->next_job;
                    free(curr);
                }
                else{
                    prev->next_job = curr->next_job;
                    free(curr);
                }
                break;
            }
            prev = curr;
            curr = curr->next_job;
        }
        fflush(stdout);
    }
}

void addJob(int pid,char *command){
    Job *new_job = malloc(sizeof(Job));
    new_job->pid = pid;
    new_job->id = next_job_id++;
    new_job->next_job = NULL;
    strcpy(new_job->status,"Running");
    strcpy(new_job->command,command);
    if(job_list == NULL){
        job_list = new_job;
    }
    else{
        Job *temp = job_list;
        while(temp->next_job != NULL){
            temp = temp->next_job;
        }
        temp->next_job = new_job;
    }
    printf("[%d] PID: %d  \n",new_job->id,new_job->pid);
}

void printJob(){
    Job* temp = job_list;
    while(temp != NULL){
        printf("[%d] %s %s\n",temp->id,temp->status,temp->command);
        temp = temp->next_job;
    }
}

//all command processing goes here
void processCommand(char *cmd){
    if(strncmp(cmd,"bg %",4)==0){
        char *temp = cmd+4;
        for (char *p =temp; *p != '\0'; p++){
            if (*p < '0' || *p > '9'){
                printf("Invalid Job ID!\n");
                last_exit_code = -1;
                return;
            }
        }
        int job_id = atoi(temp);
        Job *curr = job_list;
        while(curr != NULL){
            if (curr-> id == job_id){
                kill(curr->pid,SIGCONT);
                strcpy(curr->status,"Running");
                printf("[%d]   %s\n",curr->id,curr->command);
                return;
            }
            curr = curr->next_job;
        }
        printf("ID not found!\n");
        last_exit_code = -1;
        return;
    }
    else if(strncmp(cmd,"fg %",4)==0){
        char *temp = cmd+4;
        for (char *p =temp; *p != '\0'; p++){
            if (*p < '0' || *p > '9'){
                printf("Invalid Job ID!\n");
                last_exit_code = -1;
                return;
            }
        }
        int job_id = atoi(temp);
        pid_t process_id = getJob_pid(job_id);
        if(process_id){
            fg_child_pid = process_id;
            kill(fg_child_pid,SIGCONT);
            int status;
            waitpid(fg_child_pid,&status,WUNTRACED);
            fg_child_pid = -1;
        }
        else{
            printf("No such ID exists\n");
        }
        return;
    }
    else if(strcmp(cmd,"jobs")==0){
        printJob();
        return;
    }
    else if(strcmp(cmd,"echo $?") == 0){
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
        for (char *p = temp; *p !='\0' ; p++){
            if (*p < '0' || *p > '9'){
                printf("Invalid Exit Code!\n");
                last_exit_code = -1;
                return;
            }
        }
        int ec = atoi(temp);
        ec &= 0xFF;
        printf("echo $?\n%d\n",ec);
        printf("Bye!");
        exit(ec);
    }
    else{
        int background = 0;
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
                args[i++] = token;
            }
            token = strtok(NULL," ");
        }
        args[i] = NULL;
        if(i > 0 && strcmp(args[i-1],"&") == 0){
            background = 1;
            args[i-1] = NULL;
        }
        pid_t pid = fork();
        if(pid == 0){
            setpgid(0,0);
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
            if(background){
                addJob(pid,cmd);
            }
            else{
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
            if(WIFSTOPPED(temp)){
                Job *curr = job_list;
                int found = 0 ;
                while(curr != NULL){
                    if(curr->pid == pid){
                        strcpy(curr->status,"Stopped");
                        found = 1;
                        break;
                    }
                    curr = curr->next_job;
                }
                if(!found){
                    addJob(pid,cmd);
                    Job *temp = job_list;
                    while(temp!= NULL){
                        if(temp->pid == pid){
                            strcpy(temp->status,"Stopped");
                            break;
                        }
                        temp = temp->next_job;
                    }
                }
            }
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
    signal(SIGTSTP,signalHandler);
    signal(SIGINT,signalHandler);
    signal(SIGCHLD,checkBackground);
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
 