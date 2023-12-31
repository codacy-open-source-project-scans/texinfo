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

/* Avoid namespace conflicts. */
#define context perl_context

#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
/* Avoid warnings about Perl headers redefining symbols that gnulib
   redefined already. */
#if defined _WIN32 && !defined __CYGWIN__
  #undef free
#endif
#include "XSUB.h"

#undef context

#include "text.h"
#include "converter_types.h"
#include "utils.h"
/* for add_associated_info_integer */
#include "extra.h"
/* for newSVpv_utf8 build_texinfo_tree */
#include "build_perl_info.h"
#include "debug.h"
#include "build_html_perl_state.h"
#include "call_html_perl_function.h"

TARGET_FILENAME *
call_file_id_setting_special_unit_target_file_name (CONVERTER *self,
                         const OUTPUT_UNIT *special_unit, const char *target,
                                                const char *default_filename)
{
  SV **file_id_setting_sv;

  dTHX;

  if (!special_unit->hv)
    return 0;

  if (!self->hv)
    return 0;

  file_id_setting_sv = hv_fetch (self->hv, "file_id_setting",
                                 strlen ("file_id_setting"), 0);
  if (file_id_setting_sv)
    {
      SV **special_unit_target_file_name_sv;
      HV *file_id_setting_hv = (HV *)SvRV(*file_id_setting_sv);
      special_unit_target_file_name_sv = hv_fetch (file_id_setting_hv,
                                   "special_unit_target_file_name",
                                   strlen ("special_unit_target_file_name"), 0);

      if (special_unit_target_file_name_sv)
        {
          int count;
          char *target_ret;
          char *filename = 0;
          SV *target_ret_sv;
          SV *filename_sv;
          STRLEN len;
          TARGET_FILENAME *result
            = (TARGET_FILENAME *) malloc (sizeof (TARGET_FILENAME));
          result->filename = 0;

          dSP;

          ENTER;
          SAVETMPS;

          PUSHMARK(SP);
          EXTEND(SP, 4);

          PUSHs(sv_2mortal (newRV_inc (self->hv)));
          PUSHs(sv_2mortal (newRV_inc (special_unit->hv)));
          /* FIXME encoding */
          PUSHs(sv_2mortal (newSVpv (target, 0)));
          PUSHs(sv_2mortal (newSVpv (default_filename, 0)));
          PUTBACK;

          count = call_sv (*special_unit_target_file_name_sv, G_ARRAY);

          SPAGAIN;

          if (count != 2)
            croak("special_unit_target_file_name should return 2 items\n");

          filename_sv = POPs;
          if (SvOK (filename_sv))
            {
              STRLEN len;
              filename = SvPV (filename_sv, len);
              result->filename = strdup (filename);
            }

          target_ret_sv = POPs;
          target_ret = SvPV (target_ret_sv, len);
          result->target = strdup (target_ret);

          PUTBACK;

          FREETMPS;
          LEAVE;

          return result;
        }
    }
  return 0;
}

char *
call_file_id_setting_label_target_name (CONVERTER *self,
                const char *normalized, const ELEMENT *label_element,
                const char *target, int *called)
{
  SV **file_id_setting_sv;

  dTHX;

  *called = 0;

  if (!self->hv)
    return 0;

  file_id_setting_sv = hv_fetch (self->hv, "file_id_setting",
                                 strlen ("file_id_setting"), 0);
  if (file_id_setting_sv)
    {
      SV **label_target_name_sv;
      HV *file_id_setting_hv = (HV *)SvRV(*file_id_setting_sv);
      label_target_name_sv = hv_fetch (file_id_setting_hv,
                                   "label_target_name",
                                   strlen ("label_target_name"), 0);

      if (label_target_name_sv)
        {
          int count;
          char *target_ret;
          STRLEN len;
          char *result;
          SV *target_ret_sv;
          SV *label_element_sv;

          if (label_element)
            label_element_sv = newRV_inc (label_element->hv);
          else
            label_element_sv = newSV (0);

          *called = 1;

          dSP;

          ENTER;
          SAVETMPS;

          PUSHMARK(SP);
          EXTEND(SP, 4);

          PUSHs(sv_2mortal (newRV_inc (self->hv)));
          PUSHs(sv_2mortal (newSVpv (normalized, 0)));
          PUSHs(sv_2mortal (label_element_sv));
          PUSHs(sv_2mortal (newSVpv (target, 0)));
          PUTBACK;

          count = call_sv (*label_target_name_sv, G_ARRAY);

          SPAGAIN;

          if (count != 1)
            croak("label_target_name should return 1 item\n");

          target_ret_sv = POPs;
          target_ret = SvPV (target_ret_sv, len);
          result = strdup (target_ret);

          PUTBACK;

          FREETMPS;
          LEAVE;

          return result;
        }
    }
  return 0;
}


