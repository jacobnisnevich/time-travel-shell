 // UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */


struct
command_stream
{
  command_t command_tree;
  command_stream_t next;
};

typedef struct
first_operator
{
  int cmd_type;
  char* start_location;
  char* error;
} first_operator;

// from Linux kernel string functions
char*
strstrip(char *s)
{
  size_t size;
  char *end;

  size = strlen(s);

  if (!size)
    return s;

  end = s + size - 1;
  while (end >= s && isspace(*end))
    end--;
  *(end + 1) = '\0';

  while (*s && isspace(*s))
    s++;

  return s;
}

char**
strtok_str(char* input_string, char* delimiter, int* num_trees)
{
  size_t input_string_size = strlen(input_string);
  size_t delimiter_size = strlen(delimiter);
  char** split_input = checked_malloc(255 * sizeof(char*));
  char* buffer = checked_malloc(sizeof(char));

  char* start = input_string;
  char* end = strstr(input_string, delimiter);
  while(end != NULL)
    {
      buffer = checked_realloc(buffer, (end - start + 1) * sizeof(char));
      memcpy(buffer, start, end - start);
      buffer[end - start] = '\0';
      split_input[*num_trees] = buffer;
      (*num_trees)++;
      start += end - start + delimiter_size;
      end = strstr(start, delimiter);
    }

  buffer = checked_realloc(buffer, &input_string[input_string_size] - start);
  memcpy(buffer, start, &input_string[input_string_size] - start);
  buffer[&input_string[input_string_size] - start] = '\0';

  split_input[*num_trees] = checked_malloc(strlen(buffer) * sizeof(char));
  memcpy(split_input[(*num_trees)++], buffer, strlen(buffer));

  return split_input;
}

char*
get_outer_subshell_cmd_str(char* input_string,
  int* num_chars)
{
  int input_string_len = strlen(input_string);
  int open_parens = 0;
  int start_char = 0, end_char = 0;

  int i;
  for (i = 0; i < input_string_len; i++)
  {
    if (input_string[i] == '(')
    {
      if (open_parens == 0)
      {
        open_parens++;
        start_char = i + 1;
      }
      else
      {
        open_parens++;
        (*num_chars)++;
      }
    }
    else if (input_string[i] == ')')
    {
      if (open_parens > 1)
      {
        open_parens--;
        (*num_chars)++;
      }
      else
      {
        open_parens--;
        end_char = i - 1;
        break;
      }
    }
    else
    {
      (*num_chars)++;
    }
  }

  char* subshell_str = checked_malloc((*num_chars + 1) * sizeof(char));
  memcpy(subshell_str, input_string + start_char, *num_chars);
  subshell_str[*num_chars] = '\0';

  return subshell_str;
}

command_t
parse_simple_command(char* command_str)
{
  command_t simple_command = checked_malloc(sizeof(struct command));

  simple_command->type = SIMPLE_COMMAND;
  simple_command->input = 0;
  simple_command->output = 0;

  int word_start = 0, word_end = 0, 
      input_start = 0, input_end = 0, 
      output_start = 0, output_end = 0;

  size_t i;
  for (i = 0; i < strlen(command_str); ++i)
  {
    if ((command_str[i] == '<' || command_str[i] == '>') && word_end == 0)
    {
      word_end = i - 1;
    }

    if (command_str[i] == '>' && input_start == 0)
    {
      output_start = i + 1;
    } 
    else if (command_str[i] == '>' && input_start != 0)
    {
      output_start = i + 1;
      input_end = i - 1;
    }
    else if (command_str[i] == '<' && output_start == 0)
    {
      input_start = i + 1;
    } 
    else if (command_str[i] == '<' && output_start != 0)
    {
      input_start = i + 1;
      output_end = i - 1;
    }
  }

  if (word_end == 0)
  {
    word_end = strlen(command_str);
  }
  if (output_end == 0 && output_start != 0)
  {
    output_end = strlen(command_str);
  }
  if (input_end == 0 && input_start != 0)
  {
    input_end = strlen(command_str);
  }

  if (input_end != 0)
  {
    simple_command->input = checked_malloc((input_end - input_start + 1) * 
      sizeof(char));
    memcpy(simple_command->input, command_str + input_start, 
      input_end - input_start);
    simple_command->input[input_end - input_start] = '\0';
    simple_command->input = strstrip(simple_command->input);
  }
  if (output_end != 0)
  {
    simple_command->output = checked_malloc((output_end - output_start + 1) * 
      sizeof(char));
    memcpy(simple_command->output, command_str + output_start, 
      output_end - output_start);
    simple_command->output[output_end - output_start] = '\0';
    simple_command->output = strstrip(simple_command->output);
  }

  char* words = checked_malloc((word_end + 1) * sizeof(char));
  memcpy(words, command_str, word_end);
  words[word_end] = '\0';

  int num_words = 0;
  char** word_split = strtok_str(strstrip(words), " ", &num_words);

  simple_command->u.word = checked_malloc(num_words * sizeof(char*));
  simple_command->u.word = word_split;

  return simple_command;
}

