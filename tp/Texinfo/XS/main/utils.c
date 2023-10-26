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

/* code that does not fit anywhere else */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>
#include <iconv.h>
#include <errno.h>
#include <stdbool.h>
#include "uniconv.h"
#include "unistr.h"
#include "unicase.h"
#include "uniwidth.h"

#include "global_commands_types.h"
#include "options_types.h"
#include "tree_types.h"
#include "command_ids.h"
#include "text.h"
/* also for xvasprintf */
#include "tree.h"
#include "extra.h"
#include "errors.h"
#include "debug.h"
#include "builtin_commands.h"
#include "utils.h"

#include "cmd_structuring.c"
#include "options_init_free.c"

#define min_level command_structuring_level[CM_chapter]
#define max_level command_structuring_level[CM_subsubsection]

const char *whitespace_chars = " \t\v\f\r\n";
const char *digit_chars = "0123456789";

/* to keep synchronized with enum directions in tree_types.h */
const char *direction_names[] = {"next", "prev", "up"};
const char *direction_texts[] = {"Next", "Prev", "Up"};
const size_t directions_length = sizeof (direction_names) / sizeof (direction_names[0]);

/* to keep synchronized with enum output_unit_type in tree_types.h */
const char *output_unit_type_names[] = {"unit",
                                        "external_node_unit",
                                        "special_unit"};

char *html_global_unit_direction_names[] = {
  #define hgdt_name(name) #name,
   HTML_GLOBAL_DIRECTIONS_LIST
  #undef hgdt_name
};

char *html_conversion_context_type_names[] = {
  #define cctx_type(name) #name,
    HCC_CONTEXT_TYPES_LIST
  #undef cctx_type
};

char *html_formatting_reference_names[] = {
  #define html_fr_reference(name) #name,
   HTML_FORMATTING_REFERENCES_LIST
  #undef html_fr_reference
};

char *html_css_string_formatting_reference_names[] = {
  #define html_fr_reference(name) #name,
   HTML_CSS_FORMATTING_REFERENCES_LIST
  #undef html_fr_reference
};

ENCODING_CONVERSION_LIST output_conversions = {0, 0, 0, -1};
ENCODING_CONVERSION_LIST input_conversions = {0, 0, 0, 1};

/* to keep synchronized with enum command_location */
const char *command_location_names[]
  = {"before", "last", "preamble", "preamble_or_first"};

/* duplicated when creating a new expanded_formats */
struct expanded_format expanded_formats[] = {
    "html", 0,
    "docbook", 0,
    "plaintext", 0,
    "tex", 0,
    "xml", 0,
    "info", 0,
    "latex", 0,
};

/* wrapper for asprintf */
int
xasprintf (char **ptr, const char *template, ...)
{
  va_list v;
  va_start (v, template);
  return xvasprintf (ptr, template, v);
}

void bug (char *message)
{
  fprintf (stderr, "texi2any (XS parser): bug: %s\n", message);
}

void fatal (char *message)
{
  bug (message);
  abort ();
}

int
isascii_alnum (int c)
{
  return (((c & ~0x7f) == 0) && isalnum(c));
}

int
isascii_alpha (int c)
{
  return (((c & ~0x7f) == 0) && isalpha(c));
}

int
isascii_digit (int c)
{
  return (((c & ~0x7f) == 0) && isdigit(c));
}

int
isascii_lower (int c)
{
  return (((c & ~0x7f) == 0) && islower(c));
}

int
isascii_upper (int c)
{
  return (((c & ~0x7f) == 0) && isupper(c));
}

/* count characters, not bytes. */
size_t
count_multibyte (const char *text)
{
  /* FIXME error checking? */
  uint8_t *u8_text = u8_strconv_from_encoding (text, "UTF-8",
                                                 iconveh_question_mark);
  size_t result = u8_mbsnlen (u8_text, u8_strlen (u8_text));

  free (u8_text);

  return result;
}

