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

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "tree_types.h"
#include "command_ids.h"
/* document and options used in complete_indices */
#include "options_types.h"
#include "document_types.h"
#include "tree.h"
#include "errors_parser.h"
#include "command_stack.h"
#include "context_stack.h"
#include "builtin_commands.h"
#include "extra.h"
/* for ultimate_index xasprintf */
#include "utils.h"
/* for copy_tree */
#include "manipulate_tree.h"
#include "commands.h"
#include "translations.h"
#include "document.h"
/*
#include "convert_to_texinfo.h"
*/
#include "parser.h"
#include "indices.h"

INDEX **index_names = 0;
int number_of_indices = 0;
int space_for_indices = 0;

typedef struct {
    enum command_id cmd;
    INDEX *idx;
} CMD_TO_IDX;

/* Array mapping Texinfo commands to index data structures. */
static CMD_TO_IDX *cmd_to_idx = 0;
static size_t num_index_commands = 0;
static size_t cmd_to_idx_space = 0;

static void
associate_command_to_index (enum command_id cmd, INDEX *idx)
{
  if (num_index_commands == cmd_to_idx_space)
    {
      cmd_to_idx = realloc (cmd_to_idx,
                            sizeof (CMD_TO_IDX) * (cmd_to_idx_space += 10));
      if (!cmd_to_idx)
        fatal ("no index for command");
    }

  cmd_to_idx[num_index_commands].cmd = cmd;
  cmd_to_idx[num_index_commands++].idx = idx;
}

/* Get the index associated with CMD. */
INDEX *
index_of_command (enum command_id cmd)
{
  int i;

  for (i = 0; i < num_index_commands; i++)
    {
      if (cmd_to_idx[i].cmd == cmd)
        return cmd_to_idx[i].idx;
    }
  return 0;
}


/* Save a new Texinfo command with the name CMDNAME and record that it
   creates index entries in IDX. */
static void
add_index_command (char *cmdname, INDEX *idx)
{
  enum command_id new = add_texinfo_command (cmdname);
  user_defined_command_data[new & ~USER_COMMAND_BIT].flags
    |= CF_line | CF_index_entry_command | CF_contain_basic_inline
    /*  | CF_close_paragraph */
      | CF_no_paragraph;
  user_defined_command_data[new & ~USER_COMMAND_BIT].data = LINE_line;
  associate_command_to_index (new, idx);
}

static INDEX *
add_index_internal (char *name, int in_code)
{
  INDEX *idx = malloc (sizeof (INDEX));

  memset (idx, 0, sizeof *idx);
  idx->name = name;
  idx->prefix = name;
  idx->in_code = in_code;
  if (number_of_indices == space_for_indices)
    {
      space_for_indices += 5;
      index_names = realloc (index_names, (space_for_indices + 1)
                             * sizeof (INDEX *));
    }
  index_names[number_of_indices++] = idx;
  index_names[number_of_indices] = 0;
  return idx;
}



/* Add a user defined index with the name NAME */
void
add_index (const char *name, int in_code)
{
  INDEX *idx = indices_info_index_by_name (index_names, name);
  char *cmdname;

  if (!idx)
    idx = add_index_internal (strdup (name), in_code);

  /* For example, "rq" -> "rqindex". */
  xasprintf (&cmdname, "%s%s", name, "index");
  add_index_command (cmdname, idx);
  free (cmdname);
}

void
init_index_commands (void)
{
  INDEX *idx;

  struct def { char *name; int in_code; }
  *p, default_indices[] = {
    "cp", 0, /* concepts */
    "fn", 1, /* functions */
    "vr", 1, /* variables */
    "ky", 1, /* keystrokes */
    "pg", 1, /* programs */
    "tp", 1, /* types */
    0, 0
  };
  int i, j;

  char name[] = "?index";
  char name2[] = "??index";

#define MAX (10 * 2)

#define X(command) CM_##command, CM_##command##x
  struct def_cmds { char *name; enum command_id id[MAX]; }
    def_command_indices[] = {
      "fn",

      {X(deffn),
       X(deftypefn),
       X(deftypeop),
       X(defop),
       X(defun),
       X(defmac),
       X(defspec),
       X(deftypefun),
       X(defmethod),
       X(deftypemethod),
      },

      "vr",

      {X(defvr),
       X(deftypevr),
       X(defcv),
       X(deftypecv),
       X(defvar),
       X(defivar),
       X(defopt),
       X(deftypevar),
       X(deftypeivar),
      },

      "tp",

      {X(deftp),}
    };
#undef X

  number_of_indices = 0;
  num_index_commands = 0;

  for (p = default_indices; p->name; p++)
    {
      /* Both @cindex and @cpindex are added. */
      idx = add_index_internal (strdup (p->name), p->in_code);

      *name = p->name[0];
      add_index_command (name, idx); /* @cindex */

      name2[0] = p->name[0];
      name2[1] = p->name[1];
      add_index_command (name2, idx); /* @cpindex */
    }

  associate_command_to_index (CM_vtable,
    indices_info_index_by_name (index_names, "vr"));
  associate_command_to_index (CM_ftable,
    indices_info_index_by_name (index_names, "fn"));

  for (i = 0;
       i < sizeof (def_command_indices) / sizeof (def_command_indices[0]);
       i++)
    {
      enum command_id cmd;
      idx = indices_info_index_by_name (index_names,
                                        def_command_indices[i].name);
      if (idx)
        {
          for (j = 0; j < MAX; j++)
            {
              cmd = def_command_indices[i].id[j];
              if (cmd)
                associate_command_to_index (cmd, idx);
            }
        }
    }
#undef MAX
}