first_operator
get_first_operator(char* input_string)
{
  first_operator first_op;
  first_op.start_location = NULL;
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
        first_op.start_location = &input_string[i];
        first_op.cmd_type = SUBSHELL_COMMAND;
        return first_op;
        break;
      case '&':
        if (input_string[i + 1] == '&')
        {
          first_op.start_location = &input_string[i];
          first_op.cmd_type = AND_COMMAND;
          return first_op;
        }
        break;
      case '|':
        first_op.start_location = &input_string[i];
        if (input_string[i + 1] == '|')
        {
          first_op.cmd_type = OR_COMMAND;
        }
        else
        {
          first_op.cmd_type = PIPE_COMMAND;
        }
        return first_op;
        break;
      case ';':
      case '\n':
        first_op.start_location = &input_string[i];
        first_op.cmd_type = SEQUENCE_COMMAND;
        return first_op;
        break;
      case '\0':
      case ')':
        first_op.start_location = &input_string[i];
        first_op.cmd_type = SIMPLE_COMMAND;
        return first_op;
      default:
        break;
    }
  }

  return first_op;
}

command_t
convert_string_to_command_tree(char* input_string)
{
  command_t root_command = NULL;
  command_t current_command = root_command;

  size_t input_length = strlen(input_string);
  char* first_char = input_string;
  while (*first_char != '\0')
  {
    first_operator first_op = get_first_operator(first_char);
    if (first_op.cmd_type == SUBSHELL_COMMAND)
	  {
      // TODO: subshell command case

      // Allocate space for the simple command up to the '('
      // and store in a new buffer

      char* buffer = checked_malloc((first_op.start_location - first_char + 1) * 
        sizeof(char));
      memcpy(buffer, first_char, first_op.start_location - first_char);
      buffer[first_op.start_location - first_char] = '\0';

      // Get the command string contained within the ()
      int length = 0;
      char* sub_command = get_outer_subshell_cmd_str(first_op.start_location, &length);
      first_char += length;

      current_command = convert_string_to_command_tree(sub_command);
    }
    else if (first_op.cmd_type == SIMPLE_COMMAND)
    {
      // TODO: simple command case

      // Allocate space for the simple command found and store
      // the command in a new buffer
      char* buffer = checked_malloc((first_op.start_location - first_char + 1) * 
        sizeof(char));
      memcpy(buffer, first_char, first_op.start_location - first_char);
      buffer[first_op.start_location - first_char] = '\0';

      // Call parse simple command to generate the new node in the command tree
      if (root_command == NULL)
      {
        root_command = parse_simple_command(buffer);
      }
      else
      {
        current_command = parse_simple_command(buffer);
      }
      
      break;
    }
    else if (first_op.cmd_type == SEQUENCE_COMMAND)
    {
      // Allocate space for the simple command up to the found semicolon
      char* buffer = checked_malloc((first_op.start_location - first_char + 1) *
        sizeof(char));
      memcpy(buffer, first_char, first_op.start_location - first_char);
      buffer[first_op.start_location - first_char] = '\0';
      first_char = first_op.start_location;
      first_char++; // Advance past the semicolon

      // Allocate the new root of the tree and connect it with the
      // old root
      if (root_command == NULL)
      {
        root_command = checked_malloc(sizeof(struct command));
        root_command->type = SEQUENCE_COMMAND;
        root_command->u.command[0] = parse_simple_command(buffer);
      }
      else
      {
        current_command = parse_simple_command(buffer);
        command_t temp = checked_malloc(sizeof(struct command));
        temp->type = SEQUENCE_COMMAND;
        temp->u.command[0] = root_command;
        root_command = temp;
      }

      // Create the rest of the command tree recursively
      root_command->u.command[1] = convert_string_to_command_tree(first_char);
      break;

    }
    else
    {
      // TODO: OR_COMMAND, AND_COMMAND, PIPE_COMMAND

      // Allocate space for the simple command up to the found operator and 
      // copy into the new buffer
      char* buffer = checked_malloc((first_op.start_location - first_char + 1) * 
        sizeof(char));
      memcpy(buffer, first_char, first_op.start_location - first_char);
      buffer[first_op.start_location - first_char] = '\0';
      first_char = first_op.start_location;
      first_char++;
      if( first_op.cmd_type == AND_COMMAND || first_op.cmd_type == OR_COMMAND)
	first_char++; // Advance past the delimiter

      // Allocate the current command to contain the simple command up 
      // to the found operator. Use parse_simple_command to populate the
      // new node appropriately
      if (root_command == NULL)
      {
        root_command = parse_simple_command(buffer);
      }
      else
      {
        current_command = parse_simple_command(buffer);
      }
      // Create the new root of the command tree and assign the command
      // type to be that of the found operator. Connect the new root with
      // the old root
      command_t new_node = checked_malloc(sizeof(struct command_stream));
      new_node->u.command[0] = root_command;
      root_command = new_node;
      root_command->u.command[1] = NULL;
      root_command->type = first_op.cmd_type;

      // Assign the right node of the new root to be the next command
      // to be populated
      current_command = root_command->u.command[1];
    }
  }
  return root_command;
}

