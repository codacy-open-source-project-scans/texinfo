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

#ifdef ENABLE_NLS
#include <locale.h>
#include <gettext.h>
#include <libintl.h>
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "tree_types.h"
#include "document_types.h"
/* for debug_output */
#include "debug_parser.h"
#include "errors.h"
#include "errors_parser.h"


/* Current filename and line number.  Used for reporting. */
SOURCE_INFO current_source_info;

ERROR_MESSAGE_LIST error_messages_list;

static void
line_error_internal (enum error_type type, int continuation,
                     const SOURCE_INFO *cmd_source_info,
                     const char *format, va_list v)
{
  vmessage_list_line_error (&error_messages_list,
                      type, continuation, debug_output, cmd_source_info,
                      format, v);
}

void
line_error_ext (enum error_type type, int continuation,
                SOURCE_INFO *cmd_source_info,
                const char *format, ...)
{
  va_list v;

  va_start (v, format);
  line_error_internal (type, continuation, cmd_source_info, format, v);
}

void
line_error (const char *format, ...)
{
  va_list v;

  va_start (v, format);
  line_error_internal (MSG_error, 0, &current_source_info, format, v);
}

void
line_warn (const char *format, ...)
{
  va_list v;

  va_start (v, format);
  line_error_internal (MSG_warning, 0, &current_source_info, format, v);
}

void
command_warn (const ELEMENT *e, const char *format, ...)
{
  va_list v;

  va_start (v, format);
  line_error_internal (MSG_warning, 0, &e->source_info, format, v);
}

void
command_error (const ELEMENT *e, const char *format, ...)
{
  va_list v;

  va_start (v, format);
  line_error_internal (MSG_error, 0, &e->source_info, format, v);
}

/* not used */
void
wipe_errors (void)
{
  wipe_error_message_list (&error_messages_list);
}

void
forget_errors (void)
{
  memset (&error_messages_list, 0, sizeof (ERROR_MESSAGE_LIST));
}

static void
bug_message_internal (char *format, va_list v)
{
  fprintf (stderr, "You found a bug: ");
  vfprintf (stderr, format, v);
  fprintf (stderr, "\n");
  if (current_source_info.file_name)
    {
      fprintf (stderr,
               "last location %s:%d", current_source_info.file_name,
                                         current_source_info.line_nr);
      if (current_source_info.macro)
        fprintf (stderr, " (possibly involving @%s)", current_source_info.macro);
      fprintf (stderr, "\n");
    }
  exit (1);
}

void
bug_message (char *format, ...)
{
  va_list v;

  va_start (v, format);
  bug_message_internal (format, v);
}
