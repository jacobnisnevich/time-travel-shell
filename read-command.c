// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

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

typedef struct
subshell_redirects
{
  char* input;
  char* output;
} subshell_redirects;

command_t create_new_command()
{
  command_t temp = (command_t) checked_malloc(sizeof(struct command));
  memset(temp, 0, sizeof(struct command));
  return temp;
}

void
free_array_strings(char** array)
{
  int i;
  for (i = 0; array[i]; i++)
  {
    free(array[i]);
  }

  free(array);
}

void
free_command_tree (command_t cmd)
{
  if (cmd->type == SIMPLE_COMMAND)
  {
    free(cmd->input);
    free(cmd->output);
    free_array_strings(cmd->u.word);
    free(cmd);
  }
  else
  {
    if (cmd->type == SUBSHELL_COMMAND)
    {
      free(cmd->u.subshell_command);
    }
    else //PIPE, OR, AND
    {
      free_command_tree(cmd->u.command[0]);
      free_command_tree(cmd->u.command[1]);
    }
    free(cmd);
  }
}

void
free_command_stream (command_stream_t cmd_stream)
{
  command_stream_t cur = cmd_stream;

  while (cur)
  {
    free_command_tree(cur->command_tree);
    command_stream_t temp = cur;
    cur = cur->next;
    free(temp);
  }
}

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
  char** split_input = (char**) checked_malloc(255 * sizeof(char*));

  char* start = input_string;
  char* end = strstr(input_string, delimiter);
  while(end != NULL)
    {
      char* buffer = (char*) checked_malloc((end - start + 1) * sizeof(char));
      memcpy(buffer, start, end - start);
      buffer[end - start] = '\0';
      split_input[*num_trees] = buffer;
      (*num_trees)++;
      start += end - start + delimiter_size;
      end = strstr(start, delimiter);
    }

  char* buffer = (char*) checked_malloc((1 + &input_string[input_string_size] - 
    start) * sizeof(char));
  memcpy(buffer, start, &input_string[input_string_size] - start);
  buffer[&input_string[input_string_size] - start] = '\0';

  split_input[(*num_trees)++] = buffer;
  split_input[*num_trees] = '\0';
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

  if (end_char == 0)
  {
    fprintf(stderr, "Syntax error: Subshell command (\"%s\") has unclosed \
      parenthesis\n", input_string);
    exit(1);
  }
  
  char* subshell_str = (char*) checked_malloc((*num_chars + 1) * sizeof(char));
  memcpy(subshell_str, input_string + start_char, *num_chars);
  subshell_str[*num_chars] = '\0';

  return subshell_str;
}

void
check_for_illegal_characters(char* input_string)
{
  size_t i;
  for (i = 0; i < strlen(input_string); i++)
  {
    if (isalnum(input_string[i]) || isspace(input_string[i]) ||
          input_string[i] == '!' || input_string[i] == '>' ||
          input_string[i] == '%' || input_string[i] == '+' ||
          input_string[i] == ',' || input_string[i] == '-' ||
          input_string[i] == '.' || input_string[i] == '/' ||
          input_string[i] == ':' || input_string[i] == '@' ||
          input_string[i] == '^' || input_string[i] == '_' ||
          input_string[i] == ';' || input_string[i] == '|' ||
          input_string[i] == '&' || input_string[i] == '(' ||
          input_string[i] == ')' || input_string[i] == '<' ||
          input_string[i] == '>')
    {
      continue;
    }
    else
    {
      fprintf(stderr, "Syntax error: Illegal character %c\n", input_string[i]);
      exit(1);
    }
  }
}

char*
remove_comments(char* input_string)
{
  int is_comment = 0;

  size_t i;
  for (i = 0; i < strlen(input_string); i++)
  {
    if (input_string[i] == '#')
    {
      input_string[i] = ' ';
      is_comment = 1;
    }
    else if (is_comment == 1)
    {
      if (input_string[i] == '\n')
      {
        input_string[i] = ' ';
        is_comment = 0;
      }
      else
      {
        input_string[i] = ' ';
      }
    }
  }

  return input_string;
}

