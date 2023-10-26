/* Copyright 2010-2023 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/* corresponding to Texinfo::Convert::Utils */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "options_types.h"
#include "element_types.h"
#include "tree_types.h"
#include "command_ids.h"
#include "builtin_commands.h"
#include "tree.h"
#include "extra.h"
#include "errors.h"
#include "debug.h"
#include "manipulate_tree.h"
#include "utils.h"
#include "translations.h"
#include "convert_to_texinfo.h"
#include "convert_utils.h"

char *convert_utils_month_name[12] = {
       "January", "February", "March", "April", "May",
     "June", "July", "August", "September", "October",
     "November", "December"
};

/* in Texinfo::Common */
char *
element_associated_processing_encoding (ELEMENT *element)
{
  char *input_encoding = lookup_extra_string (element, "input_encoding_name");
  return input_encoding;
}

ACCENTS_STACK *
find_innermost_accent_contents (ELEMENT *element)
{
  ELEMENT *current = element;
  ELEMENT *argument = 0;
  ACCENTS_STACK *accent_stack = malloc (sizeof (ACCENTS_STACK));

  accent_stack->stack = new_element (ET_NONE);
  accent_stack->argument = 0;

  while (1)
    {
      ELEMENT *arg;
      int i;

      /* the following can happen if called with a bad tree */
      if (!current->cmd || !(builtin_command_flags(current) & CF_accent))
        return accent_stack;
      add_to_contents_as_array (accent_stack->stack, current);
      /* A bogus accent, that may happen */
      if (current->args.number <= 0)
        return accent_stack;
      arg = current->args.list[0];
      if (arg->contents.number <= 0)
        return accent_stack;
      for (i = 0; i < arg->contents.number; i++)
        {
          ELEMENT *content = arg->contents.list[i];
          if (!(content->cmd && (content->cmd == CM_c
                                 || content->cmd == CM_comment)))
            {
              if (content->cmd
                  && builtin_command_flags(content) & CF_accent)
                {
                  current = content;
                  if (argument)
                    {
                      destroy_element (argument);
                      argument = 0;
                    }
                  break;
                }
              else
                {
                  if (!argument)
                    argument = new_element (ET_NONE);
                  add_to_contents_as_array (argument, content);
                }
            }
        }
      if (argument)
        break;
    }
  if (argument)
    accent_stack->argument = argument;
  return accent_stack;
}

void
destroy_accent_stack (ACCENTS_STACK *accent_stack)
{
  destroy_element (accent_stack->stack);
  if (accent_stack->argument)
    destroy_element (accent_stack->argument);
  free (accent_stack);
}

/*
 TEXT can be indented, however this can only happen for
 *heading headings, which are not numbered.  If it was not the case,
 the code would need to be changed.
*/
/* caller should free return */
char *
add_heading_number (OPTIONS *options, ELEMENT *current, char *text,
                    int numbered)
{
  TEXT result;
  char *number = 0;
  if (numbered != 0)
    number = lookup_extra_string (current, "section_number");

  /* TODO translate code to use options as $self translation
     to be done when this can be tested, so when Text converter
     is called from another converter
  if ($self) {
    if (defined($number)) {
      if ($current->{'cmdname'} eq 'appendix'
          and $current->{'extra'}->{'section_level'} == 1) {
        $result = $self->gdt_string('Appendix {number} {section_title}',
                   {'number' => $number, 'section_title' => $text});
      } else {
        $result = $self->gdt_string('{number} {section_title}',
                   {'number' => $number, 'section_title' => $text});
      }
    } else {
      $result = $text;
    }
  } else
*/

  {
    text_init (&result);
    if (current->cmd == CM_appendix)
      {
        int status;
        int section_level = lookup_extra_integer (current, "section_level",
                                                  &status);
        if (section_level == 1)
          text_append (&result, "Appendix ");
      }
    if (number)
      {
        text_append (&result, number);
        text_append (&result, " ");
      }
    text_append (&result, text);
   }
  return result.text;
}

static char *
convert_to_utf8 (char *s, ENCODING_CONVERSION *conversion,
                 SOURCE_INFO *source_info)
{
  char *result;
  if (!conversion)
    return strdup (s);
  result = encode_with_iconv (conversion->iconv, s, source_info);
  return result;
}

