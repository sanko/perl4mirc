#define PERL_NO_GET_CONTEXT 1
#include <EXTERN.h>
#include <perl.h>
#include <perliol.h>
#define NO_XSLOCKS /* XSUB.h will otherwise override various things we need */
#include <XSUB.h>
#define NEED_sv_2pv_flags
#include "patchlevel.h" /* for local_patches */

const char * VERSION   = "0.999.999";
static const char * NAMESPACE = "mIRC";

#define BUFFER_SIZE  4096
#define WM_MCOMMAND  WM_USER + 200
#define WM_MEVALUATE WM_USER + 201

typedef struct {
    short major;
    short minor;
} MVERSION;

typedef struct {
    MVERSION mVersion;
    HWND   mHwnd;
    BOOL   mKeep;
} LOADINFO;

static PerlInterpreter *my_perl = NULL;

HWND mWnd;
BOOL loaded;
HANDLE hMapFile;
LPSTR mData;

EXTERN_C void xs_init ( pTHX );
EXTERN_C void boot_DynaLoader ( pTHX_ CV* cv );
EXTERN_C void boot_Win32CORE ( pTHX_ CV* cv );

EXTERN_C void xs_init( pTHX ) {
    PERL_UNUSED_CONTEXT;
    char *file = __FILE__;
    dXSUB_SYS;
    /* DynaLoader is a special case; Win32 is a special m[h?ea?d] case */
    newXS( "DynaLoader::boot_DynaLoader", boot_DynaLoader, file );

void
mIRC_execute ( const char * snippet ) {
    wsprintf( mData, snippet );
    SendMessage( mWnd, WM_MCOMMAND, ( WPARAM ) NULL, ( LPARAM ) NULL );
    return;
}

const char *
mIRC_evaluate ( const char * variable ) {
    lstrcpy( mData, variable );
    return SendMessage( mWnd, WM_MEVALUATE, ( WPARAM ) NULL, ( LPARAM ) NULL ) ?
           mData : "";
}

#ifdef PERLIO_LAYERS

#include "perliol.h"

typedef struct {
    struct _PerlIO base; /* Base "class" info */
    SV     *       var;
    SV     *       arg;
    Off_t          posn;
} PerlIOmIRC;

void
    newXS( "Win32CORE::bootstrap", boot_Win32CORE, file );
}

PERLIO_FUNCS_DECL( PerlIO_mIRC ) = {
    sizeof( PerlIO_funcs ),
    "mIRC",
    sizeof( PerlIOmIRC ),
    PERLIO_K_RAW,
    PerlIOmIRC_pushed,
    PerlIOBase_popped,
    NULL, /* PerlIOmIRC_open */
    NULL, /* PerlIOBase_binmode */
    NULL, /* PerlIOmIRC_arg */
    NULL, /* PerlIOmIRC_fileno */
    NULL, /* PerlIOBase_dup */
    PerlIOmIRC_read,
    NULL, /* PerlIOmIRC_unread */
    PerlIOmIRC_write,
    NULL, /* PerlIOmIRC_seek */
    NULL, /* PerlIOmIRC_tell */
    NULL, /* PerlIOBase_close */
    NULL, /* PerlIOmIRC_flush */
    NULL, /* PerlIOmIRC_fill */
    NULL, /* PerlIOBase_eof */
    NULL, /* PerlIOBase_error */
    NULL, /* PerlIOBase_clearerr */
    NULL, /* PerlIOBase_setlinebuf */
    NULL, /* PerlIOmIRC_get_base */
    NULL, /* PerlIOmIRC_bufsiz */
    NULL, /* PerlIOmIRC_get_ptr */
    NULL, /* PerlIOmIRC_get_cnt */
    NULL /* PerlIOmIRC_set_ptrcnt */
};

#endif /* Layers available */

