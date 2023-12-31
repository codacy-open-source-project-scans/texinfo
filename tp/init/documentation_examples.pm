# API documentation code examples implemented, to check syntax
# and also expected effects.  Only for code that is not already elsewhere,
# nor is too complex to set up.  Some of those customization results
# are silly, just for the sake of checking.

use strict;

# To check if there is no erroneous autovivification
#no autovivification qw(fetch delete exists store strict);

my $default_footnotestyle = texinfo_get_conf('footnotestyle');
my $main_program_footnotestyle;
if (not defined($default_footnotestyle)) {
  $main_program_footnotestyle = 'is undef';
} elsif ($default_footnotestyle eq 'separate') {
  $main_program_footnotestyle = 'is separate';
} else {
  $main_program_footnotestyle = 'not separate '.$default_footnotestyle;
}

my %translations = (
'fr' => {
'error--&gt;' => {'' => 'erreur--&gt;',},
# ...
},
'de' => {
'error--&gt;' => {'' => 'Fehler--&gt;',},
# ...
}
# ...
);

texinfo_register_no_arg_command_formatting('-', undef, '&shy;');

texinfo_register_no_arg_command_formatting('error', undef, undef, undef,
                                           'error--&gt;');

texinfo_register_no_arg_command_formatting('equiv', undef, undef, undef,
                                         undef, 'is the @strong{same} as');

$translations{'fr'}->{'is the @strong{same} as'}->{''}
                                     = 'est la @strong{m@^eme} que';

texinfo_register_style_command_formatting('sansserif', 'code', 0, 'normal');
texinfo_register_style_command_formatting('sansserif', 'code', 0, 'preformatted');
texinfo_register_style_command_formatting('sansserif', undef, 1, 'string');

texinfo_register_upper_case_command('sc', 0);
texinfo_register_upper_case_command('var', 1);

texinfo_register_accent_command_formatting('dotless', 'nodot', 'ij');

my $shown_styles;
my $footnotestyle;
sub my_function_set_some_css {
  my $converter = shift;
  my $all_included_rules = $converter->css_get_info('rules');
  my $all_default_selectors = $converter->css_get_info('styles');
  my $titlefont_style = $converter->css_selector_style('h1.titlefont');
  $titlefont_style = 'undefined' if (!defined($titlefont_style));
  $shown_styles = $titlefont_style.' '.
            $converter->css_selector_style('h1.shorttitlepage');
  $converter->css_set_selector_style('h1.titlefont', 'text-align:center');

  my $footnotestyle_before_setting = $converter->get_conf('footnotestyle');
  $footnotestyle_before_setting = 'UNDEF'
     if (not defined($footnotestyle_before_setting));
  $converter->set_conf('footnotestyle', 'separate');
  $footnotestyle = $main_program_footnotestyle
                    .'|'.$footnotestyle_before_setting
                    .'|'.$converter->get_conf('footnotestyle');
  # there should be nothing in @$all_included_rules for two reasons,
  # first because it requires 'CSS_FILES' to be set to parseable
  # CSS files, and CSS files parsing is done after the setup handler
  # is called.
  #print STDERR "all_included_rules: ".join('|', @$all_included_rules)."\n";
  return 0;
}

texinfo_register_handler('setup', \&my_function_set_some_css);

sub my_email_formatting_function {
  my $converter = shift;
  my $cmdname = shift;
  my $command = shift;
  my $args = shift;

  my $args_nr = 0;
  if ($args) {
    $args_nr = scalar(@$args);
  }

  my $mail = '';
  my $mail_string = '';
  if ($args_nr > 0 and defined($args->[0])) {
    my $mail_arg = $args->[0];
    $mail = $mail_arg->{'url'};
    $mail_string = $mail_arg->{'monospacestring'};
  }

  my $text = '';
  if ($args_nr > 1 and defined($args->[1])
      and defined($args->[1]->{'normal'})) {
    my $text_arg = $args->[1];
    $text = $text_arg->{'normal'};
  }
  $text = $mail_string unless ($text ne '');

  if ($converter->in_string()) {
    return "$mail_string ($text) $shown_styles, $footnotestyle";
  } else {
    return $converter->html_attribute_class('a', [$cmdname])
     ." href=\"mailto:$mail_string\">$text</a> [$shown_styles, $footnotestyle]";
  }
}

texinfo_register_command_formatting('email', \&my_email_formatting_function);

sub my_convert_paragraph_type($$$$)
{
  my $converter = shift;
  my $type = shift;
  my $element = shift;
  my $content = shift;

  $content = '' if (!defined($content));

  $content = $converter->get_associated_formatted_inline_content($element).$content;

  return $content if ($converter->in_string());

  my @contents = @{$element->{'contents'}};
  push @contents, {'text' => ' <code>HTML</code> text ',
                   'type' => '_converted'};
  my $result = $converter->convert_tree({'type' => '_code',
                                   'contents' => \@contents });
  return "<p>".$result."</p>";
}

texinfo_register_type_formatting('paragraph', \&my_convert_paragraph_type);

sub my_node_file_name($$$) {
  my ($converter, $element, $filename) = @_;
  # ....
  my $node_file_name = 'prepended_to_filenames-'.$filename;
  return $node_file_name;
}

texinfo_register_file_id_setting_function('node_file_name',
                                          \&my_node_file_name);


sub my_label_target_name($$$$) {
  my ($converter, $normalized, $label_element, $default_target) = @_;
  if (defined($normalized)) {
    my $element = $converter->label_command($normalized);
    return 'prepended_to_labels-'.$element->{'extra'}->{'normalized'};
  }
  return $default_target;
}

texinfo_register_file_id_setting_function('label_target_name',
                                          \&my_label_target_name);

sub my_format_translate_message($$$;$)
{
  my ($self, $string, $lang, $translation_context) = @_;
  $translation_context = '' if (!defined($translation_context));
  if (exists($translations{$lang})
      and exists($translations{$lang}->{$string})
      and exists($translations{$lang}->{$string}->{$translation_context})) {
    my $translation = $translations{$lang}->{$string}->{$translation_context};
    return $translation;
  }
  return undef;
}

texinfo_register_formatting_function('format_translate_message',
                                          \&my_format_translate_message);

1;
