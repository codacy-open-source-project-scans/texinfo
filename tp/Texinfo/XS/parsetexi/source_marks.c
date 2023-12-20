/* Copyright 2023 Free Software Foundation, Inc.

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
#include <stdlib.h>

#include "tree_types.h"
#include "tree.h"
#include "debug_parser.h"
/* for count_multibyte and other */
#include "utils.h"
/* for add_source_mark */
#include "manipulate_tree.h"
#include "source_marks.h"

#define sm_type(name) \
static int name##_counter = 0;
 SM_TYPES_LIST
#undef sm_type

/* it isn't much readable to use here the SM_TYPES_LIST macro defined
   in a header file, but the table should be allocated in files using
   it only if static (or in only one file if extern) */
static char *source_marks_names[SM_type_expanded_conditional_command + 1] =
{
  #define sm_type(name) #name,
    SM_TYPES_LIST
  #undef sm_type
};

SOURCE_MARK *
new_source_mark (enum source_mark_type type)
{
  SOURCE_MARK *source_mark = malloc (sizeof (SOURCE_MARK));

  /* Zero all elements */
  memset (source_mark, 0, sizeof (*source_mark));

  source_mark->type = type;
  source_mark->counter = -1;
  source_mark->status = SM_status_none;
  return source_mark;
}

/* ELEMENT should be the parent container. */
void
place_source_mark (ELEMENT *e, SOURCE_MARK *source_mark)
{
  ELEMENT *mark_element;
  /* for debug string */
  char *add_element_string = "no-add";

  source_mark->position = 0;
  if (e->contents.number > 0)
    {
      ELEMENT *last_child = last_contents_child (e);
      mark_element = last_child;
      if (last_child->text.end > 0)
        source_mark->position = count_multibyte (last_child->text.text);
    }
  else
    {
      /* add an empty element only used for source marks */
      mark_element = new_element (ET_NONE);
      /* set empty text to have merge_text work as expected */
      text_append (&mark_element->text, "");
      add_to_element_contents (e, mark_element);
      add_element_string = "add";
    }

  debug_nonl ("MARK %s c: %d p: %d %s %s ",
         source_marks_names[source_mark->type],
         source_mark->counter, source_mark->position,
         source_mark->status == SM_status_start ? "start"
          : source_mark->status == SM_status_end ? "end"
          : "UNDEF", add_element_string);
  debug_parser_print_element(mark_element, 0); debug_nonl (" ");
  debug_parser_print_element(e, 0); debug ("");

  add_source_mark (source_mark, mark_element);
}

/* ELEMENT should be the parent container. */
void
register_source_mark (ELEMENT *e, SOURCE_MARK *source_mark)
{
  if (source_mark->counter == -1)
    {
      switch (source_mark->type)
        {
#define sm_type(name) \
          case SM_type_##name: \
            {                     \
              name##_counter++;   \
              source_mark->counter = name##_counter; \
            } \
          break;

        SM_TYPES_LIST

#undef sm_type
        default:
          /* do nothing; just silence -Wswitch about SM_type_none */
          break;
        }
    }

  place_source_mark (e, source_mark);
}

void
transfer_source_marks (ELEMENT *from_e, ELEMENT *e)
{
  SOURCE_MARK_LIST *source_mark_list = &(from_e->source_mark_list);
  if (source_mark_list->number)
    {
      int i;
      for (i = 0; i < source_mark_list->number; i++)
        {
          add_source_mark (source_mark_list->list[i], e);
        }
      source_mark_list->number = 0;
    }
}


void
source_marks_reset_counters (void)
{
#define sm_type(name) name##_counter = 0;
 SM_TYPES_LIST
#undef sm_type
}