char *
to_upper_or_lower_multibyte (const char *text, int lower_or_upper)
{
  char *result;
  size_t lengthp;
  uint8_t *u8_result;
  /* FIXME error checking? */
  uint8_t *u8_text = u8_strconv_from_encoding (text, "UTF-8",
                                               iconveh_question_mark);
  if (lower_or_upper > 0)
    /* the + 1 is there to hold the terminating NULL */
    u8_result = u8_toupper (u8_text, u8_strlen (u8_text) + 1,
                            NULL, NULL, NULL, &lengthp);
  else
    u8_result = u8_tolower (u8_text, u8_strlen (u8_text) + 1,
                            NULL, NULL, NULL, &lengthp);

  free (u8_text);
  result = u8_strconv_to_encoding (u8_result, "UTF-8",
                                   iconveh_question_mark);
  free (u8_result);
  return result;
}

int
width_multibyte (const char *text)
{
  int result;
  /* FIXME error checking? */
  uint8_t *u8_text = u8_strconv_from_encoding (text, "UTF-8",
                                                 iconveh_question_mark);
  /* FIXME the libunistring documentation described encoding as
     The encoding argument identifies the encoding (e.g. "ISO-8859-2"
     for Polish).  Looking at the code, it seems that it is only
     used to determine if it is a CJK encoding in a list of upper-case
     encodings.  We probably do not want to have this dependency to
     encoding, so use UTF-8. */
  result = u8_strwidth (u8_text, "UTF-8");
  free (u8_text);
  return result;
}

/* ENCODING should always be lower cased */
ENCODING_CONVERSION *
get_encoding_conversion (char *encoding,
                         ENCODING_CONVERSION_LIST *encodings_list)
{
  char *conversion_encoding = encoding;
  int encoding_index = -1;

  /* should correspond to
     Texinfo::Common::encoding_name_conversion_map.
     Thoughts on this mapping are available near
     Texinfo::Common::encoding_name_conversion_map definition
  */
  if (!strcmp (encoding, "us-ascii"))
    conversion_encoding = "iso-8859-1";

  if (!strcmp (encoding, "utf-8"))
    {
      if (encodings_list->number > 0)
        encoding_index = 0;
      else
        encoding_index = -2;
    }
  else if (encodings_list->number > 1)
    {
      int i;
      for (i = 1; i < encodings_list->number; i++)
        {
          if (!strcmp (conversion_encoding,
                       encodings_list->list[i].encoding_name))
            {
              encoding_index = i;
              break;
            }
        }
    }

  if (encoding_index < 0)
    {
      if (encodings_list->number == 0)
        encodings_list->number++;
      if (encoding_index == -2) /* utf-8 */
        encoding_index = 0;
      else
        {
          encoding_index = encodings_list->number;
          encodings_list->number++;
        }

      if (encodings_list->number - 1 >= encodings_list->space)
        {
          encodings_list->list = realloc (encodings_list->list,
              (encodings_list->space += 3) * sizeof (ENCODING_CONVERSION));
        }

      encodings_list->list[encoding_index].encoding_name
           = strdup (conversion_encoding);
      /* Initialize conversions for the first time.  iconv_open returns
         (iconv_t) -1 on failure so these should only be called once. */
      if (encodings_list->direction > 0)
        encodings_list->list[encoding_index].iconv
           = iconv_open ("UTF-8", conversion_encoding);
      else
        encodings_list->list[encoding_index].iconv
           = iconv_open (conversion_encoding, "UTF-8");
    }

  if (encodings_list->list[encoding_index].iconv == (iconv_t) -1)
    return 0;
  else
    /* NOTE this will change when the list is reallocated.
       This seems ok for the uses in convert_utils.c.
       Should be ok too for parser, as there is one current encoding
       at a time, the new current encoding is obtained after realloc.
     */
    return &encodings_list->list[encoding_index];
}

void
reset_encoding_list (ENCODING_CONVERSION_LIST *encodings_list)
{
  int i;
  /* never reset the utf-8 encoding in position 0 */
  if (encodings_list->number > 1)
    {
      for (i = 1; i < encodings_list->number; i++)
        {
          free (encodings_list->list[i].encoding_name);
          if (encodings_list->list[i].iconv != (iconv_t) -1)
            iconv_close (encodings_list->list[i].iconv);
        }
      encodings_list->number = 1;
    }
}

