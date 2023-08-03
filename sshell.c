#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define COMMAND_SIZE 512   // The maximum length of a single command
#define MAX_ARG_SIZE 16    // Maximum number of parameters
#define MAX_TOKEN_SIZE 32  // The maximum length of a single token (parameter)
#define ENV_VAR_SIZE 26    // number of environment variables
#define MAX_PROCESS_SIZE 4 // Pipelines allow execution of up to four instructions
#define SUCCESS 1          // execution succeed
#define FAIL 0             // fail

typedef struct
{
    char arg_list[MAX_ARG_SIZE][MAX_TOKEN_SIZE]; // parameter list
    char *argv[MAX_ARG_SIZE + 1];                // Command pointer, need to be filled with 1 NULL
    int argc;                                    // number of parameters
    char meta_list[MAX_PROCESS_SIZE][5];         // number of metacharacters
    int meta_cnt;                                // record number of metacharacters
} Argument;

typedef struct
{
    int return_size;                    // Number of instructions
    int return_value[MAX_PROCESS_SIZE]; // Save return value
} CompletedInfo;

char env_var[ENV_VAR_SIZE][MAX_TOKEN_SIZE]; // array of environment variables

/* Initialize environment variables */
void initEnvVar()
{
    int env_var_ptr = 0;
    for (env_var_ptr = 0; env_var_ptr < 26; env_var_ptr++)
    {
        // environment variables are initialized to empty strings
        strcpy(env_var[env_var_ptr], "");
    }
}

/* Initialization parameters */
void initArg(Argument *arg)
{
    (*arg).argc = 0;
    (*arg).meta_cnt = 0;

    arg->argc = 0;
    arg->meta_cnt = 0;
}

/* Initialize return information */
void initCompleted(CompletedInfo *completed)
{
    completed->return_size = 0;
}

/* prompt error message */
void displayErrorInfo(char *error_info)
{
    fprintf(stderr, "Error: %s\n", error_info);
}

/* Prompt completion information */
void displayCompletedInfo(CompletedInfo completed, char *completed_info)
{
    if (completed.return_size == 0)
        return;
    int info_len = strlen(completed_info);
    if (completed_info[info_len - 1] == '\n')
    {
        completed_info[info_len - 1] = '\0';
    }
    fprintf(stderr, "+ completed '%s' ", completed_info);
    int cmd_ptr = 0;
    for (cmd_ptr = 0; cmd_ptr < completed.return_size; cmd_ptr++)
    {
        fprintf(stderr, "[%d]", completed.return_value[cmd_ptr]);
    }
    fprintf(stderr, "\n");
}

/* error handling wrapper function */
int Execvp(char *argv[]) // Error handling wrapper function for execvp()
{
    if (execvp(argv[0], argv) == -1)
    {
        displayErrorInfo("command not found");
        return FAIL;
    }
    else
    {
        return SUCCESS;
    }
}

int Open(char *argv[]) // Error handling wrapper function for open()
{
    if (open(argv[0], O_RDWR | O_CREAT | O_TRUNC, 00700) != -1)
    {
        return SUCCESS;
    }
    return FAIL;
}

/* Read commands entered by the user */
void readCommand(char *command)
{
    // if command gets value from stdin
    fgets(command, COMMAND_SIZE, stdin) ;
    
    // Print command line if stdin is not provided by terminal 
    if (!isatty(STDIN_FILENO)) 
    {
        printf("%s", command);
        fflush(stdout);
    }
}

