# Translations.pm: translate strings.
#
# Copyright 2010-2023 Free Software Foundation, Inc.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Original author: Patrice Dumas <pertusus@free.fr>

package Texinfo::Translations;

use 5.00405;
use strict;

# To check if there is no erroneous autovivification
#no autovivification qw(fetch delete exists store strict);

use Encode;
use POSIX qw(setlocale LC_ALL LC_MESSAGES);
#use Carp qw(confess);
use Carp qw(cluck);
use Locale::Messages;

# note that there is a circular dependency with the parser module, as
# the parser uses complete_indices() from this modules, while this module
# uses a parser.  This is not problematic, however, as the
# modules do not setup data such that their order of loading is not
# important, as long as they load after their dependencies.

# to load a parser
use Texinfo::Parser;

use Texinfo::DocumentXS;

use Texinfo::Convert::Unicode;

# we want a reliable way to switch locale for the document
# strings translations so we don't use the system gettext.
Locale::Messages->select_package ('gettext_pp');

our $module_loaded = 0;
sub import {
  if (!$module_loaded) {
    Texinfo::XSLoader::override(
      "Texinfo::Translations::_XS_configure",
      "Texinfo::DocumentXS::translations_configure");
    # Example of how gdt could be overriden.  Not used because
    # the approach is flawed as there won't be any substitution if the trees in
    # $replaced_substrings are not registered in C data, as is the case in
    # general.
    #Texinfo::XSLoader::override(
    #  "Texinfo::Translations::gdt",
    #  "Texinfo::DocumentXS::gdt");
    $module_loaded = 1;
  }
  # The usual import method
  goto &Exporter::import;
}

# i18n

my $DEFAULT_LANGUAGE = 'en';

my $DEFAULT_ENCODING = 'utf-8';
#my $DEFAULT_PERL_ENCODING = 'utf-8';

my $messages_textdomain = 'texinfo';
my $strings_textdomain = 'texinfo_document';

sub _XS_configure($;$)
{
  # do nothing if there is no XS code loaded
}

sub configure($;$)
{
  my $localesdir = shift;
  my $in_strings_textdomain = shift;

  if (defined($in_strings_textdomain)) {
    $strings_textdomain = $in_strings_textdomain;
  }
  Locale::Messages::bindtextdomain($strings_textdomain, $localesdir);
  # set the directory for the XS code too
  _XS_configure($localesdir, $strings_textdomain);
}

# libintl converts between encodings but doesn't decode them into the
# perl internal format.  This is only called if the encoding is a proper
# perl encoding.
sub _decode_i18n_string($$)
{
  my $string = shift;
  my $encoding = shift;
  #if (!defined($encoding)) {
  #  confess("_decode_i18n_string $string undef encoding\n");
  #}
  return Encode::decode($encoding, $string);
}

sub _switch_messages_locale
{
  my $locale;
  our $working_locale;
  if ($working_locale) {
    $locale = POSIX::setlocale(LC_MESSAGES, $working_locale);
  }
  if (!$locale) {
    $locale = POSIX::setlocale(LC_MESSAGES, "en_US.UTF-8");
  }
  if (!$locale) {
    $locale = POSIX::setlocale(LC_MESSAGES, "en_US")
  }
  # try the output of 'locale -a' (but only once)
  our $locale_command;
  if (!$locale and !$locale_command) {
    $locale_command = "locale -a";
    my @local_command_locales = split("\n", `$locale_command`);
    if ($? == 0) {
      foreach my $try (@local_command_locales) {
        next if $try eq 'C' or $try eq 'POSIX';
        $locale = POSIX::setlocale(LC_MESSAGES, $try);
        last if $locale;
      }
    }
  }
  $working_locale = $locale;
}

