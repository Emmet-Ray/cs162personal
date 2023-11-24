#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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


int exec_programs(struct tokens* tokens) {
  assert(tokens_get_length(tokens) > 0);

  pid_t pid = fork();
  if (pid == 0) {
    //printf("%p, %s\n", &arguments, argument);
    //printf("%p, %s\n", tokens->tokens, tokens->tokens[0]);
    //execv(tokens_get_token(tokens, 0), &arguments);
    char* program_to_run = tokens_get_token(tokens, 0);
    char* copy_program_to_run = (char*)malloc(strlen(program_to_run) + 1);
    strcpy(copy_program_to_run, program_to_run);

    assert(strlen(program_to_run) > 1);
    if (program_to_run[0] == '/') {
      execv(program_to_run, tokens->tokens);
    } else {
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
