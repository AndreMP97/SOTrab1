/////////////////////////////////////////////////////////
//                                                     //
//               Trabalho I: Mini-Shell                //
//                                                     //
// Compilação: gcc my_prompt.c -lreadline -o my_prompt //
// Utilização: ./my_prompt                             //
//                                                     //
/////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#define MAXARGS 100
#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct command {
    char *cmd;              // string apenas com o comando
    int argc;               // número de argumentos
    char *argv[MAXARGS+1];  // vector de argumentos do comando
    struct command *next;   // apontador para o próximo comando
} COMMAND;

// variáveis globais
char* inputfile = NULL;	    // nome de ficheiro (em caso de redireccionamento da entrada padrão)
char* outputfile = NULL;    // nome de ficheiro (em caso de redireccionamento da saída padrão)
int background_exec = 0;    // indicação de execução concorrente com a mini-shell (0/1)

// declaração de funções
COMMAND* parse(char *);
void print_parse(COMMAND *);
void execute_commands(COMMAND *);
void free_commlist(COMMAND *);
void filtro(COMMAND *);

// include do código do parser da linha de comandos
#include "parser.c"

int main(int argc, char* argv[]) {
  char *linha;
  COMMAND *com;

  while (1) {
    if ((linha = readline("my_prompt$ ")) == NULL)
      exit(0);
    if (strlen(linha) != 0) {
      add_history(linha);
      com = parse(linha);
      if (com) {
	print_parse(com);
	execute_commands(com);
	free_commlist(com);
      }
    }
    free(linha);
  }
}


void print_parse(COMMAND* commlist) {
  int n, i;

  printf("---------------------------------------------------------\n");
  printf("BG: %d IN: %s OUT: %s\n", background_exec, inputfile, outputfile);
  n = 1;
  while (commlist != NULL) {
    printf("#%d: cmd '%s' argc '%d' argv[] '", n, commlist->cmd, commlist->argc);
    i = 0;
    while (commlist->argv[i] != NULL) {
      printf("%s,", commlist->argv[i]);
      i++;
    }
    printf("%s'\n", commlist->argv[i]);
    commlist = commlist->next;
    n++;
  }
  printf("---------------------------------------------------------\n");
}


void free_commlist(COMMAND *commlist){
    COMMAND *prev = NULL;
    while (commlist != NULL){
      prev = commlist;
      commlist = commlist -> next;
      free(prev);
    }
}
printf("teste\n");

void execute_commands(COMMAND *commlist) {
  pid_t pid;
  int fd;
  if (strcmp(commlist -> cmd, "exit") == 0) {
    exit(0);
  }
  else if (strcmp(commlist -> cmd, "filtro") == 0) {
    filtro(commlist);
  }
  if ((pid = fork()) < 0) {
    printf("***ERROR: fork failed***\n");
    exit(-1);
  }
  else if (pid == 0) {
    if (inputfile != NULL) {
      fd = open(inputfile, O_RDONLY);
      if (fd < 0) {
        printf("***ERROR! File not found!***\n");
        exit(-1);
      }
      dup2(fd, STDIN_FILENO);
      close(fd);
    }
    if (outputfile != NULL) {
      int fd = open(outputfile, O_CREAT | O_RDWR, S_IRWXU);
      dup2(fd, STDOUT_FILENO);
      close(fd);
    }
    if (execvp(commlist -> cmd, commlist -> argv) < 0) {
      printf("***ERROR: exec failed!***\n");
      exit(-1);
    }
  }
  else {
    waitpid(pid,NULL,0);
  }
}

void filtro(COMMAND *commlist) {
  int fd[2], file1, file2;
  pid_t pid1, pid2;
  if (pipe(fd) < 0) {
    printf("***ERROR: PIPE FAILED!***\n");
    exit(-1);
  }
  if ((pid1 = fork()) < 0) {
    printf("***ERROR: FORK CHILD 1 FAILED!***\n");
    exit(-1);
  }
  else if (pid1 == 0) {
    printf("Debug Filho 1 entrada\n");
    file1 = open(commlist -> argv[1], O_RDONLY);
    if (file1 < 0) {
      printf("***ERROR: FILE NOT FOUND!***\n");
      exit(-1);
    }
    printf("teste 1\n");
    dup2(file1, STDIN_FILENO);
    printf("teste 2\n");
    dup2(fd[PIPE_WRITE], STDOUT_FILENO);
    printf("teste 3\n");
    close(fd[PIPE_READ]);
    printf("teste 4\n");
    close(fd[PIPE_WRITE]);
    printf("teste 5\n");
    execlp("cat", "cat", NULL);
    printf("Debug filho 1 saída\n");
  }
  else {
    if ((pid2 = fork()) < 0) {
      printf("***ERROR: FORK CHILD 2 FAILED!***\n");
      exit(-1);
    }
    else if (pid2 == 0) {
      file2 = open(commlist -> argv[2], O_CREAT | O_TRUNC | O_RDWR, S_IRWXU);
      dup2(fd[PIPE_READ],STDIN_FILENO);
      dup2(file2, STDOUT_FILENO);
      close(fd[PIPE_WRITE]);
      close(fd[PIPE_READ]);
      execlp("grep", "grep", commlist -> argv[2], NULL);
      printf("Debug\n");
    }
    else {
      waitpid(pid1, NULL, 0);
      waitpid(pid2, NULL, 0);
    }
  }
}