char *
call_file_id_setting_node_file_name (CONVERTER *self,
                   const ELEMENT *target_element, const char *node_filename,
                   int *called)
{
  SV **file_id_setting_sv;

  dTHX;

  *called = 0;

  if (!target_element->hv)
    return 0;

  if (!self->hv)
    return 0;

  file_id_setting_sv = hv_fetch (self->hv, "file_id_setting",
                                 strlen ("file_id_setting"), 0);
  if (file_id_setting_sv)
    {
      SV **node_file_name_sv;
      HV *file_id_setting_hv = (HV *)SvRV(*file_id_setting_sv);
      node_file_name_sv = hv_fetch (file_id_setting_hv,
                                   "node_file_name",
                                   strlen ("node_file_name"), 0);

      if (node_file_name_sv)
        {
          int count;
          char *result;
          SV *node_filename_ret_sv;
          *called = 1;

          dSP;

          ENTER;
          SAVETMPS;

          PUSHMARK(SP);
          EXTEND(SP, 3);

          PUSHs(sv_2mortal (newRV_inc (self->hv)));
          PUSHs(sv_2mortal (newRV_inc (target_element->hv)));
          PUSHs(sv_2mortal (newSVpv (node_filename, 0)));
          PUTBACK;

          count = call_sv (*node_file_name_sv, G_ARRAY);

          SPAGAIN;

          if (count != 1)
            croak("node_file_name should return 1 item\n");

          node_filename_ret_sv = POPs;
          /* user can return undef */
          if (SvOK (node_filename_ret_sv))
            {
              char *node_filename_ret;
              STRLEN len;
              node_filename_ret = SvPV (node_filename_ret_sv, len);
              result = strdup (node_filename_ret);
            }
          else
            result = 0;

          PUTBACK;

          FREETMPS;
          LEAVE;

          return result;
        }
    }
  return 0;
}

TARGET_CONTENTS_FILENAME *
call_file_id_setting_sectioning_command_target_name (CONVERTER *self,
                      const ELEMENT *command, const char *target,
                      const char *target_contents,
                      const char *target_shortcontents, const char *filename)
{
  SV **file_id_setting_sv;

  dTHX;

  if (!command->hv)
    return 0;

  if (!self->hv)
    return 0;

  file_id_setting_sv = hv_fetch (self->hv, "file_id_setting",
                                 strlen ("file_id_setting"), 0);
  if (file_id_setting_sv)
    {
      SV **sectioning_command_target_name_sv;
      HV *file_id_setting_hv = (HV *)SvRV(*file_id_setting_sv);
      sectioning_command_target_name_sv = hv_fetch (file_id_setting_hv,
                                 "sectioning_command_target_name",
                                 strlen ("sectioning_command_target_name"), 0);

      if (sectioning_command_target_name_sv)
        {
          int count;
          STRLEN len;
          SV *target_ret_sv;
          SV *target_contents_ret_sv;
          SV *target_shortcontents_ret_sv;
          SV *filename_ret_sv;
          char *target_ret;
          char *target_contents_ret;
          char *target_shortcontents_ret;
          char *filename_ret;
          TARGET_CONTENTS_FILENAME *result = (TARGET_CONTENTS_FILENAME *)
                                malloc (sizeof (TARGET_CONTENTS_FILENAME));

          dSP;

          ENTER;
          SAVETMPS;

          PUSHMARK(SP);
          EXTEND(SP, 6);

          PUSHs(sv_2mortal (newRV_inc (self->hv)));
          PUSHs(sv_2mortal (newRV_inc (command->hv)));
          PUSHs(sv_2mortal (newSVpv (target, 0)));
          PUSHs(sv_2mortal (newSVpv (target_contents, 0)));
          PUSHs(sv_2mortal (newSVpv (target_shortcontents, 0)));
          PUSHs(sv_2mortal (newSVpv (filename, 0)));
          PUTBACK;

          count = call_sv (*sectioning_command_target_name_sv, G_ARRAY);

          SPAGAIN;

          if (count != 4)
            croak("sectioning_command_target_name should return 4 items\n");

          filename_ret_sv = POPs;
          filename_ret = SvPV (filename_ret_sv, len);
          result->filename = strdup (filename_ret);
          target_shortcontents_ret_sv = POPs;
          target_shortcontents_ret = SvPV (target_shortcontents_ret_sv,
                                           len);
          result->target_shortcontents = strdup (target_shortcontents_ret);
          target_contents_ret_sv = POPs;
          target_contents_ret = SvPV (target_contents_ret_sv, len);
          result->target_contents = strdup (target_contents_ret);
          target_ret_sv = POPs;
          target_ret = SvPV (target_ret_sv, len);
          result->target = strdup (target_ret);

          PUTBACK;

          FREETMPS;
          LEAVE;

          return result;
        }
    }
  return 0;
}