int execute_perl( const char *function, char **args, char *data ) {
    int count = 0, i, ret_value = 1;
    STRLEN na;
    SV *sv_args[0];
    dSP;
    PERL_SET_CONTEXT( my_perl );
    /*
     * Set up the perl environment, push arguments onto the
     * perl stack, then call the given function
     */
    SPAGAIN;
    ENTER;
    SAVETMPS;
    PUSHMARK( sp );
    for ( i = 0; i < ( int )sizeof( args ) - 1; i++ ) {
        if ( args[i] != NULL ) {
            sv_args[i] = sv_2mortal( newSVpv( args[i], 0 ) );
            XPUSHs( sv_args[i] );
        }
    }
    PUTBACK;
    PERL_SET_CONTEXT( my_perl );
    count = call_pv( function, G_EVAL | G_SCALAR );
    SPAGAIN;
    /*
     * Check for "die," make sure we have 1 argument, and set our
     * return value.
     */
    if ( SvTRUE( ERRSV ) ) {
        sprintf( data,
                 "%sPerl function (%s) exited abnormally: %s",
                 ( loaded ? "ERR " : "" ), function, SvPV( ERRSV, na ) );
        ( void )POPs;
    }
    else if ( count != 1 ) {
        /*
         * This should NEVER happen.  G_SCALAR ensures that we WILL
         * have 1 parameter.
         */
        sprintf( data,
                 "%sPerl error executing '%s': expected 1 return value; received %s",
                 ( loaded ? "ERR " : "" ), function, count );
    }
    else {
        sprintf( data, "%s%s", ( loaded ? "OK " : "" ), POPpx );
    }
    /* Check for changed arguments */
    for ( i = 0; i < ( int )sizeof( args ) - 1; i++ ) {
        if ( args[i] && strcmp( args[i], SvPVX( sv_args[i] ) ) ) {
            args[i] = strdup( SvPV( sv_args[i], na ) );
        }
    }
    PUTBACK;
    FREETMPS;
    LEAVE;
    return ret_value;
}

// Get everything going...
int __declspec( dllexport ) __stdcall LoadDll( LOADINFO *mIRC ) {
    if ( my_perl == NULL ) {
        mWnd = mIRC->mHwnd;
        mIRC->mKeep = TRUE; // TODO: Set to FALSE if the inline perl fails
        char *atmp[3] = { NULL, NULL, NULL };
        char sWnd[20];
        sprintf( sWnd, "%i", mWnd );
        atmp[0] = sWnd;
        if ( my_perl == NULL ) {
            char *perl_args[] = { "", "-e", "", "0", "", "-w" };
            PERL_SYS_INIT3( NULL, NULL, NULL );
            if ( ( my_perl = perl_alloc() ) == NULL ) {
                MessageBox( 0, "No memory!",
                            "Cannot load DLL!" , MB_ICONSTOP );
                mIRC->mKeep = FALSE;
                return 0;
            }
            perl_construct( my_perl );
            perl_parse( my_perl, xs_init, 6, perl_args, NULL );
            PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
            perl_run( my_perl );

            // make sure W::A is actually installed
            SV* result = eval_pv( "require Win32::API;", FALSE );
            if ( ! SvTRUE( ERRSV ) ) {
                SV* result = eval_pv(
                                 "use FindBin;"
                                 "use lib qq[$FindBin::Bin/lib];"
                                 "use lib qq[$FindBin::Bin/perl];"
                                 "perl4mIRC->import() if eval 'require perl4mIRC';",
                                 FALSE );
            }
            else {
                MessageBox( 0, "Please install Win32::API from CPAN. See Readme.txt for more.",
                            "Missing dependancy!" , MB_ICONSTOP );
                SV* result = eval_pv(
                                 "*perl4mIRC::eval_string = sub {eval shift};",
                                 FALSE );
                // TODO: let the user know that things have gone wrong without
                //       being too disruptive
            }

            if ( SvTRUE( ERRSV ) )
                loaded = FALSE;
            else
                loaded = TRUE;
            PERL_SET_CONTEXT( my_perl );
            perl_run( my_perl );
        }
        PERL_SET_CONTEXT( my_perl );
        char data[1024]; // waste...
        execute_perl( "perl4mIRC::init", atmp, data );
    }
    return 0;
}
int __declspec( dllexport ) __stdcall UnloadDll( int mTimeout ) {
    if ( mTimeout == 0 ) { /* user called /dll -u*/  }
    if ( my_perl == NULL )
        return 0;
    PL_perl_destruct_level = 1;
    PERL_SET_CONTEXT( my_perl );
    SV* result = eval_pv(
                     "foreach my $lib (@DynaLoader::dl_modules) {"
                     "  if ($lib =~ m[^perl4mIRC::]) {"
                     "    $lib .= q[::deinit();];"
                     "    eval $lib;"
                     "  }"
                     "}"
                     "perl4mIRC::deinit();",
                     FALSE );
    PL_perl_destruct_level = 1;
    PERL_SET_CONTEXT( my_perl );
    perl_destruct( my_perl );
    perl_free( my_perl );
    my_perl = NULL;
    return 0;
}

