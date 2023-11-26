#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "tokenizer.h"

/* Convenience macro to silence compiler warnings about unused function parameters. */
#define unused __attribute__((unused))
/* Whether the shell is connected to an actual terminal or not. */
bool shell_is_interactive;

/* File descriptor for the shell input */
int shell_terminal;

/* Terminal mode settings for the shell */
struct termios shell_tmodes;

/* Process group id for the shell */
pid_t shell_pgid;

int cmd_exit(struct tokens* tokens);
int cmd_help(struct tokens* tokens);
int cmd_pwd(struct tokens* tokens);
int cmd_cd(struct tokens* tokens);
int exec_programs(struct tokens* tokens);
char* pre_redirect(struct tokens* tokens, bool* redirect_output); 
void redirect(char* file_name, bool redirect_output);
void exec_programs_helper(char* exe_name, char** arguments);

int find_num_pipes(struct tokens* tokens);
void close_pipes_array(int** pipe_array, int num_pipes);
void find_execv_arguments(struct tokens* token, char** exe_name, char*** arguments, int j);

/* Built-in command functions take token array (see parse.h) and return int */
typedef int cmd_fun_t(struct tokens* tokens);

/* Built-in command struct and lookup table */
typedef struct fun_desc {
  cmd_fun_t* fun;
  char* cmd;
  char* doc;
} fun_desc_t;

fun_desc_t cmd_table[] = {
    {cmd_help, "?", "show this help menu"},
    {cmd_exit, "exit", "exit the command shell"},
    {cmd_pwd, "pwd", "print current directory's absolute path"},
    {cmd_cd, "cd", "usage : cd <directory path> , change directory"},
};


/* helper function to get absolute path */
void get_absolute_path(char absolute_path[], char path[], char program[]) {
        absolute_path[0] = '\0';
        strcat(absolute_path, path);
        strcat(absolute_path, "/");
        strcat(absolute_path, program);
}

/* helper function to print error & exit */
void failed_to_error_exit(char* message) {
    fprintf(stdout, "failed to %s\n", message);
    exit(0);
}

/* helper function to print error */
void failed_to_error(char* message) {
    fprintf(stdout, "failed to %s\n", message);
}

/* Prints a helpful description for the given command */
int cmd_help(unused struct tokens* tokens) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    printf("%s - %s\n", cmd_table[i].cmd, cmd_table[i].doc);
  return 1;
}

/* Exits this shell */
int cmd_exit(unused struct tokens* tokens) { exit(0); }

/* print the absolute path of current directory */
int cmd_pwd(unused struct tokens* tokens) {
  char pwd[1024];
  char* success = getcwd(pwd, 1024);
  if (!success) {
    failed_to_error_exit("get pwd");
  }
  printf("%s\n", pwd);
  return 0;
}

/* change directory */
int cmd_cd(struct tokens* tokens) {
  assert(tokens_get_length(tokens) == 2);

  int failed = chdir(tokens_get_token(tokens, 1));
  if (failed) {
    printf("nu such directory : '%s'\n", tokens->tokens[1]); 
  }
  return 0;
}

/**
 *  try to find '<' or '>' and the [file_name]
 * 
 *  if find return the [file_name]
 *  if find '>', change [redirect_output] to true
 * 
 *  cut the tokens from '<' / '>' if any 
*/

char* pre_redirect(struct tokens* tokens, bool* redirect_output) {
  int len = tokens_get_length(tokens);
  char* pointer;
  char* file_name = NULL;
  for(int i = 0; i < len; i++) {
    pointer = tokens_get_token(tokens, i);
    if (strcmp(pointer, ">") == 0 || strcmp(pointer, "<") == 0) {
      // maybe there is no file name after the redirection symbol
      if (i + 1 < len) {
        char* file = tokens_get_token(tokens, i + 1);
        file_name = (char*) malloc(strlen(file) + 1);
        strcpy(file_name, file);
        if (strcmp(pointer, ">") == 0) {
          *redirect_output = true;  
        }
      } 
      // free the token  '<' or '>' and after token(s)
      for (int j = i; j < len; j++) {
        free(tokens->tokens[j]);
      }
      // set tokens[i] = NULL & tokens_length
      tokens->tokens_length = i;
      tokens->tokens[i] = NULL;
      break;
    }
  }
  return file_name;
}

