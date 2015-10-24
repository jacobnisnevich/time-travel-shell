// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

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

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
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

void
execute_command (command_t c, int time_travel)
{
  pid_t pid;
  switch (c->type)
  {
    case SEQUENCE_COMMAND:
    {
      execute_command(c->u.command[0], time_travel);
      execute_command(c->u.command[1], time_travel);
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
      execute_command(c->u.subshell_command, time_travel);
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
        execute_command(c->u.command[0], time_travel);
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

          execute_command(c->u.command[1], time_travel);

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
      execute_command(c->u.command[0], time_travel);
      if (command_status(c->u.command[0]) == 0)
      {
        execute_command(c->u.command[1], time_travel);

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
      execute_command(c->u.command[0], time_travel);

      if (command_status(c->u.command[0]) == 0)
      {
        c->status = 0;
        break;
      }
      else
      {
        execute_command(c->u.command[1], time_travel);

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