int __declspec( dllexport ) __stdcall version (
    HWND mWnd,   HWND aWnd,
    char *data, char *parms,
    BOOL print,  BOOL nopause
) {
    sprintf(
        data, "perl4mIRC v%s by Sanko Robinson <sanko@cpan.org>", VERSION );
    return 3;
}

int __declspec( dllexport ) __stdcall perl_eval_string (
    HWND mWnd,   HWND aWnd,
    char *data, char *parms,
    BOOL print,  BOOL nopause
) { /* ...what is this junk? Oh, it's...
     * mWnd    - the handle to the main mIRC window.
     * aWnd    - the handle of the window in which the command is being issued,
     *             this might not be the currently active window if the command
     *             is being called by a remote script.
     * data    - the information that you wish to send to the DLL. On return,
     *             the DLL can fill this variable with the command it wants
     *             mIRC to perform if any.
     * parms   - filled by the DLL on return with parameters that it wants mIRC
     *             to use when performing the command that it returns in the
     *             data variable.
     *           Note: The data and parms variables can each hold 900 chars
     *             maximum.
     * show    - FALSE if the . prefix was specified to make the command quiet,
     *            or TRUE otherwise.
     * nopause - TRUE if mIRC is in a critical routine and the DLL must not do
     *            anything that pauses processing in mIRC, eg. the DLL should
     *            not pop up a dialog.
     *
     *  We basically ignore the majority of these which is just simply wrong.
     *  This WILL change in the future.
     */
    if ( my_perl == NULL ) {
        return 0;
    }
    char *package;
    sprintf( package, "perl4mIRC::Eval::%d", rand() ); // TODO - generate in perl4mIRC?
    char *atmp[3] = { data, package, NULL };
    PERL_SET_CONTEXT( my_perl );
    execute_perl( "perl4mIRC::eval_string", atmp, data );
    return 3;
    /* We can return an integer to indicate what we want mIRC to do:
     * 0 means that mIRC should /halt processing
     * 1 means that mIRC should continue processing
     * 2 means that we have filled the data variable with a command which mIRC
     *   should perform and we filled parms with the parameters to use, if any,
     *   when performing the command.
     * 3 means that the DLL has filled the data variable with the result that
     *   $dll() as an identifier should return.
     *
     * For now, we always return 3. This may change in future.
     */
}

/*

=pod

=head1 NAME

perl4mIRC

=head1 Synopsis

    ; From mIRC
    //echo $perl(5.6 + 456)

    ; With the included perl4mIRC module;

=head1 Description

Yo, dawg! We heard you like one liners so we put perl in your mIRC so
you can write Perl while on IRC!

=head1 To Do

...plenty.

=over

=item Cache packages according to perlembed

=item bugfixes?

=item See inline TODO comments in perl4mirc.c

=item handle $ identifiers

=back

=head 1 Author

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

=for svn $Id: perl4mirc.c 4 2008-12-05 05:12:08Z sanko@cpan.org $

=cut

*/
