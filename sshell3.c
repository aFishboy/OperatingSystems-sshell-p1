#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define COMMAND_SIZE 513   // The maximum length of a single command, plus '\0'
#define MAX_ARG_SIZE 16    // Maximum number of parameters 16
#define MAX_TOKEN_SIZE 33  // The maximum length of a single token (parameter) is 32, plus '\0'
#define ENV_VAR_SIZE 26    // number of environment variables
#define MAX_PROCESS_SIZE 4 // Pipelines allow execution of up to four instructions
#define SUCCESS 1
#define FAIL 0

char env_var[ENV_VAR_SIZE][MAX_TOKEN_SIZE]; // array of environment variables

struct cmdLineData{
    char *cmdData[MAX_ARG_SIZE+1];
    char symbolAfterCmd[2];
    struct cmdLineData *next;
};

void init();  
int getCommandArgs(int *argc, char *stringToTok, struct cmdLineData **head, struct cmdLineData **tail); 
void listAppend(struct cmdLineData **head, struct cmdLineData **tail);
void print_list(struct cmdLineData **head);
void deallocate(struct cmdLineData **head);                                                                                                                                                                           // Initialize environment variables
int readCommand(char *command);                                                                                                                                                                            // Read commands entered by the user
int parse(char *command, int *argc, char *argv[MAX_ARG_SIZE + 1], char arg_list[MAX_ARG_SIZE][MAX_TOKEN_SIZE], int *meta_cnt, char meta_list[MAX_PROCESS_SIZE][5]);                                        // Process the commands entered by the user
int executeCmd(struct cmdLineData **head, struct cmdLineData **tail, int argc, char *argv[MAX_ARG_SIZE + 1], int meta_cnt, char meta_list[MAX_PROCESS_SIZE][5], int *cmd_size, int return_value[MAX_PROCESS_SIZE]); // Depends on the instruction, execute corresponding function
void displayErrorInfo(char *error_info);                                                                                                                                                                   // prompt error message
void displeyCompletedInfo(int cmd_size, int *return_value, char *cmd_info);                                                                                                                                // Prompt completion information

int Execvp(struct cmdLineData **currentNode);
int checkFileOpen(char *argv[]);
int processMultiCmd(char *curr_cmd[], char *next_cmd[], int curr_meta, int meta_cnt, char meta_list[MAX_PROCESS_SIZE][5], int *cmd_size, int return_value[MAX_PROCESS_SIZE], int file_des);




int main()
{
    /* Variables required by the shell to run */
    char command[COMMAND_SIZE];                  // command entered by the user
    char bak_command[COMMAND_SIZE];              // Backup, for output to stderr
   // char arg_list[MAX_ARG_SIZE][MAX_TOKEN_SIZE]; // parameter list
    char *argv[MAX_ARG_SIZE + 1];                // Command pointer, need to be filled with 1 NULL
    int argc = 0;                                // number of parameters
    int cmd_size = 0;                            // number of commands
    char meta_list[MAX_PROCESS_SIZE][5];         // number of metacharacters
    int meta_cnt;
    int return_value[MAX_PROCESS_SIZE];
    

    while (1)
    {
        /*Initialization parameters*/
        argc = 0;
        meta_cnt = 0;
        cmd_size = 0;
        struct cmdLineData *head = NULL;
        struct cmdLineData *tail = NULL; 

        // main part of the program
        fprintf(stdout, "sshell@ucd$ "); // output prompt

        if (readCommand(command) == FAIL)
        {
            // read error
            continue;
        }

        strcpy(bak_command, command);                                            // Backup command, used to output end information
        if (getCommandArgs(&argc, command, &head, &tail) == FAIL) // 处理命令
        {
            // An error occurred while processing the command, read the next command
            continue;
        }
        // if (parse(command, &argc, argv, arg_list, &meta_cnt, meta_list) == FAIL) // 处理命令
        // {
        //     // An error occurred while processing the command, read the next command
        //     continue;
        // }

        if (executeCmd(&head, &tail, argc, argv, meta_cnt, meta_list, &cmd_size, return_value) == FAIL) // start executing command
        {
            // execution error
            displeyCompletedInfo(cmd_size, return_value, bak_command);
            continue;
        }

        displeyCompletedInfo(cmd_size, return_value, bak_command);

    }
    return 0;
}