char* 
deal_with_incomplete_commands(char* input_string)
{
  int is_incomplete_command = 0;

  size_t i;
  for (i = 0; i < strlen(input_string); i++)
  {
    if ((input_string[i] == '&' && input_string[i + 1] == '&') ||
        (input_string[i] == '|' && input_string[i + 1] == '|'))
    {
      is_incomplete_command = 1;
      i++;
    }
    else if (input_string[i] == '|' || input_string[i] == ';')
    {
      is_incomplete_command = 1;
    }
    else if (input_string[i] == '\n' && is_incomplete_command == 1)
    {
      input_string[i] = ' ';
    }
    else
    {
      is_incomplete_command = 0;
    }
  }

  return input_string;
}

char*
remove_whitespace_from_end(char* input_string)
{
  size_t i = strlen(input_string) - 1;
  for ( ; ; i--)
  {
    if (input_string[i] == '\n' || input_string[i] == ' ')
    {
      input_string[i] = ' ';
    }
    else
    {
      break;
    }

    if (i == 0)
    {
      break;
    }
  }

  return input_string;
}

char*
preprocess_input(char* input_string)
{
  input_string = remove_comments(input_string);
  check_for_illegal_characters(input_string);
  input_string = deal_with_incomplete_commands(input_string);
  input_string = remove_whitespace_from_end(input_string);

  return input_string;
}

subshell_redirects
get_subshell_redirects(char* command_str)
{
  command_str = strstrip(command_str);
  subshell_redirects redirects;
  redirects.input = "";
  redirects.output = "";

  int input_start = 0, input_end = 0, 
      output_start = 0, output_end = 0;

  size_t i;
  for (i = 0; i < strlen(command_str); ++i)
  {
    if (command_str[i] != '<' && command_str[i] != '>' && input_start == 0 &&
      output_end == 0)
    {
      fprintf(stderr, "Syntax error: Subshell contains characters before \
redirects\n");
      exit(1);
    }
    if (command_str[i] == '>' && input_start == 0)
    {
      if (output_start != 0)
      {
        fprintf(stderr, "Syntax error: Subshell command (\"%s\") contains \
multiple of the same redirect\n", command_str);
        exit(1);
      }
      output_start = i + 1;
    } 
    else if (command_str[i] == '>' && input_start != 0)
    {
      output_start = i + 1;
      input_end = i;
    }
    else if (command_str[i] == '<' && output_start == 0)
    {
      if (input_start != 0)
      {
        fprintf(stderr, "Syntax error: Subshell command (\"%s\") contains \
multiple of the same redirect\n", command_str);
        exit(1);
      }
      input_start = i + 1;
    } 
    else if (command_str[i] == '<' && output_start != 0)
    {
      input_start = i + 1;
      output_end = i;
    }
    if (command_str[i] == ')')
    {
      fprintf(stderr, "Syntax error: Command (\"%s\") contains unopened \
parenthesis\n", command_str);
      exit(1);
    }
  }

  if (output_end == 0 && output_start != 0)
  {
    output_end = strlen(command_str);
    if (output_start == output_end)
    {
      fprintf(stderr, "Syntax error: Subshell command (\"%s\") contains empty \
redirect\n", command_str);
        exit(1);
    }
  }
  if (input_end == 0 && input_start != 0)
  {
    input_end = strlen(command_str);
    if (input_start == input_end)
    {
      fprintf(stderr, "Syntax error: Simple command (\"%s\") contains empty \
redirect\n", command_str);
        exit(1);
    }
  }

  if (input_end != 0)
  {
    redirects.input = (char*) checked_malloc((input_end - input_start + 1) * 
      sizeof(char));
    memcpy(redirects.input, command_str + input_start, 
      input_end - input_start);
    redirects.input[input_end - input_start] = '\0';
    redirects.input = strstrip(redirects.input);
  }
  if (output_end != 0)
  {
    redirects.output = (char*) checked_malloc((output_end - output_start +
      1) * sizeof(char));
    memcpy(redirects.output, command_str + output_start, 
      output_end - output_start);
    redirects.output[output_end - output_start] = '\0';
    redirects.output = strstrip(redirects.output);
  }

  return redirects;
}

