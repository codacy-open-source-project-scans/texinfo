# StructTransf.pm: load structuring and tree transformation XS
# Copyright 2023 Free Software Foundation, Inc.
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

package Texinfo::StructTransfXS;

use strict;
use warnings;

our $VERSION = '7.1dev';

use Texinfo::XSLoader;

BEGIN {
  my $XS_structuring = ((not defined($ENV{TEXINFO_XS_PARSER})
                            or $ENV{TEXINFO_XS_PARSER} eq '1')
                       and (not defined($ENV{TEXINFO_XS_STRUCTURE})
                            or $ENV{TEXINFO_XS_STRUCTURE} ne '0'));
  if ($XS_structuring) {
    Texinfo::XSLoader::init (
      "Texinfo::StructTransfXS",
      "Texinfo::StructTransfXS",
      "StructuringTransfoXS",
      undef,
      0,
      undef,
      undef,
    );
  }
}

# NB Don't add more functions down here, because this can cause an error
# with some versions of Perl, connected with any typeglob assignments done
# above.  ("Can't call mro_method_changed_in() on anonymous symbol table").
#
# See http://perl5.git.perl.org/perl.git/commitdiff/03d9f026ae253e9e69212a3cf6f1944437e9f070?hp=ac73ea1ec401df889d312b067f78b618f7ffecc3
#
# (change to Perl interpreter on 22 Oct 2011)


1;
__END__