/* Run iconv using text buffer as output buffer. */
size_t
text_buffer_iconv (TEXT *buf, iconv_t iconv_state,
                   ICONV_CONST char **inbuf, size_t *inbytesleft)
{
  size_t out_bytes_left;
  char *outptr;
  size_t iconv_ret;

  outptr = buf->text + buf->end;
  if (buf->end == buf->space - 1)
    {
      errno = E2BIG;
      return (size_t) -1;
    }
  out_bytes_left = buf->space - buf->end - 1;
  iconv_ret = iconv (iconv_state, inbuf, inbytesleft,
                     &outptr, &out_bytes_left);

  buf->end = outptr - buf->text;

  return iconv_ret;
}

char *
encode_with_iconv (iconv_t our_iconv, char *s, SOURCE_INFO *source_info)
{
  static TEXT t;
  ICONV_CONST char *inptr; size_t bytes_left;
  size_t iconv_ret;

  t.end = 0; /* reset internal TEXT buffer */
  inptr = s;
  bytes_left = strlen (s);
  text_alloc (&t, 10);

  while (1)
    {
      iconv_ret = text_buffer_iconv (&t, our_iconv,
                                     &inptr, &bytes_left);

      /* Make sure libiconv flushes out the last converted character.
         This is required when the conversion is stateful, in which
         case libiconv might not output the last character, waiting to
         see whether it should be combined with the next one.  */
      if (iconv_ret != (size_t) -1
          && text_buffer_iconv (&t, our_iconv, 0, 0) != (size_t) -1)
        /* Success: all of input converted. */
        break;

      if (bytes_left == 0)
        break;

      switch (errno)
        {
        case E2BIG:
          text_alloc (&t, t.space + 20);
          break;
        case EILSEQ:
        default:
          if (source_info)
            fprintf(stderr, "%s:%d: encoding error at byte 0x%2x\n",
              source_info->file_name, source_info->line_nr,
                                             *(unsigned char *)inptr);
          else
            fprintf(stderr, "encoding error at byte 0x%2x\n",
                    *(unsigned char *)inptr);
          inptr++; bytes_left--;
          break;
        }
    }

  t.text[t.end] = '\0';
  return strdup (t.text);
}

char *
decode_string (char *input_string, char *encoding, int *status,
               SOURCE_INFO *source_info)
{
  char *result;
  *status = 0;
  /* not sure this can happen */
  if (!encoding)
    return strdup(input_string);

  ENCODING_CONVERSION *conversion
    = get_encoding_conversion (encoding, &input_conversions);

  if (!conversion)
    return strdup(input_string);

  *status = 1;

  result = encode_with_iconv (conversion->iconv, input_string, source_info);
  return result;
}

char *
encode_string (char *input_string, char *encoding, int *status,
               SOURCE_INFO *source_info)
{
  char *result;
  *status = 0;
  /* could happen in specific cases, such as, for file names,
     DOC_ENCODING_FOR_INPUT_FILE_NAME set to 0 and no locales encoding
     information */
  if (!encoding)
    return strdup(input_string);

  ENCODING_CONVERSION *conversion
    = get_encoding_conversion (encoding, &output_conversions);

  if (!conversion)
    return strdup(input_string);

  *status = 1;

  result = encode_with_iconv (conversion->iconv, input_string, source_info);
  return result;
}

void
clear_expanded_formats (struct expanded_format *formats)
{
  int i;
  for (i = 0; i < sizeof (expanded_formats)/sizeof (*expanded_formats);
       i++)
    {
      formats[i].expandedp = 0;
    }
}

void
add_expanded_format (struct expanded_format *formats, char *format)
{
  int i;
  for (i = 0; i < sizeof (expanded_formats)/sizeof (*expanded_formats);
       i++)
    {
      if (!strcmp (format, formats[i].format))
        {
          formats[i].expandedp = 1;
          break;
        }
    }
  if (!strcmp (format, "plaintext"))
    add_expanded_format (formats, "info");
}