command_stream_t
split_to_command_trees(char* input_string)
{
  int num_trees = 0;

  char** command_tree_strings = strtok_str(input_string, "\n\n", &num_trees);
  command_stream_t head = NULL;
  command_stream_t cur = NULL;

  int i;
  for (i = 0; i < num_trees; ++i)
  {
    if (i == 0)
    {
      head = checked_malloc(sizeof(command_stream_t));
      cur = head;
      cur->command_tree = convert_string_to_command_tree(command_tree_strings[i]);
      cur->next = NULL;
    }
    else
    {
      cur->next = checked_malloc(sizeof(command_stream_t));
      cur = cur->next;
      cur->command_tree = convert_string_to_command_tree(command_tree_strings[i]);
      cur->next = NULL;
    }
  }
  return head;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *), 
  void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  char* input_string = (char*) checked_malloc(255 * sizeof(char));
  size_t input_string_size = 0;
  size_t curr_max_size = 255;
  char next_byte = get_next_byte(get_next_byte_argument);

  while (next_byte != EOF)
  {
    ++input_string_size;

    if (input_string_size > curr_max_size)
    {
      input_string = checked_realloc(input_string, 255 * sizeof(char));
      curr_max_size += 255;
    }

    input_string[input_string_size - 1] = next_byte;
    
    next_byte = get_next_byte(get_next_byte_argument);
  }

  command_stream_t command_trees = split_to_command_trees(input_string);
  // parse_simple_command("cat < simple.sh > out.sh");
  // first_operator firstop = get_first_operator(input_string);

  return command_trees;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  command_t current_command = s->command_tree;

  if (s)
  {
    s = s->next;
  }

  return current_command;
}
