use strict;

use lib '.';
use Texinfo::ModulePath (undef, undef, undef, 'updirs' => 2);

require 't/test_utils.pl';

my @test_cases = (
['heading_in_copying',
'@copying

@heading in copying @@ @emph{heading}
@cindex cindex copying

@subheading second in copying @@ @emph{subheading}
@end copying

@node Top
@node chap

@insertcopying
'],
['heading_in_quotation',
'@quotation

@heading in quotation @@ @emph{heading}

@end quotation
'],
['heading_in_example',
'@example
@heading in example @@ @emph{heading}
@end example
']
);

foreach my $test (@test_cases) {
  $test->[2]->{'test_formats'} = ['plaintext', 'html'];
}

run_all('heading', \@test_cases);
