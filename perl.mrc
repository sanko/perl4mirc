; perl4mIRC support script for version 0.9.600
;
; Written by Sanko Robinson <sanko@cpan.org>
;
; This file is not needed to use perl4mirc.dll but
; provides a simplified interface to access it.
;
; See README.txt for information on how to use
; the commands defined here, or look at the
; Examples below.
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of The Artistic License 2.0.  See the F<LICENSE>
; file included with this distribution or
; http://www.perlfoundation.org/artistic_license_2_0.  For
; clarification, see http://www.perlfoundation.org/artistic_2_0_notes.

; Convenience methods
alias perl_dll { return " $+ $scriptdirperl4mIRC.dll $+ "  }
alias perl_unload { dll -u $perl_dll }
alias perl_str { return $qt($replace($1,\,/,$,\$)) }
alias perl {  if ($isid)  return $dll($perl_dll,perl_eval_string,$1-) | dll $perl_dll perl_eval_string $1- }

; Perl interpeter bridge methods for embedded scripts
alias perl_embed { perl eval_embed( $perl_str($1) $+ , $2-) | return $false }
alias has_perl { return $!perl_embed($script,$scriptline) }

; Initialization callback
on *:SIGNAL:PERL_ONLOAD: {
  perl mIRC->{'PerlVer'} = qq[$^V]
  perl mIRC->{'version'} = qq[$VERSION]
  echo $color(info2) -ae * Loaded perl4mIRC %version (using Perl %PerlVer $+ ). Edit line $scriptline of $qt($remove($script,$mircdir)) to change this message.
  perl delete mIRC->{'PerlVer'}
  perl delete mIRC->{'version'}
}

on *:SIGNAL:PERL_UNLOAD: {
  echo $color(info2) -ae * Unloaded perl4mIRC
}

on *:LOAD: { echo $color(info2) -ae * Running /perl_test to see if Perl works: | perl_test }

; Examples

; One-liners

; Classic hello world
alias perl_hello_world { perl print q[Hello world] }

; Version
alias perl_hello_world { perl version }
alias perl_version { if ($isid) return $dll($perl_dll,version,$1-) | dll $perl_dll version $1- }

; Perl timer-delays (needs multithreaded Perl)
; Use threads only at your own risk!
alias perl_threads { perl use threads; async{sleep 10; print q[Done]}; print q[Go!]; }

; Download a file from a website and print the first line (LWP::Simple is needed)
alias perl_get_versions_file {
  perl use LWP::Simple qw[get];my@l=split(m[\n],get(q[http://www.mirc.co.uk/versions.txt]));print$l[0];
}

; Shows how to pass data to and from Perl when certain identifiers
; are not accessible such as $1-
alias perl_strlen {
  set %data $1-
  perl mIRC(q[//echo len:] . length(mIRC->{'data'}));
  unset %data
}

; Embedded Perl

; Test method
alias perl_test {
  if $($has_perl,2) {
    mIRC("/linesep -a");
    my @array = qw[3 5 1 2 4 9 7 6];
    printf qq[Testing Perl.\n\tOriginal array: %s\n\tSorted array  : %s],
    join(q{, },@array),
    join(q[, ], sort @array);
  }
}
; Lists the modules currently loaded in Perl
alias perl_list_modules {
  if $($has_perl,2) {
    my @modules;
    for my $module(keys %INC) {
      if ($module =~ m[\.pm$]) {
        $module =~ s|/|::|g;
        $module =~ s|.pm$||;
      }
      push @modules, $module;
    }
    # Bring information back to mIRC in a var rather
    # than using the mirc proc to /echo the results
    mIRC->{modules} = join(q[, ], sort {lc $a cmp lc $b} @modules);
  }
  echo -a Perl Modules: %modules
  unset %modules
}

; REAL inline C :D
; You asked for it, so here it is...
; Requires you to install this script without spaces in the path
alias inlinec {
  if $($has_perl,2) {
    use Inline C => q[
    void greet() {
      printf("Hello, world\n");
    }
    ];
    greet;
  }
}

; Here is an example using event specific identifiers from Perl
; It's not perfect, but I'll work on it...
on *:TEXT:*hello*:#perl-moo-moo: {
  %nick = $nick
  %chan = $chan
  if $($has_perl,2) {
    printf q[%s just said hi on %s],mIRC->{nick}, mIRC->{chan};
    mIRC("/msg ". mIRC->{nick} . " Yo");
  }
}
