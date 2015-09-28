// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */


struct
command_stream
{
  command_t current_command;
  command_stream_t next_command;
};

typedef struct
first_operator
{
  int cmd_type;
  int start_location;
  char* error;
} first_operator;


char**
strtok_str(char* input_string, char* delimiter, int* num_trees)
{
  size_t input_string_size = strlen(input_string);
  size_t delimiter_size = strlen(delimiter);
  char** split_input = malloc(255 * sizeof(char*));
  char* buffer = malloc(sizeof(char));

  char* start = input_string;
  char* end = strstr(input_string, delimiter);
  while(end != NULL)
    {
      buffer = realloc(buffer, (end - start + 1) * sizeof(char*));
      memcpy(buffer, start, end - start);
      buffer[end - start] = '\0';
      split_input[*num_trees] = malloc(strlen(buffer) * sizeof(char));
      memcpy(split_input[(*num_trees)++], buffer, strlen(buffer));
      start += end - start + delimiter_size;
      end = strstr(start, delimiter);
    }

  buffer = realloc(buffer, &input_string[input_string_size] - start);
  memcpy(buffer, start, &input_string[input_string_size] - start);
  buffer[&input_string[input_string_size] - start] = '\0';

  split_input[*num_trees] = malloc(strlen(buffer) * sizeof(char));
  memcpy(split_input[(*num_trees)++], buffer, strlen(buffer));

  return split_input;
}

void
convert_string_to_command_tree(char* input_string, command_stream_t prev_command_tree)
{
  command_stream_t current_command_tree;
  command_t root_command;
  commant_t current_command = root_commmand;

  prev_command_tree->next_command = current_command_tree;
  current_command_tree=>next_command = NULL;

  size_t input_length = strlen(input_string);
  char* firstChar = input_string;
  while(*firstChar != '\0')
    {
      first_operator first_op = get_first_operator(input_string);
      if (first_op->cmd_type == SUBSHELL_COMMAND)
	{
	  //TODO: subshell command case
	}
      else if (first_op->cmd_type == SIMPLE_COMMAND)
	{
	}
      else
	{
	  //OR_COMMAND, AND_COMMAND, SEQUENCE_COMMAND, PIPE_COMMAND
	}
    }
  
}

command_stream_t
split_to_command_trees(char* input_string)
{
  int num_trees = 0;

  char** command_tree_strings = strtok_str(input_string, "\n\n", &num_trees);
  command_stream_t prev_command_tree;
  prev_command_tree->current_command = NULL;
  prev_command_tree->next_command = NULL;
  int i = 0;
  command_stream_t first_command_tree = prev_command_tree;
  for(; i < num_trees; ++i)
    {
      convert_string_to_command_tree(command_tree_strings[i], prev_command_tree)
    }
  return first_command_tree;
}

first_operator
get_first_operator(char* input_string)
{
  first_operator first_op;
  first_op.start_location = -1;
  first_op.cmd_type = -1;
  first_op.error = "";

  int input_length = strlen(input_string);

  int i;
  for (i = 0; i < input_length; ++i)
  {
    char ch = input_string[i];
    switch (ch)
    {
      case '(':
        first_op.start_location = i;
        first_op.cmd_type = SUBSHELL_COMMAND;
        return first_op;
        break;
      case '&':
        if (input_string[i + 1] == '&')
        {
          first_op.start_location = i;
          first_op.cmd_type = AND_COMMAND;
          return first_op;
        }
        break;
      case '|':
	first_op.start_location = i;
	if (input_string[i + 1] == '|')
	  {
	    first_op.cmd_type = OR_COMMAND;
	  }
	else
	  {
	    first_op.cmd_type == PIPE_COMMAND;
	  }
	return first_op;
        break;
    case ';':
    case '\n':
      first_op.start_location = i;
      first_op.cmd_type = SEQUENCE_COMMAND;
      return first_op;
      break;
    case '\0':
      first_op.start_location = -1;
      first_op.cmd_type = SIMPLE_COMMAND;
      return first_op;
    default:
      break;
    }
  }
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *), 
  void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  char* input_string = (char*) malloc(255 * sizeof(char));
  size_t input_string_size = 0;
  size_t curr_max_size = 255;
  char next_byte = get_next_byte(get_next_byte_argument);

  while (next_byte != EOF)
  {
    ++input_string_size;

    if (input_string_size > curr_max_size)
    {
      input_string = realloc(input_string, 255 * sizeof(char));
      curr_max_size += 255;
    }

    input_string[input_string_size - 1] = next_byte;
    
    next_byte = get_next_byte(get_next_byte_argument);
  }

  char** command_trees = split_to_command_trees(input_string);

  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  command_t current_command = s->current_command;

  if (s)
  {
    s = s->next_command;
  }

  return current_command;
}