/* Split commands into arguments */
int splitCommand(char *command, Argument *arg)
{

    int cmd_len = strlen(command);
    int cmd_ptr = 0;
    int arg_ptr = 0;
    if (strlen(command) == 1)
        return FAIL; // empty string with only carriage return
    for (cmd_ptr = 0; cmd_ptr < cmd_len; cmd_ptr++)
    {
        // ls -a -l(\n\0)
        if (command[cmd_ptr] == ' ' && (cmd_ptr == 0 || command[cmd_ptr - 1] == ' '))
        { // filter consecutive spaces
            continue;
        }
        if (command[cmd_ptr] == '\n' && command[cmd_ptr - 1] == ' ')
        { // filter end-of-line spaces
            continue;
        }
        if (command[cmd_ptr] == '"' && command[cmd_ptr - 1] != '\\')
        { // filter quotes if they are not escaped first 
            continue;
        }
        if (arg->argc == MAX_ARG_SIZE)
        {
            // The parameter has reached the maximum, and it is still reading in, the parameter exceeds the limit, and an error is prompted
            displayErrorInfo("too many process arguments");
            return FAIL;
        }
        if ((command[cmd_ptr] == ' ' || command[cmd_ptr] == '\n') && command[cmd_ptr - 1] != ' ')
        {
            // end of last parameter
            arg->arg_list[arg->argc][arg_ptr] = '\0';
            (arg->argc)++;
            arg_ptr = 0;
            continue;
        }
        if (command[cmd_ptr] != ' ')
        {
            // If it is a non-null character, first see whether it is a metacharacter, and then add it to arg_list

            if (command[cmd_ptr] == '>' || command[cmd_ptr] == '|')
            {
                if (cmd_ptr > 0 && command[cmd_ptr - 1] != ' ')
                {
                    // example： echo hello> file store the last parameter
                    arg->arg_list[arg->argc][arg_ptr] = '\0';
                    (arg->argc)++;
                    arg_ptr = 0;
                }

                // store the current character in the argument list
                arg->arg_list[arg->argc][arg_ptr++] = command[cmd_ptr];
                if (cmd_ptr + 1 < cmd_len && command[cmd_ptr + 1] == '&')
                {
                    // When >& |& is encountered, move the pointer back and store it in the parameter list
                    cmd_ptr++;
                    arg->arg_list[arg->argc][arg_ptr++] = command[cmd_ptr];
                }

                if (command[cmd_ptr + 1] != ' ')
                {
                    // In the form of echo hello >file to store the current parameters
                    arg->arg_list[arg->argc][arg_ptr] = '\0';
                    (arg->argc)++;
                    arg_ptr = 0;
                }
                continue;
            }
            arg->arg_list[arg->argc][arg_ptr++] = command[cmd_ptr];
            continue;
        }
    }
    return SUCCESS;
}

/* replace environment variables */
int replaceEnvVar(Argument *arg)
{
    int arg_ptr = 0;
    for (arg_ptr = 0; arg_ptr < arg->argc; arg_ptr++)
    {
        if (arg->arg_list[arg_ptr][0] == '$')
        {
            // This is an environment variable
            if (strlen(arg->arg_list[arg_ptr]) != 2)
            {
                // Invalid variable name, prompt invalid
                displayErrorInfo("invalid variable name");
                return FAIL;
            }
            else if (arg->arg_list[arg_ptr][1] >= 'a' && arg->arg_list[arg_ptr][1] <= 'z')
            {
                // Valid variable name, to replace
                int env_var_idx = arg->arg_list[arg_ptr][1] - 'a';
                strcpy(arg->arg_list[arg_ptr], env_var[env_var_idx]);
            }
            else
            {
                // Invalid variable name, prompt invalid
                displayErrorInfo("invalid variable name");
                return FAIL;
            }
        }
    }
    return SUCCESS;
}