/* FORMAT is an optional argument, to set a format at the expanded_format
   structure creation */
struct expanded_format *
new_expanded_formats (char *format)
{
  struct expanded_format *formats;

  formats = malloc (sizeof (expanded_formats));
  memcpy (formats, expanded_formats, sizeof (expanded_formats));

  if (format)
    add_expanded_format (formats, format);

  return formats;
}

int
format_expanded_p (struct expanded_format *formats, char *format)
{
  int i;
  for (i = 0; i < sizeof (expanded_formats)/sizeof (*expanded_formats);
       i++)
    {
      if (!strcmp (format, formats[i].format))
        return formats[i].expandedp;
    }
  return 0;
}

/* Return the parent if in an item_line command, @*table */
ELEMENT *
item_line_parent (ELEMENT *current)
{
  if (current->type == ET_before_item && current->parent)
    current = current->parent;

  /* FIXME could it be possible that current is a user defined command? */
  /* if (command_data(current->cmd).data == BLOCK_item_line) */
  if (builtin_command_data[current->cmd].data == BLOCK_item_line)
    return current;

  return 0;
}

ELEMENT *
get_label_element (ELEMENT *e)
{
  if ((e->cmd == CM_node || e->cmd == CM_anchor)
      && e->args.number > 0)
    return e->args.list[0];
  else if (e->cmd == CM_float && e->args.number >= 2)
    return e->args.list[1];
  return 0;
}

/* NAME is the name of an index, e.g. "cp" */
INDEX *
indices_info_index_by_name (INDEX **indices_information, char *name)
{
  INDEX **i, *idx;

  for (i = indices_information; (idx = *i); i++)
    if (!strcmp (idx->name, name))
      return idx;
  return 0;
}

INDEX *
ultimate_index (INDEX *index)
{
  while (index->merged_in)
    index = index->merged_in;
  return index;
}

/* Read a name used for @set, @value and translations arguments. */
char *
read_flag_name (char **ptr)
{
  char *p = *ptr, *q;
  char *ret = 0;

  q = p;
  if (!isascii_alnum (*q) && *q != '-' && *q != '_')
    return 0; /* Invalid. */

  while (!strchr (whitespace_chars, *q)
         && !strchr ("{\\}~`^+\"<>|@", *q))
    q++;
  ret = strndup (p, q - p);
  p = q;

  *ptr = p;
  return ret;
}

/* s/\s+/ /g with re => '/a' in perl */
char *
collapse_spaces (char *text)
{
  TEXT result;
  char *p = text;

  if (!text)
    return 0;

  text_init (&result);
  text_append (&result, "");

  while (*p)
    {
      int n = strcspn (p, whitespace_chars);
      if (n)
        {
          text_append_n (&result, p, n);
          p += n;
        }
      if (*p)
        {
          int n = strspn (p, whitespace_chars);
          if (n)
            {
              text_append (&result, " ");
              p += n;
            }
        }
    }
  return result.text;
}

/* Parse a #line directive.
   The filename of the line directive is returned.
   The line number value is in OUT_LINE_NO.
   RETVAL value is 1 for valid line directive, 0 otherwise.
*/
char *
parse_line_directive (char *line, int *retval, int *out_line_no)
{
  char *p = line, *q;
  char *filename = 0;
  int line_no = 0;

  *out_line_no = 0;
  *retval = 0;

  p += strspn (p, " \t");
  if (*p != '#')
    return 0;
  p++;

  q = p + strspn (p, " \t");
  if (!memcmp (q, "line", strlen ("line")))
    p = q + strlen ("line");

  if (!strchr (" \t", *p))
    return 0;
  p += strspn (p, " \t");

  /* p should now be at the line number */
  if (!strchr (digit_chars, *p))
    return 0;
  line_no = strtoul (p, &p, 10);

  p += strspn (p, " \t");
  if (*p == '"')
    {
      char saved;
      p++;
      q = strchr (p, '"');
      if (!q)
        return 0;
      saved = *q;
      *q = 0;
      filename = strdup (p);
      *q = saved;
      p = q + 1;
      p += strspn (p, " \t");

      p += strspn (p, digit_chars);
      p += strspn (p, " \t");
    }
  if (*p && *p != '\n')
    {
      free (filename);
      return 0; /* trailing text on line */
    }

  *retval = 1;
  *out_line_no = line_no;

  return filename;
}