int getCommandArgs(int *argc, char *stringToTok, struct cmdLineData **head, struct cmdLineData **tail){
    const char* delimiter = " "; 
    char buffer[32];
    char *token = strtok(stringToTok, delimiter); // tokenize string from space delimiter 

    listAppend(head, tail); // create node to store current cmd line arguements and meta chars

    //loop while there are still valid tokens and are in bounds of MAX_ARGS
    while (token != NULL){ 
        // if there are too many args fail
        if ((*argc) == 16){
            displayErrorInfo("too many process arguments");
            return FAIL;
        }
        // check if token is an enviroment variable
        if (token[0] == '$'){
            // env var cannot be any size besides 3 '$' + 'letter' + '\0'
            if (strlen(token) != 3){
                displayErrorInfo("invalid variable name");
                return FAIL;
            }
            else if (token[1] >= 'a' && token[1] <= 'z'){
                // valid variable name, to replace
                //int env_var_idx = arg_list[arg_ptr][1] - 'a';
                //strcmp(arg_list[arg_ptr], env_var[env_var_idx]);//////////////////come back
            }else{
                // invalid variable name, prompt invalid
                displayErrorInfo("invalid variable name");
                return FAIL;
            }

        }
        char *delim_pos = strchr(token, '|');   // pointer to location of delimiter
        if (!delim_pos) 
            delim_pos = strchr(token, '>');

        // if the current token has a '|' or '>'
        if (delim_pos) {
            printf("token1: %s\n", token);
            printf("argc1: %i\n", *argc);
            printf("cmdlinearg: %s\n", (*tail)->cmdData[(0)]);//////////////////////////////////come back delete printf

            int tokenLength = strlen(token);
            //if token is only a metachar and no previous commands have been given fail
            if (tokenLength <= 3 && (*tail)->cmdData[(0)] == NULL){
                    displayErrorInfo("missing command");
                    return FAIL;
            }
            // if metachar is by itself
            if (tokenLength <= 3 && (token[0] == '>' || token[0] == '|')){
                printf("by itslef: \n");////////////////////////////////////////////////delete
                // check if  the metachar has a &
                if (token[delim_pos - token + 1] == '&'){
                    //add metachar to linked list
                    (*tail)->symbolAfterCmd[0] = token[delim_pos - token];
                    (*tail)->symbolAfterCmd[1] = token[delim_pos - token + 1];
                } else{
                    (*tail)->symbolAfterCmd[0] = token[delim_pos - token]; 
                    (*tail)->symbolAfterCmd[1] = '\0';   
                }
                // create a new node because last command has all its info
                listAppend(head, tail);
                *argc = 0;
                // get next token to loop again with
                token = strtok(NULL, delimiter);
            }
            // if token has metachar at beggining ie >file
            else if(token[0] == '>' || token[0] == '|'){
                printf("-1117\n");///////////////////////////////////////////////////delete
                // check for & after metachar
                if (token[1] == '&'){
                    //add metachar to linked list
                    (*tail)->symbolAfterCmd[0] = token[delim_pos - token];
                    (*tail)->symbolAfterCmd[1] = token[delim_pos - token + 1];
                    (*tail)->cmdData[*argc] = NULL;
                    // create a new node because last command has all its info
                    listAppend(head, tail);
                    *argc = 0;
                    // make token = to token without the metachar at the beginning
                    token = &token[2];
                }else{
                    // there is only one metachar
                    (*tail)->symbolAfterCmd[0] = token[delim_pos - token]; 
                    (*tail)->symbolAfterCmd[1] = '\0';
                    (*tail)->cmdData[*argc] = NULL;
                    listAppend(head, tail);
                    *argc = 0;
                    // since there is only one metachar make token = to second char in string
                    token = &token[1];
                }
            }
            // if metachar is at the end of the token ie hello>
            else if (token[tokenLength-1] == '|' || token[tokenLength-1] == '>'){
                printf("-1111\n");/////////////////////////////////////////////////////delete//////////////////////
                // copy token into a buffer that has everything except metachar
                strncpy(buffer, token, delim_pos - token);
                buffer[delim_pos - token] = '\0'; // make final char null
                (*tail)->cmdData[(*argc)++] = buffer; // store the command in linked list
                // end of linked list command so make final string null
                (*tail)->cmdData[*argc] = NULL;
                (*tail)->symbolAfterCmd[0] = token[delim_pos - token]; // store metachar
                (*tail)->symbolAfterCmd[1] = '\0'; 
                listAppend(head, tail); // end of command so make new node for next command
                *argc = 0;
                token = strtok(NULL, delimiter); // get next token
            }
            // if metachar is at the end of the token with & ie hello>&
            else if (token[tokenLength-2] == '|' || token[tokenLength-2] == '>'){
                printf("-1112\n");/////////////////////////////////////////////////delete//////////////////////////////
                // copy token into a buffer that has everything except metachar
                strncpy(buffer, token, delim_pos - token);
                buffer[delim_pos - token] = '\0'; // make final char null
                (*tail)->cmdData[(*argc)++] = buffer;
                (*tail)->cmdData[*argc] = NULL;
                (*tail)->symbolAfterCmd[0] = token[delim_pos - token]; 
                (*tail)->symbolAfterCmd[1] = token[delim_pos - token + 1];
                listAppend(head, tail);
                *argc = 0;
                token = strtok(NULL, delimiter);
            }else{ // token has no spaces ie hello>file
                printf("-1119\n");////////////////////////////////////////////////////////////////////delete
                strncpy(buffer, token, delim_pos - token); // make buffer everything before the metachar
                buffer[delim_pos - token] = '\0';
                (*tail)->cmdData[(*argc)++] = buffer; // store buffer in linked list
                token = &token[delim_pos - token]; // move token pointer to after metachare
            }
        } else {
            printf("argc2: %i\n", *argc);
            // otherwise, treat the token as a regular token
            int tokenLength = strlen(token);
            if (token[tokenLength-1] == '\n'){
                token[tokenLength-1] = '\0';
            }
            //token[strlen(token)] = '\0'; // make final char null
            (*tail)->cmdData[(*argc)++] = token;   //may need to change to: args[argc++] = strdup(token); 
            (*tail)->symbolAfterCmd[0] = '\0';
            printf("cmdlineargElse: %s\n", (*tail)->cmdData[(0)]);
            token = strtok(NULL, delimiter);   //get next token
        }
        //increment argc while also setting args to the token    
        
          
    }
    //(*tail)->cmdData[*argc] = NULL;      //set final string in array to null
    print_list(head);
    return SUCCESS;

}