FILE_NAME_PATH *
call_file_id_setting_unit_file_name (CONVERTER *self,
                                 const OUTPUT_UNIT *output_unit,
                                 const char *filename, const char *filepath)
{
  SV **file_id_setting_sv;

  dTHX;

  if (!output_unit->hv)
    return 0;

  if (!self->hv)
    return 0;

  file_id_setting_sv = hv_fetch (self->hv, "file_id_setting",
                                 strlen ("file_id_setting"), 0);
  if (file_id_setting_sv)
    {
      SV **unit_file_name_sv;
      HV *file_id_setting_hv = (HV *)SvRV(*file_id_setting_sv);
      unit_file_name_sv = hv_fetch (file_id_setting_hv,
                                   "unit_file_name",
                                   strlen ("unit_file_name"), 0);

      if (unit_file_name_sv)
        {
          int count;
          SV *filepath_ret_sv;
          SV *filename_ret_sv;
          FILE_NAME_PATH *result
            = (FILE_NAME_PATH *) malloc (sizeof (FILE_NAME_PATH));
          memset (result, 0, sizeof (FILE_NAME_PATH));

          dSP;

          ENTER;
          SAVETMPS;

          PUSHMARK(SP);
          EXTEND(SP, 4);

          PUSHs(sv_2mortal (newRV_inc (self->hv)));
          PUSHs(sv_2mortal (newRV_inc (output_unit->hv)));
          PUSHs(sv_2mortal (newSVpv (filename, 0)));
          PUSHs(sv_2mortal (newSVpv (filepath, 0)));
          PUTBACK;

          count = call_sv (*unit_file_name_sv, G_ARRAY);

          SPAGAIN;

          if (count != 2)
            croak("unit_file_name should return 2 items\n");

          filepath_ret_sv = POPs;
          if (SvOK (filepath_ret_sv))
            {
              STRLEN len;
              char *filepath_ret = SvPV (filepath_ret_sv, len);
              result->filepath = strdup (filepath_ret);
            }

          filename_ret_sv = POPs;
          if (SvOK (filename_ret_sv))
            {
              STRLEN len;
              char *filename_ret = SvPV (filename_ret_sv, len);
              result->filename = strdup (filename_ret);
            }

          PUTBACK;

          FREETMPS;
          LEAVE;

          return result;
        }
    }
  return 0;
}