/* return should be freed by the caller */
char *
normalize_encoding_name (char *text, int *possible_encoding)
{
  char *p;
  char *normalized_text = strdup (text);
  char *q = normalized_text;
  *possible_encoding = 0;
    /* lower case, trim non-ascii characters and keep only alphanumeric
       characters, - and _.  iconv also seems to trim non alphanumeric
       non - _ characters */
  for (p = text; *p; p++)
    {
      /* check if ascii and alphanumeric */
      if (isascii_alnum(*p))
        {
          *possible_encoding = 1;
          *q = tolower (*p);
          q++;
        }
      else if (*p == '_' || *p == '-')
        {
          *q = *p;
          q++;
        }
    }
  *q = '\0';
  return normalized_text;
}

void
wipe_index (INDEX *idx)
{
  free (idx->name);
  free (idx->index_entries);
}

void
wipe_index_names (INDEX **index_names)
{
  INDEX **i, *idx;
  if (index_names)
    {
      for (i = index_names; (idx = *i); i++)
        {
          wipe_index (idx);
          free (idx);
        }
    }
  free (index_names);
}

/* options */

OPTIONS *
new_options (void)
{
  OPTIONS *options = (OPTIONS *) malloc (sizeof (OPTIONS));
  initialize_options (options);
  return options;
}


/* include directories and include file */

void
add_include_directory (char *input_filename, STRING_LIST *include_dirs_list)
{
  int len;
  char *filename = strdup (input_filename);
  len = strlen (filename);
  if (len > 0 && filename[len - 1] == '/')
    filename[len - 1] = '\0';
  add_string (filename, include_dirs_list);
  free (filename);
}

void
add_string (char *string, STRING_LIST *strings_list)
{
  if (strings_list->number == strings_list->space)
    {
      strings_list->list = realloc (strings_list->list,
                   sizeof (char *) * (strings_list->space += 5));
    }
  string = strdup (string);
  strings_list->list[strings_list->number++] = string;
}

void
merge_strings (STRING_LIST *strings_list, STRING_LIST *merged_strings)
{
  int i;
  if (strings_list->number + merged_strings->number > strings_list->space)
    {
      strings_list->space = strings_list->number + merged_strings->number +5;
      strings_list->list = realloc (strings_list->list,
                                  sizeof (char *) * strings_list->space);
    }
  for (i = 0; i < merged_strings->number; i++)
    {
      strings_list->list[strings_list->number +i] = merged_strings->list[i];
    }
  strings_list->number += merged_strings->number;
}

/* Return value to be freed by caller. */
/* try to locate a file called FILENAME, looking for it in the list of include
   directories. */
char *
locate_include_file (char *filename, STRING_LIST *include_dirs_list)
{
  char *fullpath;
  struct stat dummy;
  int i, status;

  /* Checks if filename is absolute or relative to current directory. */
  /* Note: the Perl code (in Common.pm, 'locate_include_file') handles
     a volume in a path (like "A:") using the File::Spec module. */
  if (!memcmp (filename, "/", 1)
      || (strlen (filename) >= 3 && !memcmp (filename, "../", 3))
      || (strlen (filename) >= 2 && !memcmp (filename, "./", 2)))
    {
      status = stat (filename, &dummy);
      if (status == 0)
        return strdup (filename);
    }
  else if (!include_dirs_list)
    {
      return 0;
    }
  else
    {
      for (i = 0; i < include_dirs_list->number; i++)
        {
          xasprintf (&fullpath, "%s/%s", include_dirs_list->list[i], filename);
          status = stat (fullpath, &dummy);
          if (status == 0)
            return fullpath;
          free (fullpath);
        }
    }
  return 0;
}