/* INDEX_TYPE_CMD is used to determine which index to enter the entry in.
   index entry.  ELEMENT is the element in the main body of the manual that
   the index entry refers/belongs to.
*/
void
enter_index_entry (enum command_id index_type_cmd,
                   ELEMENT *element)
{
  INDEX *idx;
  INDEX_ENTRY *entry;
  TEXT ignored_chars;

  if (global_restricted)
    return;

  idx = index_of_command (index_type_cmd);
  if (idx->entries_number == idx->entries_space)
    {
      idx->index_entries = realloc (idx->index_entries,
                     sizeof (INDEX_ENTRY) * (idx->entries_space += 20));
      if (!idx->index_entries)
        fatal ("realloc failed");
    }
  entry = &idx->index_entries[idx->entries_number++];
  memset (entry, 0, sizeof (INDEX_ENTRY));

  entry->index_name = idx->name;
  /* not needed in the parser, the position in the index is directly used.
     Used for sorting */
  entry->number = idx->entries_number;
  entry->entry_element = element;

  /* Create ignored_chars string. */
  text_init (&ignored_chars);
  if (global_info.ignored_chars.backslash)
    text_append (&ignored_chars, "\\");
  if (global_info.ignored_chars.hyphen)
    text_append (&ignored_chars, "-");
  if (global_info.ignored_chars.lessthan)
    text_append (&ignored_chars, "<");
  if (global_info.ignored_chars.atsign)
    text_append (&ignored_chars, "@");
  if (ignored_chars.end > 0)
    {
      add_extra_string_dup (element, "index_ignore_chars", ignored_chars.text);
      free (ignored_chars.text);
    }

  /* index_entry is an array with two elements.  Use
     extra_misc_args to pass that information as an array */
  {
    ELEMENT *index_entry = new_element (ET_NONE);
    ELEMENT *e = new_element (ET_NONE);
    text_append (&e->text, idx->name);
    add_to_element_contents (index_entry, e);
    e = new_element (ET_NONE);
    add_extra_integer (e, "integer", idx->entries_number);
    add_to_element_contents (index_entry, e);
    add_extra_misc_args (element, "index_entry", index_entry);
  }

  if (nesting_context.regions_stack.top > 0)
    {
      enum command_id region = top_command (&nesting_context.regions_stack);
      add_extra_string_dup (element, "element_region", command_name (region));
    }
  else if (current_node)
    add_extra_element (element, "element_node", current_node);

  if (nesting_context.regions_stack.top == 0
      && !current_node && !current_section)
    line_warn ("entry for index `%s' outside of any node", idx->name);
}

/* turn spaces that are ignored before @-commands like @sortas{} and
   @seeentry{} back to regular spaces if there is content after the @-command
 */
void
set_non_ignored_space_in_index_before_command (ELEMENT *content)
{
  ELEMENT *e;
  ELEMENT *pending_spaces_element = 0;
  int i;
  for (i = 0; i < content->contents.number; i++)
    {
      /* could also be, but it does not seems to be needed here:
         e = contents_child_by_index (content, i); */
      e = content->contents.list[i];
      if (e->type == ET_internal_spaces_before_brace_in_index)
        {
          pending_spaces_element = e;
          /* set to "spaces_at_end" in case there are only spaces after */
          e->type = ET_spaces_at_end;
        }
      else if (pending_spaces_element
                && ! (e->cmd == CM_sortas
                       || e->cmd == CM_seeentry
                       || e->cmd == CM_seealso
                       || e->type == ET_spaces_after_close_brace)
                && (! check_space_element(e)))
        {
          pending_spaces_element->type = ET_NONE;
          pending_spaces_element = 0;
        }
    }
}



