#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

#define BUFLEN 4096
#define DELIMS " \t\r\n\a"
#define LOOKUP_SIZE 150

typedef struct {
    int num_of_commands;
    char ** command_list;
    int p; // 0 if only single pipes, 1 if a double pipe, 2 if a triple pipe
}commands;

char* lookup[LOOKUP_SIZE];
char *buf;
bool skipFlag = false;

char *trimSpaces(char *str);
void DieWithError(char *s);
void DieWithoutError(char *s);
void printCommandList(commands *comm);
void SCmode(char* buf);
bool searchCharacter(char ch, char* str);
char ** tokenize(char *line);
commands* process_command(char * str);
char* findPath(char* token);
void printTokens(char **tok);
void shell(char* buf);
char* trimleadingspaces(char* buf);
char* sliceString(int left, int right, char* str);
void printLookup();

void clearBuff(char *buf, int len){
    for(int i=0; i<len; ++i){
        buf[i] = '\0';
    }
}

void DieWithError(char *s){
    perror(s);
    exit(1);
}

void DieWithoutError(char *s){
    fprintf(stdout, "Exiting...%s\n", s);
    exit(EXIT_SUCCESS);
}

void signal_handler(int signo){
    int index;
    
    fprintf(stdout, "Short cut sc mode entered\n");
    printf("Enter index: \n");
    fscanf(stdin, "%d", &index);
    fflush(stdin);
    fflush(stdout);

    if(lookup[index] == NULL) {
        fprintf(stdout, "No entry exists for index %d\n", index);
        return;
    }
    bool isBackground = searchCharacter('&', lookup[index]);
    pid_t pid;
    if((pid = fork()) < 0){
        DieWithError("fork() error in signal_handler\n");
    }
    else if(pid > 0) { //top level parent
        if(!isBackground){
            int status;
            tcsetpgrp(STDIN_FILENO, pid); //STDIN_FILENO is standard input file descriptor 0
            fflush(stdout);
            waitpid(pid, &status, WUNTRACED);
            // fprintf(stdout, "Process id is %d and exit status is %d\n", pid, status);
        }
    }
    else{
        setpgid(0, getpid()); //Commenting out this, terminates the program after 1 command only
        shell(lookup[index]);
        if(!isBackground){
            tcsetpgrp(STDIN_FILENO, getppid());
        }
        fflush(stdin);
        fflush(stdout);
        exit(0);
    }

    printf("SC mode end! Enter a single character to continue to main prompt:\n");
    fflush(stdout);
    clearBuff(buf, BUFLEN);
    skipFlag = true;
    // if(pid > 0) { //top level parent
    //     if(!isBackground) {
    //         tcsetpgrp(STDIN_FILENO, pid);
    //         waitpid(pid, NULL, WUNTRACED);
    //     }
    // }
    // else{
    //     setpgid(0, getpid());
    //     shell(lookup[index]);
    //     if(!isBackground) {
    //         tcsetpgrp(STDIN_FILENO, getppid());
    //     }
    //     exit(0);
    // }
}

int main(){
    for(int i=0; i<LOOKUP_SIZE; ++i){
        lookup[i] = NULL;
    }

    buf = malloc(sizeof(char) * BUFLEN);
    memset(buf, 0, sizeof(char) * BUFLEN);
    signal(SIGINT, signal_handler); //initiates short cut mode
    // dup2(1, 20);

    while(true){
        fprintf(stdout, ">");
        clearBuff(buf, BUFLEN);
        fgets(buf, BUFLEN, stdin);
        buf[strlen(buf)-1] = '\0';

        if(skipFlag){
            skipFlag = false;
            getchar();
            getchar();
            continue;
        }

        if(strcmp(buf, "exit") == 0) {
            DieWithoutError("main-via exit command\n");
        }
        else{
            buf = trimleadingspaces(buf);
            
            //sc mode
            if(buf[0] != '\0' && buf[1] != '\0' && buf[2]==' ' && buf[0] == 's' && buf[1] == 'c'){
                fprintf(stdout, "Entering command for SC mode\n");
                SCmode(buf);
                clearBuff(buf, BUFLEN);
                continue;
            }
            //print sc mode lookup table
            else if(strcmp(buf, "printlookup") == 0){
                printLookup();
                clearBuff(buf, BUFLEN);
            }
            else{
                //** Can modify isBackground to search for '&' only at the end of the command
                //Currently background is initiated only when the '&' is at the end
                bool isBackground = searchCharacter('&', buf);
                pid_t pid;
                if((pid = fork()) < 0){
                    DieWithError("fork() in main\n");
                }
                else if(pid > 0) { //top level parent
                    if(!isBackground){
                        int status;
                        tcsetpgrp(STDIN_FILENO, pid); //STDIN_FILENO is standard input file descriptor 0
                        waitpid(pid, &status, WUNTRACED);
                        // fprintf(stdout, "Process id is %d and exit status is %d\n", pid, status);
                    }
                }
                else{
                    setpgid(0, getpid()); //Commenting out this, terminates the program after 1 program only
                    shell(buf);
                    if(!isBackground){
                        tcsetpgrp(STDIN_FILENO, getppid());
                    }
                    clearBuff(buf, BUFLEN);
                    exit(0);
                }
            }
        }
    }

    return 0;
}

