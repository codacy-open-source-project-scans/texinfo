/* Copyright 2014-2024 Free Software Foundation, Inc.

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

#include "commands.h"
#include "tree_types.h"
#include "text.h"
#include "element_types.h"
#include "debug.h"
#include "conf.h"
#include "debug_parser.h"

/* debug functions used in parser, depending on parser_conf.debug */

void
debug (const char *s, ...)
{
  va_list v;

  if (!parser_conf.debug)
    return;
  va_start (v, s);
  vfprintf (stderr, s, v);
  fputc ('\n', stderr);
}

void
debug_nonl (const char *s, ...)
{
  va_list v;

  if (!parser_conf.debug)
    return;
  va_start (v, s);
  vfprintf (stderr, s, v);
}

void
debug_print_element (const ELEMENT *e, int print_parent)
{
  if (parser_conf.debug)
    {
      char *result;
      result = print_element_debug (e, print_parent);
      fputs (result, stderr);
      free (result);
    }
}

void
debug_print_protected_string (const char *input_string)
{
  if (parser_conf.debug)
    {
      char *result = debug_protect_eol (input_string);
      fputs (result, stderr);
      free (result);
    }
}

/* Here use command_name to get command names, using information on
   user-defined commands.  To be used in parser.

   There are corresponding functions in debug.c, which do not use
   user-defined commands information.
*/

const char *
debug_parser_command_name (enum command_id cmd)
{
  if (cmd == CM_TAB)
    return "\\t";
  else if (cmd == CM_NEWLINE)
    return "\\n";
  else
    return command_name (cmd);
}

char *
print_element_debug_parser (const ELEMENT *e, int print_parent)
{
  TEXT text;
  char *result;

  text_init (&text);
  text_append (&text, "");
  if (e->cmd)
    text_printf (&text, "@%s", debug_parser_command_name (e->cmd));
  if (e->type)
    text_printf (&text, "(%s)", element_type_names[e->type]);
  if (e->text.end > 0)
    {
      char *element_text = debug_protect_eol (e->text.text);
      text_printf (&text, "[T: %s]", element_text);
      free (element_text);
    }
  if (e->args.number)
    text_printf (&text, "[A%d]", e->args.number);
  if (e->contents.number)
    text_printf (&text, "[C%d]", e->contents.number);
  if (print_parent && e->parent)
    {
      text_append (&text, " <- ");
      if (e->parent->cmd)
        text_printf (&text, "@%s", command_name (e->parent->cmd));
      if (e->parent->type)
        text_printf (&text, "(%s)", element_type_names[e->parent->type]);
    }
  result = strdup (text.text);
  free (text.text);
  return result;
}

void
debug_parser_print_element (const ELEMENT *e, int print_parent)
{
  if (parser_conf.debug)
    {
      char *result;
      result = print_element_debug_parser (e, print_parent);
      fputs (result, stderr);
      free (result);
    }
}
