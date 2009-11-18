AUTHOR & WEBSITE

	The software provided was written by Sanko Robinson.

	Please visit http://github.com/sanko/perl4mirc/ for updates.

DESCRIPTION

	Inspired by the TCL4mIRC[1] and Python4mIRC[2] projects, Perl4mIRC is
	a DLL for the mIRC chat client[3] that allows a scripter to execute Perl
	programs from mIRC's edit box and in mIRC's msl script files.

REQUIREMENTS

	any version of mIRC (developed on v6.35)
	Perl (developed on v5.10.1)

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

	Midway through perl.mirc, you come upon the following alias...

		; Shows how to pass data to and from Perl
		alias perl_strlen {
			set %data $1-
			perl mIRC->echo('len: ' . length(mIRC->var('data')));
			unset %data
		}

C4mIRC

	Yep, C. Inside mIRC. This is made possible with the very crafty
	Inline::C[4] module available on CPAN[5], you'll find several Inline
	modules that evaluate ASM[6], Awk, Basic, C++, Guile, Java[7], Lua[8],
	Python[9], Ruby[10], Tcl[11], and several other[12] languages.

	For an example of C4mIRC, check out the inlinec alias in perl.mrc. Also,
	note that you may run into problems if you try this from a directory with
	spaces.

RELEASE INFORMATION

	See CHANGES.txt

	For future updates, check http://github.com/sanko/perl4mirc/

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
	[15] http://www.cpan.org/authors/id/R/RG/RGARCIA/perl-5.10.0.tar.gz
