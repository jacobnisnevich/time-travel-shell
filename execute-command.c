// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

typedef struct dependencies {
  char** inputs;
  char** outputs;
} dependencies;

int childpid;

int
command_status (command_t c)
{
  return c->status;
}

void handle_sigpipe(int sig)
{
  (void) sig;
  kill(childpid, SIGPIPE);
}

void
execute_simple_command(command_t c)
{
  int file_descriptor;

  if (c->input)
  {
    file_descriptor = open(c->input, O_RDONLY);
    if (file_descriptor < 0)
    {
      c->status = 1;
      _exit(1);
    }
    if (dup2(file_descriptor, STDIN_FILENO) == -1)
    {
      c->status = 1;
      _exit(1);
    }
    close(file_descriptor);
  }

  if (c->output)
  {
    file_descriptor = open(c->output, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if (file_descriptor < 0)
    {
      c->status = 1;
      _exit(1);
    }
    if (dup2(file_descriptor, STDOUT_FILENO) == -1)
    {
      c->status = 1;
      _exit(1);
    }
    close(file_descriptor);
  }
  c->status = 0;
  execvp(*(c->u.word), c->u.word);
}

dependencies
merge_dependencies (dependencies dependencies_1, dependencies dependencies_2)
{
  dependencies new_dependencies;
  new_dependencies.inputs = checked_malloc(256 * sizeof(char*));
  new_dependencies.outputs = checked_malloc(256 * sizeof(char*));
  int inputs_index = 0, outputs_index = 0;

  int i = 0;
  while (dependencies_1.inputs[i] != NULL)
  {
    new_dependencies.inputs[inputs_index] = dependencies_1.inputs[i];
    inputs_index++;
    i++;
  }

  i = 0;
  while (dependencies_1.outputs[i] != NULL)
  {  
    new_dependencies.outputs[outputs_index] = dependencies_1.outputs[i];
    outputs_index++;
    i++;
  }

  i = 0;
  while (dependencies_2.inputs[i] != NULL)
  {
    new_dependencies.inputs[inputs_index] = dependencies_2.inputs[i];
    inputs_index++;
    i++;
  }

  i = 0;
  while (dependencies_2.outputs[i] != NULL)
  {  
    new_dependencies.outputs[outputs_index] = dependencies_2.outputs[i];
    outputs_index++;
    i++;
  }

  new_dependencies.inputs[inputs_index] = NULL;
  new_dependencies.outputs[outputs_index] = NULL;

  return new_dependencies;
}

dependencies
get_tree_dependencies (command_t c)
{
  dependencies tree_dependencies;

  if (c->type == SIMPLE_COMMAND)
  {
    tree_dependencies.inputs = checked_malloc(2 * sizeof(char*));

    int n_words = 0;
    while (c->u.word[n_words] != NULL) {
      n_words++;
    }

    if (c->input != NULL) {
      char** words_and_input = checked_malloc(n_words * sizeof(char*) + 1);
      memcpy(words_and_input, c->u.word, n_words * sizeof(char*));
      words_and_input[0] = c->input;
      words_and_input[n_words] = NULL;

      tree_dependencies.inputs = checked_malloc(n_words * 
        sizeof(char*) + 1);
      memcpy(tree_dependencies.inputs, words_and_input, n_words * 
        sizeof(char*) + 1);
    } else {
      char** words_and_input = checked_malloc(n_words * sizeof(char*) + 1);

      int i = 1;
      for (; i < n_words; i++)
      {
        words_and_input[i - 1] = c->u.word[i];
      }

      words_and_input[n_words - 1] = NULL;

      tree_dependencies.inputs = checked_malloc(n_words * 
        sizeof(char*) + 1);
      memcpy(tree_dependencies.inputs, words_and_input, n_words * 
        sizeof(char*) + 1);
    }

    tree_dependencies.outputs = checked_malloc(2 * sizeof(char*));
    tree_dependencies.outputs[0] = c->output;
    tree_dependencies.outputs[1] = NULL;
  }
  else if (c->type == SUBSHELL_COMMAND)
  {
    tree_dependencies.inputs = checked_malloc(sizeof(char*));
    tree_dependencies.inputs[0] = c->input;
    tree_dependencies.outputs = checked_malloc(sizeof(char*));
    tree_dependencies.outputs[0] = c->output;

    tree_dependencies = merge_dependencies(tree_dependencies, 
      get_tree_dependencies(c->u.subshell_command));
  }
  else
  {
    dependencies left_dependencies = get_tree_dependencies(c->u.command[0]);
    dependencies right_dependencies = get_tree_dependencies(c->u.command[1]);
    tree_dependencies = merge_dependencies(left_dependencies, 
      right_dependencies);
  }

  return tree_dependencies;
};

dependencies* 
get_dependencies (command_t c, int* n_deps)
{
  dependencies* dep_arr = checked_malloc(256 * sizeof(dependencies));

  command_t cur = c;
  *n_deps = 0;
  while (1)
  {
    if (cur->type != SEQUENCE_COMMAND)
    {
      dep_arr[*n_deps] = get_tree_dependencies(cur);
      (*n_deps)++;
	break;
    }
    else
    {
       dep_arr[*n_deps] = get_tree_dependencies(cur->u.command[1]);
       (*n_deps)++;
       cur = cur->u.command[0];
    }
  }

  dependencies* res = checked_malloc((*n_deps + 1) * sizeof(dependencies));

  int i = *n_deps - 1;
  int j = 0;
  while (i >= 0)
  {
    res[j] = dep_arr[i];
    i--;
    j++;
  }
  free(dep_arr);
  return res;
}

command_t* 
get_sequence_commands (command_t c, int* n_seqs)
{
  command_t* seq_arr = checked_malloc(256 * sizeof(command_t));

  command_t cur = c;
  *n_seqs = 0;
  while (1)
  {
    if (cur->type != SEQUENCE_COMMAND)
    {
      seq_arr[*n_seqs] = cur;
	(*n_seqs)++; 
     break;
    }
    else
    {
      seq_arr[*n_seqs] = cur->u.command[1];
      (*n_seqs)++;
      cur = cur->u.command[0];
    }
  }

  command_t* res = checked_malloc((*n_seqs + 1) * sizeof(command_t));

  int i = *n_seqs - 1;
  int j = 0;
  while (i >= 0)
  {
    res[j] = seq_arr[i];
    i--;
    j++;
  }
  free(seq_arr);
  return res;

  return seq_arr;
}

int check_dependencies (dependencies dep1, dependencies dep2)
{
  int i = 0;
  int j = 0;

  while (dep1.outputs[i] != NULL)
  {
    while (dep2.inputs[j] != NULL)
    {
      if (dep1.outputs[i] == dep2.inputs[j])
      {
        return 1;
      }

      j++;
    }
    i++;
  }

  return 0;
}

void 
execute_parallel (command_t c)
{
  int n_deps;
  int n_seqs;
  dependencies* dep_arr = get_dependencies(c, &n_deps);
  command_t* seq_cmds = get_sequence_commands(c, &n_seqs);

  int** adj_mat = checked_malloc((n_deps + 1) * sizeof(int*));
  adj_mat[n_deps] = NULL;

  int i = 0;
  int j = 0;
  for (; i < n_deps; i++)
  {
    adj_mat[i] = checked_malloc((n_deps + 1) * sizeof(int));
    j = 0;
    for (; j < n_deps; j++)
    {
      adj_mat[i][j] = 0;
    }
    adj_mat[i][n_deps] = -1;
  }

  i = 0;
  j = 0;

  for (; i < n_deps; i++)
  {
    for (; j < n_deps; j++)
    {
      if (i < j)
      {
        adj_mat[i][j] = check_dependencies(dep_arr[i], dep_arr[j]);
      }
    }
  }

  while (n_seqs != 0)
  {
    // scan to find column of all zeroes (guranteed to be at least one)
    j = 0;
    i = 0;
    int n_parallel = 0;

    int* parallels = checked_malloc(n_deps * sizeof(int));
    pid_t* pids = checked_malloc(n_deps * sizeof(int));

    for (; j < n_deps; j++)
    {
      int all_zeroes = 1;
      for (; i < n_deps; i++)
      {
        if (adj_mat[i][j] == 1)
        {
          all_zeroes = 0;
        }
      }
      if (all_zeroes == 1)
      {
        // we can execute this in parallel
        // add command to buffer of parallel commands
        parallels[n_parallel] = j;
        n_parallel++;
      }
    }

    // execute all the commands found to be parallel

    int z = 0;
    for (; z < n_parallel; z++)
    {
      n_seqs--;

      // fork and execute seq_commands[parallels[z]]
      int k = 0;
      pids[z] = fork();
      if (pids[z] == 0)
      {
        // child
        execute_command(seq_cmds[parallels[z]]);
        _exit(1);
      }

      // zero out the row to say that this commmand is done running
      for (; k < n_deps; k++)
      {
        adj_mat[parallels[z]][k] = 0;
      }

      adj_mat[parallels[z]][parallels[z]] = 1;
    }

    // parent, collect all the children
    i = 0;
    for (; i < n_parallel; i++)
    {
      int status;
      waitpid(pids[i], &status, 0);
      if (!WIFEXITED(status))
      {
        exit(1);
      }
    } 
  }
}

void
execute_command (command_t c)
{
  pid_t pid;
  switch (c->type)
  {
    case SEQUENCE_COMMAND:
    {
      execute_command(c->u.command[0]);
      execute_command(c->u.command[1]);
      c->status = c->u.command[1]->status;

      break;
    }
    case SUBSHELL_COMMAND:
    {
      int file_descriptor;
      if (c->input)
      {
        file_descriptor = open(c->input, O_RDONLY);
        if (file_descriptor < 0)
        {
          c->status = 1;
          _exit(1);
        }
        if (dup2(file_descriptor, STDIN_FILENO) == -1)
        {
          c->status = 1;
          _exit(1);
        }
        close(file_descriptor);
      }

      if (c->output)
      {
        file_descriptor = open(c->output, O_WRONLY|O_TRUNC|O_CREAT, 0644);
        if (file_descriptor < 0)
        {
          c->status = 1;
          _exit(1);
        }
        if (dup2(file_descriptor, STDOUT_FILENO) == -1)
        {
          c->status = 1;
          _exit(1);
        }
        close(file_descriptor);
      } 
      execute_command(c->u.subshell_command);
      c->status = c->u.subshell_command->status;

      break;
    }
    case PIPE_COMMAND:
    {
      int status;
      int fds[2];

      if (pipe(fds) == -1)
      {
        error(1, errno, "Failed to pipe");
        _exit(1);
      }

      pid_t left = fork();
      if (left == -1)
      {
        fprintf(stderr, "Execute error: failed to fork");
        _exit(1);
      }
      if (left == 0)
      {
        // child
        if (close(fds[0]) == -1) // close the read end
        {
          fprintf(stderr, "Execute error: failed to close pipe");
          _exit(1);
        }
        if (dup2(fds[1], STDOUT_FILENO) == -1)
        {
          fprintf(stderr, "Execute error: failed to dup");
          _exit(1);
        }
        execute_command(c->u.command[0]);
        _exit(c->u.command[0]->status);
      }
      else
      {
        //parent
        childpid = left;
      signal(SIGPIPE, handle_sigpipe);
        pid_t right = fork();
        if (right == -1)
        {
          fprintf(stderr, "Execute error: failed to fork");
          _exit(1);
        }
        if (right == 0)
        {
          // child 2
          if (close(fds[1]) == -1) // close the write end
          {
            fprintf(stderr, "Execute error: failed to close pipe");
          _exit(1);
          }
          if (dup2(fds[0], STDIN_FILENO) == -1)
          {
            fprintf(stderr, "Execute error: failed to dup");
          _exit(1);
          }

          execute_command(c->u.command[1]);

          if (waitpid(left, NULL, WNOHANG))
          {
            kill(left, SIGPIPE);
          }
          _exit(c->u.command[1]->status);
        }

        else if (waitpid(left, &status, 0) == -1) // parent again
        {
          // failed to waitpid
        }
        if (close(fds[1]) == -1) // close write end after left command ends
        {
          // failed to close pipe
        }

        childpid = right; 

        if (waitpid(right, &status, 0) == -1)
        {
          // failed to waitpid
        }
        if (close(fds[0]) == -1)
        {
          // failed to close pipe
        }
        c->status = WEXITSTATUS(status);
        signal(SIGPIPE, SIG_DFL);
      }
      break;
    }
    case AND_COMMAND:
    {
      execute_command(c->u.command[0]);
      if (command_status(c->u.command[0]) == 0)
      {
        execute_command(c->u.command[1]);

        if (command_status(c->u.command[1]) == 0)
        {
          c->status = 0;
        }
        else
        {
          c->status = 1;
        }
      }
      else
      {
        c->status = 1;
      }
      break;
    }
    case OR_COMMAND:
    {
      execute_command(c->u.command[0]);

      if (command_status(c->u.command[0]) == 0)
      {
        c->status = 0;
        break;
      }
      else
      {
        execute_command(c->u.command[1]);

        if (command_status(c->u.command[1]) == 0)
        {
          c->status = 0;
        }
        else
        {
          c->status = 1;
        }
      }
    }
    case SIMPLE_COMMAND:
    {
      int status;
      pid = fork();

      if (pid == 0) // child
      {
        execute_simple_command(c);
        _exit(0);
      }
      else // parent
      {
        childpid = pid;
        signal(SIGPIPE, handle_sigpipe);
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
          c->status = WEXITSTATUS(status);
        }
      }

      break;
    }
    default:
    {
      break;
    }
  }
}