command_t
parse_simple_command(char* command_str)
{
  command_str = strstrip(command_str);
  command_t simple_command = create_new_command();

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
      if (i == 0)
      {
        fprintf(stderr, "Syntax error: Simple command (\"%s\") contains word \
of length 0\n", command_str);
        exit(1);
      }
      word_end = i;
    }
    if (command_str[i] == '>' && input_start == 0)
    {
      if (output_start != 0)
      {
        fprintf(stderr, "Syntax error: Simple command (\"%s\") contains \
multiple of the same redirect\n", command_str);
        exit(1);
      }
      output_start = i + 1;
    } 
    else if (command_str[i] == '>' && input_start != 0)
    {
      output_start = i + 1;
      input_end = i;
    }
    else if (command_str[i] == '<' && output_start == 0)
    {
      if (input_start != 0)
      {
        fprintf(stderr, "Syntax error: Simple command (\"%s\") contains \
multiple of the same redirect\n", command_str);
        exit(1);
      }
      input_start = i + 1;
    } 
    else if (command_str[i] == '<' && output_start != 0)
    {
      input_start = i + 1;
      output_end = i;
    }
    if (command_str[i] == ')')
    {
      fprintf(stderr, "Syntax error: Command (\"%s\") contains unopened \
parenthesis\n", command_str);
      exit(1);
    }
  }

  if (word_end == 0)
  {
    word_end = strlen(command_str);
  }
  if (output_end == 0 && output_start != 0)
  {
    output_end = strlen(command_str);
    if (output_start == output_end)
    {
      fprintf(stderr, "Syntax error: Simple command (\"%s\") contains empty \
redirect\n", command_str);
        exit(1);
    }
  }
  if (input_end == 0 && input_start != 0)
  {
    input_end = strlen(command_str);
    if (input_start == input_end)
    {
      fprintf(stderr, "Syntax error: Simple command (\"%s\") contains empty \
redirect\n", command_str);
        exit(1);
    }
  }

  if (input_end != 0)
  {
    simple_command->input = (char*) checked_malloc((input_end - input_start + 
      1) * sizeof(char));
    memcpy(simple_command->input, command_str + input_start, 
      input_end - input_start);
    simple_command->input[input_end - input_start] = '\0';
    simple_command->input = strstrip(simple_command->input);
  }
  if (output_end != 0)
  {
    simple_command->output = (char*) checked_malloc((output_end - output_start +
      1) * sizeof(char));
    memcpy(simple_command->output, command_str + output_start, 
      output_end - output_start);
    simple_command->output[output_end - output_start] = '\0';
    simple_command->output = strstrip(simple_command->output);
  }

  char* words = (char*) checked_malloc((word_end + 1) * sizeof(char));
  memcpy(words, command_str, word_end);
  words[word_end] = '\0';

  if (strlen(words) == 0)
  {
    fprintf(stderr, "Syntax error: Simple command (\"%s\") contains word of \
length 0\n", command_str);
    exit(1);
  }

  int num_words = 0;
  char** word_split = strtok_str(strstrip(words), " ", &num_words);

  simple_command->u.word = (char**) checked_malloc(num_words * sizeof(char*));
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
  for (i = 0; i < input_length + 1; ++i)
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
  else
  {
    fprintf(stderr, "Syntax error: Input (\"%s\") contains too many \
consecutive &s\n", input_string);
    exit(1);
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
  int after_paren = 0;
  size_t input_length = strlen(input_string);
  char* first_char = input_string;
  while (*first_char != '\0')
  {
    first_operator first_op = get_first_operator(first_char);
    // Allocate space for the simple command up to the operator
    // and store in a new buffer

    char* buffer = (char*) checked_malloc((first_op.start_location - 
      first_char + 1) * sizeof(char));
    memcpy(buffer, first_char, first_op.start_location - first_char);
    buffer[first_op.start_location - first_char] = '\0';
    first_char = first_op.start_location;

    if (first_op.cmd_type == SUBSHELL_COMMAND)
    {
      // Get the command string contained within the ()
      int length = 0;
      char* sub_command = get_outer_subshell_cmd_str(first_op.start_location, 
        &length);
      if (strcmp(strstrip(buffer), "") != 0)
      {
	      fprintf(stderr, "Syntax error: No operator before open paren");
	      exit(1);	
      }
      free(buffer);
      first_char += (length + 2); // Account for outer parentheses
      
      first_operator first_op_after_subshell = get_first_operator(first_char);
      char* subshell_parse_buffer = (char*) checked_malloc((
        first_op_after_subshell.start_location - first_char + 1) * 
        sizeof(char));
      memcpy(subshell_parse_buffer, first_char, 
        first_op_after_subshell.start_location - first_char);
      subshell_parse_buffer[first_op_after_subshell.start_location - 
        first_char] = '\0';
      first_char = first_op_after_subshell.start_location;

      subshell_redirects redirects = get_subshell_redirects(
        subshell_parse_buffer);

      if (root_command == NULL)
      {
        root_command = create_new_command();
        root_command->type = SUBSHELL_COMMAND;
        root_command->u.subshell_command = convert_string_to_command_tree(
          sub_command);
        if (strcmp(redirects.input, "") != 0)
        {
          root_command->input = redirects.input;
        }
        if (strcmp(redirects.output, "") != 0)
        {
          root_command->output = redirects.output;
        }
	current_command = root_command;
      }
      else
      {
        command_t temp = create_new_command();
        temp->type = SUBSHELL_COMMAND;
        temp->u.subshell_command = convert_string_to_command_tree(sub_command);
        if (strcmp(redirects.input, "") != 0)
        {
          temp->input = redirects.input;
        }
        if (strcmp(redirects.output, "") != 0)
        {
          temp->output = redirects.output;
        }
        current_command->u.command[1] = temp;
        current_command = current_command->u.command[1];
      }
	after_paren = 1;
    }
    else if (first_op.cmd_type == SIMPLE_COMMAND)
    {
      // Call parse simple command to generate the new node in the command tree
      if (root_command == NULL)
      {
        root_command = parse_simple_command(buffer);
        current_command = root_command;
      }
      else
      {
        current_command->u.command[1] = parse_simple_command(buffer);
      }
      free(buffer);
      
      break;
    }
    else if (first_op.cmd_type == SEQUENCE_COMMAND)
    {
      first_char++; // Advance past the semicolon

      // Allocate the new root of the tree and connect it with the
      // old root
      if (root_command == NULL)
      {
        root_command = create_new_command();
        root_command->type = SEQUENCE_COMMAND;
        root_command->u.command[0] = parse_simple_command(buffer);
      }
      else
      {

        command_t temp = create_new_command();
        temp->type = SEQUENCE_COMMAND;
        temp->u.command[0] = root_command;
	if (after_paren == 0)
        	current_command->u.command[1] = parse_simple_command(buffer);
        after_paren = 0;
	root_command = temp;
      }

      // Create the rest of the command tree recursively
      free(buffer);
      current_command = root_command;
      root_command->u.command[1] = convert_string_to_command_tree(first_char);
      break;

    }
    else if (first_op.cmd_type == PIPE_COMMAND)
    {
      // TODO: PIPE_COMMAND
      first_char++;

      // Allocate the current command to contain the simple command up 
      // to the found operator. Use parse_simple_command to populate the
      // new node appropriately
      if (root_command == NULL)
      {
        root_command = create_new_command();
        current_command = root_command;
        root_command->type = PIPE_COMMAND;
        root_command->u.command[0] = parse_simple_command(buffer);
        root_command->u.command[1] = NULL;
        free(buffer);
        continue;
      }

      // Check the root operator. If it is of lesser precedence (AND/OR)
      // then create a new root. Otherwise, place the pipe command the same
      // as AND/OR command
      if (current_command->type == PIPE_COMMAND)
      {
        command_t temp = current_command->u.command[0];
        current_command->u.command[0] = create_new_command();
        current_command->u.command[0]->u.command[0] = temp;
        current_command->u.command[0]->u.command[1] = parse_simple_command(
          buffer);
        current_command->u.command[0]->type = PIPE_COMMAND;
        current_command->u.command[1] = NULL;
      }
      else
      {
		
        command_t temp  = create_new_command();
       
        temp->type = PIPE_COMMAND;
        temp->u.command[1] = NULL;
	if (after_paren == 0)
	{ 
		temp->u.command[0] = parse_simple_command(buffer);
		current_command->u.command[1] = temp;
		current_command = current_command->u.command[1];
	 }
	else
	{
		command_t temp2 = current_command->u.subshell_command;
		current_command->type = PIPE_COMMAND;
		current_command->u.command[0] = temp;
		temp->type = SUBSHELL_COMMAND;
		temp->u.subshell_command = temp2;
		current_command->u.command[1] = NULL;
		after_paren = 0;
	}
	  
     }
      free(buffer);
    }
    else
    {
      // TODO: OR_COMMAND, AND_COMMAND
      first_char++;
      first_char++; // Advance past the delimiter

      // Allocate the current command to contain the simple command up 
      // to the found operator. Use parse_simple_command to populate the
      // new node appropriately
      if (root_command == NULL)
      {
        root_command = create_new_command();
        root_command->u.command[0] = parse_simple_command(buffer);
        root_command->type = first_op.cmd_type;
        root_command->u.command[1] = NULL;
        current_command = root_command;
        free(buffer);
        continue;
      }

      if (current_command->type == AND_COMMAND || current_command->type == 
        OR_COMMAND)
      {
        command_t temp = current_command->u.command[0];
        enum command_type temp_type = current_command->type;
        current_command->u.command[0] = create_new_command();
        current_command->type = first_op.cmd_type;
        current_command->u.command[0]->u.command[0] = temp;
        current_command->u.command[0]->u.command[1] = parse_simple_command(
          buffer);
        current_command->u.command[0]->type = temp_type;
        current_command->u.command[1] = NULL;
      }
      else
      {
        command_t temp = root_command;
        root_command = create_new_command();
	if (after_paren == 0)
        {
	  current_command->u.command[1] = parse_simple_command(buffer);
        }
	after_paren = 0;
	root_command->u.command[0] = temp;
        root_command->type = first_op.cmd_type;
        current_command = root_command;
        current_command->u.command[1] = NULL;
      }

      free(buffer);    
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
      head = (command_stream_t) checked_malloc(sizeof(command_stream_t));
      cur = head;
      cur->command_tree = convert_string_to_command_tree(
        command_tree_strings[i]);
      cur->next = NULL;
    }
    else
    {
      cur->next = (command_stream_t) checked_malloc(sizeof(command_stream_t));
      cur = cur->next;
      cur->command_tree = convert_string_to_command_tree(
        command_tree_strings[i]);
      cur->next = NULL;
    }
  }

  free_array_strings(command_tree_strings);

  return head;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *), 
  void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  char* input_string = (char*) checked_malloc(256 * sizeof(char));
  size_t input_string_size = 0;
  size_t curr_max_size = 256;
  char next_byte = get_next_byte(get_next_byte_argument);

  while (next_byte != EOF)
  {
    ++input_string_size;

    if (input_string_size > curr_max_size - 1)
    {
      input_string = (char*) checked_realloc(input_string, (curr_max_size + 
        256) * sizeof(char));
      curr_max_size += 256;
    }

    input_string[input_string_size - 1] = next_byte;
    
    next_byte = get_next_byte(get_next_byte_argument);
  }

  input_string[input_string_size] = '\0';

  input_string = preprocess_input(input_string);

  command_stream_t command_trees = split_to_command_trees(input_string);

  free(input_string);

  return command_trees;
}

command_t
read_command_stream (command_stream_t* s)
{
  /* FIXME: Replace this with your implementation too.  */
  command_stream_t temp = *s;

  if (temp)
  {
    command_t current_command = temp->command_tree;
    *s = temp->next;
    return current_command;
  }
  else
  {
    return NULL;
  }
}