void listAppend(struct cmdLineData **head, struct cmdLineData **tail){
	struct cmdLineData* newNode = malloc(sizeof(struct cmdLineData));
	newNode->next = NULL;
        for (int index = 0; index < MAX_ARG_SIZE+1; index++){
        newNode->cmdData[index] = NULL;        
        }
	if (*head == NULL){
		*head = newNode;
		*tail = newNode;
	}
	else{
		(*tail)->next = newNode;
		*tail = newNode;
	}
}

void print_list(struct cmdLineData **head){
    struct cmdLineData* current = *head;
    while (current != NULL) {
        for (int i = 0; i < MAX_ARG_SIZE; i++) {
            if (current->cmdData[i] != NULL) {
                printf("%s -> ", current->cmdData[i]);
            }
        }
        printf("\n %s\n", current->symbolAfterCmd);
        current = current->next;
    }
}

void deallocate(struct cmdLineData **head){
	struct cmdLineData* currentNode;
	while (*head != NULL){
		currentNode = *head;
		*head = (*head)->next;
		free(currentNode);
		printf("deallocated\n");
	}
}


void init()
{
    int env_var_ptr = 0;
    for (env_var_ptr = 0; env_var_ptr < 26; env_var_ptr++)
    {
        // Environment variables are initialized to empty strings
        strcpy(env_var[env_var_ptr], "");
    }
}

void displayErrorInfo(char *error_info)
{
    fprintf(stderr, "Error: %s\n", error_info);
}

