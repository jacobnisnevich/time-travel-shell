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
  size_t split_input_size = 0;

  char* buffer = malloc(sizeof(char));
  size_t buffer_start = 0;
  char* matching_string = malloc(input_string_size * sizeof(char));
  size_t match_start = 0;
  char* token_string = malloc(input_string_size * sizeof(char));

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

char**
split_to_command_trees(char* input_string)
{
  int num_trees = 0;

  char** command_trees = strtok_str(input_string, "\n\n", &num_trees);

  return command_trees;
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
        if (i == 0)
        {
          first_op.error = "AND operator with no left operand";
          return first_op;
        }
        else if (input_string[i - 1] == '&')
        {
          first_op.start_location = i - 1;
          first_op.cmd_type = AND_COMMAND;
          return first_op;
        }
        break;
      case '|':
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
