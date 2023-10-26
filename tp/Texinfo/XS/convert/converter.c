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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* functions in this file correspond to Texinfo::Convert::Converter */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include "command_ids.h"
#include "tree_types.h"
#include "tree.h"
#include "extra.h"
#include "utils.h"
#include "builtin_commands.h"
#include "node_name_normalization.h"
#include "converter.h"

static CONVERTER *converter_list;
static size_t converter_number;
static size_t converter_space;

CONVERTER *
retrieve_converter (int converter_descriptor)
{
  if (converter_descriptor <= converter_number
      && converter_list[converter_descriptor -1].document != 0)
    return &converter_list[converter_descriptor -1];
  return 0;
}

/* descriptor starts at 1, 0 is an error */
size_t
register_converter (CONVERTER *converter)
{
  size_t converter_index;
  int slot_found = 0;
  int i;
  CONVERTER *registered_converter;

  for (i = 0; i < converter_number; i++)
    {
      if (converter_list[i].document == 0)
        {
          slot_found = 1;
          converter_index = i;
        }
    }
  if (!slot_found)
    {
      if (converter_number == converter_space)
        {
          converter_list = realloc (converter_list,
                              (converter_space += 5) * sizeof (CONVERTER));
          if (!converter_list)
            fatal ("realloc failed");
        }
      converter_index = converter_number;
      converter_number++;
    }
  registered_converter = &converter_list[converter_index];
  memcpy (registered_converter, converter, sizeof (CONVERTER));

  /*
  fprintf(stderr, "REGISTER CONVERTER %zu %p %p %p\n", converter_index +1,
                       converter, registered_converter, converter->document);
   */

  free (converter);

  registered_converter->converter_descriptor = converter_index +1;

  return converter_index +1;
}

/* freed by caller */
static COMMAND_OPTION_VALUE *
new_option_value (int type, int int_value, char *char_value)
{
  COMMAND_OPTION_VALUE *result
    = (COMMAND_OPTION_VALUE *) malloc (sizeof (COMMAND_OPTION_VALUE));
  result->type = type;
  if (type == GO_int)
    result->int_value = int_value;
  else
    result->char_value = char_value;
  return result;
}

/* freed by caller */
static COMMAND_OPTION_VALUE *
command_init (enum command_id cmd, OPTIONS *init_conf)
{
  COMMAND_OPTION_REF *init_conf_ref;
  COMMAND_OPTION_DEFAULT *option_default;
  COMMAND_OPTION_VALUE *option_value = 0;
  if (init_conf)
    {
      init_conf_ref = get_command_option (init_conf, cmd);
      if (init_conf_ref)
        {
          if (init_conf_ref->type == GO_int)
            {
              if (*(init_conf_ref->int_ref) >= 0)
                {
                  option_value
                    = new_option_value (GO_int, *(init_conf_ref->int_ref), 0);
                  free (init_conf_ref);
                  return option_value;
                }
            }
          else
            {
              if (*(init_conf_ref->char_ref))
                {
                  option_value
                    = new_option_value (GO_char, -1, *(init_conf_ref->char_ref));
                  free (init_conf_ref);
                  return option_value;
                }
            }
        }
      free (init_conf_ref);
    }
  option_default = &command_option_default_table[cmd];
  if (option_default->type == GO_int)
    {
      if (option_default->value >= 0)
        option_value = new_option_value (GO_int, option_default->value, 0);
    }
  else if (option_default->type == GO_char)
    {
      if (option_default->string)
        option_value = new_option_value (GO_char, -1, option_default->string);
    }
  return 0;
}

