{

    package mIRC;
    use Text::Balanced qw[extract_codeblock];
    use Symbol qw[delete_package];

    sub AUTOLOAD {
        no strict 'vars';
        (my $function = $AUTOLOAD) =~ s|.*::||;
        return if $function eq 'DESTROY' or $function eq 'AUTOLOAD';
        $function = shift if $function =~ m[^mIRC$]i;
        shift @_;
        return mIRC->execute(sprintf '/.%s %s', lc($function), join(' ', @_));
    }

    sub eval_embed {
        my ($file, $line) = @_;
        carp(sprintf q[Could not open '%s': %s], $file, $^E) and return
            if !open(my ($FH), '<', $file);
        carp(sprintf q[Could not read '%s': %s], $file, $^E) and return
            if sysread($FH, my ($CODE), -s $FH) != -s $file;
        close($FH);
        $line--;
        $CODE =~ s|(.*\n){$line}||;
        my $package = 'mIRC::eval::' . int(rand(time));
        (undef, my $bad) = extract_codeblock $CODE, '(){}', '[^(]*';
        my $strCode = extract_codeblock $bad;
        my $return = eval sprintf <<'EVAL', $package, $line, $file, $strCode;
{package %s;
*mIRC       = *mIRC::mIRC;
*eval_embed = *mIRC::eval_embed;
#line %d "%s"
sub { %s }->();}
EVAL
        warn($@) if $@;
        delete_package($package);
        return $return;
    }
}
{

    package mIRC;
    use Tie::Hash;
    use base 'Tie::ExtraHash';
    my %var;
    tie %var, 'mIRC';

    sub var : lvalue {
        my ($class, $key) = @_;
        $var{$key};
    }

    sub TIEHASH {
        my $class = shift;
        my $storage = bless [{}, @_], $class;
        return $storage;
    }

    sub DELETE {
        my ($self, $key) = @_;
        return mIRC->execute( '/unset %%' . $key );
    }

    sub STORE {
        my ($self, $key, $value) = @_;
        return mIRC->execute( '/set -n %%' . $key . ' ' . $value );
    }

    sub FETCH {
        my ($self, $key) = @_;
        return mIRC->evaluate( '%' . $key );
    }
    1;
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