void
clear_strings_list (STRING_LIST *strings)
{
  int i;
  for (i = 0; i < strings->number; i++)
    {
      free (strings->list[i]);
    }
  strings->number = 0;
}

/* very similar to parsetexi/input.c free_small_strings */
void
free_strings_list (STRING_LIST *strings)
{
  size_t i;
  for (i = 0; i < strings->number; i++)
    {
      free (strings->list[i]);
    }
  free (strings->list);
}

void
destroy_strings_list (STRING_LIST *strings)
{
  free_strings_list (strings);
  free (strings);
}

void
delete_global_info (GLOBAL_INFO *global_info_ref)
{
  GLOBAL_INFO global_info = *global_info_ref;

  free (global_info.dircategory_direntry.contents.list);

  free (global_info.input_encoding_name);
  free (global_info.input_file_name);
  free (global_info.input_directory);
}

void
delete_global_commands (GLOBAL_COMMANDS *global_commands_ref)
{
  GLOBAL_COMMANDS global_commands = *global_commands_ref;

#define GLOBAL_CASE(cmx) \
  free (global_commands.cmx.contents.list)

  GLOBAL_CASE(floats);
  GLOBAL_CASE(footnotes);

#include "global_multi_commands_case.c"

#undef GLOBAL_CASE
}

ELEMENT *
get_cmd_global_command (GLOBAL_COMMANDS *global_commands_ref,
                        enum command_id cmd)
{

  switch (cmd)
    {
#define GLOBAL_CASE(cmx) \
     case CM_##cmx:   \
       return &global_commands_ref->cmx;

     case CM_footnote:
       return &global_commands_ref->footnotes;

     case CM_float:
       return &global_commands_ref->floats;

#include "global_multi_commands_case.c"

#undef GLOBAL_CASE

#define GLOBAL_UNIQUE_CASE(cmd) \
     case CM_##cmd: \
       return global_commands_ref->cmd;

#include "global_unique_commands_case.c"

#undef GLOBAL_UNIQUE_CASE
     default:
       return 0;
   }
}

static char *
informative_command_value (ELEMENT *element)
{
  ELEMENT *misc_args;
  char *text_arg;

  enum command_id cmd = element_builtin_data_cmd (element);
  if (builtin_command_data[cmd].flags & CF_line
      && builtin_command_data[cmd].data == LINE_lineraw)
    {
      if (builtin_command_data[cmd].args_number <= 0)
        return "1";
      /* FIXME is it possible to have args.number > 1? */
      else if (element->args.number > 0)
        {
          TEXT text;
          int i;
          int text_seen = 0;
          text_init (&text);
          for (i = 0; i < element->args.number; i++)
            {
              ELEMENT *arg = element->args.list[i];
              if (arg->text.end)
                {
                  if (!text_seen)
                    text_seen = 1;
                  else
                    text_append (&text, " ");
                  text_append (&text, arg->text.text);
                }
            }
          /* FIXME to be freed */
          return text.text;
        }
    }
  text_arg = lookup_extra_string (element, "text_arg");
  if (text_arg)
    return text_arg;
  misc_args = lookup_extra_element (element, "misc_args");
  if (misc_args && misc_args->contents.number > 0)
    return misc_args->contents.list[0]->text.text;
  if (builtin_command_data[cmd].flags & CF_line
      && builtin_command_data[cmd].data == LINE_line
      && element->args.number >= 1
      && element->args.list[0]->contents.number >= 1
      && element->args.list[0]->contents.list[0]->text.end > 0)
    return element->args.list[0]->contents.list[0]->text.text;

  return 0;
}