# Get document translation - handle translations of in-document strings.
# Return a parsed Texinfo tree
# $LANG set the language if set, otherwise the documentlanguage configuration
# variable is used; if still undef, the $DEFAULT_LANGUAGE variable is used.
# NOTE whether documentlanguage is set as a configuration variable depends a
# lot on the caller.
#  * If called from a Parser, or from the main program, documentlanguage is in
#    general not set, except if set from the command line.
#    When called from the parser, however, it should only be for @def* object
#    oriented def* index name translation and $LANG is likely to be set from the
#    tree is there was a @documentlanguage.
#  * If called from a converter, documentlanguage will in general be set from
#    the document when it is encountered.  Before the first @documentlanguage,
#    it depends on the converter.  Some do not set @documentlanguage before it
#    is encountered.  Some set some default based on @documentlanguage if in
#    the preamble, some set some default language (in general en) in any
#    case.
sub translate_string($$;$$)
{
  my ($customization_information, $string, $translation_context, $lang) = @_;
  if (ref($customization_information) eq 'Texinfo::Document') {
    cluck;
  }
  # In addition to being settable from the command line,
  # the language needs to be dynamic in case there is an untranslated string
  # from another language that needs to be translated.
  $lang = $customization_information->get_conf('documentlanguage')
    if ($customization_information and !defined($lang));
  if (defined($lang) and $lang eq '') {
    cluck ("BUG: defined but empty documentlanguage: $customization_information: '$string'\n");
  }
  $lang = $DEFAULT_LANGUAGE if (!defined($lang));

  my ($saved_LC_MESSAGES, $saved_LANGUAGE);

  # We need to set LC_MESSAGES to a valid locale other than "C" or "POSIX"
  # for translation via LANGUAGE to work.  (The locale is "C" if the
  # tests are being run.)
  #   LC_MESSAGES was reported not to exist for Perl on MS-Windows.  We
  # could use LC_ALL instead, but (a) it's not clear if this would help,
  # and (b) this could interfere with the LC_CTYPE setting in XSParagraph.

  if ($^O ne 'MSWin32') {
    $saved_LC_MESSAGES = POSIX::setlocale(LC_MESSAGES);
    _switch_messages_locale();
  }
  $saved_LANGUAGE = $ENV{'LANGUAGE'};

  Locale::Messages::textdomain($strings_textdomain);

  # FIXME do this only once when encoding is seen (or at beginning)
  # instead of here, each time that gdt is called?
  my $encoding;
  #my $perl_encoding;
  if ($customization_information) {
    # NOTE the following customization variables are not set for
    # a Parser, so the encoding will be undef when gdt is called from
    # parsers.
    if ($customization_information->get_conf('OUTPUT_ENCODING_NAME')) {
      $encoding = $customization_information->get_conf('OUTPUT_ENCODING_NAME');
    }
    #if (defined($customization_information->get_conf('OUTPUT_PERL_ENCODING'))) {
    #  $perl_encoding = $customization_information->get_conf('OUTPUT_PERL_ENCODING');
    #}
  } else {
    # NOTE never happens in the tests, unlikely to happen at all.
    $encoding = $DEFAULT_ENCODING;
    #$perl_encoding = $DEFAULT_PERL_ENCODING;
  }
  Locale::Messages::bind_textdomain_codeset($strings_textdomain, 'UTF-8');
  Locale::Messages::bind_textdomain_filter($strings_textdomain,
                          \&_decode_i18n_string, 'UTF-8');
  # Previously we used the encoding used for input or output to be converted
  # to and then decoded to the perl internal encoding.  But it should be safer
  # to use UTF-8 as we cannot know in advance if the encoding actually used
  # is compatible with the specified encoding, while it should be compatible
  # with UTF-8.  If there are actually characters that cannot be encoded in the
  # output encoding issues will still show up when encoding to output, though.
  # Should be more similar with code used in XS modules, too.
  # As a side note, the best would have been to directly decode using the
  # charset used in the po/gmo files, but it does not seems to be available.
  #Locale::Messages::bind_textdomain_codeset($strings_textdomain, $encoding)
  #  if (defined($encoding) and $encoding ne 'us-ascii');
  #if (!($encoding and $encoding eq 'us-ascii')) {
  #  if (defined($perl_encoding)) {
  #    Locale::Messages::bind_textdomain_filter($strings_textdomain,
  #      \&_decode_i18n_string, $perl_encoding);
  #  }
  #}

  my @langs = ($lang);
  if ($lang =~ /^([a-z]+)_([A-Z]+)/) {
    my $main_lang = $1;
    my $region_code = $2;
    push @langs, $main_lang;
  }

  my @locales;
  foreach my $language (@langs) {
    # NOTE the locale file with appended encoding are searched for, but if
    # not found, files with stripped encoding are searched for too:
    # https://www.gnu.org/software/libc/manual/html_node/Using-gettextized-software.html
    if (defined($encoding)) {
      push @locales, "$language.$encoding";
    } else {
      push @locales, $language;
    }
    # also try us-ascii, the charset should be compatible with other
    # charset, and should resort to @-commands if needed for non
    # ascii characters
    # REMARK this is not necessarily true for every language/encoding.
    # This can be true for latin1, and maybe some other 8 bit encodings
    # with accents available as @-commands, but not for most
    # language.  However, for those languages, it is unlikely that
    # the locale with .us-ascii are set, so it should not hurt
    # to add this possibility.
    if (!$encoding or ($encoding and $encoding ne 'us-ascii')) {
      push @locales, "$language.us-ascii";
    }
  }

  my $locales = join(':', @locales);

  Locale::Messages::nl_putenv("LANGUAGE=$locales");

  my $translated_string;

  if (defined($translation_context)) {
    $translated_string = Locale::Messages::pgettext($translation_context,
                                                     $string);
  } else {
    $translated_string = Locale::Messages::gettext($string);
  }

  Locale::Messages::textdomain($messages_textdomain);

  if (!defined($saved_LANGUAGE)) {
    delete ($ENV{'LANGUAGE'});
  } else {
    $ENV{'LANGUAGE'} = $saved_LANGUAGE;
  }

  if ($^O ne 'MSWin32') {
    if (defined($saved_LC_MESSAGES)) {
      POSIX::setlocale(LC_MESSAGES, $saved_LC_MESSAGES);
    } else {
      POSIX::setlocale(LC_MESSAGES, '');
    }
  }
  #print STDERR "_GDT '$string' '$translated_string'\n";
  return $translated_string;
}

