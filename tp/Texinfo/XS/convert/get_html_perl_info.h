/* get_html_perl_info.h - declarations for get_perl_info.c */
#ifndef GET_HTML_PERL_INFO_H
#define GET_HTML_PERL_INFO_H

#include "EXTERN.h"
#include "perl.h"

int get_output_units_descriptor_converter_sv (SV *converter_in);

void html_converter_initialize_sv (SV *converter_sv,
                                  SV *default_formatting_references,
                                  SV *default_css_string_formatting_references,
                                  SV *default_commands_open,
                                  SV *default_commands_conversion,
                                  SV *default_css_string_commands_conversion,
                                  SV *default_types_open,
                                  SV *default_types_conversion,
                                  SV *default_css_string_types_conversion,
                                  SV *default_output_units_conversion,
                                  SV *default_special_unit_body);

void html_converter_prepare_output_sv (SV *converter_sv, CONVERTER *converter);

ELEMENT *find_element_from_sv (CONVERTER *converter, SV *element_sv,
                               int output_units_descriptor);

ELEMENT *element_converter_from_sv (SV *converter_in, SV *element_sv,
                       const char *warn_string, CONVERTER **converter_out);

void html_set_shared_conversion_state (CONVERTER *converter, SV *converter_in,
                               const char *cmdname, const char *state_name,
                               const int args_nr, SV **args_sv);
SV *html_get_shared_conversion_state (CONVERTER *converter, SV *converter_in,
                               const char *cmdname, const char *state_name,
                               const int args_nr, SV **args_sv);
#endif
