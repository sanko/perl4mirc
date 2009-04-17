AUTHOR & WEBSITE

	The software provided was written by Sanko Robinson.

	Please visit http://sanko.googlecode.com/ for updates.

DESCRIPTION

	Inspired by the TCL4mIRC[1] and Python4mIRC[2] projects, Perl4mIRC is
	a DLL for the mIRC chat client[3] that allows a scripter to execute Perl
	programs from mIRC's edit box and in mIRC's msl script files.

REQUIREMENTS

	any version of mIRC (built on v6.21)
	Perl (built on v5.10.0)
	Win32::API

INSTALLATION

	To install this script and DLL, it is recommended (but not necessary)
	to copy the files in this package into a directory within mIRC's
	installation directory. At that point you can type

		/load -rs C:\path\to\perl.mrc

	To load the script file. This will run /perl_hello_world to test the
	installation. You should see "Hello world" if the test ran successfully.

	If you receive an error about Win32::API, type the following at the
	command line:

	   ppm install http://www.bribes.org/perl/ppm/Win32-API.ppd

USAGE & EXAMPLES

	Use /perl <perl syntax> to execute Perl code.

	Several examples are in perl.mrc, I'll explain the nifty bits...

	Midway through perl.mirc, you come upon the following alias...

                ; Shows how to pass data to and from Perl
                alias perl_strlen {
                   set %data $1-
                   perl mIRC(q[//echo len:] . length(mIRC->{'data'}));
		   unset %data
                }

	We use the pseudo-hash mIRC to get and set variables inside mIRC and the
	coderef mIRC executes msl inline. You could toss down a banana peel with:

		/perl use strict; mIRC->{here} = q[TEST];

	No, it won't die, but it breaks my little fake hash/coderef thing. So,
	don't.

	Yes, this is very very misleading in code and a very very bad idea in
	general practice but until I find a solution as fast and light as this,
	I'll make use of it. See perldoc perlref to investigate on your own.
	...or contact me and I'll do my best to fill you in.

        Near the bottom of perl.mrc, you'll find this...

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

	Yep, C. Inside mIRC. This is made possible with the very crafty
	Inline::C[4] module available on CPAN[5], you'll find several Inline
	modules that evaluate assembler[6], Java[7], Lua[8], Python[9], Ruby[10],
	Tcl[11], and several other[12] languages. Please note, several of these
	Inline:: modules (Inline::C for sure) require you to install the script
	to a path WITHOUT spaces. And PLEASE read the docs for these before
	jumping into it... You'll save yourself some time and effort.

RELEASE INFORMATION

	See CHANGES.txt

	For future updates, check http://sanko.googlecode.com/

LICENSES

	Perl4mIRC is released under the Perl/Artistic license. See LICENSE.txt
	for a very legalese definition of what I'm talking about. To understand
	what rights I claim to this code and how to handle derivative work, see
	the Artistic 2.0 Notes[13].

	All textual content is provided under the Creative Commons Attribution-
	Share Alike 3.0 United States License[14] as all documentation should
	be.

	Now that you're completely confused, you can ask me any time to clarify
	my licensing choices.

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