void
set_global_document_commands (CONVERTER *converter,
                              const enum command_location location,
                              const enum command_id *cmd_list)
{
  if (location == CL_before)
    {
      int i;
      for (i = 0; cmd_list[i] > 0; i++)
        {
          enum command_id cmd = cmd_list[i];
          COMMAND_OPTION_VALUE *option_value = command_init (cmd,
                                                converter->init_conf);
          if (option_value)
            {
              COMMAND_OPTION_REF *option_ref
               = get_command_option (converter->conf, cmd);
              if (option_value->type == GO_int)
                *(option_ref->int_ref) = option_value->int_value;
              else
                {
                  free (*(option_ref->char_ref));
                  *(option_ref->char_ref) = strdup (option_value->char_value);
                }
              free (option_ref);
              free (option_value);
            }
        }
    }
  else
    {
      int i;
      for (i = 0; cmd_list[i] > 0; i++)
        {
          ELEMENT *element;
          enum command_id cmd = cmd_list[i];
          if (converter->conf->DEBUG > 0)
            {
              fprintf (stderr, "SET_global(%s) %s\n",
                       command_location_names[location],
                       builtin_command_data[cmd].cmdname);
            }
          element = set_global_document_command (converter, cmd, location);
          if (!element)
            {
              COMMAND_OPTION_VALUE *option_value = command_init (cmd,
                                                      converter->init_conf);
              if (option_value)
                {
                  COMMAND_OPTION_REF *option_ref
                    = get_command_option (converter->conf, cmd);
                  if (option_value->type == GO_int)
                    *(option_ref->int_ref) = option_value->int_value;
                  else
                    {
                      free (*(option_ref->char_ref));
                      *(option_ref->char_ref)
                        = strdup (option_value->char_value);
                    }

                  free (option_ref);
                  free (option_value);
                }
            }
        }
    }
}

static void
id_to_filename (CONVERTER *self, char **id_ref)
{
  if (self->conf->BASEFILENAME_LENGTH < 0)
    return;
  char *id = *id_ref;
  if (strlen (id) > self->conf->BASEFILENAME_LENGTH)
    {
      id[self->conf->BASEFILENAME_LENGTH] = '\0';
    }
}

TARGET_FILENAME *
normalized_sectioning_command_filename (CONVERTER *self, ELEMENT *command)
{
  TARGET_FILENAME *result
     = (TARGET_FILENAME *) malloc (sizeof (TARGET_FILENAME));
  TEXT filename;
  char *normalized_file_name;
  char *normalized_name
    = normalize_transliterate_texinfo_contents (command->args.list[0],
                                                (self->conf->TEST > 0));
  normalized_file_name = strdup (normalized_name);
  id_to_filename (self, &normalized_file_name);

  text_init (&filename);
  text_append (&filename, normalized_file_name);
  if (self->conf->EXTENSION && strlen (self->conf->EXTENSION))
    {
      text_append (&filename, ".");
      text_append (&filename, self->conf->EXTENSION);
    }

  free (normalized_file_name);

  result->filename = filename.text;
  result->target = normalized_name;

  return result;
}

char *
node_information_filename (CONVERTER *self, char *normalized,
                           ELEMENT *label_element)
{
  char *filename;

  if (normalized)
    {
      if (self->conf->TRANSLITERATE_FILE_NAMES > 0)
        {
          filename = normalize_transliterate_texinfo_contents (label_element,
                                                       (self->conf->TEST > 0));
        }
      else
        filename = strdup (normalized);
    }
  else if (label_element)
    {
      filename = convert_contents_to_identifier (label_element);
    }
  else
    filename = strdup ("");

  id_to_filename (self, &filename);
  return filename;
}

ELEMENT *
comma_index_subentries_tree (ELEMENT *current_entry,
                             char *separator)
{
  ELEMENT *result = new_element (ET_NONE);
  char *subentry_separator = separator;
  if (!separator)
    subentry_separator = ", ";

  while (1)
    {
      ELEMENT *subentry = lookup_extra_element (current_entry, "subentry");
      if (subentry)
        {
          ELEMENT *separator = new_element (ET_NONE);
          text_append (&separator->text, subentry_separator);
          current_entry = subentry;
          add_to_contents_as_array (result, separator);
          add_to_contents_as_array (result, current_entry->args.list[0]);
        }
      else
        break;
    }
  if (result->contents.number > 0)
    return result;
  else
    {
      destroy_element (result);
      return 0;
    }
}

void
free_comma_index_subentries_tree (ELEMENT *element)
{
  /* destroy separator elements */
  int i;
  for (i = 0; i < element->contents.number; i++)
    {
      ELEMENT *content = element->contents.list[i];
      if (content->type == ET_NONE)
        destroy_element (content);
    }
  destroy_element (element);
}

/* to be freed by caller */
char *
top_node_filename (CONVERTER *self, char *document_name)
{
  TEXT top_node_filename;

  if (self->conf->TOP_FILE && strlen (self->conf->TOP_FILE))
    {
      return strdup (self->conf->TOP_FILE);
    }

  if (document_name)
    {
      text_init (&top_node_filename);
      text_append (&top_node_filename, document_name);
      if (self->conf->EXTENSION && strlen (self->conf->EXTENSION))
        {
          text_append (&top_node_filename, ".");
          text_append (&top_node_filename, self->conf->EXTENSION);
        }
      return top_node_filename.text;
    }
  return 0;
}

