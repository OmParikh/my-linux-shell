#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>


#define MAX_INPUT_SIZE 1024
#define MAX_ARGUMENTS 10


void tokenize(char *input, char **args);

//function to print the prompt of current working directory
void directoryprompt() {
    char host[MAX_INPUT_SIZE];
    gethostname(host, sizeof(host));
    char path[MAX_INPUT_SIZE];
    getcwd(path, sizeof(path));
    printf("%s@%s:%s$ ", getenv("USER"), host, path);
}

//function to handle commands with pipe '|'

void pipe_handler(char **command1, char **command2) {
    
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) < 0) {
        perror("Pipe could not be initialized");
        return;
    }

    p1 = fork();
    if (p1 < 0) {
        perror("Could not fork");
        return;
    }

    if (p1 == 0) {
        
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(command1[0], command1) < 0) {
            perror("Could not execute command 1");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent executing
        p2 = fork();

        if (p2 < 0) {
            perror("Could not fork");
            return;
        }

        
        if (p2 == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            if (execvp(command2[0], command2) < 0) {
                perror("Could not execute command 2");
                exit(EXIT_FAILURE);
            }
        } else {
            // parent executing, waiting for two children
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(p1, NULL, 0);
            waitpid(p2, NULL, 0);
        }
    }
}


//function to handle IO redirection
void redirect_handler(char *output_file, char *command) {
    int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (output_fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    FILE *input_file;
    if (access(command, F_OK) == 0) {
        // If the command exists as a file, open it
        input_file = fopen(command, "r");
    } else {
        // If the command is not a file, execute it using popen
        input_file = popen(command, "r");
    }

    if (input_file == NULL) {
        perror("open/popen");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_INPUT_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_file)) > 0) {
        if (write(output_fd, buffer, bytes_read) != bytes_read) {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }

    fclose(input_file);
    close(output_fd);
}

//function to handle the cat command
void cat_handler(char *path){
    
    FILE *fp;
    char buffer;
    if(*path=='/'){     
        printf("%s is a directory",path);
        return ;
    }    
    fp=fopen(path,"r");
        if(fp==NULL){
            printf("ERROR! FILE %s NOT FOUND\n",path);
            return ;
        }
        while(fscanf(fp,"%c",&buffer)!=EOF){
            printf("%c",buffer);
        }
    return;
}

//function to handle the wc command
void wc_handler(const char *path) {
    FILE *fp;
    char buffer;
    int char_count = 0;
    int word_count = 0;
    int line_count = 0;
    int in_word = 0;

    if (*path == '/') {
        printf("%s is a directory\n", path);
        return;
    }
    
    if (*path == '.') {
        return;
    }

    fp = fopen(path, "r");
    if (fp == NULL) {
        printf("ERROR! FILE %s NOT FOUND\n", path);
        return;
    }

    while (fscanf(fp, "%c", &buffer) != EOF) {
        char_count++;

        // Check for newline character
        if (buffer == '\n') {
            line_count++;
        }

        // Check for word boundary
        if (buffer == ' ' || buffer == '\n' || buffer == '\t') {
            in_word = 0;
        } else if (in_word == 0) {
            in_word = 1;
            word_count++;
        }
    }

    fclose(fp);

    printf(" %d  %d  %d %s\n", line_count, word_count, char_count, path);
}

// function to implement mkdir command
void mkdir_handler(const char *dir_name) {
    int result = mkdir(dir_name, 0777); // 0777 gives full read, write, and execute permissions for owner, group, and others
    if (result == 0) {
        printf("Directory %s created successfully\n", dir_name);
    } else {
        perror("mkdir");
    }
}
 