/* fill command pointer */
int fillArg(Argument *arg)
{
    char *meta_char[4] = {">", "|", ">&", "|&"}; // metacharacter list
    int arg_ptr = 0;
    int redir_flag = 0; // tag redirect
    for (arg_ptr = 0; arg_ptr < arg->argc; arg_ptr++)
    {
        arg->argv[arg_ptr] = arg->arg_list[arg_ptr];
        int i = 0;
        for (i = 0; i < 4; i++)
        {
            // Compare whether it is a metacharacter
            if (strcmp(meta_char[i], arg->arg_list[arg_ptr]) == 0)
            {
                // is a metacharacter
                if (redir_flag) // Metacharacters appear after redirection, appear as error
                {
                    displayErrorInfo("mislocated output redirection");
                    return FAIL;
                }

                if (arg_ptr == 0 || arg_ptr == arg->argc - 1)
                {
                    // If a metacharacter appears as the first or last argument
                    if (arg->arg_list[arg_ptr][0] == '>' && arg_ptr == arg->argc - 1)
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
                if (arg->arg_list[arg_ptr][0] == '>')
                {
                    redir_flag = 1; // tag redirection appears
                }

                arg->argv[arg_ptr] = NULL;
                strcpy(arg->meta_list[arg->meta_cnt], arg->arg_list[arg_ptr]);
                (arg->meta_cnt)++;
                break;
            }
        }
    }
    arg->argv[arg_ptr] = NULL;
    return SUCCESS;
}

/* check redirection */
int checkRedir(Argument *arg)
{
    int arg_ptr = 0;
    // Loop though arguements checking for redirection
    for (arg_ptr = 0; arg_ptr < arg->argc; arg_ptr++)
    {
        if (arg_ptr != arg->argc && arg->arg_list[arg_ptr][0] == '>')
        {
            if (Open(arg->argv + (arg_ptr + 1)) == FAIL)
            {
                displayErrorInfo("cannot open output file");
                return FAIL;
            }
        }
    }
    return SUCCESS;
}

/* Handle commands entered by the user */
int parse(char *command, Argument *arg)
{
    // split string
    if (splitCommand(command, arg) == FAIL)
    {
        return FAIL;
    }

    // replace environment variables
    if (replaceEnvVar(arg) == FAIL)
    {
        return FAIL;
    }

    // Initialize the argv array and check the arguments
    if (fillArg(arg) == FAIL)
    {
        return FAIL;
    }

    // check redirection
    if (checkRedir(arg) == FAIL)
    {
        return FAIL;
    }

    return SUCCESS;
}

/* Check builtin commands */
int checkBuiltinCmd(Argument arg, CompletedInfo *completed)
{
    // check cd command
    if (strcmp(arg.arg_list[0], "cd") == 0)
    {
        if (chdir(arg.arg_list[1]) == -1) // attempt to change directory
        {
            displayErrorInfo("cannot cd into directory");
            completed->return_value[(completed->return_size)++] = 1;
            return FAIL;
        }
        completed->return_value[(completed->return_size)++] = 0;
        return SUCCESS;
    }

    // check pwd command
    if (strcmp(arg.arg_list[0], "pwd") == 0)
    {
        char *cwd; // String to hold current working directory
        int size_of_cwd = 0;
        // loop while cwd is too smale to store full command
        while ((cwd = getcwd(NULL, size_of_cwd)) == NULL) {
            size_of_cwd += 64;
            // try reallocatting with new size
            cwd = realloc(cwd, size_of_cwd);
            // if realloc fails cwd will be null
            if (cwd == NULL) 
            {
                completed->return_value[(completed->return_size)++] = 1;
                return FAIL;
            }
        }
        printf("%s\n", cwd);
        free(cwd); // deallocate cwd
        completed->return_value[(completed->return_size)++] = 0;
        return SUCCESS;
    }

    // check set command
    if (strcmp(arg.arg_list[0], "set") == 0)
    {
        int env_var_idx = arg.arg_list[1][0] - 'a';
        strcpy(env_var[env_var_idx], arg.arg_list[2]);
        return SUCCESS;
    }

    // check exit command
    if (strcmp(arg.arg_list[0], "exit") == 0)
    {
        fprintf(stderr, "Bye...\n");
        completed->return_value[(completed->return_size)++] = 0;
        displayCompletedInfo(*completed, "exit");
        exit(0);
    }

    return FAIL;
}

/* The child process executes this function when the pipeline command */
int pipeCmd(char *curr_cmd[], int des[2], int file_des, int pipeERR)
{
    close(des[0]); // Close the read end of the child process
    if (file_des != -1)
        dup2(file_des, STDIN_FILENO); // The output (read) of the previous command is redirected to the child process's read
    dup2(des[1], STDOUT_FILENO);      // Use the write port of the child process as standard output
    if (pipeERR)
        dup2(des[1], STDERR_FILENO);      // Also se the write port of the child process as standard error
    if (Execvp(curr_cmd) == FAIL)
    {
        exit(1);
    }
    return FAIL;
}

/* The child process executes this function when the command is redirected */
int redirCmd(char *curr_cmd[], char *next_cmd[], int file_des, int redirectERR)
{
    /* Open a file in read-write mode or create it and truncate contents 00700 specifies 
    the files permissions. File can be read written and excecuted by owner of file*/
    int file = open(next_cmd[0], O_RDWR | O_CREAT | O_TRUNC, 00700); 
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
        if (redirectERR){
            dup2(file, STDERR_FILENO); // also redirect file to standard error
        }
        if (Execvp(curr_cmd) == FAIL)
        {
            exit(1);
        }
    }
    return SUCCESS;
}

/* simgle command */
int singleCmd(char *argv[MAX_ARG_SIZE + 1], CompletedInfo *completed)
{
    // include builtin
    // Only one command, no metacharacters
    pid_t pid = fork();
    if (pid == 0) // This is the child
    {
        Execvp(argv);
        exit(1);
    }
    else if (pid > 0) // This is the parent
    {
        int status;
        waitpid(pid, &status, 0);
        completed->return_value[(completed->return_size)++] = WEXITSTATUS(status);
    }
    else
    {
        perror("fork()");
        return FAIL;
    }
    return SUCCESS;
}

