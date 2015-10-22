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

int
command_status (command_t c)
{
  return c->status;
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
    if (dup2(file_descriptor, 0) == -1)
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
    if (dup2(file_descriptor, 1) == -1)
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
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  pid_t pid;
  switch (c->type)
  {
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
    if (dup2(file_descriptor, 0) == -1)
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
    if (dup2(file_descriptor, 1) == -1)
    {
      c->status = 1;
      _exit(1);
    }
    close(file_descriptor);
  }
	
	execute_command(c->u.subshell_command, time_travel);
	c->status = c->u.subshell_command->status;
	}
	break;
    case PIPE_COMMAND:
    {
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
        if (dup2(fds[1], 1) == -1)
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
        pid_t right = fork();
        if (right == -1)
        {
          fprintf(stderr, "Execute error: failed to fork");
          _exit(1);
        }
        if (right == 0)
        {
          // child
          if (close(fds[1]) == -1) // close the read end
          {
            fprintf(stderr, "Execute error: failed to close pipe");
          _exit(1);
          }
          if (dup2(fds[1], 0) == -1)
          {
            fprintf(stderr, "Execute error: failed to dup");
          _exit(1);
          }
          execute_command(c->u.command[1], time_travel);
          _exit(c->u.command[1]->status);
        }
      }

    }
    case AND_COMMAND:
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
    case OR_COMMAND:
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
    case SIMPLE_COMMAND:
      pid = fork();
      if (pid == 0)
      {
        // child
        execute_simple_command(c);
        _exit(0);
      }
      else
      {
        // parent
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
          c->status = WEXITSTATUS(status);
        }
      }
      break;
    default:
      break;
  }
}