/** 
 *  if [redirect_output] redirect STDOUT_FILENO to file_name's fd
 *  else redirect STDIN_FILENO to file_name's fd
*/
void redirect(char* file_name, bool redirect_output) {
  int new_fd;
  if (redirect_output) {
    if ((new_fd = open(file_name, O_WRONLY)) < 0) {
      fprintf(stderr, "redirect failed: couldn't open file '%s'\n", file_name);
    }
    if (dup2(new_fd, STDOUT_FILENO) < 0) {
      fprintf(stderr, "redirect failed: dup2 failed\n");
    }
  } else {
    if ((new_fd = open(file_name, O_RDONLY)) < 0) {
      fprintf(stderr, "redirect failed: couldn't open file '%s'\n", file_name);
    }
    if (dup2(new_fd, STDIN_FILENO) < 0) {
      fprintf(stderr, "redirect failed: dup2 failed\n");
    }
  }
}

void exec_programs_helper(char* exe_name, char** arguments) {

}

int find_num_pipes(struct tokens* tokens) {
  int len = tokens_get_length(tokens);
  int result = 0;
  for(int i = 0; i < len; i++) {
    if (strcmp(tokens_get_token(tokens, i), "|") == 0) {
      result++;
    }
  }
  return result;
}

void close_pipes_array(int** pipe_array, int num_pipes) {
    for(int i = 0; i < num_pipes; i++) {
      close(pipe_array[i][0]);
      close(pipe_array[i][1]);
    }
}

void find_execv_arguments(struct tokens* tokens, char** exe_name, char*** arguments, int i) {
          int len = tokens_get_length(tokens);
          int cnt = 0; // num of "|"
          char* current = NULL;
          for(int j = 0; j < len; j++) {
            current = tokens_get_token(tokens, j);
            if (cnt == i) {
              // begin collecting the arguments
              *exe_name = current;
              //fprintf(stderr, "exe_name : %s\n", exe_name);
              int arguments_cnt = 1;
              int index = j + arguments_cnt;
              while (index < len && strcmp(current, "|") != 0) {
                arguments_cnt++; 
                index = j + arguments_cnt;
                current = tokens_get_token(tokens, index);
              }
              //fprintf(stderr, "1 : %s   2 : %s\n", tokens_get_token(tokens, j), tokens_get_token(tokens, j + 1));
              //fprintf(stderr, "arguments_cnt : %d\n", arguments_cnt);

              *arguments = (char**) malloc((arguments_cnt + 1) * sizeof(char *)); 
              (*arguments)[0] = *exe_name;
              for (int k = 1; k < arguments_cnt; k++) {
                (*arguments)[k] = tokens_get_token(tokens, j + 1);
              }
              (*arguments)[arguments_cnt] = NULL;
              break;
            } else {
              if (strcmp(current, "|") == 0) {
                cnt++;
              }  
            }
          }
/*
          for (int l = 0; (*arguments)[l] != NULL; l++) {
            fprintf(stderr, "%s  ", (*arguments)[l]);
          }
          fprintf(stderr, "\n");
          */
}