void
initialize_output_units_files (CONVERTER *self)
{
  self->output_unit_files = (FILE_NAME_PATH_COUNTER_LIST *)
    malloc (sizeof (FILE_NAME_PATH_COUNTER_LIST));
  memset (self->output_unit_files, 0,
    sizeof (FILE_NAME_PATH_COUNTER_LIST));
}

static FILE_NAME_PATH_COUNTER *
find_output_unit_file (CONVERTER *self, char *filename)
{
  FILE_NAME_PATH_COUNTER_LIST *output_unit_files
    = self->output_unit_files;
  int i;
  for (i = 0; i < output_unit_files->number; i++)
    {
      if (!strcmp (output_unit_files->list[i].normalized_filename, filename))
        return &output_unit_files->list[i];
    }
  return 0;
}

static FILE_NAME_PATH_COUNTER *
add_output_units_file (CONVERTER *self, char *filename,
                       char *normalized_filename)
{
  FILE_NAME_PATH_COUNTER *new_output_unit_file;
  FILE_NAME_PATH_COUNTER_LIST *output_unit_files
    = self->output_unit_files;

  if (output_unit_files->number == output_unit_files->space)
    {
      output_unit_files->list = realloc (output_unit_files->list,
         (output_unit_files->space += 5) * sizeof (FILE_NAME_PATH_COUNTER));
      if (!output_unit_files->list)
        fatal ("realloc failed");
    }

  new_output_unit_file = &output_unit_files->list[output_unit_files->number];
  memset (new_output_unit_file, 0, sizeof (FILE_NAME_PATH_COUNTER));
  new_output_unit_file->filename = strdup (filename);
  if (normalized_filename)
    new_output_unit_file->normalized_filename = strdup (normalized_filename);
  else
    new_output_unit_file->normalized_filename = strdup (filename);

  output_unit_files->number++;

  return new_output_unit_file;
}

/*
  If CASE_INSENSITIVE_FILENAMES is set, reuse the first
  filename with the same name insensitive to the case.
 */
static FILE_NAME_PATH_COUNTER *
register_normalize_case_filename (CONVERTER *self, char *filename)
{
  FILE_NAME_PATH_COUNTER *output_unit_file;
  if (self->conf->CASE_INSENSITIVE_FILENAMES > 0)
    {
      char *lc_filename = to_upper_or_lower_multibyte (filename, -1);
      output_unit_file = find_output_unit_file (self, lc_filename);
      if (output_unit_file)
        {
          if (self->conf->DEBUG > 0)
            {
              fprintf (stderr, "Reusing case-insensitive %s for %s\n",
                       output_unit_file->filename, filename);
            }
          free (lc_filename);
        }
      else
        {
          output_unit_file = add_output_units_file (self, filename,
                                                    lc_filename);
          free (lc_filename);
        }
    }
  else
    {
      output_unit_file = find_output_unit_file (self, filename);
      if (output_unit_file)
        {
          if (self->conf->DEBUG > 0)
            {
              fprintf (stderr, "Reusing %s for %s\n",
                       output_unit_file->filename, filename);
            }
        }
      else
        output_unit_file = add_output_units_file (self, filename, 0);
    }
  return output_unit_file;
}

FILE_NAME_PATH_COUNTER *
set_output_unit_file (CONVERTER *self, OUTPUT_UNIT *output_unit,
                      char *filename, int set_counter)
{
  FILE_NAME_PATH_COUNTER *output_unit_file
    = register_normalize_case_filename (self, filename);
  if (set_counter)
    output_unit_file->counter++;
  output_unit->unit_filename = output_unit_file->filename;
  return output_unit_file;
}

void
set_file_path (CONVERTER *self, char *filename, char *filepath,
               char *destination_directory)
{
  FILE_NAME_PATH_COUNTER *output_unit_file
    = register_normalize_case_filename (self, filename);
  char *filepath_str;
  int free_filepath = 0;

  if (!filepath)
    if (destination_directory && strlen (destination_directory))
      {
        xasprintf (&filepath_str, "%s/%s", destination_directory,
                                  output_unit_file->filename);
        free_filepath = 1;
      }
    else
      filepath_str = output_unit_file->filename;
  else
    filepath_str = filepath;

  output_unit_file->filepath = strdup (filepath_str);
  if (free_filepath)
    free (filepath_str);
}