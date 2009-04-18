package perl4mIRC;
use strict;
use warnings;
use Carp qw[carp];
use Text::Balanced qw[extract_codeblock];
use Symbol qw[delete_package];
use Tie::Hash;
use base q[Tie::ExtraHash];
use Win32::API;    # Not in CORE
*mIRC = *mIRC = *execute;    # is _this_ your card?
use constant BUFFER_SIZE         => 4096;
use constant WM_USER             => 0x400;
use constant WM_MCOMMAND         => (WM_USER + 200);
use constant WM_MEVALUATE        => (WM_USER + 201);
use constant NULL                => 0;
use constant PAGE_READWRITE      => 4;
use constant FILE_MAP_ALL_ACCESS => 0x000f001f;
our $VERSION = 0.95;
my ($hFileMap, $mData, $mWnd, %mIRC);
my $NAMESPACE = q[mIRC];
my $gap       = chr(160) x 2;
my $tab       = $gap x 2;
AUTOLOAD {
    no strict q[vars];
    (my $function = $AUTOLOAD) =~ s|.*::||;
    return if $function eq q[DESTROY] or $function eq q[AUTOLOAD];
    $function = shift if $function =~ m[^mIRC$]i;
    return execute(sprintf q[//%s %s], lc($function), join(q[ ], @_));
}
$|++;
Win32::API->Import(q[user32],
                q[int SendMessage(int hWnd, int Msg, int wParam, int lParam)])
    or die $!;
Win32::API->Import(
    q[kernel32],
    q[INT CreateFileMapping(int hFile,int lAttr,int fProt,int dMaxHi,int dMaxLo,char* pName)]
) or die $^E;
Win32::API->Import(
    q[Kernel32],
    q[INT MapViewOfFile(int hFMapObj,int dAcs, int dFOffHi,int dFOffLo,int dNumOBytes)]
) or die $^E;
Win32::API->Import(q[kernel32], q[BOOL UnmapViewOfFile(char* lBAddr)])
    or die $^E;
Win32::API->Import(q[kernel32], q[BOOL CloseHandle(char* hObject)])
    or die $^E;
Win32::API->Import(q[kernel32],
                   q[VOID RtlMoveMemory(int hDst, char* pSrc, int lLen)])
    or die $^E;
my $RTLMoveMemory_R =    # above to write, this to read
    Win32::API->new(q[kernel32], q[RtlMoveMemory], [qw[P I I]], q[V])
    or die $^E;

sub init {
    ($mWnd) = @_;
    $hFileMap =
        CreateFileMapping(0xFFFFFFFF, NULL, PAGE_READWRITE, 0, BUFFER_SIZE,
                          $NAMESPACE);
    return 0 if !$hFileMap;
    $mData = MapViewOfFile($hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 16);
    tie *STDOUT, q[perl4mIRC];       # redirect STDOUT
    tie *STDERR, q[perl4mIRC], 1;    # redirect STDERR
    tie %mIRC,   q[perl4mIRC];       # deceitful mess
    execute(q[/.signal PERL_ONLOAD]);
    return 1;
}

sub deinit {
    execute(q[/.signal PERL_UNLOAD]);
    UnmapViewOfFile($mData);
    CloseHandle($hFileMap);
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
%s
EVAL
    warn($@) if $@;
    delete_package($package);
    return $return;
}

sub eval_embed {
    my ($file, $line) = @_;
    open(my ($FH), q[<], $file) || carp sprintf q[Could not open '%s': %s],
        $file, $^E
        and return;
    sysread($FH, my ($CODE), -s $FH) == -s $file
        || carp sprintf q[Could not read '%s': %s], $file, $^E
        and return;
    close($FH);
    $line--;    # ??? eh?
    $CODE =~ s|(.*\n){$line}||;
    my $package = q[Perl4mIRC::Eval::] . int(rand(time));
    my (undef, $bad) = extract_codeblock $CODE, q[({}], q[[^(}]*];
    my $strCode = extract_codeblock $bad, q[({}];
    my $return = eval sprintf <<'EVAL', $package, $line, $file, $strCode;
package %s;
*mIRC       = *perl4mIRC::mIRC;
*eval_embed = *perl4mIRC::eval_embed;
#line %d "%s"
%s
EVAL
    warn($@) if $@;
    delete_package($package);
    return $return;
}

sub evaluate {
    my ($command) = @_;
    RtlMoveMemory($mData, chr(0) x BUFFER_SIZE, BUFFER_SIZE);
    RtlMoveMemory($mData, $command,             length($command) + 15);
    my $return = SendMessage($mWnd, WM_MEVALUATE, 0, 0);
    $command = chr(0) x BUFFER_SIZE;
    $RTLMoveMemory_R->Call($command, $mData, BUFFER_SIZE);
    ($command, undef) = split(qq[\0], $command, 2);
    return $command;
}

sub execute {
    my ($command) = @_;
    RtlMoveMemory($mData, chr(0) x BUFFER_SIZE, BUFFER_SIZE);
    RtlMoveMemory($mData, $command,             length($command) + 15);
    my $return = SendMessage($mWnd, WM_MCOMMAND, 1 | 4, 0);
    $command = chr(0) x BUFFER_SIZE;
    $RTLMoveMemory_R->Call($command, $mData, BUFFER_SIZE);
    ($command, undef) = split(chr(0), $command, 2);
    return $command;
}
sub TIEHANDLE { $_[1] ||= 0; return bless \pop, pop; }

sub PRINT {
    my $handle = shift;
    for my $l (@_) {
        for my $p (split m[\n], $l) {
            $p =~ s[\t][$tab]g;
            $p =~ s[  ][$gap]g;
            $p =~ s[^(\W)][chr(0xFEFF) . $1]eg;
            perl4mIRC::execute(sprintf q[//echo %s %s],
                               ($$handle
                                ? q[$color(ctcp) * ]
                                : q[$color(normal)]
                               ),
                               $p
            );
        }
    }
    return 1;
}
sub PRINTF { (shift)->PRINT(sprintf shift, @_); }

sub TIEHASH {
    my $class = shift;
    my $storage = bless [{}, @_], $class;
    return $storage;
}

sub DELETE {
    my ($self, $key) = @_;
    return perl4mIRC::execute(sprintf(q[/unset %%%s], $key));
}

sub STORE {
    my ($self, $key, $value) = @_;
    return perl4mIRC::execute(sprintf(q[/set %%%s %s], $key, $value));
}

sub FETCH {
    my ($self, $key) = @_;
    return perl4mIRC::evaluate(sprintf(q[%%%s], $key));
}
1;

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

Copyright (C) 2008 by Sanko Robinson E<lt>sanko@cpan.orgE<gt>

This program is free software; you can redistribute it and/or modify
it under the terms of The Artistic License 2.0.  See the F<LICENSE>
file included with this distribution or
http://www.perlfoundation.org/artistic_license_2_0.  For
clarification, see http://www.perlfoundation.org/artistic_2_0_notes.

When separated from the distribution, all POD documentation is covered
by the Creative Commons Attribution-Share Alike 3.0 License.  See
http://creativecommons.org/licenses/by-sa/3.0/us/legalcode.  For
clarification, see http://creativecommons.org/licenses/by-sa/3.0/us/.

=for svn $Id: perl4mIRC.pm 6 2009-02-13 06:11:02Z sanko@cpan.org $

=cut