/*
  Reverse the decoding of the file name from the input encoding.
  FILE_NAME_ENCODING is used to return the encoding.
  The caller should free the return value and FILE_NAME_ENCODING.
*/
char *
encoded_input_file_name (OPTIONS *options,
                         GLOBAL_INFO *global_information,
                         char *file_name, char *input_file_encoding,
                         char **file_name_encoding, SOURCE_INFO *source_info)
{
  char *result;
  char *encoding = 0;
  int status;

  if (options && options->INPUT_FILE_NAME_ENCODING)
    encoding = options->INPUT_FILE_NAME_ENCODING;
  else if (options && options->DOC_ENCODING_FOR_INPUT_FILE_NAME != 0
           || (!options))
    {
      if (input_file_encoding)
        encoding = input_file_encoding;
      else if (global_information && global_information->input_encoding_name)
        encoding = global_information->input_encoding_name;
    }
  else if (options)
    encoding = options->LOCALE_ENCODING;

  result = encode_string (file_name, encoding, &status, source_info);

  if (status)
    *file_name_encoding = strdup(encoding);
   else
    *file_name_encoding = 0;
  return result;
}

ELEMENT *
expand_verbatiminclude (ERROR_MESSAGE_LIST *error_messages,
                        OPTIONS *options, GLOBAL_INFO *global_information,
                        ELEMENT *current)
{
  ELEMENT *verbatiminclude = 0;
  char *file_name_encoding;
  char *file_name_text = lookup_extra_string (current, "text_arg");
  char *file_name;
  char *file;
  STRING_LIST *include_directories = 0;

  if (!file_name_text)
    return 0;

  char *input_encoding = element_associated_processing_encoding (current);

  file_name = encoded_input_file_name (options, global_information,
                                       file_name_text, input_encoding,
                                       &file_name_encoding,
                                       &current->source_info);

  if (options)
    include_directories = &options->INCLUDE_DIRECTORIES;

  file = locate_include_file (file_name, include_directories);

  if (file)
    {
      FILE *stream = 0;
      ENCODING_CONVERSION *conversion;

      stream = fopen (file, "r");
      if (!stream)
        {
          if (error_messages)
            {
              int status;
              char *decoded_file;
              if (file_name_encoding)
                decoded_file = decode_string (file, file_name_encoding,
                                              &status, &current->source_info);
              else
                decoded_file = file;
              message_list_command_error (error_messages, current,
                                          "could not read %s: %s",
                                          decoded_file, strerror (errno));
              if (file_name_encoding)
                free (decoded_file);
            }
        }
      else
        {
          conversion
           = get_encoding_conversion (input_encoding, &input_conversions);
          verbatiminclude = new_element (ET_NONE);
          verbatiminclude->cmd = CM_verbatim;
          verbatiminclude->parent = current->parent;
          while (1)
            {
              size_t n;
              char *line = 0;
              char *text;
              ELEMENT *raw;
              ssize_t status = getline (&line, &n, stream);
              if (status == -1)
                {
                  free (line);
                  break;
                }

              text = convert_to_utf8 (line, conversion, &current->source_info);
              free (line);
              raw = new_element (ET_raw);
              text_append (&raw->text, text);
              add_to_element_contents (verbatiminclude, raw);
              free (text);
            }
          if (fclose (stream) == EOF)
            {
              if (error_messages)
                {
                  int status;
                  char *decoded_file;
                  if (file_name_encoding)
                    decoded_file = decode_string (file, file_name_encoding,
                                                  &status,
                                                  &current->source_info);
                  else
                    decoded_file = file;
                  message_list_command_error (error_messages, current,
                             "error on closing @verbatiminclude file %s: %s",
                                 decoded_file, strerror (errno));
                  if (file_name_encoding)
                    free (decoded_file);
                }
            }
        }
      free (file);
    }
  else if (error_messages)
    {
      message_list_command_error (error_messages, current,
                                  "@%s: could not find %s",
                                  builtin_command_name (current->cmd),
                                  file_name_text);
    }
  free (file_name);
  free (file_name_encoding);
  return verbatiminclude;
}

PARSED_DEF *
definition_arguments_content (ELEMENT *element)
{
  PARSED_DEF *result = malloc (sizeof (PARSED_DEF));
  memset (result, 0, sizeof (PARSED_DEF));
  if (element->args.number >= 0)
    {
      int i;
      ELEMENT *def_line = element->args.list[0];
      if (def_line->contents.number > 0)
        {
          for (i = 0; i < def_line->contents.number; i++)
            {
              ELEMENT *arg = def_line->contents.list[i];
              char *role = lookup_extra_string (arg, "def_role");
              if (!role)
                fprintf (stderr, "BUG: NO ROLE %s\n", print_element_debug (arg, 0));
              if (!strcmp (role, "class"))
                result->class = arg;
              else if (!strcmp (role, "category"))
                result->category = arg;
              else if (!strcmp (role, "type"))
                result->type = arg;
              else if (!strcmp (role, "name"))
                result->name = arg;
              else if (!strcmp (role, "arg") || !strcmp (role, "typearg")
                       || !strcmp (role, "delimiter"))
                {
                  i--;
                  break;
                }
            }
          if (i < def_line->contents.number - 1)
            {
              ELEMENT *args = new_element (ET_NONE);
              insert_slice_into_contents (args, 0, def_line,
                                          i + 1, def_line->contents.number);
              result->args = args;
            }
        }
    }
  return result;
}