TARGET_DIRECTORY_FILENAME *
call_file_id_setting_external_target_split_name (CONVERTER *self,
                     const char *normalized, const ELEMENT *element,
                     const char *target, const char *directory,
                     const char *file_name)
{
  SV **file_id_setting_sv;

  dTHX;

  if (!self->hv)
    return 0;

  file_id_setting_sv = hv_fetch (self->hv, "file_id_setting",
                                 strlen ("file_id_setting"), 0);
  if (file_id_setting_sv)
    {
      SV **external_target_split_name_sv;
      HV *file_id_setting_hv = (HV *)SvRV(*file_id_setting_sv);
      external_target_split_name_sv = hv_fetch (file_id_setting_hv,
                                   "external_target_split_name",
                                   strlen ("external_target_split_name"), 0);

      if (external_target_split_name_sv)
        {
          int count;
          SV *target_sv;
          SV *filename_sv;
          SV *directory_sv;
          TARGET_DIRECTORY_FILENAME *result = (TARGET_DIRECTORY_FILENAME *)
              malloc (sizeof (TARGET_DIRECTORY_FILENAME));
          memset (result, 0, sizeof (TARGET_DIRECTORY_FILENAME));

          dSP;

          ENTER;
          SAVETMPS;

          PUSHMARK(SP);
          EXTEND(SP, 6);

          PUSHs(sv_2mortal (newRV_inc (self->hv)));
          PUSHs(sv_2mortal (newSVpv (normalized, 0)));
          PUSHs(sv_2mortal (newRV_inc (element->hv)));
          /* FIXME encoding */
          PUSHs(sv_2mortal (newSVpv (target, 0)));
          PUSHs(sv_2mortal (newSVpv (directory, 0)));
          PUSHs(sv_2mortal (newSVpv (file_name, 0)));
          PUTBACK;

          count = call_sv (*external_target_split_name_sv, G_ARRAY);

          SPAGAIN;

          if (count != 3)
            croak("external_target_split_name should return 3 items\n");

          filename_sv = POPs;
          if (SvOK (filename_sv))
            {
              STRLEN len;
              char *filename_ret = SvPV (filename_sv, len);
              result->filename = strdup (filename_ret);
            }
          else
            result->filename = strdup ("");

          directory_sv = POPs;
          if (SvOK (directory_sv))
            {
              STRLEN len;
              char *directory_ret = SvPV (directory_sv, len);
              result->directory = strdup (directory_ret);
            }
          else
            result->directory = strdup ("");

          target_sv = POPs;
          if (SvOK (target_sv))
            {
              STRLEN len;
              char *target_ret = SvPV (target_sv, len);
              result->target = strdup (target_ret);
            }
          else
            result->target = strdup ("");

          PUTBACK;

          FREETMPS;
          LEAVE;

          return result;
        }
    }
  return 0;
}

TARGET_FILENAME *
call_file_id_setting_external_target_non_split_name (CONVERTER *self,
                     const char *normalized, const ELEMENT *element,
                     const char *target, const char *file)
{
  SV **file_id_setting_sv;

  dTHX;

  if (!self->hv)
    return 0;

  file_id_setting_sv = hv_fetch (self->hv, "file_id_setting",
                                 strlen ("file_id_setting"), 0);
  if (file_id_setting_sv)
    {
      SV **external_target_non_split_name_sv;
      HV *file_id_setting_hv = (HV *)SvRV(*file_id_setting_sv);
      external_target_non_split_name_sv = hv_fetch (file_id_setting_hv,
                                   "external_target_non_split_name",
                                strlen ("external_target_non_split_name"), 0);

      if (external_target_non_split_name_sv)
        {
          int count;
          SV *target_sv;
          SV *file_sv;
          TARGET_FILENAME *result
            = (TARGET_FILENAME *) malloc (sizeof (TARGET_FILENAME));
          result->filename = 0;

          dSP;

          ENTER;
          SAVETMPS;

          PUSHMARK(SP);
          EXTEND(SP, 5);

          PUSHs(sv_2mortal (newRV_inc (self->hv)));
          PUSHs(sv_2mortal (newSVpv (normalized, 0)));
          PUSHs(sv_2mortal (newRV_inc (element->hv)));
          /* FIXME encoding */
          PUSHs(sv_2mortal (newSVpv (target, 0)));
          PUSHs(sv_2mortal (newSVpv (file, 0)));
          PUTBACK;

          count = call_sv (*external_target_non_split_name_sv, G_ARRAY);

          SPAGAIN;

          if (count != 2)
            croak("external_target_non_split_name should return 2 items\n");

          file_sv = POPs;
          if (SvOK (file_sv))
            {
              STRLEN len;
              char *file_ret = SvPV (file_sv, len);
              result->filename = strdup (file_ret);
            }

          target_sv = POPs;
          if (SvOK (target_sv))
            {
              STRLEN len;
              char *target_ret = SvPV (target_sv, len);
              result->target = strdup (target_ret);
            }

          PUTBACK;

          FREETMPS;
          LEAVE;

          return result;
        }
    }
  return 0;
}