void
set_informative_command_value (CONVERTER *self, ELEMENT *element)
{
  char *value = 0;
  enum command_id cmd = element_builtin_cmd (element);
  if (cmd == CM_summarycontents)
    cmd = CM_shortcontents;

  value = informative_command_value (element);

  if (value)
    {
      COMMAND_OPTION_REF *option_ref = get_command_option (self->conf, cmd);
      if (option_ref)
        {
          if (option_ref->type == GO_int)
            *(option_ref->int_ref) = strtoul (value, NULL, 10);
          else
            {
              free (*(option_ref->char_ref));
              *(option_ref->char_ref) = strdup (value);
            }
          free (option_ref);
        }
    }
}

static int
in_preamble (ELEMENT *element)
{
  ELEMENT *current_element = element;
  while (current_element->parent)
    {
      if (current_element->parent->type == ET_preamble_before_content)
        return 1;
      current_element = current_element->parent;
    }
  return 0;
}

CONVERTER *
new_converter (void)
{
  CONVERTER *converter
   = (CONVERTER *) malloc (sizeof (CONVERTER));
  memset (converter, 0, sizeof (CONVERTER));
  return converter;
}

/*
  COMMAND_LOCATION is 'last', 'preamble' or 'preamble_or_first'
  'preamble' means setting sequentially to the values in the preamble.
  'preamble_or_first'  means setting to the first value for the command
  in the document if the first command is not in the preamble, else set
  sequentially to the values in the preamble.
  'last' means setting to the last value for the command in the document.

  For unique command, the last may be considered to be the same as the first.

  Notice that the only effect is to use set_conf (directly or through
  set_informative_command_value), no @-commands setting side effects are done
  and associated customization variables are not set/reset either.
 */
ELEMENT *
set_global_document_command (CONVERTER *self, enum command_id cmd,
                             enum command_location command_location)
{
  ELEMENT *element = 0;
  if (command_location != CL_last && command_location != CL_preamble_or_first
      && command_location != CL_preamble)
    fprintf (stderr, "BUG: set_global_document_command: unknown CL: %d\n",
                     command_location);

  ELEMENT *command
     = get_cmd_global_command (self->document->global_commands, cmd);
  if (builtin_command_data[cmd].flags & CF_global)
    {
      if (command->contents.number)
        {
          if (command_location == CL_last)
            {
              element = command->contents.list[command->contents.number -1];
              set_informative_command_value (self, element);
            }
          else
            {
              if (command_location == CL_preamble_or_first
                   && !in_preamble (command->contents.list[0]))
                {
                  element = command->contents.list[0];
                  set_informative_command_value (self, element);
                }
              else
                {
                  int i;
                  for (i = 0; i < command->contents.number; i++)
                    {
                      ELEMENT *command_element = command->contents.list[i];
                      if (in_preamble (command_element))
                        {
                          element = command_element;
                          set_informative_command_value (self, element);
                        }
                      else
                        break;
                    }
                }
            }
        }
    }
  else if (command)
    {
      element = command;
      set_informative_command_value (self, element);
    }
  return element;
}

/* in Common.pm */
/* the returned level will be < 0 if the command is not supposed
   to be associated to a level. */
int
section_level (ELEMENT *section)
{
  int level = command_structuring_level[section->cmd];
  KEY_PAIR *k_level_modifier = lookup_extra (section, "level_modifier");
  if (k_level_modifier && level >= 0)
    {
      int section_modifier = (intptr_t) k_level_modifier->value;
      level -= section_modifier;
      if (level < min_level)
        if (command_structuring_level[section->cmd] < min_level)
          level = command_structuring_level[section->cmd];
        else
          level = min_level;
      else if (level > max_level)
        level = max_level;
    }
  return level;
}

