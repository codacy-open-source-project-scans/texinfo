# Report.pm: prepare error messages.
#
# Copyright 2010-2024 Free Software Foundation, Inc.
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

package Texinfo::Report;

use 5.00405;
use strict;

# To check if there is no erroneous autovivification
#no autovivification qw(fetch delete exists store strict);

# for fileparse
use File::Basename;

use Locale::Messages;

my $messages_textdomain = 'texinfo';

# this module does not use Texinfo::Common, therefore does not
# obtain those functions, they are defined here
sub __($) {
  my $msgid = shift;
  return Locale::Messages::dgettext($messages_textdomain, $msgid);
}

sub __p($$) {
  my $context = shift;
  my $msgid = shift;
  return Locale::Messages::dpgettext($messages_textdomain, $context, $msgid);
}



sub new(;$)
{
  my $self = shift;
  # if there is no argument, setup a separate Texinfo::Report object,
  # otherwise the structure is added to the converter, nothing is "blessed".
  if (not defined($self)) {
    $self = {};
    bless $self;
  }
  $self->{'errors_warnings'} = [];
  #print STDERR "REPORT NEW $self $self->{'errors_warnings'}\n";
  $self->{'errors_nrs'} = 0;
  return $self;
}

# return the errors and warnings
sub errors($)
{
  my $self = shift;
  return ($self->{'errors_warnings'}, $self->{'error_nrs'});
}

# add an already formatted/setup message
sub add_formatted_message($$)
{
  my $self = shift;
  my $message = shift;

  $self->{'error_nrs'}++ if ($message->{'type'} eq 'error'
                             and !$message->{'continuation'});
  push @{$self->{'errors_warnings'}}, $message;
}

sub format_line_message($$$$;$)
{
  my $type = shift;
  my $text = shift;
  my $error_location_info = shift;
  my $continuation = shift;
  my $warn = shift;

  if (!defined($error_location_info)) {
    cluck("BUG: format_line_message: error_location_info undef");
    return;
  }

  my $message_line;

  if (defined($error_location_info->{'macro'})) {
    if ($type eq 'warning') {
      $message_line = sprintf(__p("Texinfo source file warning in macro",
                               "warning: %s (possibly involving \@%s)")."\n",
                           $text, $error_location_info->{'macro'});
    } else {
      $message_line = sprintf(__p("Texinfo source file error in macro",
                            "%s (possibly involving \@%s)")."\n",
                         $text, $error_location_info->{'macro'});
    }
  } else {
    if ($type eq 'warning') {
      $message_line = sprintf(__p("Texinfo source file warning",
                               "warning: %s")."\n",
                           $text);
    } else {
      $message_line = $text."\n";
    }
  }
  warn $message_line if ($warn);
  my %location_info = %{$error_location_info};
  delete $location_info{'file_name'} if (exists ($location_info{'file_name'})
                                  and not defined($location_info{'file_name'}));
  my $result
    = { 'type' => $type, 'text' => $text, 'error_line' => $message_line,
         %location_info };
  $result->{'continuation'} = $continuation if ($continuation);
  return $result;
}


# format a line warning
sub line_warn($$$$;$$)
{
  my $self = shift;
  my $configuration_information = shift;
  my $text = shift;
  my $error_location_info = shift;
  my $continuation = shift;
  my $silent = shift;

  if (!defined($error_location_info)) {
    cluck("BUG: line_warn: error_location_info undef");
    return;
  }

  my $warn = (defined($configuration_information)
              and $configuration_information->get_conf('DEBUG')
              and not $silent);

  my $warning = format_line_message('warning', $text, $error_location_info,
                                    $continuation, $warn);
  $self->add_formatted_message($warning);
}

sub line_error($$$$;$$)
{
  my $self = shift;
  my $configuration_information = shift;
  my $text = shift;
  my $error_location_info = shift;
  my $continuation = shift;
  my $silent = shift;

  if (!defined($error_location_info)) {
    cluck("BUG: line_error: error_location_info undef");
    return;
  }

  my $warn = (defined($configuration_information)
              and $configuration_information->get_conf('DEBUG')
              and not $silent);

  my $error = format_line_message('error', $text, $error_location_info,
                                  $continuation, $warn);
  $self->add_formatted_message($error);
}

sub format_document_message($$;$$)
{
  my $type = shift;
  my $text = shift;
  my $program_name = shift;
  my $continuation = shift;

  my $message_line;
  if (defined($program_name)) {
    if ($type eq 'warning') {
      $message_line = sprintf(__p("whole document warning", "%s: warning: %s")."\n",
                              $program_name, $text);
    } else {
      $message_line = sprintf("%s: %s\n",
            $program_name, $text);
    }
  } else {
    if ($type eq 'warning') {
      $message_line = sprintf(__p("whole document warning", "warning: %s")."\n",
                           $text);
    } else {
      $message_line = "$text\n";
    }
  }
  my $result = { 'type' => $type, 'text' => $text, 'error_line' => $message_line };
  $result->{'continuation'} = $continuation if ($continuation);
  return $result;
}