void
destroy_parsed_def (PARSED_DEF *parsed_def)
{
  if (parsed_def->args)
    destroy_element (parsed_def->args);
  free (parsed_def);
}

ELEMENT *
definition_category_tree (OPTIONS * options, ELEMENT *current)
{
  ELEMENT *result = 0;
  ELEMENT *arg_category = 0;
  ELEMENT *arg_class = 0;
  ELEMENT *arg_class_code;
  ELEMENT *class_copy;
  char *def_command;

  if (current->args.number >= 0)
    {
      int i;
      ELEMENT *def_line = current->args.list[0];
      for (i = 0; i < def_line->contents.number; i++)
        {
          ELEMENT *arg = def_line->contents.list[i];
          char *role = lookup_extra_string (arg, "def_role");
          if (!strcmp (role, "class"))
            arg_class = arg;
          else if (!strcmp (role, "category"))
            arg_category = arg;
          else if (!strcmp (role, "arg") || !strcmp (role, "typearg")
                   || !strcmp (role, "delimiter"))
            break;
        }
    }
  else
    return 0;

  if (!arg_class)
    {
      if (arg_category)
        {
          ELEMENT *category_copy = copy_tree (arg_category, 0);
          return category_copy;
        }
      else
       return 0;
    }

  class_copy = copy_tree (arg_class, 0);

  if (!options)
    {
      ELEMENT *brace_command_arg = new_element (ET_brace_command_arg);
      arg_class_code = new_element (ET_NONE);
      arg_class_code->cmd = CM_code;
      add_to_element_contents (brace_command_arg, class_copy);
      add_to_element_args (arg_class_code, brace_command_arg);
    }

  def_command = lookup_extra_string (current, "def_command");

  /* do something more efficient */
  if (!strcmp(def_command, "defop")
      || !strcmp(def_command, "deftypeop")
      || !strcmp(def_command, "defmethod")
      || !strcmp(def_command, "deftypemethod"))
    {
      ELEMENT *category_copy = copy_tree (arg_category, 0);
      if (options)
        {
          NAMED_STRING_ELEMENT_LIST *substrings
                                       = new_named_string_element_list ();
          add_element_to_named_string_element_list (substrings,
                                                     "category", category_copy);
          add_element_to_named_string_element_list (substrings,
                                                           "class", class_copy);
          /*
          TRANSLATORS: association of a method or operation name with a class
          in descriptions of object-oriented programming methods or operations.
           */

          result = gdt_tree ("{category} on @code{{class}}", 0, options,
                             substrings, 0, 0);
          destroy_named_string_element_list (substrings);
        }
      else
        {
          result = new_element (ET_NONE);
          ELEMENT *text_element = new_element (ET_NONE);
          add_to_element_contents (result, category_copy);
          text_append (&text_element->text, " on ");
          add_to_element_contents (result, text_element);
          add_to_element_contents (result, arg_class_code);
        }
    } else if (!strcmp(def_command, "defivar")
      || !strcmp(def_command, "deftypeivar")
      || !strcmp(def_command, "defcv")
      || !strcmp(def_command, "deftypecv"))
    {
      ELEMENT *category_copy = copy_tree (arg_category, 0);
      if (options)
        {
          NAMED_STRING_ELEMENT_LIST *substrings
                                       = new_named_string_element_list ();
          add_element_to_named_string_element_list (substrings,
                                                     "category", category_copy);
          add_element_to_named_string_element_list (substrings,
                                                           "class", class_copy);
          /*
          TRANSLATORS: association of a method or operation name with a class
          in descriptions of object-oriented programming methods or operations.
           */

          result = gdt_tree ("{category} of @code{{class}}", 0, options,
                             substrings, 0, 0);
          destroy_named_string_element_list (substrings);
        }
      else
        {
          result = new_element (ET_NONE);
          ELEMENT *text_element = new_element (ET_NONE);
          add_to_element_contents (result, category_copy);
          text_append (&text_element->text, " of ");
          add_to_element_contents (result, text_element);
          add_to_element_contents (result, arg_class_code);
        }
    }
  return result;
}

ELEMENT *
translated_command_tree (CONVERTER *self, enum command_id cmd)
{
  if (self->translated_commands)
    {
      TRANSLATED_COMMAND **i;
      TRANSLATED_COMMAND *tc;
      for (i = self->translated_commands; (tc = *i); i++)
        {
          if (tc->cmd == cmd && tc->translation)
            {
              ELEMENT *result = gdt_tree (tc->translation, 0, self->conf,
                                          0, 0, 0);
              return result;
            }
        }
    }
  return 0;
}