void printCommandList(commands *comm){
    fprintf(stdout, "-------------------Command List-------------------------------------------\n");
    fprintf(stdout, "No. of commands: %d\n", comm->num_of_commands);
    for(int i=0; i<comm->num_of_commands; ++i){
        // comm->command_list[i] = trimSpaces(comm->command_list[i]);
        fprintf(stdout, "%s\n", comm->command_list[i]);
    }
    fprintf(stdout, "p value: %d\n", comm->p);
    fprintf(stdout, "---------------------------------------------------------------------------\n");
}

void printLookup(){
    printf("----------------------------Lookup Table-----------------------------------\n");
    for(int i=0; i<LOOKUP_SIZE; ++i){
        if(lookup[i]!=NULL && (lookup[i][0] != '\0')){
            printf("index: %d ; command: %s\n", i, lookup[i]);
        }
    }
    printf("----------------------------------------------------------------------------\n");
}

void SCmode(char* buf){
    char option = buf[4]; //sc -i 1

    int index = atoi(&buf[6]);
    if(index >= LOOKUP_SIZE){
        printf("Enter a index less than %d\n", LOOKUP_SIZE);
    }
    // printf("index: %d\n", index);

    if(option=='i'){
        lookup[index] = (char*)malloc(sizeof(char)*LOOKUP_SIZE);
        for(int i=0; i<LOOKUP_SIZE; ++i){
            lookup[index][i] = '\0';
        }

        int i = 6;

        while((buf[i]-'0') >=0 && (buf[i]-'0') <= 9){
            ++i;
        }
        while(buf[i] == ' '){
            ++i;
        }

        strcpy(lookup[index], &buf[i]);

        printf("command: %s stored in lookup table at index %d\n", lookup[index], index);
    }
    else if(option = 'd'){
        // printf("AAAA\n");
        if(lookup[index][0] == '\0'){
            printf("No command exists at %d index\n", index);
        }
        else{
            for(int i=0; i<LOOKUP_SIZE; ++i){
                lookup[index][i] = '\0';
            }

            printf("command deleted from index: %d\n", index);
        }
    }
}

char* findPath(char* token){
    const char* path = getenv("PATH");
    // fprintf(stdout, "PATH returned by getenv(): %s\n", path);
    char* buf;
    char buf3[400];
    strcpy(buf3, path);
    char* buf2 = (char*)malloc(50*sizeof(char));
    int i = 0;int left = 0;
    while(buf3[i] != '\0') {
        if(buf3[i] == ':') {
            buf = sliceString(left, i-1, buf3);
            left = i+1;
            strcpy(buf2, buf);
            buf2 = strcat(buf2, "/");
            buf2 = strcat(buf2, token);
            if(access(buf2, X_OK) == 0) {
                return buf2;
            }
        }
        i++;
    }
    buf = sliceString(left, i-1, buf3);
    strcpy(buf2, buf);
    buf2 = strcat(buf2, "/");
    buf2 = strcat(buf2, token);
    if(access(buf2, X_OK) == 0) {
        return buf2;
    }
    return NULL;
}

void printTokens(char **tok){
    printf("------------------------------Tokens-----------------------------------\n");
    for(int i=0; i<64; ++i){
        if(tok[i]==NULL){
            break;
        }
        fprintf(stdout, "i: %d ;; %s\n", i, tok[i]);
    }
    printf("------------------------------------------------------------------------\n");
}