/* Recursive implementation of multi-level commands */
int multiCmd(char *curr_cmd[], char *next_cmd[], int curr_meta, Argument arg, CompletedInfo *completed, int file_des)
{
    int des[2]; // Store the file descriptor generated by the pipeline

    if ((strcmp(arg.meta_list[curr_meta], "|") == 0) || (strcmp(arg.meta_list[curr_meta], "|&") == 0))
    {
        if (pipe(des) < 0)
        {
            perror("pipe()");
            return FAIL;
        }
    }

    pid_t pid = fork();

    if (pid == 0)
    {
        int pipeERR;
        int redirectERR;
        // child process execute command
        if (strcmp(arg.meta_list[curr_meta], "|") == 0)
        {
            pipeERR = 0;
            pipeCmd(curr_cmd, des, file_des, pipeERR);
        }
        else if (strcmp(arg.meta_list[curr_meta], ">") == 0)
        {
            redirectERR = 0;
            redirCmd(curr_cmd, next_cmd, file_des, redirectERR);
        }
        // check combination redirection
        if (strcmp(arg.meta_list[curr_meta], "|&") == 0)
        {
            pipeERR = 1;
            pipeCmd(curr_cmd, des, file_des, pipeERR);
        }
        else if (strcmp(arg.meta_list[curr_meta], ">&") == 0)
        {
            redirectERR = 1;
            redirCmd(curr_cmd, next_cmd, file_des, redirectERR);
        }
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        completed->return_value[(completed->return_size)++] = WEXITSTATUS(status);

        // The next level of recursion in the parent process, > >& has been executed last time, only multi-level pipelines are processed here
        if ((strcmp(arg.meta_list[curr_meta], "|") == 0) || (strcmp(arg.meta_list[curr_meta], "|&") == 0))
        {
            close(des[1]);     // Close the write end of the parent process pipe
            file_des = des[0]; // store the file descriptor for the read end
        }

        if (curr_meta != arg.meta_cnt - 1)
        {
            // Not the last one, continue recursively
            int i = 0;
            while (next_cmd[i])
                i++;
            if (!(next_cmd[i])) // Here is the edge of the command
                return multiCmd(next_cmd, next_cmd + i + 1, curr_meta + 1, arg, completed, file_des);
        }

        // the last one
        int meta_pipe = strcmp(arg.meta_list[curr_meta], "|&");
        if ((strcmp(arg.meta_list[curr_meta], "|") == 0) || (meta_pipe == 0))
        {
            // no longer recursive
            pid_t last_pid = fork();
            if (last_pid == 0)
            {
                dup2(des[0], STDIN_FILENO); // The redirection read by the pipe reader is standard input
                if (Execvp(next_cmd) == FAIL)
                {
                    exit(1);
                }
            }
            else if (last_pid > 0) // This is the parent
            {
                int status;
                waitpid(last_pid, &status, 0);
                completed->return_value[(completed->return_size)++] = WEXITSTATUS(status);
                return SUCCESS;
            }
            else
            {
                perror("fork()");
                return FAIL;
            }
        }
        if (strcmp(arg.meta_list[curr_meta], ">") == 0) // if its redirection
        {
            return SUCCESS;
        }
    }
    else
    {
        perror("fork()");
        return FAIL;
    }
    return SUCCESS;
}

/* check the command and execute the corresponding function */
int executeCmd(Argument arg, CompletedInfo *completed)
{
    //int arg_ptr = 0; // Parameter pointer, pointing to arg_list

    // Check builtin commands
    if (checkBuiltinCmd(arg, completed) == SUCCESS)
    {
        return SUCCESS;
    } // FAIL No need to return, there are other commands

    // Does not contain builtin commands, executes other commands sequentially (single command, redirection, pipe)
    int i = 0;

    // single command, executed directly
    if (arg.meta_cnt == 0)
    {
        return singleCmd(arg.argv, completed);
    }
    else
    {
        // Multiple commands, find the first NULL, as a division of instructions
        while ((arg.argv[i]))
            i++;
        return multiCmd(arg.argv, arg.argv + i + 1, 0, arg, completed, -1);
    }

    return SUCCESS;
}

int main()
{
    /* shell Variables needed */
    char command[COMMAND_SIZE];     // command entered by the user
    char bak_command[COMMAND_SIZE]; // Backup, for output to stderr
    Argument arg;                   // Parameter struct, including parameter list, command pointer and metacharacter data
    CompletedInfo completed;        // Complete information struct and record the return value of the command

    initEnvVar();

    while (1)
    {
        /* Initialization parameters */
        initArg(&arg);
        initCompleted(&completed);

        /* main */
        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        // /* Get command line */
        readCommand(command);
    
        strcpy(bak_command, command);     // Backup command, used to output ending information
        if (parse(command, &arg) == FAIL) // process command
        {
            // If there is an error in the processing command, the next command will be read directly without prompting for completion information
            continue;
        }

        if (executeCmd(arg, &completed) == FAIL) // start executing command
        {
            // Execution error, some syscalls failed to execute
            continue;
        }

        displayCompletedInfo(completed, bak_command);
    }
    return 0;
}
