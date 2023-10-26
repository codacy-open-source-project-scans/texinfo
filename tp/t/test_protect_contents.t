use strict;

use lib '.';
use Texinfo::ModulePath (undef, undef, undef, 'updirs' => 2);

use Test::More;

BEGIN { plan tests => 2; }

use Texinfo::Parser qw(parse_texi_line parse_texi_piece);
use Texinfo::Common qw(protect_first_parenthesis);
use Texinfo::Structuring;
use Texinfo::Transformations;
use Texinfo::Convert::Texinfo;

ok(1);

sub run_test($$$$)
{
  my $do = shift;
  my $in = shift;
  my $out = shift;
  my $name = shift;

  my $document = parse_texi_piece(undef, $in);
  my $tree = $document->tree();
  my $texi_result;

  if ($do->{'protect_first_parenthesis'}) {
    Texinfo::Transformations::protect_first_parenthesis_in_targets($tree);

    $tree = Texinfo::Structuring::rebuild_tree($tree);

    $texi_result
        = Texinfo::Convert::Texinfo::convert_to_texinfo($tree);
  }

  if (!defined($out)) {
    print STDERR " --> $name: $texi_result\n";
  } else {
    is($texi_result, $out, $name);
  }
}

run_test({'protect_first_parenthesis' => 1},
'@node (man) t',
'@node @asis{(}man) t',
'protect parenthesis');