# Get document translation - handle translations of in-document strings.
# Return a parsed Texinfo tree
sub gdt($$;$$$)
{
  my ($customization_information, $string, $replaced_substrings,
      $translation_context, $lang) = @_;

  # allows to redefine translate_string, as done in the HTML converter.  Cannot
  # directly call translate_string on $customization_information, as it may not
  # provide the method if it does not inherit from Texinfo::Translations, as is
  # the case for Texinfo::Parser.
  my $translate_string_method
     = $customization_information->can('translate_string');
  $translate_string_method = \&translate_string if (!$translate_string_method);
  my $translated_string = &$translate_string_method($customization_information,
                                       $string, $translation_context, $lang);

  my $result_tree = replace_convert_substrings($customization_information,
                                    $translated_string,
                                    $replaced_substrings);
  #print STDERR "GDT '$string' '$translated_string' '".
  #     Texinfo::Convert::Texinfo::convert_to_texinfo($result_tree)."'\n";
  return $result_tree;
}

# Get document translation - handle translations of in-document strings.
# In general for already converted strings that do not need to go through
# a Texinfo tree.
sub gdt_string($$;$$$)
{
  my ($customization_information, $string, $replaced_substrings,
      $translation_context, $lang) = @_;

  # Following code allows to redefine translate_string, as done in the HTML
  # converter.  We check with can() explicitely instead of letting inheritance
  # rules determine which method to call (which would be achieved with
  # $customization_information->translate_string) because
  # $customization_information may not provide the method if it does not
  # inherit from Texinfo::Translations, as is the case for Texinfo::Parser.
  # (Same is done in gdt_string_columns.)
  my $translate_string_method
     = $customization_information->can('translate_string');
  $translate_string_method = \&translate_string if (!$translate_string_method);

  my $translated_string = &$translate_string_method($customization_information,
                                       $string, $translation_context, $lang);

  return replace_substrings ($customization_information, $translated_string,
                             $replaced_substrings);
}

# Like gdt_string, but additionally return the width of the result in
# screen columns, not counting the width of substituted strings.
#
# TODO: In the future, this function may return an encoded string, and
# take encoded arguments.  The plan is to save the width in columns before
# encoding the string.
sub gdt_string_columns($$;$$$)
{
  my ($customization_information, $string, $replaced_substrings,
      $translation_context, $lang) = @_;

  # see comment in gdt_string
  my $translate_string_method
     = $customization_information->can('translate_string');
  $translate_string_method = \&translate_string if (!$translate_string_method);

  my $translated_string = &$translate_string_method($customization_information,
                                       $string, $translation_context, $lang);

  my ($result, $result_counted) = ($translated_string, $translated_string);

  my $re;
  if (defined($replaced_substrings) and ref($replaced_substrings)) {
    $re = join '|', map { quotemeta $_ } keys %$replaced_substrings;

    # Replace placeholders
    $result =~
      s/\{($re)\}/defined $replaced_substrings->{$1} ? $replaced_substrings->{$1} : "{$1}"/ge;

    # Strip out placeholders
    $result_counted =~ s/\{($re)\}//g;
  }

  return ($result, Texinfo::Convert::Unicode::string_width($result_counted));
}