void displeyCompletedInfo(int cmd_size, int *return_value, char *completed_info)
{
    if (cmd_size == 0)
        return;
    int info_len = strlen(completed_info);
    if (completed_info[info_len - 1] == '\n')
    {
        completed_info[info_len - 1] = '\0';
    }
    fprintf(stderr, "+ completed '%s' ", completed_info);
    int cmd_ptr = 0;
    for (cmd_ptr = 0; cmd_ptr < cmd_size; cmd_ptr++)
    {
        fprintf(stderr, "[%d]", return_value[cmd_ptr]);
    }
    fprintf(stderr, "\n");
}

int readCommand(char *command) // Read user-entered commands from stdin
{
    if (fgets(command, COMMAND_SIZE, stdin) != NULL)
    {
        return SUCCESS;
    }
    else
    {
        perror("fgets()");
        return FAIL;
    }
}



int executeCmd(struct cmdLineData **head, struct cmdLineData **tail, int argc, char *argv[MAX_ARG_SIZE + 1], int meta_cnt, char meta_list[MAX_PROCESS_SIZE][5], int *cmd_size, int return_value[MAX_PROCESS_SIZE])
{
    if (meta_cnt && meta_list){

    }
    char *cwd; // String to hold current working directory
    int size_of_cwd = 0;
    if(!tail){

    }
    // Check builtin commands
    if (strcmp((*head)->cmdData[0], "cd") == 0){
        if ((*head)->cmdData[1] == NULL){
            displayErrorInfo("cannot cd into directory");
            return_value[(*cmd_size)++] = 1;
            return FAIL;
        }
        else if (chdir((*head)->cmdData[1]) == -1){
            displayErrorInfo("cannot cd into directory");
            return_value[(*cmd_size)++] = 1;
            return FAIL;
        }
        return_value[(*cmd_size)++] = 0;
        return SUCCESS; 
    }
    if (strcmp((*head)->cmdData[0], "pwd") == 0){
        // loop while cwd is too smale to store full command
        while ((cwd = getcwd(NULL, size_of_cwd)) == NULL) {
            //increment size variable by 64
            size_of_cwd += 64;
            // try reallocatting with new size
            cwd = realloc(cwd, size_of_cwd);
            // if realloc fails cwd will be null
            if (cwd == NULL) {
                return_value[(*cmd_size)++] = 1;
                return FAIL;
            }
        }
        printf("%s\n", cwd);
        // deallocate cwd
        free(cwd);
        return_value[(*cmd_size)++] = 0;
        return SUCCESS;
    }
    if (strcmp((*head)->cmdData[0], "set") == 0)
    {
        printf("Called set\n");
        // if (setEnvVar(argv + arg_ptr) != FAIL)
        // {
        //     return_value[(*cmd_size)++] = 1;
        //     return FAIL;
        // }
        // return_value[(*cmd_size)++] = 0;
        return SUCCESS;
    }
    if (strcmp((*head)->cmdData[0], "exit") == 0)
    {
        fprintf(stderr, "Bye...\n");
        return_value[(*cmd_size)++] = 0;
        displeyCompletedInfo(*cmd_size, return_value, "exit");
        exit(0);
    }
    

    // Does not contain builtin commands, executes other commands sequentially (single command, redirection, pipe)
    int i = 0;//, cmd_cnt = 0;
    //char *out_buf;
   ///////// if ((*head)->symbolAfterCmd[0] == '\0')
    ///////printf("symbol: %s\n", (*head)->symbolAfterCmd);
    if ((*head)->symbolAfterCmd[0] == '\0')
    {
        // Only one command, no metacharacters
        pid_t pid = fork();
        if (pid == 0)
        {
            if (Execvp(head) == FAIL)
            {
                displayErrorInfo("command not found");
            }
            exit(1);
        }
        else if (pid > 0)
        {
            int status;
            waitpid(pid, &status, 0);
            return_value[(*cmd_size)++] = WEXITSTATUS(status);
        }
        else
        {
            perror("fork()");
        }
    }

    for (i = 0; i < argc; i++)
    {
        if (!(argv[i]))
        {
            // Here is the demarcation of the command
            processMultiCmd(argv, argv + i + 1, 0, meta_cnt, meta_list, cmd_size, return_value, -1);
            break;
        }
    }
    return SUCCESS;
}

