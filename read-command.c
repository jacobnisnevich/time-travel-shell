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

char**
strtok_str(char* input_string, char* delimiter)
{
  size_t input_string_size = strlen(input_string);

  size_t delimiter_size = strlen(delimiter);

  char** split_input = malloc(255 * sizeof(char*));
  size_t split_input_size = 0;

  char* buffer = malloc(input_string_size * sizeof(char));
  size_t buffer_start = 0;
  char* matching_string = malloc(input_string_size * sizeof(char));
  size_t match_start = 0;
  char* token_string = malloc(input_string_size * sizeof(char));

  size_t i;
  for (i = 0; i < input_string_size; ++i)
  {
    if (i - buffer_start > 0)
    {
      memcpy(buffer, input_string + buffer_start, i - buffer_start);
      buffer[i - buffer_start] = '\0';

      matching_string = strstr(buffer, delimiter);
      if (matching_string)
      {
        match_start = matching_string - buffer;
        memcpy(token_string, buffer, i - delimiter_size);
        token_string[i - delimiter_size] = '\0';

        buffer_start = i;

        split_input[split_input_size] = malloc(strlen(token_string) * 
          sizeof(char));
        strcpy(split_input[split_input_size], token_string);
        ++split_input_size;
      }
    }
  }

  buffer = realloc(buffer, input_string_size * sizeof(char));
  memcpy(buffer, input_string + buffer_start, input_string_size - buffer_start);
  buffer[input_string_size - buffer_start] = '\0';

  split_input[split_input_size] = malloc(strlen(buffer) * sizeof(char));
  strcpy(split_input[split_input_size], buffer);

  return split_input;
}

char**
split_to_command_trees(char* input_string)
{
  int command_trees_count = 0;
  char** command_trees = malloc(255 * sizeof(char*));

  command_trees = strtok_str(input_string, "\n\n");

  return command_trees;
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

  printf("%s \n", command_trees[0]);
  printf("%s", command_trees[1]);

  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  command_t current_command = s->current_command;
  s = s->next_command;
  return current_command;
}