//Return file descriptors for remapped commands
int* getFDs(char* command){
    int* fds = (int*)malloc(2*sizeof(int));
    //Default file numbers
    fds[0] = STDIN_FILENO;
    fds[1] = STDOUT_FILENO;

    int inputdf, outputfd;
    
    char* inputfile = (char*)malloc(sizeof(char)*20);
    char* outputfile = (char*)malloc(sizeof(char)*20);
    int i = 0, j=0;
    while(command[i] != '\0'){
        if(command[i] == '<') {
            command[i-1] = '\0';
            j = 0;
            if(command[i+1] != ' '){
                ++i;
                while(command[i]!= '\0' && command[i] != ' '){
                    inputfile[j] = command[i];
                    ++j;
                    ++i;
                }
                inputfile[j] = '\0';
                int inputfd = open(inputfile, O_RDONLY);
                fds[0] = inputfd;
            }
            else{
                //single <
                ++i;
                ++i;
                while(command[i]!= '\0' && command[i] != ' '){
                    inputfile[j] = command[i];
                    ++j;
                    ++i;
                }
                inputfile[j] = '\0';
                int inputfd = open(inputfile, O_RDONLY);
                fds[0] = inputfd;
            }
        }
        else if(command[i] == '>'){
            command[i-1] = '\0';
            j = 0;
            if(command[i+1] == '>') { //append mode >>
                if(command[i+2] == ' '){
                    ++i;
                    ++i;
                    while(command[i] == ' '){
                        i++;
                    }
                    while(command[i]!= '\0' && command[i] != ' '){
                        outputfile[j] = command[i];
                        ++j;
                        ++i;
                    }
                    int outputfd = open(outputfile, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
                    fds[1] = outputfd;
                }
                else{
                    ++i;
                    ++i;
                    while(command[i]!= '\0' && command[i]!= ' '){
                        outputfile[j] = command[i];
                        ++j;
                        ++i;
                    }
                    outputfile[j] = '\0';
                    int outputfd = open(outputfile, O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
                    fds[1] = outputfd;
                }
            }
            else if(command[i+1] == ' '){ //write mode, simgle >
                ++i;
                ++i;
                while(command[i]!= '\0' && command[i] != ' '){
                    outputfile[j] = command[i];
                    ++j;
                    ++i;
                }
                outputfile[j] = '\0';
                int outputfd = open(outputfile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                fds[1] = outputfd;
            }
            else{ //write mode, single >
                ++i;
                while(command[i]!= '\0' && command[i] != ' '){
                    outputfile[j] = command[i];
                    ++j;
                    ++i;
                }
                outputfile[j] = '\0';
                int outputfd = open(outputfile, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                fds[1] = outputfd;
            }
        }
        i++;
    }
    return fds;
}

commands* process_command(char * str){
	commands* c = (commands*)malloc(sizeof(commands));
  	c->num_of_commands = 1;
    c->p = 0; 
    int i = 0;
    
    while(str[i] != '\0') {
        if(str[i] == '|'){
            if((i+1) < strlen(str) && str[i+1] == '|') {
                if((i+2) < strlen(str) && str[i+2] == '|') {
                    //triple pipe
                    c->num_of_commands++;
                    i+=2;
                    c->p = 2;
                }
                else {
                    //double pipe
                    c->num_of_commands++;
                    i+=1;
                    c-> p = 1;
                }
            }
            else {
                c->num_of_commands++;
            }
        }
        i++;
    }

    // No of commands obtained

    c->command_list = (char**) malloc(sizeof(char*) * c->num_of_commands);
    
    i = 0;
    int left = 0;
    int command_num = 0;

    while(str[i] != '\0') {
        if(str[i] == '|'){
            if(str[i+1] == '|') {
                if(str[i+2] == '|'){
                    //triple pipe
                    c->command_list[command_num] = sliceString(left, i-1, str);
                    ++command_num;
                    left = i + 3;
                    i+=2;
                }
                else {
                    //double pipe
                    c->command_list[command_num] = sliceString(left, i-1, str);
                    ++command_num;
                    left = i + 2;
                    i+=1;
                }
            }
            else {
                c->command_list[command_num] = sliceString(left, i-1, str);
                ++command_num;
                left = i + 1;
            }
        }
        i++;
    }

    c->command_list[command_num] =  sliceString(left, i-1, str);

    for(int i=0; i<c->num_of_commands; ++i){
        c->command_list[i] = trimSpaces(c->command_list[i]);
    }

    return c;
}

//Used only to search '&' for only the background processes
bool searchCharacter(char ch, char* str){
    int i = 0;
    while(str[i] != '\0') {
        if(str[i] == ch) {
            str[i] = '\0';
            return 1;
        }
        i++;
    }
    return 0;
}

char* sliceString(int left, int right, char* str) {
    char* s = (char*) malloc( (right - left + 1) * sizeof(char));
    int j = 0;
    for(int i = left; i <= right; i++){
        s[j++] = str[i];
    }

    return s;
}

char *trimSpaces(char *str){
    char *end;
    // Trim leading space
    while(*str==' '){
        ++str;
    }

    if(*str == 0){
        return str;
    }  // All spaces?

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && (*end)==' '){
        --end;
    }

    // Write new null terminator character
    end[1] = '\0';
    return str;
}

char ** tokenize(char *line) {
    int bufsize = 64, position = 0;
    char **tokens = (char**)malloc(bufsize * sizeof(char*));
    char *token;

    if(!tokens){
        DieWithError("1. allocation error in tokenize\n");
    }

    token = strtok(line, DELIMS);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens) {
                DieWithError("2. allocation error in tokenize\n");
            }
        }

        token = strtok(NULL, DELIMS);
    }
    tokens[position] = NULL;
    return tokens;
}