int Execvp(struct cmdLineData **currentNode) // Error handling wrapper function for execvp()
{
    if (execvp((*currentNode)->cmdData[0], (*currentNode)->cmdData) == -1)
    {
        return FAIL;
    }
    else
    {
        return SUCCESS;
    }
}

int checkFileOpen(char *argv[])
{
    return SUCCESS;
    if (open(argv[0], O_RDWR, 00700) != -1)
    {
        return SUCCESS;
    }
    return FAIL;
}

int processMultiCmd(char *curr_cmd[], char *next_cmd[], int curr_meta, int meta_cnt, char meta_list[MAX_PROCESS_SIZE][5], int *cmd_size, int return_value[MAX_PROCESS_SIZE], int file_des)
{
    int des[2];
    //int i = 0;

    if (strcmp(meta_list[curr_meta], "|") == 0)
    {
        if (pipe(des) < 0)
        {
            perror("pipe()");
            exit(1);
        }
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        // child process execute command
        if (strcmp(meta_list[curr_meta], "|") == 0)
        {
            close(des[0]); // Close the read end of the child process
            if (file_des != -1)
                dup2(file_des, STDIN_FILENO); // The output (read) of the previous command is redirected to the child process's read
            dup2(des[1], STDOUT_FILENO);      // Use the write port of the child process as standard output
            if (Execvp(curr_cmd) == FAIL)
            {
                displayErrorInfo("command not found");
            }
            exit(1);
        }

        if (strcmp(meta_list[curr_meta], ">") == 0)
        {
            int file = open(next_cmd[0], O_RDWR | O_CREAT, 00700);
            if (file == -1)
            {
                perror("open()");
                exit(1);
            }
            else
            {
                if (file_des != -1)
                {
                    dup2(file_des, STDIN_FILENO); // The output (read) of the previous command is redirected to the child process's read
                }
                dup2(file, STDOUT_FILENO); // redirect file to standard output
                if (Execvp(curr_cmd) == FAIL)
                {
                    displayErrorInfo("command not found");
                }

                if (file != STDOUT_FILENO)
                { // restore stdout
                    close(file);
                }

                exit(1);
            }
        }
        // if (strcmp(meta_list[curr_meta], "|&") == 0)
        // {
        // }
        // if (strcmp(meta_list[curr_meta], ">&") == 0)
        // {
        // }
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        return_value[(*cmd_size)++] = WEXITSTATUS(status);
        // The next level of recursion in the parent process, > >& has been executed last time, only multi-level pipelines are processed here
        if (strcmp(meta_list[curr_meta], "|") == 0)
        {
            close(des[1]);     // Close the write end of the pipe for the parent process
            file_des = des[0]; // store the file descriptor for the read end
            if (curr_meta == meta_cnt - 1)
            {
                // The last instruction, no longer recursive
                pid_t last_pid = fork();
                if (last_pid == 0)
                {
                    dup2(des[0], STDIN_FILENO); // The redirection read by the pipe reader is standard input
                    if (Execvp(next_cmd) == FAIL)
                    {
                        displayErrorInfo("command not found");
                    }
                }
                else if (last_pid > 0)
                {
                    int status;
                    waitpid(last_pid, &status, 0);
                    return_value[(*cmd_size)++] = WEXITSTATUS(status);
                    return SUCCESS;
                }
                else
                {
                    perror("fork()");
                    exit(1);
                }
            }
            else
            {
                // keep recursing
                int i = 0;
                while (next_cmd[i])
                {

                    i++;
                }
                if (!(next_cmd[i]))
                {
                    // Here is the demarcation of the command
                    processMultiCmd(next_cmd, next_cmd + i + 1, curr_meta + 1, meta_cnt, meta_list, cmd_size, return_value, file_des);
                }
            }
        }
    }
    else
    {
        perror("fork()");
    }
    return 0;
}