/* from Common.pm */
int
is_content_empty (ELEMENT *tree, int do_not_ignore_index_entries)
{
  int i;
  if (!tree || !tree->contents.number)
    return 1;

  for (i = 0; i < tree->contents.number; i++)
    {
      ELEMENT *content = tree->contents.list[i];
      if (content->cmd)
        {
          if (content->type == ET_index_entry_command)
            {
              if (do_not_ignore_index_entries)
                return 0;
              else
               continue;
            }
          if (builtin_command_flags(content) & CF_line)
            {
              if (command_other_flags(content) & CF_formatted_line
                  || command_other_flags(content) & CF_formattable_line)
                return 0;
              else
                continue;
            }
          else if (builtin_command_flags(content) & CF_nobrace)
            {
              if (command_other_flags(content) & CF_formatted_nobrace)
                return 0;
              else
                continue;
            }
          else if (command_other_flags(content) & CF_non_formatted_brace
                   || command_other_flags(content) & CF_non_formatted_block)
            continue;
          else
            return 0;
        }
      if (content->type == ET_paragraph)
        return 0;
      if (content->text.end > 0)
        {
          char *text = element_text (content);
          /* only whitespace characters */
          if (! text[strspn (text, whitespace_chars)] == '\0')
            return 0;
        }
      if (! is_content_empty (content, do_not_ignore_index_entries))
        return 0;
    }
  return 1;
}

/*
  NOTE this code does nothing with the current tree, as comments
  are in 'info' 'comment_at_end', ignorable_spaces_after_command only appear
  in specific situations, spaces_after_close_brace are only after some
  commands and not first in contents, while spaces_at_end are before commands
  and not at the end of contents. Spaces will also be in 'info'
  if at the end of contents/line.
  Also this code is not often called, likely because it is not useful with
  the current tree.
  In 2023, nothing was trimmed in the test suite.
 */
ELEMENT *
trim_spaces_comment_from_content (ELEMENT *element)
{
  if (element->contents.number <= 0)
    return 0;

  if (element->contents.number > 0)
    {
      int i, j;

      /* index to start from, from the beginning */
      for (i = 0; i < element->contents.number; i++)
        {
          ELEMENT *content = element->contents.list[i];
          if (!content->type
              || (content->type != ET_ignorable_spaces_after_command
                  && content->type != ET_spaces_after_close_brace))
            break;
        }

      for (j = element->contents.number -1; j >= 0; j--)
        {
          ELEMENT *content = element->contents.list[j];
          if ((content->cmd == CM_c || content->cmd == CM_comment)
              || content->type == ET_spaces_at_end)
            {}
          else
            break;
        }

      if (j < i)
        return 0;
      else
        {
          ELEMENT *result = new_element (ET_NONE);
          insert_slice_into_contents (result, 0, element, i, j);
          return result;
        }
    }
  return 0;
}

/*
  decompose a decimal number on a given base.  It is not the
  decomposition used for counting as we start at 0, not 1 for all
  the factors.  This is in order to get aa and not ba in calling
  code.

  DECOMPOSED_NR points to the number of factors value.  There is
  also a -1 after the last factor in the return array.
 */
static int *
decompose_integer (int number, int base, int *decomposed_nr)
{
  int i;
  /* in practice we are with letters in base 26, 10 is
     more than enough */
  int space = 11;
  int *result = malloc (space * sizeof(int));
  *decomposed_nr = space - 1;
  for (i = 0; i < space; i++)
    {
      if (number >= 0)
        {
          int factor = number % base;
          result[i] = factor;
          number = (number - factor) / base - 1;
        }
      else
        {
          *decomposed_nr = i;
          break;
        }
    }
  result[*decomposed_nr] = -1;

  return result;
}

char *
enumerate_item_representation (char *specification, int number)
{
  TEXT result;
  int i;
  int decomposed_nr;

  if (!strlen (specification))
    return strdup ("");

  text_init (&result);

  if (specification[strspn (specification, digit_chars)] == '\0')
    {
      int spec = strtol(specification, NULL, 10) + number -1;
      text_printf (&result, "%d", spec);
      return result.text;
    }

  char base_letter = 'a';
  if (isascii_alpha (specification[0]) && isascii_upper (specification[0]))
    base_letter = 'A';

  int *letter_ords
    = decompose_integer (specification[0] - base_letter + number - 1, 26,
                         &decomposed_nr);

  for (i = decomposed_nr - 1; i >= 0; i--)
    text_printf (&result, "%c", base_letter + letter_ords[i]);

  free (letter_ords);
  return result.text;
}
