package perl4mIRC;
{
    use strict;
    use warnings;
    use Carp qw[carp];
    use Text::Balanced qw[extract_codeblock];
    use Symbol qw[delete_package];
    our $VERSION = 0.999.800;
    my $NAMESPACE = 'mIRC';
    my $gap       = chr(160) x 2;
    my $tab       = $gap x 2;
    *mIRC = *mIRC = *execute;    # is _this_ your card?
    $|++;

    sub init {
        ($mWnd) = @_;
             #tie %mIRC,   'perl4mIRC';       # deceitful mess
        mIRC->signal('-n PERL_ONLOAD');
        return 1;
    }

    sub deinit {
        mIRC->signal('-n PERL_UNLOAD');
        return 1;
    }

    sub eval_string {
        my ($code, $package) = @_;
        my $return = eval sprintf <<'EVAL', $package, $code;
package %s;
no strict;
*mIRC       = *perl4mIRC::mIRC;
*eval_embed = *perl4mIRC::eval_embed;
#line 1 mIRC
sub { %s }->();
EVAL
        warn($@) if $@;
        delete_package($package);
        return $return;
    }

    sub eval_embed {
        my ($file, $line) = @_;
        carp(sprintf 'Could not open "%s": %s', $file, $^E) and return
            if !open(my ($FH), '<', $file);
        carp(sprintf 'Could not read "%s": %s', $file, $^E) and return
            if sysread($FH, my ($CODE), -s $FH) != -s $file;
        close($FH);
        $line--;    # ??? eh?
        $CODE =~ s|(.*\n){$line}||;
        my $package = 'Perl4mIRC::Eval::' . int(rand(time));
        my (undef, $bad) = extract_codeblock $CODE, "(){}", '[^(]*';
        my $strCode = extract_codeblock $bad;
        my $return = eval sprintf <<'EVAL', $package, $line, $file, $strCode;
package %s;
*mIRC       = *perl4mIRC::mIRC;
*eval_embed = *perl4mIRC::eval_embed;
#line %d "%s"
sub { %s }->();
EVAL
        warn($@) if $@;
        delete_package($package);
        return $return;
    }
    sub TIEHANDLE { $_[1] ||= 0; return bless \pop, pop; }

    sub PRINT {
        my $handle = shift;
        for my $l (@_) {
            for my $p (split m[\n], $l) {
                $p =~ s[\t][$tab]g;
                $p =~ s[  ][$gap]g;
                $p =~ s[^(\W)][chr(0xFEFF) . $1]eg;
                mIRC->signal('-n PERL_STD' . ($$handle ? 'ERR' : 'OUT'), $p);
            }
        }
        return 1;
    }
    sub PRINTF { (shift)->PRINT(sprintf shift, @_); }
}
1;

package mIRC;
{
    use Tie::Hash;
    use base 'Tie::ExtraHash';
    my %var;
    tie %var, 'mIRC';

    sub var : lvalue {
        my ($class, $key) = @_;
        $var{$key};
    }

    sub AUTOLOAD {
        no strict 'vars';
        (my $function = $AUTOLOAD) =~ s|.*::||;
        return if $function eq 'DESTROY' or $function eq 'AUTOLOAD';
        $function = shift if $function =~ m[^mIRC$]i;
        shift @_;
        return
            perl4mIRC::execute(sprintf '/.%s %s',
                               lc($function),
                               join(' ', @_)
            );
    }

    sub TIEHASH {
        my $class = shift;
        my $storage = bless [{}, @_], $class;
        return $storage;
    }

    sub DELETE {
        my ($self, $key) = @_;
        return mIRC->unset(sprintf('%%%s', $key));
    }

    sub STORE {
        my ($self, $key, $value) = @_;
        return mIRC->set(sprintf('%%%s %s', $key, $value));
    }

    sub FETCH {
        my ($self, $key) = @_;
        return perl4mIRC::evaluate(sprintf('%%%s', $key));
    }
    1
}

=pod

=head1 NAME

perl4mIRC.pm

=head1 Synopsis

=head1 Description

Without this module, perl4mIRC provides basic functionality.

=head1 Notes

=head2 The mIRC pseudo object

To make perl4mIRC as nifty as possible, a little magic is involved.

=head2 Requirements

This module requires L<Win32::API> be installed.

=head2 Installation

=head1 See Also

Project page - http://github.com/sanko/perl4mirc/

mIRC - http://mirc.co.uk/

=head1 Author

Sanko Robinson <sanko@cpan.org> - http://sankorobinson.com/

CPAN ID: SANKO

=head1 License and Legal

Copyright (C) 2008-209 by Sanko Robinson E<lt>sanko@cpan.orgE<gt>

This program is free software; you can redistribute it and/or modify
it under the terms of The Artistic License 2.0.  See the F<LICENSE>
file included with this distribution or
http://www.perlfoundation.org/artistic_license_2_0.  For
clarification, see http://www.perlfoundation.org/artistic_2_0_notes.

When separated from the distribution, all POD documentation is covered
by the Creative Commons Attribution-Share Alike 3.0 License.  See
http://creativecommons.org/licenses/by-sa/3.0/us/legalcode.  For
clarification, see http://creativecommons.org/licenses/by-sa/3.0/us/.

=cut