sub replace_substrings($$;$)
{
  my $customization_information = shift;
  my $translated_string = shift;
  my $replaced_substrings = shift;

  my $translation_result = $translated_string;

  if (defined($replaced_substrings) and ref($replaced_substrings)) {
    my $re = join '|', map { quotemeta $_ } keys %$replaced_substrings;
    $translation_result
  =~ s/\{($re)\}/defined $replaced_substrings->{$1} ? $replaced_substrings->{$1} : "{$1}"/ge;
  }
  return $translation_result;
}


sub replace_convert_substrings($$;$)
{
  my $customization_information = shift;
  my $translated_string = shift;
  my $replaced_substrings = shift;

  my $texinfo_line = $translated_string;

  # we change the substituted brace-enclosed strings to internal
  # values marked by @txiinternalvalue such that their location
  # in the Texinfo tree can be marked.  They are substituted
  # after the parsing in the final tree.
  # Using a special command that is invalid unless a special
  # configuration is set means that there should be no clash
  # with @-commands used in translations.
  if (defined($replaced_substrings) and ref($replaced_substrings)) {
    my $re = join '|', map { quotemeta $_ } keys %$replaced_substrings;
    $texinfo_line =~ s/\{($re)\}/\@txiinternalvalue\{$1\}/g;
  }

  # accept @txiinternalvalue as a valid Texinfo command, used to mark
  # location in tree of substituted brace enclosed strings.
  my $parser_conf = {'accept_internalvalue' => 1};

  # general customization relevant for parser
  if ($customization_information) {
    my $debug_level = $customization_information->get_conf('DEBUG');
    # one less debug level for the gdt parser.
    if (defined($debug_level)) {
      if ($debug_level > 0) {
        $debug_level--;
      }
      $parser_conf->{'DEBUG'} = $debug_level;
    }
    #foreach my $conf_variable () {
    #  if (defined($customization_information->get_conf($conf_variable))) {
    #    $parser_conf->{$conf_variable}
    #      = $customization_information->get_conf($conf_variable);
    #  }
    #}
  }
  my $parser = Texinfo::Parser::simple_parser($parser_conf);

  if ($customization_information->get_conf('DEBUG')) {
    print STDERR "IN TR PARSER '$texinfo_line'\n";
  }

  my $tree = $parser->parse_texi_line($texinfo_line, undef, 0, 1);
  my $registrar = $parser->registered_errors();
  my ($errors, $errors_count) = $registrar->errors();
  if ($errors_count) {
    warn "translation $errors_count error(s)\n";
    warn "translated string: $translated_string\n";
    warn "Error messages: \n";
    foreach my $error_message (@$errors) {
      warn $error_message->{'error_line'};
    }
  }
  $tree = _substitute($tree, $replaced_substrings);
  if ($customization_information->get_conf('DEBUG')) {
    print STDERR "RESULT GDT: '".
       Texinfo::Convert::Texinfo::convert_to_texinfo($tree)."'\n";
  }
  return $tree;
}

sub _substitute($$);
sub _substitute_element_array($$) {
  my $array = shift;
  my $replaced_substrings = shift;

  my $nr = scalar(@$array);

  for (my $idx = 0; $idx < $nr; $idx++) {
    my $element = $array->[$idx];
    if ($element->{'cmdname'} and $element->{'cmdname'} eq 'txiinternalvalue') {
      my $name = $element->{'args'}->[0]->{'contents'}->[0]->{'text'};
      if ($replaced_substrings->{$name}) {
        $array->[$idx] = $replaced_substrings->{$name};
      }
    } else {
      _substitute($element, $replaced_substrings);
    }
  }
}

# Recursively substitute @txiinternalvalue elements in $TREE with
# their values given in $CONTEXT.
sub _substitute($$) {
  my $tree = shift;
  my $replaced_substrings = shift;

  if ($tree->{'contents'}) {
    _substitute_element_array($tree->{'contents'}, $replaced_substrings);
  }

  if ($tree->{'args'}) {
    _substitute_element_array($tree->{'args'}, $replaced_substrings);
  }

  return $tree;
}

# Same as gdt but with mandatory translation context, used for marking
# of strings with translation contexts
sub pgdt($$$;$$)
{
  my ($customization_information, $translation_context, $string,
      $replaced_substrings, $lang) = @_;
  return $customization_information->gdt($string, $replaced_substrings,
                                         $translation_context, $lang);
}