int exec_programs(struct tokens* tokens) {
  assert(tokens_get_length(tokens) > 0);

  // redirection stdin or stdout
  bool redirect_output = false;
  char* file_name = pre_redirect(tokens, &redirect_output);

  // deal with pipes 
  int num_pipes = find_num_pipes(tokens);
  int num_child_processes = num_pipes + 1;
  //printf("num_pipes : %d\n", num_pipes);
  if (num_pipes != 0) {
    int** pipe_array = (int**) malloc(num_pipes * sizeof(int*));   
    for(int i = 0; i < num_pipes; i++) {
      pipe_array[i] = (int*) malloc(2 * sizeof(int));
      pipe(pipe_array[i]);
    }

    for(int i = 0; i < num_child_processes; i++) {
      //printf("fork, i : %d\n", i);
      char* exe_name = NULL;
      char** arguments = NULL;
      pid_t pid = fork();
      if (pid == 0) {
        if (i == 0) {
          if (dup2(pipe_array[i][1], STDOUT_FILENO) < 0) {
            fprintf(stderr, "pipes, i = %d : dup2 failed\n", i);
          }
          close_pipes_array(pipe_array, num_pipes);

          find_execv_arguments(tokens, &exe_name, &arguments, i);
          execvp(exe_name, arguments);
        } else if (i == num_child_processes - 1) {
          if (dup2(pipe_array[i - 1][0], STDIN_FILENO) < 0) {
            fprintf(stderr, "pipes, i = %d : dup2 failed\n", i);
          }
          close_pipes_array(pipe_array, num_pipes);

          find_execv_arguments(tokens, &exe_name, &arguments, i);
          execvp(exe_name, arguments);
        } else {

        }
      }
    }
      close_pipes_array(pipe_array, num_pipes);
      int terminated_pid;
      while((terminated_pid = waitpid(-1, NULL, 0)) > 0) {
        //printf("terminated_pid : %d\n", terminated_pid); 
      }
      //printf("all child process exit\n");
  } else {
    pid_t pid = fork();
    if (pid == 0) {
      //redirect
      if (file_name != NULL) {
          redirect(file_name, redirect_output); 
      }
      // malloced in [pre_redirect()]
      free(file_name);

      char* program_to_run = tokens_get_token(tokens, 0);
      char* copy_program_to_run = (char*)malloc(strlen(program_to_run) + 1);
      strcpy(copy_program_to_run, program_to_run);

      assert(strlen(program_to_run) > 1);
      if (program_to_run[0] == '/') {
        // absolute path
        execv(program_to_run, tokens->tokens);
      } else {
        // path resolution
        char* environment_path = getenv("PATH"); 
        // store the program name

        char* saveptr;
        char dliem[] = ":";
        char* current_directory;
        char absolute_path[1024];
        for (current_directory = strtok_r(environment_path, dliem, &saveptr); current_directory != NULL; current_directory = strtok_r(NULL, dliem, &saveptr)) {
          get_absolute_path(absolute_path, current_directory, copy_program_to_run);

          // change the program to absolute path
          free(tokens->tokens[0]);
          tokens->tokens[0] = (char*) malloc(strlen(absolute_path) + 1);
          strcpy(tokens->tokens[0], absolute_path);
          //printf("%s\n", tokens->tokens[0]);
          execv(tokens_get_token(tokens, 0), tokens->tokens);
        }
      }

      fprintf(stdout, "failed to execute %s\n", copy_program_to_run);
      exit(-1);
    } else {
      wait(&pid);
    }
  }
  return 0;
}

/* Looks up the built-in command, if it exists. */
int lookup(char cmd[]) {
  for (unsigned int i = 0; i < sizeof(cmd_table) / sizeof(fun_desc_t); i++)
    if (cmd && (strcmp(cmd_table[i].cmd, cmd) == 0))
      return i;
  return -1;
}

/* Intialization procedures for this shell */
void init_shell() {
  /* Our shell is connected to standard input. */
  shell_terminal = STDIN_FILENO;

  /* Check if we are running interactively */
  shell_is_interactive = isatty(shell_terminal);

  if (shell_is_interactive) {
    /* If the shell is not currently in the foreground, we must pause the shell until it becomes a
     * foreground process. We use SIGTTIN to pause the shell. When the shell gets moved to the
     * foreground, we'll receive a SIGCONT. */
    while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
      kill(-shell_pgid, SIGTTIN);

    /* Saves the shell's process id */
    shell_pgid = getpid();

    /* Take control of the terminal */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Save the current termios to a variable, so it can be restored later. */
    tcgetattr(shell_terminal, &shell_tmodes);
  }
}

int main(unused int argc, unused char* argv[]) {
  init_shell();

  static char line[4096];
  int line_num = 0;

  /* Please only print shell prompts when standard input is not a tty */
  if (shell_is_interactive)
    fprintf(stdout, "%d: ", line_num);


  while (fgets(line, 4096, stdin)) {
    /* Split our line into words. */
    struct tokens* tokens = tokenize(line);

    /* Find which built-in function to run. */
    int fundex = lookup(tokens_get_token(tokens, 0));

    if (fundex >= 0) {
      cmd_table[fundex].fun(tokens);
    } else {
      // try to run as programs
      if (tokens_get_length(tokens) > 0) {
        exec_programs(tokens);
      }
    }

    if (shell_is_interactive)
      /* Please only print shell prompts when standard input is not a tty */
      fprintf(stdout, "%d: ", ++line_num);

    /* Clean up memory */
    tokens_destroy(tokens);
  }

  return 0;
}