//function to implement the ls command
void ls_directory(const char *directory_path) {
    if (strcmp(directory_path, "--help") == 0) {
    printf("ls: List directory contents\n");
    printf("Usage: ls [path] (path is optional)\n");
    return;
    }
    DIR *directory = opendir(directory_path);
    if (directory == NULL) {
        perror("Error opening directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    closedir(directory);
}

//Function to get the command, state and TTY of a process
void read_process_info(const char *pid) {
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%s/stat", pid);

    FILE *stat_file = fopen(stat_path, "r");
    if (stat_file == NULL) {
        perror("Error opening stat file");
        return;
    }

    
    char comm[256], state;
    int tty;
    

    fscanf(stat_file, "%*d %s %c %*d %*d %*d %*d %*u %*lu %*lu %*lu %*lu %*lu %*ld %*ld %*ld %*ld %*ld %*ld %*ld %*ld %*ld %*llu %*lu %*ld %*lu %*lu %*lu %d", comm, &state, &tty);

    printf("PID: %s, Command: %s, State: %c, TTY: %d\n", pid, comm, state, tty);

    fclose(stat_file);
}
//function to print help
void print_help() {
    printf("Available commands:\n");
    printf("  help         Display help information\n");
    printf("  ls [path]    List directory contents\n");
    printf("  ps           Display process information\n");
    printf("  exit         Exit the shell\n");
    printf("  echo [...]   Display a message\n");
    printf("  cat [file]   Display the contents of a file\n");
    printf("  wc [file]    Display the line, word, and byte counts for a file\n");
    printf("  mkdir [dir]  Create a directory\n");
    printf("  cd [dir]     Change the current working directory\n");
    printf("  [command]    Execute a system command\n");
}


//Function to implement ps command
void ps_process_list() {
    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Error opening /proc directory");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        // Check if the entry is a directory and its name consists of digits (PID)
        if (entry->d_type == DT_DIR && strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
            read_process_info(entry->d_name);
        }
    }
    

    closedir(proc_dir);
}
void tokenize(char *input, char **args) {
   
    int arg_count = 0;
    char *token = strtok(input, " \t\n");
    while (token != NULL && arg_count < MAX_ARGUMENTS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[arg_count] = NULL;
}

void commandhandler(char *input) {
    char *args[MAX_ARGUMENTS];
    char *token;
    int arg_count = 0;
    token = strtok(input, " \t\n");
    while (token != NULL && arg_count < MAX_ARGUMENTS - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[arg_count] = NULL;

    // check for pipe '|'
    for (int i = 0; i < arg_count; ++i) {
        if (strcmp(args[i], "|") == 0) {
            if (i > 0 && i < arg_count - 1) {
                char *command1[MAX_ARGUMENTS];
                char *command2[MAX_ARGUMENTS];

                // Split the commands at the pipe
                for (int j = 0; j < i; ++j) {
                    command1[j] = args[j];
                }
                command1[i] = NULL;

                for (int j = i + 1, k = 0; j < arg_count; ++j, ++k) {
                    command2[k] = args[j];
                }
                command2[arg_count - i - 1] = NULL;

                // Execute commands on both sides of the pipe
                pipe_handler(command1, command2);
                return;
            } else {
                printf("Invalid use of |\n");
                return;
            }
        }
    }
      
    // Check for IO redirection
    for (int i = 0; i < arg_count; ++i) {
        if (strcmp(args[i], ">") == 0) {
            if (i > 0 && i < arg_count - 1) {
                // Redirect output to the specified file
                redirect_handler(args[i + 1], args[i-1]);
                return;
            } else {
                printf("Invalid use of >\n");
                return;
            }
        }
    }
    
    if (arg_count == 0) {
        return;
    }
    if (strcmp(args[0], "exit") == 0) {                            // 'exit' command
        printf("Exiting the shell....!\n");
        exit(EXIT_SUCCESS);
    } else if (strcmp(args[0], "ls") == 0) {                      // 'ls' command
        
        const char *directory_path = (arg_count > 1) ? args[1] : ".";

     
            ls_directory(directory_path);
    }
      else if (strcmp(args[0], "ps") == 0) {                       // 'ps' command  
        ps_process_list();
    }
       else if (strcmp(args[0], "echo") == 0) {                    // 'echo' command
        if(arg_count == 1)
         printf("\n ");
        else{
         for (int i = 1; i < arg_count; i++) {
          printf("%s ", args[i]);
         }
         printf("\n");
        }
        
    }
        
    else if(strcmp(args[0], "cat") == 0){                          // 'cat' command  
    
    char *filename = (arg_count > 1) ? args[1] : ".";
    cat_handler(filename);
    
    }
    else if(strcmp(args[0], "wc") == 0){                           //  'wc' command
   
   char *filename = (arg_count > 1) ? args[1] : ".";
   wc_handler(filename);
   
   }
   else if (strcmp(args[0], "mkdir") == 0) {                      // 'mkdir' command
        
        const char *directory = (arg_count > 1) ? args[1] : ".";

     
            mkdir_handler(directory);
    }
   
    else if (strcmp(args[0], "cd") == 0) {                        //handling 'cd' command using chdir here itself (no function created)
        if (args[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } 
        else
         {
            if (chdir(args[1]) != 0) {
                perror("chdir");
            }
        }
    }    
    else if (strcmp(args[0], "help") == 0) {
        print_help();
    } else {
       
        printf("mysh: command not found: %s\n", args[0]);
    }
     
}


int main() {
      while (1) {
    
    directoryprompt();
        char *input = readline(">> ");
        if (input == NULL) {
            printf("\nExiting the shell...!\n");
            break;
        }

        if (strlen(input) > 0) {
            add_history(input);
            commandhandler(input);
        }

        free(input);
    }

    return 0;
}