if (0) {
  # it is needed to mark the translation as gdt is called like
  # gdt($customization_information, '....')
  # and not like gdt('....')
  # TRANSLATORS: association of a method or operation name with a class
  # in descriptions of object-oriented programming methods or operations.
  gdt('{name} on {class}', undef, undef);
  # TRANSLATORS: association of a variable or instance variable with
  # a class in descriptions of object-oriented programming variables or
  # instance variable.
  gdt('{name} of {class}', undef, undef);
}

# For some @def* commands, we delay storing the contents of the
# index entry until now to avoid needing Texinfo::Translations::gdt
# in the main code of ParserNonXS.pm.
sub complete_indices($$)
{
  my $customization_information = shift;
  my $index_names = shift;

  foreach my $index_name (sort(keys(%{$index_names}))) {
    next if (not defined($index_names->{$index_name}->{'index_entries'}));
    foreach my $entry (@{$index_names->{$index_name}->{'index_entries'}}) {
      my $main_entry_element = $entry->{'entry_element'};
      if ($main_entry_element->{'extra'}
          and $main_entry_element->{'extra'}->{'def_command'}
          and not $main_entry_element->{'extra'}->{'def_index_element'}) {
        my ($name, $class);
        if ($main_entry_element->{'args'}->[0]->{'contents'}) {
          foreach my $arg (@{$main_entry_element->{'args'}->[0]->{'contents'}}) {
            my $role = $arg->{'extra'}->{'def_role'};
            if ($role eq 'name') {
              $name = $arg;
            } elsif ($role eq 'class') {
              $class = $arg;
            } elsif ($role eq 'arg' or $role eq 'typearg' or $role eq 'delimiter') {
              last;
            }
          }
        }

        if ($name and $class) {
          my ($index_entry, $text_element);
          my $index_entry_normalized = {};

          my $def_command = $main_entry_element->{'extra'}->{'def_command'};

          my $class_copy = Texinfo::Common::copy_treeNonXS($class);
          my $name_copy = Texinfo::Common::copy_treeNonXS($name);
          my $ref_class_copy = Texinfo::Common::copy_treeNonXS($class);
          my $ref_name_copy = Texinfo::Common::copy_treeNonXS($name);

          # Use the document language that was current when the command was
          # used for getting the translation.
          my $entry_language
             = $main_entry_element->{'extra'}->{'documentlanguage'};
          if ($def_command eq 'defop'
              or $def_command eq 'deftypeop'
              or $def_command eq 'defmethod'
              or $def_command eq 'deftypemethod') {
            $index_entry = gdt($customization_information, '{name} on {class}',
                               {'name' => $name_copy, 'class' => $class_copy},
                                undef, $entry_language);
            $text_element = {'text' => ' on ',
                             'parent' => $index_entry_normalized};
          } elsif ($def_command eq 'defcv'
                   or $def_command eq 'defivar'
                   or $def_command eq 'deftypeivar'
                   or $def_command eq 'deftypecv') {
            $index_entry = gdt($customization_information, '{name} of {class}',
                               {'name' => $name_copy, 'class' => $class_copy},
                               undef, $entry_language);
            $text_element = {'text' => ' of ',
                             'parent' => $index_entry_normalized};
          }
          $ref_name_copy->{'parent'} = $index_entry_normalized;
          $ref_class_copy->{'parent'} = $index_entry_normalized;
          $index_entry_normalized->{'contents'}
              = [$ref_name_copy, $text_element, $ref_class_copy];

          # prefer a type-less container rather than 'root_line' returned by gdt
          delete $index_entry->{'type'};

          $main_entry_element->{'extra'}->{'def_index_element'} = $index_entry;
          $main_entry_element->{'extra'}->{'def_index_ref_element'}
                                                  = $index_entry_normalized;
        }
      }
    }
  }
}



1;

__END__

=head1 NAME

Texinfo::Translations - Translations of output documents strings for Texinfo modules

=head1 SYNOPSIS

  @ISA = qw(Texinfo::Translations);

  Texinfo::Translations::configure('LocaleData');

  my $tree_translated = $converter->gdt('See {reference} in @cite{{book}}',
                       {'reference' => $tree_reference,
                        'book'  => {'text' => $book_name}});


=head1 NOTES

The Texinfo Perl module main purpose is to be used in C<texi2any> to convert
Texinfo to other formats.  There is no promise of API stability.

=head1 DESCRIPTION

The C<Texinfo::Translations> module helps with translations
in output documents.