char *
call_formatting_function_format_comment (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                                              const char *text)
{
  int count;
  char *result;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 2);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv_utf8 (text, 0)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_comment should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_program_string (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference)
{
  int count;
  char *result;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 1);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_program_string should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_titlepage (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference)
{
  int count;
  char *result;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 1);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_titlepage should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_title_titlepage (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference)
{
  int count;
  char *result;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 1);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_title_titlepage should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_protect_text (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                                              const char *text)
{
  int count;
  char *result;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 2);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv_utf8 (text, 0)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_protect_text should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_footnotes_segment (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference)
{
  int count;
  char *result;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 1);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_footnotes_segment should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_footnotes_sequence (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference)
{
  int count;
  char *result;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 1);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_footnotes_sequence should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_css_lines (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                                              const char *filename)
{
  int count;
  char *result;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 2);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv_utf8 (filename, 0)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_css_lines should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_end_file (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                    const char *filename, const OUTPUT_UNIT *output_unit)
{
  int count;
  char *result;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;
  SV *output_unit_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  if (output_unit)
    output_unit_sv = newRV_inc (output_unit->hv);
  else
    output_unit_sv = newSV (0);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 3);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv_utf8 (filename, 0)));
  PUSHs(sv_2mortal (output_unit_sv));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_end_file should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_begin_file (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                                            const char *filename,
                                            const OUTPUT_UNIT *output_unit)
{
  int count;
  char *result;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;
  SV *output_unit_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  if (output_unit)
    output_unit_sv = newRV_inc (output_unit->hv);
  else
    output_unit_sv = newSV (0);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 3);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv_utf8 (filename, 0)));
  PUSHs(sv_2mortal (output_unit_sv));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_begin_file should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_translate_message (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                                  const char *message, const char *lang,
                                  const char *message_context)
{
  int count;
  char *result = 0;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 4);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv_utf8 (message, 0)));
  PUSHs(sv_2mortal (newSVpv (lang, 0)));
  PUSHs(sv_2mortal (newSVpv_utf8 (message_context, 0)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_translate_message should return 1 item\n");

  result_sv = POPs;
  if (SvOK (result_sv))
    {
      result_ret = SvPVutf8 (result_sv, len);
      result = strdup (result_ret);
    }

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_button_icon_img (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                         const char *button_name,
                         const char *icon, const char *name)

{
  int count;
  SV *name_sv;
  char *result = 0;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  build_tree_to_build (&self->tree_to_build);

  if (name)
    name_sv = newSVpv_utf8 (name, 0);
  else
    name_sv = newSV (0);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 4);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv_utf8 (button_name, 0)));
  /* FIXME could also be a byte string */
  PUSHs(sv_2mortal (newSVpv_utf8 (icon, 0)));
  PUSHs(sv_2mortal (name_sv));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_button_icon_img should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

