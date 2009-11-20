AUTHOR & WEBSITE

    The software provided was written by Sanko Robinson.

    Please visit http://github.com/sanko/perl4mirc/ for updates.

DESCRIPTION

    Inspired by the TCL4mIRC[1] and Python4mIRC[2] projects, Perl4mIRC is a
    DLL for the mIRC chat client[3] that allows a scripter to execute Perl
    programs from mIRC's edit box and in mIRC's msl script files.

REQUIREMENTS

    any version of mIRC (developed on v6.35)
    Perl (developed and built for v5.10.1)

INSTALLATION

    To install this script and DLL, to copy the files in this package into a
    mIRC's directory and type...

        /load -rs C:\[path\to\mirc]\perl.mrc

    ...to load the script file. This will run /perl_test to test the
    installation.

USAGE & EXAMPLES

    Use /perl <perl syntax> to execute Perl code. Several examples of this and
    the embedded syntax are in perl.mrc. I'll explain some of the nifty bits
    here...

  STDOUT/STDERR

    Somewhere near the top of perl.mrc, you should see the following lines:

        ; Standard input/output handling
        on *:SIGNAL:PERL_STDOUT:if ($1 != $null) echo -a $1-
        on *:SIGNAL:PERL_STDERR:if ($1 != $null) echo $color(info) -a $1-

    Instead of redirecting all IO to the status window, perl4mIRC redirects
    everything to signals which can then be used any way you see fit. The
    defaults are reasonable but you can customize these if you're bored.

  Inline Snippets

    Midway through perl.mrc, you come upon the following alias...

        alias perl_list_modules {
            if $($use_perl,2) {
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
                mIRC->var('modules') = join(q[, ], sort {lc $a cmp lc $b} @modules);
            }
            echo -a Perl Modules: %modules
            unset %modules
        }

    This bit of code pretty much summarizes the best of what perl4mIRC has to
    offer.

    The 'if $($use_perl,2) {' line starts our embedded perl snippet which ends
    with the matching closing brace. In this example, we are simply sifting
    through the list of loaded modules but any amount of code could be in one
    of these sections.

    In this example, you'll also notice our use of the var() method from the
    mIRC package. This method provides both read and write access to the
    variables defined within mIRC. Here, instead of printing out the list in
    perl, we hand it back to mIRC and echo the result from there.

  mIRC Commands

    To access mIRC's internal commands, you have two options. You man call
    them with the execute() method or directly. Here's an example of each:

        ; execute()
        /perl mIRC->execute("/echo echo echo echo cho cho cho ho ho ho o o o");

        ; directly
        /perl mIRC->echo("Mmmmm... Namespace hacking.");

  mIRC Identifiers

    To evaluate mIRC's internal identifiers, the current API provides an
    evaluate(...) method. Usage is as follows:

        ; quick access to the clipboard's contents
        /perl my $clip = mIRC->evaluate('$cb')

        ; prompt the user for information
        /perl warn mIRC->evaluate('$?="This is a test"')

    Please note that I haven't really smoothed the rough edges of this out and
    may tweak it a little sometime in the future. This evaluate(...) method
    will always work as it's currently documented, but there may be a better
    way to interface this data in perl.

  Foo4mIRC: The Power of the CPAN

    At the very bottom, you'll see...

        alias inlinec {
            if $($use_perl,2) {
                use Inline (C => <<'');
                int add(int x, int y)      { return x + y; }
                int subtract(int x, int y) { return x - y; }

                print "9 + 16 = " . add(9, 16) . "\n";
                print "9 - 16 = " . subtract(9, 16) . "\n";
            }
        }

    ...yep, C. Inside mIRC. For this example, you'll need the Inline::C module
    which may be installed from the CPAN shell. This bit of awesome isn't
    perl4mIRC-specific, but is a great example of how powerful perl itself is.

    A quick search on CPAN will bring you to several Inline modules that
    evaluate ASM, Awk, Basic, C++, Guile, Java, Lua, Python, Ruby, Tcl, and
    many other languages. See http://search.cpan.org/search?q=Inline for a
    list of Inline modules.

    Thanks to perl4mIRC and CPAN's library of awesome, you're really only
    limited by your imagination.

  Review

    A few bullet points to review:

    * Perl is radtastic.

    * You may access mIRC's variables with the var() method:

        /perl warn mIRC->var('someval');

    * You may even use the var() method as an lvalue to set the variables:

        /perl mIRC->var('blahblah') = ucfirst reverse 'gnitset';

    * All of mIRC's commands may be accessed via the execute() method:

        /perl mIRC->execute("/echo This is a test");

    * Or directly by name as methods like so:

        /perl mIRC->echo("Yet another test.");

    * Identifiers may be evaluated with the obviously named evaluate() method:

        /perl printf 'You are using mIRC v%s', mIRC->id('$version')

RELEASE INFORMATION

    See CHANGES.txt

    For future updates, check http://github.com/sanko/perl4mirc/ or the
    project's website http://sankorobinson.com/perl4mirc/.

LICENSES

    Perl4mIRC is released under the Perl/Artistic license. See LICENSE.txt for
    a very legalese definition of what I'm talking about. To understand what
    rights I claim to this code and how to handle derivative work, see the
    Artistic 2.0 Notes[13].

    All textual content is provided under the Creative Commons Attribution-
    Share Alike 3.0 United States License[14] as all documentation should.

    Now that you're completely confused, you can ask me any time to clarify my
    licensing choices.

TRADEMARK NOTICES

    mIRC is a registered trademark of mIRC Co. Ltd.[3]

LINKS
    [ 1] http://kthx.net/clb/tcl4mirc/
    [ 2] http://www.mircscripts.org/comments.php?cid=3864
    [ 3] http://www.mirc.co.uk/
    [ 4] http://search.cpan.org/perldoc?Inline::C
    [ 5] http://search.cpan.org/
    [ 6] http://search.cpan.org/dist/Inline-ASM/
    [ 7] http://search.cpan.org/dist/Inline-Java/
    [ 8] http://search.cpan.org/dist/Inline-Lua/
    [ 9] http://search.cpan.org/dist/Inline-Python/
    [10] http://search.cpan.org/dist/Inline-Ruby/
    [11] http://search.cpan.org/dist/Inline-Tcl/
    [12] http://search.cpan.org/search?m=dist&q=Inline::
    [13] http://www.perlfoundation.org/artistic_2_0_notes
    [14] http://creativecommons.org/licenses/by-sa/3.0/us/
    [15] http://www.cpan.org/authors/id/R/RG/RGARCIA/perl-5.10.0.tar.gz