Translation of error messages is not described here, some
elements are in L<Texinfo::Common C<__> and C<__p>|Texinfo::Common/$translated_string = __($msgid)>.

=head1 METHODS

No method is exported.

The C<configure> method sets the translation files base directory.  If not
called, system defaults are used.

=over

=item configure($localesdir, $strings_textdomain)

I<$localesdir> is the directory where translation files are found. The
directory structure and files format should follow the L<conventions expected
for gettext based
internationalization|https://www.gnu.org/software/gettext/manual/html_node/Locating-Catalogs.html>.
The I<$strings_textdomain> is optional, if set, it determines the translation
domain.

=back

The C<gdt> and C<pgdt> methods are used to translate strings to be output in
converted documents, and return a Texinfo tree.  The C<gdt_string> is similar
but returns a simple string, for already converted strings.

The C<replace_convert_substrings> method is called by C<gdt> to substitute
replaced substrings in a translated string and convert to a Texinfo tree.
It may be especially useful when overriding or reimplementing C<gdt>.  The
C<replace_substrings> method is called by C<gdt_string> to substitute
replaced substrings in a translated string.

=over

=item $tree = $object->gdt($string, $replaced_substrings, $translation_context, $lang)

=item $string = $object->gdt_string($string, $replaced_substrings, $translation_context, $lang)

X<C<gdt>> X<C<gdt_string>>

The I<$string> is a string to be translated.  With C<gdt>
the function returns a Texinfo tree, as the string is interpreted
as Texinfo code after translation.  With C<gdt_string> a string
is returned.  I<$replaced_substrings> is an optional hash reference specifying
some substitution to be done after the translation.  The key of the
I<$replaced_substrings> hash reference identifies what is to be substituted,
and the value is, for C<gdt>, some string, texinfo tree or array content
that is substituted in the resulting texinfo tree. The substitutions may only
be strings with C<gdt_string>. In the string to be translated word in brace
matching keys of I<$replaced_substrings> are replaced.

The I<$object> is typically a converter, but can be any object that implements
C<get_conf>, or undefined (C<undef>).  If not undefined, the information in the
I<$object> is used to determine the encoding, the documentlanguage and get some
customization information.

The I<$translation_context> is optional.  If not C<undef> this is a translation
context string for I<$string>.  It is the first argument of C<pgettext>
in the C API of Gettext.  I<$lang> is optional. If set, it overrides the
documentlanguage.

For example, in the following call, the string
C<See {reference} in @cite{{book}}> is translated, then
parsed as a Texinfo string, with I<{reference}> substituted by
I<$tree_reference> in the resulting tree, and I<{book}>
replaced by the associated texinfo tree text element:

  $tree = $converter->gdt('See {reference} in @cite{{book}}',
                       {'reference' => $tree_reference,
                        'book'  => {'text' => $book_name}});

C<gdt> uses a gettext-like infrastructure to retrieve the
translated strings, using the I<texinfo_document> domain.

=item $tree = $object->pgdt($translation_context, $string, $replaced_substrings, $lang)
X<C<pgdt>>

Same to C<gdt> except that the I<$translation_context> is not optional.
Calls C<gdt>.  This function is useful to mark strings with a
translation context for translation.  This function is similar to pgettext
in the Gettext C API.

=item $tree = $object->replace_convert_substrings($translated_string, $replaced_substrings)

=item $string = $object->replace_substrings($translated_string, $replaced_substrings)

X<C<replace_convert_substrings>> X<C<replace_substrings>>

I<$translated_string> is a string already translated.  I<$replaced_substrings>
is an optional hash reference specifying some substitution to be done.
I<$object> is typically a converter, but can be any object that implements
C<get_conf>, or undefined (C<undef>).  If not undefined, the information in the
I<$object> is used to get some customization information.

C<replace_convert_substrings> performs the substitutions of substrings in the
translated string and converts to a Texinfo tree.  It is called from C<gdt>
after the retrieval of the translated string.

C<replace_substrings> performs the substitutions of substrings in the
translated string.  It is called from C<gdt_string> after the retrieval of the
translated string.

=back

=head1 SEE ALSO

L<GNU gettext utilities manual|https://www.gnu.org/software/gettext/manual/>.

=head1 AUTHOR

Patrice Dumas, E<lt>pertusus@free.frE<gt>

=head1 COPYRIGHT AND LICENSE

Copyright 2010- Free Software Foundation, Inc.  See the source file for
all copyright years.

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at
your option) any later version.

=cut