void shell(char* buf){
    commands* c = process_command(buf);
    printCommandList(c);
    int rem_commands = c->num_of_commands - 1;
    
    int pipeArray[c->num_of_commands + c->p][2];
    for(int i = 0; i < c->num_of_commands + c->p; i++){
        pipe(pipeArray[i]);
        fprintf(stdout, "Pipe fds of pipe %d : Read end %d, Write end %d\n", i, pipeArray[i][0], pipeArray[i][1]);
    }

    int i=0;
    while(i < c->num_of_commands){
        //tokenize ith command
        char commandbuf[50];
        strcpy(commandbuf, c->command_list[i]);
        char ** tokens = tokenize(c->command_list[i]);
        printTokens(tokens);

        //Case 1- First command in a chain of commands
        if(i == 0){
            fprintf(stdout, "Case 1: %s\n", commandbuf);
            int* filedescriptors = getFDs(commandbuf);
            if(filedescriptors[0] != STDIN_FILENO) {
                fprintf(stdout, "Standard input 0 remapped to fd %d\n", filedescriptors[0]);
            }
            else{
                fprintf(stdout, "Standard input is 0\n");
            }
            if(filedescriptors[1] != STDOUT_FILENO) {
                fprintf(stdout, "Standard output 1 remapped to fd %d\n", filedescriptors[1]);
            }
            else{
                fprintf(stdout, "Standard output is 0\n");
            }

            pid_t pid;
            if((pid = fork()) < 0){
                DieWithError("fork() error in case 1 of shell\n");
            }
            else if(pid == 0){ // First command always reads in from STDIN
                dup2(filedescriptors[0], STDIN_FILENO);
                
                // Case 1.1- The chain of commands is empty
                if(rem_commands == 0){
                    fprintf(stdout, "Case 1.1\n");
                    dup2(filedescriptors[1], STDOUT_FILENO);
                }
                else{ // Case 1.2- The chain of commands in non-empty
                    // close(pipeArray[0][0]); //Close read end of first pipe
                    fprintf(stdout, "Case 1.2\n");
                    dup2(pipeArray[0][1], STDOUT_FILENO); // Write to the pipe-array created for command communication 
                }

                tokens = tokenize(commandbuf);                
                char* result = findPath(tokens[0]);

                fprintf(stdout, "Path returned: %s\n", result);
                if(result == NULL){
                    perror("Error! No such executable file");
                }
                else{
                    if(execv(result, tokens) < 0){
                        DieWithError("execve() error in case 1 of shell\n");
                    }
                }
            }
            else{
                waitpid(pid, NULL, WUNTRACED);
                fprintf(stdout, "Process id for process running %s used is %d\n",commandbuf, pid);
            }
        }
        else if (i == c->num_of_commands - 1){ // Case 2- last command, read from some pipe, write to stdout / file descriptors
            fprintf(stdout, "Case 2: %s\n", commandbuf);
            
            if(c-> p == 0){ // Case 2.1 - No || or |||
                fprintf(stdout, "Case 2.1\n");
                int* filedescriptors = getFDs(commandbuf);
                if(filedescriptors[0] != STDIN_FILENO) {
                    fprintf(stdout, "Standard input 0 remapped to fd %d\n", filedescriptors[0]);
                }
                if(filedescriptors[1] != STDOUT_FILENO) {
                    fprintf(stdout, "Standard output 1 remapped to fd %d\n", filedescriptors[1]);
                }

                pid_t pid;
                if((pid = fork()) < 0){
                    DieWithError("fork() in case 2 of shell\n");
                }
                else if(pid == 0){
                    close(pipeArray[i-1][1]);
                    dup2(pipeArray[i-1][0], STDIN_FILENO);
                    dup2(filedescriptors[1], STDOUT_FILENO);
                    tokens = tokenize(commandbuf);

                    char* result = findPath(tokens[0]);
                    if(result == NULL){
                        perror("Error! No such executable file");
                    }
                    else{
                        if (execv(result, tokens) < 0){
                            DieWithError("execve() error in case 2.1 of shell\n");
                        }
                    }
                }
                else {
                    close(pipeArray[i-1][0]);
                    close(pipeArray[i-1][1]);
                    waitpid(pid, NULL, WUNTRACED);
                    fprintf(stdout, "Process id for process running %s used is %d\n",commandbuf, pid);
                }
            }
            else if(c->p == 1) {
                pid_t pid1;
                pid_t pid2;
                fprintf(stdout, "Case 2.2\n");
                //delimit last command using commas
                
                char* command1 = strtok(commandbuf, ",");
                char* command2 = strtok(NULL, ",");
                
                fprintf(stdout, "Command1: %s\n", command1);
                fprintf(stdout, "Command2: %s\n", command2);
                char ** tokens1 = tokenize(command1);
                char ** tokens2 = tokenize(command2);
                
                //read from pipe into a buffer
                char buf[BUFLEN];
                int nbread = read(pipeArray[c->num_of_commands - 2][0], buf, BUFLEN);

                printf("Reading from pipe end %d\n", pipeArray[c->num_of_commands - 2][0]);
                
                //write buffer into write ends of both pipes
                   
                //create two new processes, each associated with a pipe
                if((pid1 = fork()) < 0){
                    DieWithError("1. fork() in case 2.2 of shell\n");
                }
                else if(pid1 == 0){
                    close(pipeArray[c->num_of_commands -1][1]);
                    // printf("A. Closing pipe end %d in child process of %s\n", pipeArray[c->num_of_commands -1][1], command1);
                    dup2(pipeArray[c->num_of_commands - 1][0], STDIN_FILENO);
                    close(pipeArray[c->num_of_commands - 0][1]);
                    // printf("B. Closing pipe end %d by child in %s\n", pipeArray[c->num_of_commands -0][1], command1);                    
                    close(pipeArray[c->num_of_commands - 0][0]);
                    // printf("C. Closing pipe end %d by child in %s\n", pipeArray[c->num_of_commands -1][0], command1);

                    char* result = findPath(tokens1[0]);
                    if(result == NULL){
                        perror("Error! No such executable file");
                    }
                    else{
                        if(execv(result, tokens1) < 0){
                            DieWithError("1. execve() error in case 2.2 of shell\n");
                        }
                    }
                }
                else {
                    write(pipeArray[c->num_of_commands - 1][1], buf, nbread*sizeof(char));

                    // printf("D. Writing to pipe end %d by parent of %s\n", pipeArray[c->num_of_commands -1][1], command1);
                    close(pipeArray[c->num_of_commands - 1][0]);
                    // printf("E. Closing pipe end %d in parent of %s\n", pipeArray[c->num_of_commands -1][0], command1);
                    close(pipeArray[c->num_of_commands -1][1]); 
                    // printf("F. Closing pipe end %d in parent of %s\n", pipeArray[c->num_of_commands -1][1], command1);
                }

                if((pid2 = fork()) < 0){
                    DieWithError("2. fork() in case 2.2 of shell\n");
                }
                else if(pid2 == 0) {
                    close(pipeArray[c->num_of_commands][1]);
                    // printf("G. Closing pipe end %din child of %s\n", pipeArray[c->num_of_commands][1], command2);
                    dup2(pipeArray[c->num_of_commands + c->p  - 1][0], STDIN_FILENO);
                    close(pipeArray[c->num_of_commands-1][1]);
                    // printf("H. Closing pipe end %d in child of %s\n", pipeArray[c->num_of_commands -1][1], command2);
                    close(pipeArray[c->num_of_commands -1][0]);
                    // printf("I. Closing pipe end %d in child of %s\n", pipeArray[c->num_of_commands -1][0], command2);

                    char* result = findPath(tokens2[0]);
                    if(result == NULL){
                        perror("Error! No such executable file");
                    }
                    else{
                        if(execv(result, tokens2) == -1){
                            DieWithError("2. execve() error in case 2.2 of shell\n");  
                        }
                    }
                }
                else {
                    write(pipeArray[c->num_of_commands][1], buf, nbread * sizeof(char));
                    // printf("J. Writing to pipe end %d by parent of %s\n", pipeArray[c->num_of_commands][1], command2);
                    close(pipeArray[c->num_of_commands][0]);
                    // printf("K. Closing pipe end %d by parent of %s\n", pipeArray[c->num_of_commands][0], command2);
                    close(pipeArray[c->num_of_commands][1]);
                    // printf("L. Closing pipe end %d by parent of %s\n", pipeArray[c->num_of_commands][1], command2);
                    
                    waitpid(pid2, NULL, WUNTRACED);
                    fprintf(stdout, "Process id for process running %s used is %d\n", command1, pid1);                    
                    waitpid(pid1, NULL, WUNTRACED);
                    fprintf(stdout, "Process id for process running %s used is %d\n", command2, pid2);
                }
            }
            else if(c-> p == 2) {
                pid_t pid1, pid2, pid3;
                fprintf(stdout, "Case 2.3\n");

                char* command1 = strtok(commandbuf, ",");
                char* command2 = strtok(NULL, ",");
                char* command3 = strtok(NULL, ",");

                char ** tokens1 = tokenize(command1);
                char ** tokens2 = tokenize(command2);
                char ** tokens3 = tokenize(command3);

                //read from pipe into a buffer
                char buf[BUFLEN];
                int nbread = read(pipeArray[c->num_of_commands + c->p - 4][0], buf, BUFLEN);
                //write buffer into write ends of both pipes

                //create two new processes, each associated with a pipe

                if((pid1 = fork()) < 0){
                    DieWithError("1. fork() case 2.3 of shell\n");
                }
                else if(pid1 == 0) {
                    close(pipeArray[c->num_of_commands + c->p -3][1]);
                    dup2(pipeArray[c->num_of_commands + c->p - 3][0], STDIN_FILENO);
                    close(pipeArray[c->num_of_commands + c->p -2][1]);
                    close(pipeArray[c->num_of_commands + c->p -2][0]);
                    close(pipeArray[c->num_of_commands + c->p -1][1]);
                    close(pipeArray[c->num_of_commands + c->p -1][0]);

                    char* result = findPath(tokens1[0]);
                    if(result == NULL){
                        perror("Error! No such executable file");
                    }
                    else{
                        if(execv(result, tokens1) == -1){
                            DieWithError("1. execve() error in case 2.3 of shell\n");
                        }
                    }
                }
                else {
                    write(pipeArray[c->num_of_commands + c->p - 3][1], buf, nbread*sizeof(char));
                    close(pipeArray[c->num_of_commands + c->p - 3][0]);
                    close(pipeArray[c->num_of_commands + c->p - 3][1]);
                }

                
                if((pid2 = fork()) < 0){
                    DieWithError("2. fork() in case 2.3 of shell\n");
                }
                else if(pid2 == 0) {
                    //child
                    close(pipeArray[c->num_of_commands + c->p -2][1]);
                    dup2(pipeArray[c->num_of_commands + c->p - 2][0], STDIN_FILENO);
                    close(pipeArray[c->num_of_commands + c->p -1][1]);
                    close(pipeArray[c->num_of_commands + c->p -1][0]);
                    close(pipeArray[c->num_of_commands + c->p -3][1]);
                    close(pipeArray[c->num_of_commands + c->p -3][0]);

                    char* result = findPath(tokens2[0]);
                    if(result == NULL){
                        perror("Error! No such executable file");
                    }
                    else{
                        if (execv(result, tokens2) == -1) {
                            DieWithError("2. execve() error in case 2.3 of shell\n");
                        }
                    }
                }
                else {
                    write(pipeArray[c->num_of_commands + c->p - 2][1], buf, nbread*sizeof(char));
                    close(pipeArray[c->num_of_commands + c->p - 2][0]);
                    close(pipeArray[c->num_of_commands + c->p - 2][1]);
                }

                if((pid3 = fork()) < 0){
                    DieWithError("3. fork() in case 2.3 of shell\n");
                }
                else if(pid3 == 0){
                    close(pipeArray[c->num_of_commands + c->p -1][1]);
                    dup2(pipeArray[c->num_of_commands + c->p - 1][0], STDIN_FILENO);
                    close(pipeArray[c->num_of_commands + c->p -2][1]);
                    close(pipeArray[c->num_of_commands + c->p -2][0]);
                    close(pipeArray[c->num_of_commands + c->p -3][1]);
                    close(pipeArray[c->num_of_commands + c->p -3][0]);

                    char* result = findPath(tokens3[0]);
                    if(result == NULL){
                        perror("Error! No such executable file");
                    }
                    else{
                        if (execv(result, tokens3) == -1) {
                            DieWithError("3. execve() error in case 2.3 of shell\n");
                        }
                    }
                }
                else{
                    write(pipeArray[c->num_of_commands + c->p - 1][1], buf, nbread*sizeof(char));
                    close(pipeArray[c->num_of_commands + c->p - 1][0]);
                    close(pipeArray[c->num_of_commands + c->p - 1][1]);
                    
                    waitpid(pid3, NULL, WUNTRACED);
                    waitpid(pid2, NULL, WUNTRACED);
                    waitpid(pid1, NULL, WUNTRACED);
                    fprintf(stdout, "Process id for process running %s used is %d\n",command1, pid1);
                    fprintf(stdout, "Process id for process running %s used is %d\n",command2, pid2);
                    fprintf(stdout, "Process id for process running %s used is %d\n",command3, pid3);
                }
            }
        }
        else {
            pid_t pid;
            fprintf(stdout, "Case 3: %s\n", commandbuf);
            //middle command, read from some pipe, write to some pipe

            if((pid = fork()) < 0){
                DieWithError("fork() in case 3 of shell\n");
            }
            else if(pid == 0) {
                //child
                close(pipeArray[i-1][1]);
                dup2(pipeArray[i-1][0], STDIN_FILENO);
                close(pipeArray[i][0]);
                dup2(pipeArray[i][1], STDOUT_FILENO);

                char* result = findPath(tokens[0]);
                if(result == NULL){
                    perror("No such executable file");
                }
                else{
                    if (execv(result, tokens) == -1) {
                        DieWithError("execve() error in case 3 of shell\n");
                    }
                }
            }
            else {
                close(pipeArray[i-1][0]);
                close(pipeArray[i-1][1]);
                waitpid(pid, NULL, WUNTRACED);
                
                fprintf(stdout, "Process id for process running %s used is %d\n",commandbuf, pid);
            }
        }

        ++i;
        --rem_commands;
    }
}

char* trimleadingspaces(char* buf){
    while(buf[0] == ' '){
        buf++;
    }
    return buf;
}

/* TCSETGRP(int fd, int pgid) 
 *tcsetgrp sets the pid as the foreground process in the controlling terminal, which is the main terminal in our case.

 * If there is a foreground process group for a terminal, we can control the foreground process group via the terminal, e.g., Ctrl-C/Ctrl-\ to terminate the foreground process group. (A terminal may have only one foreground process group, or may not have any).

 * To sum up, with controlling terminal, kernel is aware of where to deliver terminal-generated signal and terminal input if they are expected by someone.
 
*/

/* ls > "filename" is tokenized as a single command, getFDs function is for these type of commands */