/* reset indices without unallocating them nor the list of indices */
void
forget_indices (void)
{
  index_names = 0;
  number_of_indices = 0;
  space_for_indices = 0;
}

void
resolve_indices_merged_in (void)
{
  INDEX **i, *idx;

  for (i = index_names; (idx = *i); i++)
    {
      if (idx->merged_in)
        {
          /* This index is merged in another one. */
          INDEX *ultimate = ultimate_index (idx);
          idx->merged_in = ultimate;
        }
    }
}

/* complete some @def* index information that require translations.
   Done in a separate function and not inside the main parser loop because
   it requires parsing Texinfo code in gdt_tree too */
void
complete_indices (int document_descriptor)
{
  INDEX **i, *idx;
  DOCUMENT *document;
  INDEX **index_names;
  OPTIONS *options;

  /* beware that document may have a change in adress if realloc on
     the documents list is called in gdt.  So only use it here and
     not after gdt call */
  document = retrieve_document (document_descriptor);

  index_names = document->index_names;
  options = document->options;

  for (i = index_names; (idx = *i); i++)
    {
      if (idx->entries_number > 0)
        {
          int j;
          for (j = 0; j < idx->entries_number; j++)
            {
              INDEX_ENTRY *entry;
              ELEMENT *main_entry_element;
              ELEMENT *idx_element;
              char *def_cmdname;

              entry = &idx->index_entries[j];
              main_entry_element = entry->entry_element;

              def_cmdname = lookup_extra_string (main_entry_element,
                                                 "def_command");

              idx_element = lookup_extra_element (main_entry_element,
                                                  "def_index_element");
              if (def_cmdname && !idx_element)
                {
                  ELEMENT *name = 0;
                  ELEMENT *class = 0;
                  ELEMENT *def_l_e = main_entry_element->args.list[0];
                  if (def_l_e->contents.number > 0)
                    {
                      int ic;
                      for (ic = 0; ic < def_l_e->contents.number; ic++)
                        {
                          ELEMENT *arg = def_l_e->contents.list[ic];
                          char *role = lookup_extra_string (arg, "def_role");
                          if (!strcmp (role, "name"))
                            name = arg;
                          else if (!strcmp (role, "class"))
                            class = arg;
                          else if (!strcmp (role, "arg")
                                   || !strcmp (role, "typearg")
                                   || !strcmp (role, "delimiter"))
                            break;
                        }
                    }

                  if (name && class)
                    {
                      char *lang = lookup_extra_string (main_entry_element,
                                                       "documentlanguage");
                      ELEMENT *index_entry;
                      ELEMENT *index_entry_normalized = new_element (ET_NONE);
                      ELEMENT *text_element = new_element (ET_NONE);
                      enum command_id def_command
                        = lookup_command (def_cmdname);
                      NAMED_STRING_ELEMENT_LIST *substrings
                                       = new_named_string_element_list ();
                      ELEMENT *name_copy = copy_tree (name);
                      ELEMENT *class_copy = copy_tree (class);
                      ELEMENT *ref_name_copy = copy_tree (name);
                      ELEMENT *ref_class_copy = copy_tree (class);

                      add_element_to_named_string_element_list (substrings,
                                                           "name", name_copy);
                      add_element_to_named_string_element_list (substrings,
                                                           "class", class_copy);
                      if (def_command == CM_defop
                          || def_command == CM_deftypeop
                          || def_command == CM_defmethod
                          || def_command == CM_deftypemethod)
                        { /* note that at that point, options are unlikely
                          to be set, but we use the language of the element */
                          index_entry = gdt_tree ("{name} on {class}",
                                                  document, options,
                                                  lang, substrings, 0);

                          text_append (&text_element->text, " on ");
                        }
                      else if (def_command == CM_defcv
                               || def_command == CM_defivar
                               || def_command == CM_deftypeivar
                               || def_command == CM_deftypecv)
                        {
                          index_entry = gdt_tree ("{name} of {class}",
                                                  document, options, lang,
                                                  substrings, 0);

                          text_append (&text_element->text, " of ");
                        }
                      destroy_named_string_element_list (substrings);

                      add_to_element_contents
                                   (index_entry_normalized, ref_name_copy);
                      add_to_element_contents
                                   (index_entry_normalized, text_element);
                      add_to_element_contents
                                   (index_entry_normalized, ref_class_copy);
                      /*
         prefer a type-less container rather than 'root_line' returned by gdt
                       */
                      index_entry->type = ET_NONE;

                      add_extra_element_oot (main_entry_element,
                                             "def_index_element",
                                             index_entry);
                      add_extra_element_oot (main_entry_element,
                                             "def_index_ref_element",
                                             index_entry_normalized);
                    }
                }
            }
        }
    }
}