sub document_warn($$$;$)
{
  my $self = shift;
  my $configuration_information = shift;
  my $text = shift;
  my $continuation = shift;

  my $program_name;

  if (defined($configuration_information)
      and defined($configuration_information->get_conf('PROGRAM'))
      and $configuration_information->get_conf('PROGRAM') ne '') {
    $program_name = $configuration_information->get_conf('PROGRAM');
  }

  my $warning = format_document_message('warning', $text, $program_name,
                                        $continuation);
  $self->add_formatted_message($warning);
}

sub document_error($$$;$)
{
  my $self = shift;
  my $configuration_information = shift;
  my $text = shift;
  my $continuation = shift;

  my $program_name;

  if (defined($configuration_information)
      and defined($configuration_information->get_conf('PROGRAM'))
      and $configuration_information->get_conf('PROGRAM') ne '') {
    $program_name = $configuration_information->get_conf('PROGRAM');
  }

  my $error = format_document_message('error', $text, $program_name,
                                      $continuation);
  $self->add_formatted_message($error);
}

1;

__END__

=head1 NAME

Texinfo::Report - Error storing for Texinfo modules

=head1 SYNOPSIS

  use Texinfo::Report;

  my $registrar = Texinfo::Report::new();

  if ($warning_happened) {
    $registrar->line_warn($converter, sprintf(__("\@%s is wrongly used"),
                       $current->{'cmdname'}), $current->{'source_info'});
  }

  my ($errors, $errors_count) = $registrar->errors();
  foreach my $error_message (@$errors) {
    warn $error_message->{'error_line'};
  }

=head1 NOTES

The Texinfo Perl module main purpose is to be used in C<texi2any> to convert
Texinfo to other formats.  There is no promise of API stability.

=head1 DESCRIPTION

The C<Texinfo::Report> module helps with error handling.  It is
used by the Texinfo modules L<Texinfo::Parser> and
L<Texinfo::Convert::Converter>.  To use this module, either create
a new C<Texinfo::Report> object or initialize another object
such as to be able to call C<Texinfo::Report> methods.  In any
case, C<Texinfo::Report::new()> is called to setup the module.

Besides the C<new> method, C<errors> is used for reporting errors, and the
other methods to store errors (and warnings).

=head1 METHODS

No method is exported in the default case.

The C<new> method initializes C<Texinfo::Report> related fields.
The errors collected are available through the C<errors> method, the other
methods allow registering errors and warnings.

=over

=item my $registrar = Texinfo::Report::new()

=item $object->Texinfo::Report::new()
X<C<Texinfo::Report::new>>

If called without argument, a C<Texinfo::Report> object is initialized and
returned.  This is how the module is used in the Texinfo Parsers, as
a separate object.

If called on an object C<$object>, the C<$object> is initialized itself
such as to be able to call C<Texinfo::Report> methods.

=item ($error_warnings_list, $error_count) = errors($registrar)
X<C<errors>>

This function returns as I<$error_count> the count of errors since
calling C<new>.  The I<$error_warnings_list> is an array of hash references
one for each error, warning or error line continuation.  Each of these has
the following keys:

=over

=item type

May be C<warning>, or C<error>.

=item text

The text of the error.

=item error_line

The text of the error formatted with the macro name, as needed.

=item line_nr

The line number of the error or warning.

=item file_name

The file name where the error or warning occurs.

=item macro

The user macro name that is expanded at the location of
the error or warning.

=back

=item $registrar->add_formatted_message ($msg)
X<C<add_formatted_message>>

Register the I<$msg> hash reference corresponding to an error, warning or error line
continuation.  The I<$msg> hash reference should correspond to the structure returned
by C<errors>.

=item $registrar->line_warn($text, $configuration_information, $error_location_info, $continuation, $silent)

=item $registrar->line_error($text, $configuration_information, $error_location_info, $continuation, $silent)
X<C<line_warn>>
X<C<line_error>>

Register a warning or an error.  The I<$text> is the text of the
error or warning.  The I<$configuration_information> object gives
some information that can modify the messages or their delivery.
The mandatory I<$error_location_info> holds the information on the error or
warning location.  The I<$error_location_info> reference on hash may be
obtained from Texinfo elements I<source_info> keys.   It may also
be setup to point to a file name, using the C<file_name> key and
to a line number, using the C<line_nr> key.  The C<file_name> key value
should be a binary string.

The I<$continuation> optional arguments, if true, conveys that
the line is a continuation line of a message.

The I<$silent> optional arguments, if true, suppresses the output of
a message that is output immediatly if debugging is set.

The I<source_info> key of Texinfo tree elements is described
in more details in L<Texinfo::Parser/source_info>.

=item $registrar->document_warn($configuration_information, $text, $continuation)

=item $registrar->document_error($configuration_information, $text, $continuation)
X<C<document_warn>>
X<C<document_error>>

Register a document-wide error or warning.  I<$text> is the error or
warning message.  The I<$configuration_information> object gives
some information that can modify the messages or their delivery.
The I<$continuation> optional arguments, if true, conveys that
the line is a continuation line of a message.

=back

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