FORMATTED_BUTTON_INFO *
call_formatting_function_format_button (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                                  const BUTTON_SPECIFICATION *button,
                                  const ELEMENT *element)
{
  int count;
  SV *need_delimiter_sv;
  SV *passive_sv;
  SV *active_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  build_tree_to_build (&self->tree_to_build);

  FORMATTED_BUTTON_INFO *result
   = (FORMATTED_BUTTON_INFO *) malloc (sizeof (FORMATTED_BUTTON_INFO));
  memset (result, 0, sizeof (FORMATTED_BUTTON_INFO));

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 3);

  SvREFCNT_inc (button->sv);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (button->sv));
  PUSHs(sv_2mortal (newRV_inc (element->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_ARRAY);

  SPAGAIN;

  if (count != 3)
    croak("format_button should return 3 items\n");

  need_delimiter_sv = POPs;
  if (SvOK (need_delimiter_sv))
    {
      result->need_delimiter = SvIV (need_delimiter_sv);
    }

  passive_sv = POPs;
  if (SvOK (passive_sv))
    {
      STRLEN len;
      char *passive_ret = SvPVutf8 (passive_sv, len);
      result->passive = strdup (passive_ret);
    }

  active_sv = POPs;
  if (SvOK (active_sv))
    {
      STRLEN len;
      char *active_ret = SvPVutf8 (active_sv, len);
      result->active = strdup (active_ret);
    }

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_navigation_panel (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                                  const BUTTON_SPECIFICATION_LIST *buttons,
                                  const char *cmdname, const ELEMENT *element,
                                  int vertical)
{
  int count;
  char *result = 0;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  build_tree_to_build (&self->tree_to_build);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 5);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newRV_inc (buttons->av)));
  PUSHs(sv_2mortal (newSVpv (cmdname, 0)));
  PUSHs(sv_2mortal (newRV_inc (element->hv)));
  PUSHs(sv_2mortal (newSViv ((IV) vertical)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_navigation_panel should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_navigation_header (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                                  const BUTTON_SPECIFICATION_LIST *buttons,
                                  const char *cmdname,
                                  const ELEMENT *element)
{
  int count;
  char *result = 0;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  build_tree_to_build (&self->tree_to_build);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 4);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newRV_inc (buttons->av)));
  PUSHs(sv_2mortal (newSVpv (cmdname, 0)));
  PUSHs(sv_2mortal (newRV_inc (element->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_navigation_header should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_heading_text (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                                  const char *cmdname,
                                  const STRING_LIST *classes,
                                  const char *text,
                                  int level, const char *id,
                                  const ELEMENT *element, const char *target)
{
  int count;
  char *result = 0;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;
  SV *classes_sv;
  SV *element_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  build_tree_to_build (&self->tree_to_build);

  if (classes && classes->number)
    {
      size_t i;
      AV *classes_av = newAV ();
      for (i = 0; i < classes->number; i++)
        {
          char *class = classes->list[i];
          SV *class_sv = newSVpv_utf8 (class, 0);
          av_push (classes_av, class_sv);
        }
      classes_sv = newRV_noinc ((SV *) classes_av);
    }
  else
    classes_sv = newSV (0);

  if (element)
    element_sv = newRV_inc (element->hv);
  else
    element_sv = newSV (0);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 7);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (cmdname, 0)));
  PUSHs(sv_2mortal (classes_sv));
  PUSHs(sv_2mortal (newSVpv_utf8 (text, 0)));
  PUSHs(sv_2mortal (newSViv ((IV) level)));
  PUSHs(sv_2mortal (newSVpv_utf8 (id, 0)));
  PUSHs(sv_2mortal (element_sv));
  PUSHs(sv_2mortal (newSVpv_utf8 (target, 0)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_heading_text should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_contents (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                              const char *cmdname, const ELEMENT *command,
                              const char *filename)
{
  int count;
  char *result = 0;
  char *result_ret;
  STRLEN len;
  SV *command_sv;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  build_tree_to_build (&self->tree_to_build);

  if (command)
    command_sv = newRV_inc (command->hv);
  else
    command_sv = newSV (0);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 4);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (cmdname, 0)));
  PUSHs(sv_2mortal (command_sv));
  PUSHs(sv_2mortal (newSVpv (filename, 0)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_contents should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_separate_anchor (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                                   const char *id, const char *class)
{
  int count;
  char *result = 0;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  build_tree_to_build (&self->tree_to_build);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 4);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv_utf8 (id, 0)));
  PUSHs(sv_2mortal (newSVpv_utf8 (class, 0)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_separate_anchor should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_element_header (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                              const char *cmdname, const ELEMENT *command,
                              const OUTPUT_UNIT *output_unit)
{
  int count;
  char *result = 0;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  build_tree_to_build (&self->tree_to_build);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 4);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (cmdname, 0)));
  PUSHs(sv_2mortal (newRV_inc (command->hv)));
  PUSHs(sv_2mortal (newRV_inc (output_unit->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_element_header should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

char *
call_formatting_function_format_element_footer (CONVERTER *self,
                         const FORMATTING_REFERENCE *formatting_reference,
                              const enum output_unit_type unit_type,
                              const OUTPUT_UNIT *output_unit,
                              const char *content, ELEMENT *command)
{
  int count;
  char *result = 0;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return 0;

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  build_tree_to_build (&self->tree_to_build);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 5);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (output_unit_type_names[unit_type], 0)));
  PUSHs(sv_2mortal (newRV_inc (output_unit->hv)));
  /* content == 0 is possible, hope that newSVpv result corresponds to
     undef in that case, but could also need to explicitely use newSV(0) */
  PUSHs(sv_2mortal (newSVpv_utf8 (content, 0)));
  PUSHs(sv_2mortal (newRV_inc (command->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("format_element_footer should return 1 item\n");

  result_sv = POPs;
  result_ret = SvPVutf8 (result_sv, len);
  result = strdup (result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}





void
call_types_conversion (CONVERTER *self, const enum element_type type,
                       const FORMATTING_REFERENCE *formatting_reference,
                       const ELEMENT *element, const char *content,
                       TEXT *result)
{
  int count;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return;

  build_tree_to_build (&self->tree_to_build);

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  if (!element->hv)
    {
      char *element_str = print_element_debug (element, 0);
      fprintf (stderr, "BUG: no hv: %p %s\n", element, element_str);
      free (element_str);
      abort ();
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 4);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (element_type_names[type], 0)));
  PUSHs(sv_2mortal (newRV_inc (element->hv)));
  /* content == 0 is possible, hope that newSVpv result corresponds to
     undef in that case, but could also need to explicitely use newSV(0) */
  PUSHs(sv_2mortal (newSVpv_utf8 (content, 0)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("types_conversion should return 1 item\n");

  result_sv = POPs;
  /* it is encoded using non strict encoding, so the UTF-8 could be invalid.
     It could be possible to add a wrapper in perl that encode to UTF-8,
     but probably not worth it */
  result_ret = SvPVutf8 (result_sv, len);
  text_append (result, result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;
}

void
call_types_open (CONVERTER *self, const enum element_type type,
                 const ELEMENT *element, TEXT *result)
{
  int count;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return;

  build_tree_to_build (&self->tree_to_build);

  formatting_reference_sv = self->types_open[type].sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 3);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (element_type_names[type], 0)));
  PUSHs(sv_2mortal (newRV_inc (element->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("types_open should return 1 item\n");

  result_sv = POPs;
  /* it is encoded using non strict encoding, so the UTF-8 could be invalid.
     It could be possible to add a wrapper in perl that encode to UTF-8,
     but probably not worth it */
  result_ret = SvPVutf8 (result_sv, len);
  text_append (result, result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;
}

void
call_commands_conversion (CONVERTER *self, const enum command_id cmd,
                          const FORMATTING_REFERENCE *formatting_reference,
                          const ELEMENT *element,
                          const HTML_ARGS_FORMATTED *args_formatted,
                          const char *content, TEXT *result)
{
  int count;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;
  SV *args_formatted_sv;
  const char *command_name;

  dTHX;

  if (!self->hv)
    return;

  build_tree_to_build (&self->tree_to_build);

  /* could also be builtin_command_data[cmd].cmdname) */
  command_name = element_command_name (element);

  formatting_reference_sv = formatting_reference->sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  args_formatted_sv = build_html_command_formatted_args (args_formatted);

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 5);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (command_name, 0)));
  PUSHs(sv_2mortal (newRV_inc (element->hv)));
  PUSHs(sv_2mortal (args_formatted_sv));
  /* content == 0 is possible, hope that newSVpv result corresponds to
     undef in that case, but could also need to explicitely use newSV(0) */
  PUSHs(sv_2mortal (newSVpv_utf8 (content, 0)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("commands_conversion should return 1 item\n");

  result_sv = POPs;
  /* it is encoded using non strict encoding, so the UTF-8 could be invalid.
     It could be possible to add a wrapper in perl that encode to UTF-8,
     but probably not worth it */
  result_ret = SvPVutf8 (result_sv, len);
  text_append (result, result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;
}

void
call_commands_open (CONVERTER *self, const enum command_id cmd,
                    const ELEMENT *element, TEXT *result)
{
  int count;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;
  const char *command_name;

  dTHX;

  if (!self->hv)
    return;

  build_tree_to_build (&self->tree_to_build);

  formatting_reference_sv = self->commands_open[cmd].sv_reference;

  /* could also be builtin_command_data[cmd].cmdname) */
  command_name = element_command_name (element);

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 3);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (command_name, 0)));
  PUSHs(sv_2mortal (newRV_inc (element->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("commands_open should return 1 item\n");

  result_sv = POPs;
  /* it is encoded using non strict encoding, so the UTF-8 could be invalid.
     It could be possible to add a wrapper in perl that encode to UTF-8,
     but probably not worth it */
  result_ret = SvPVutf8 (result_sv, len);
  text_append (result, result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;
}

void
call_output_units_conversion (CONVERTER *self,
                              const enum output_unit_type unit_type,
                              const OUTPUT_UNIT *output_unit,
                              const char *content, TEXT *result)
{
  int count;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return;

  build_tree_to_build (&self->tree_to_build);

  formatting_reference_sv
     = self->output_units_conversion[unit_type].sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 4);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (output_unit_type_names[unit_type], 0)));
  PUSHs(sv_2mortal (newRV_inc (output_unit->hv)));
  /* content == 0 is possible, hope that newSVpv result corresponds to
     undef in that case, but could also need to explicitely use newSV(0) */
  PUSHs(sv_2mortal (newSVpv_utf8 (content, 0)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("output_units_conversion should return 1 item\n");


  result_sv = POPs;
  /* it is encoded using non strict encoding, so the UTF-8 could be invalid.
     It could be possible to add a wrapper in perl that encode to UTF-8,
     but probably not worth it */
  result_ret = SvPVutf8 (result_sv, len);
  text_append (result, result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;
}

void
call_special_unit_body_formatting (CONVERTER *self,
                              const size_t special_unit_number,
                              const char *special_unit_variety,
                              const OUTPUT_UNIT *output_unit,
                              TEXT *result)
{
  int count;
  char *result_ret;
  STRLEN len;
  SV *result_sv;
  SV *formatting_reference_sv;

  dTHX;

  if (!self->hv)
    return;

  build_tree_to_build (&self->tree_to_build);

  formatting_reference_sv
     = self->special_unit_body[special_unit_number -1].sv_reference;

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 4);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (special_unit_variety, 0)));
  PUSHs(sv_2mortal (newRV_inc (output_unit->hv)));
  PUTBACK;

  count = call_sv (formatting_reference_sv,
                   G_SCALAR);

  SPAGAIN;

  if (count != 1)
    croak("special_unit_body_formatting should return 1 item\n");


  result_sv = POPs;
  /* it is encoded using non strict encoding, so the UTF-8 could be invalid.
     It could be possible to add a wrapper in perl that encode to UTF-8,
     but probably not worth it */
  result_ret = SvPVutf8 (result_sv, len);
  text_append (result, result_ret);

  PUTBACK;

  FREETMPS;
  LEAVE;
}



FORMATTED_BUTTON_INFO *
call_button_simple_function (CONVERTER *self,
                             void *formatting_reference_sv)
{
  int count;
  FORMATTED_BUTTON_INFO *result;
  SV *need_delimiter_sv;
  SV *active_sv;

  dTHX;

  if (!self->hv)
    return 0;

  build_tree_to_build (&self->tree_to_build);

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  result
   = (FORMATTED_BUTTON_INFO *) malloc (sizeof (FORMATTED_BUTTON_INFO));
  memset (result, 0, sizeof (FORMATTED_BUTTON_INFO));

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 1);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUTBACK;

  count = call_sv ((SV *) formatting_reference_sv,
                   G_ARRAY);

  SPAGAIN;

  if (count != 2)
    croak("button_simple_function should return 2 items\n");

  need_delimiter_sv = POPs;
  if (SvOK (need_delimiter_sv))
    {
      result->need_delimiter = SvIV (need_delimiter_sv);
    }

  active_sv = POPs;
  if (SvOK (active_sv))
    {
      STRLEN len;
      char *active_ret = SvPVutf8 (active_sv, len);
      result->active = strdup (active_ret);
    }

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}

FORMATTED_BUTTON_INFO *
call_button_direction_function (CONVERTER *self,
                             void *formatting_reference_sv,
                             int direction, const ELEMENT *element)
{
  int count;
  FORMATTED_BUTTON_INFO *result;
  SV *need_delimiter_sv;
  SV *active_sv;

  dTHX;

  if (!self->hv)
    return 0;

  build_tree_to_build (&self->tree_to_build);

  if (self->modified_state)
    {
      build_html_formatting_state (self, self->modified_state);
      self->modified_state = 0;
    }

  result
   = (FORMATTED_BUTTON_INFO *) malloc (sizeof (FORMATTED_BUTTON_INFO));
  memset (result, 0, sizeof (FORMATTED_BUTTON_INFO));

  dSP;

  ENTER;
  SAVETMPS;

  PUSHMARK(SP);
  EXTEND(SP, 3);

  PUSHs(sv_2mortal (newRV_inc (self->hv)));
  PUSHs(sv_2mortal (newSVpv (self->direction_unit_direction_name[direction],
                             0)));
  PUSHs(sv_2mortal (newRV_inc (element->hv)));

  PUTBACK;

  count = call_sv ((SV *) formatting_reference_sv,
                   G_ARRAY);

  SPAGAIN;

  if (count != 2)
    croak("button_direction_function should return 2 items\n");

  need_delimiter_sv = POPs;
  if (SvOK (need_delimiter_sv))
    {
      result->need_delimiter = SvIV (need_delimiter_sv);
    }

  active_sv = POPs;
  if (SvOK (active_sv))
    {
      STRLEN len;
      char *active_ret = SvPVutf8 (active_sv, len);
      result->active = strdup (active_ret);
    }

  PUTBACK;

  FREETMPS;
  LEAVE;

  return result;
}