int parse(char *command, int *argc, char *argv[MAX_ARG_SIZE + 1], char arg_list[MAX_ARG_SIZE][MAX_TOKEN_SIZE], int *meta_cnt, char meta_list[MAX_PROCESS_SIZE][5]) // Process the commands entered by the user
{
    // Group commands, count quantities
    // Replace the environment variable with the corresponding content
    int cmd_len = strlen(command);
    int cmd_ptr = 0;                             // Subscript pointer S
    int arg_ptr = 0;                             // Subscript pointer
   // int null_cnt = 0;                            // In order to be recognized by the execv function, NULL must be added after each command, so MAX_ARG_SIZE+null_cnt is the maximum value of argc
    char *meta_char[4] = {">", "|", ">&", "|&"}; // meta character list

    // 分割字符串
    for (cmd_ptr = 0; cmd_ptr < cmd_len; cmd_ptr++)
    {
        if (command[cmd_ptr] == ' ' && (cmd_ptr == 0 || command[cmd_ptr - 1] == ' '))
        {
            // filter consecutive spaces
            continue;
        }
        if (command[cmd_ptr] == '\n' && command[cmd_ptr - 1] == ' ')
        {
            // filter end-of-line spaces
            continue;
        }
        if (*argc == MAX_ARG_SIZE)
        {
            // The parameter has reached the maximum, and it is still reading in, the parameter exceeds the limit, and an error is prompted
            displayErrorInfo("too many process arguments");
            return FAIL;
        }
        if ((command[cmd_ptr] == ' ' || command[cmd_ptr] == '\n') && command[cmd_ptr - 1] != ' ')
        {
            // end of last parameter
            arg_list[*argc][arg_ptr] = '\0';
            (*argc)++;
            arg_ptr = 0;
            continue;
        }
        if (command[cmd_ptr] != ' ')
        {
            // If it is a non-null character, store it in the current parameter
            arg_list[*argc][arg_ptr++] = command[cmd_ptr];
            continue;
        }
    }
    // replace environment variables
    for (arg_ptr = 0; arg_ptr < *argc; arg_ptr++)
    {
        if (arg_list[arg_ptr][0] == '$')
        {
            // This is an environment variable
            if (strlen(arg_list[arg_ptr]) != 2)
            {
                // invalid variable name, prompt invalid
                displayErrorInfo("invalid variable name");
                return FAIL;
            }
            else if (arg_list[arg_ptr][1] >= 'a' && arg_list[arg_ptr][1] <= 'z')
            {
                // valid variable name, to replace
                //int env_var_idx = arg_list[arg_ptr][1] - 'a';
                //strcmp(arg_list[arg_ptr], env_var[env_var_idx]);
            }
            else
            {
                // invalid variable name, prompt invalid
                displayErrorInfo("invalid variable name");
                return FAIL;
            }
        }
    }

    // Initialize the argv array and check the arguments
    int redir_flag = 0; // tag redirection
    for (arg_ptr = 0; arg_ptr < *argc; arg_ptr++)
    {
        argv[arg_ptr] = arg_list[arg_ptr];
        int i = 0;
        for (i = 0; i < 4; i++)
        {
            // Compare whether it is a metacharacter
            if (strcmp(meta_char[i], arg_list[arg_ptr]) == 0)
            {
                // yes it's a metacharacter

                if (redir_flag) // Metacharacters appear after redirection, as error
                {
                    displayErrorInfo("mislocated output redirection");
                    return FAIL;
                }

                if (arg_ptr == 0 || arg_ptr == (*argc) - 1)
                {
                    // If a metacharacter appears as the first or last argument
                    if (arg_list[arg_ptr][0] == '>' && arg_ptr == (*argc) - 1)
                    {
                        // if redirection
                        displayErrorInfo("no output file");
                    }
                    else
                    {
                        displayErrorInfo("missing command");
                    }
                    return FAIL;
                }
                if (arg_list[arg_ptr][0] == '>')
                {
                    redir_flag = 1; // tag redirection appear
                }

                argv[arg_ptr] = NULL;
                strcpy(meta_list[*meta_cnt], arg_list[arg_ptr]);
                (*meta_cnt)++;
                break;
            }
        }
    }
    argv[arg_ptr] = NULL;

    for (arg_ptr = 0; arg_ptr < *argc; arg_ptr++)
    {
        if (arg_ptr != (*argc) && arg_list[arg_ptr][0] == '>')
        {
            if (checkFileOpen(argv + (arg_ptr + 1)) == FAIL)
            {
                displayErrorInfo("cannot open output file");
                return FAIL;
            }
        }
    }
    return SUCCESS;
}