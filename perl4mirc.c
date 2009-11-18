#define PERL_NO_GET_CONTEXT 1
#include <EXTERN.h>
#include <perl.h>
#include <perliol.h>
#define NO_XSLOCKS /* XSUB.h will otherwise override various things we need */
#include <XSUB.h>
#define NEED_sv_2pv_flags
#include "patchlevel.h" /* for local_patches */

const char * VERSION   = "1.0";
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
    HWND     mHwnd;
    BOOL     mKeep;
} LOADINFO;

static PerlInterpreter * my_perl = NULL;

HWND mWnd;
BOOL loaded;
HANDLE hMapFile;
LPSTR mData;

EXTERN_C void xs_init ( pTHX );
EXTERN_C void boot_DynaLoader  ( pTHX_ CV* cv );
EXTERN_C void boot_Win32CORE   ( pTHX_ CV* cv );
EXTERN_C void XS_mIRC_evaluate ( pTHX_ CV* cv );
EXTERN_C void XS_mIRC_execute  ( pTHX_ CV* cv );

EXTERN_C void xs_init( pTHX ) {
    PERL_UNUSED_CONTEXT;
    char * file = __FILE__;
    dXSUB_SYS;
    /* DynaLoader is a special case; Win32 is a special m[h?ea?d] case */
    ( void )newXS( "DynaLoader::boot_DynaLoader", boot_DynaLoader,  file );
    ( void )newXS( "Win32CORE::bootstrap",        boot_Win32CORE,   file );
    ( void )newXS( "mIRC::evaluate",              XS_mIRC_evaluate, file );
    ( void )newXS( "mIRC::execute",               XS_mIRC_execute,  file );
}

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

SSize_t PerlIOmIRC_read( pTHX_ PerlIO *f, void *vbuf, Size_t count ) {
    STDCHAR *buf = ( STDCHAR * ) vbuf;
    if ( f ) {
        if ( !( PerlIOBase( f )->flags & PERLIO_F_CANREAD ) ) {
            PerlIOBase( f )->flags |= PERLIO_F_ERROR;
            SETERRNO( EBADF, SS_IVCHAN );
            return 0;
        }
        while ( count > 0 ) {
get_cnt: {
                SSize_t avail = PerlIO_get_cnt( f );
                SSize_t take = 0;
                if ( avail > 0 )
                    take = ( ( SSize_t )count < avail ) ? ( SSize_t )count : avail;
                if ( take > 0 ) {
                    STDCHAR *ptr = PerlIO_get_ptr( f );
                    Copy( ptr, buf, take, STDCHAR );
                    PerlIO_set_ptrcnt( f, ptr + take, ( avail -= take ) );
                    count -= take;
                    buf += take;
                    if ( avail == 0 )  /* set_ptrcnt could have reset avail */
                        goto get_cnt;
                }
                if ( count > 0 && avail <= 0 ) {
                    if ( PerlIO_fill( f ) != 0 )
                        break;
                }
            }
        }
        return ( buf - ( STDCHAR * ) vbuf );
    }
    return 0;
}

SSize_t PerlIOmIRC_write( pTHX_ PerlIO * f, const void *vbuf, Size_t count ) {
    PerlIOmIRC * e = PerlIOSelf( f, PerlIOmIRC );
    AV * av = newAV();
    const char * fh = "UNKNOWN";
    if      ( f == PerlIO_stdin( ) )
        fh = "STDIN"; /* Should never get write */
    else if ( f == PerlIO_stdout( ) )
        fh = "STDOUT";
    else if ( f == PerlIO_stderr( ) )
        fh = "STDERR";
    mIRC_execute( form( "/.signal -n PERL_%s %s%s", fh,
                        ( isdigit( *( const char * )vbuf ) ? "﻿ " : "" ),
                        vbuf ) );
    return count;
}

PERLIO_FUNCS_DECL( PerlIO_mIRC ) = {
    sizeof( PerlIO_funcs ),
    "mIRC",
    sizeof( PerlIOmIRC ),
    PERLIO_K_RAW,
    PerlIOBase_pushed,
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
     * Set up the perl environment, push arguments onto the perl stack, then
     * call the given function
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
     * Check for "die," make sure we have 1 argument, and set our return value
     */
    if ( SvTRUE( ERRSV ) ) {
        sprintf( data,
                 "%sPerl function (%s) exited abnormally: %s",
                 ( loaded ? "ERR " : "" ), function, SvPV( ERRSV, na ) );
        ( void )POPs;
    }
    else if ( count != 1 ) {
        /*
         * This should NEVER happen. G_SCALAR ensures that we WILL have 1
         * parameter
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
int __declspec( dllexport ) __stdcall LoadDll( LOADINFO * limIRC ) {
    mWnd = limIRC->mHwnd;
    limIRC->mKeep = TRUE; // TODO: Set to FALSE if the inline perl fails

    if ( my_perl == NULL ) {
        /* Get things set for mIRC<=>perl IO */
        hMapFile = CreateFileMapping( INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 4096, NAMESPACE );
        mData = ( LPSTR )MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
        /* Create our persistant interpreter */
        char * perl_args[] = { "", "-e", "", "0" };
        PERL_SYS_INIT3( NULL, NULL, NULL );
        if ( ( my_perl = perl_alloc() ) == NULL ) {
            mIRC_execute( "/echo Failed to load DLL: No memory" ); /* TODO: make this an error message */
            limIRC->mKeep = FALSE;
            return 0;
        }
        perl_construct( my_perl );
        PL_origalen = 1; /* Don't let $0 assignment update the proctitle or perl_args[0] */
        perl_parse( my_perl, xs_init, 6, perl_args, NULL );
        PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
        perl_run( my_perl );
        {
#ifdef PERLIO_LAYERS
            PerlIO_define_layer( aTHX_ PERLIO_FUNCS_CAST( &PerlIO_mIRC ) );
            PerlIO_apply_layers( aTHX_ PerlIO_stderr( ), NULL, ":mIRC" );
            PerlIO_apply_layers( aTHX_ PerlIO_stdout( ), NULL, ":mIRC" );
#endif
        }
        SV * result = eval_pv( form(
                                   "use FindBin;"                    /* CORE */
                                   "use lib qq[$FindBin::Bin/lib];"  /* Search %mIRC%/lib for modules */
                                   "use lib qq[$FindBin::Bin/perl];" /* Look for modules in %mIRC%/perl */
                                   "my $mIRC = bless \{ }, 'mIRC';"
                                   "*mIRC = *mIRC = %mIRC = $mIRC;"
                                   "require mIRC;"
                                   "$mIRC::VERSION = '%s'", VERSION ), FALSE );
        if ( SvTRUE( ERRSV ) ) {
            char * err;
            sprintf( err, "/echo Failed to load DLL: %s", SvPVx_nolen ( ERRSV ) );/* TODO: make this an error message */
            mIRC_execute( err );
            limIRC->mKeep = FALSE;
            return 0;
        }
        mIRC_execute( "/.signal -n PERL_ONLOAD" );
        loaded = SvTRUE( ERRSV ) ? FALSE : TRUE;
        PERL_SET_CONTEXT( my_perl );
        perl_run( my_perl );
    }
    PERL_SET_CONTEXT( my_perl );
    return 0;
}

int __declspec( dllexport ) __stdcall UnloadDll( int mTimeout ) {
    SV* result = eval_pv( /* auto clean */
                     "foreach my $lib ( @DynaLoader::dl_modules ) {\n"
                     "   if ( $lib =~ m[^mIRC::eval::\\d+$] ) {\n"
                     "       $lib .= q[->deinit();];\n"
                     "       eval $lib;\n"
                     "   }\n" /* TODO: delete the packages? */
                     "}", FALSE );

    if ( mTimeout == 0 ) { /* user called /dll -u */
        if ( my_perl == NULL )
            return 0;
        PL_perl_destruct_level = 1;
        PERL_SET_CONTEXT( my_perl );
        perl_destruct( my_perl );
        perl_free( my_perl );
        my_perl = NULL;
        mIRC_execute( "/.signal -n PERL_UNLOAD" );
        UnmapViewOfFile( mData );
        CloseHandle( hMapFile );
    }
    return 0;
}

#ifdef __cplusplus
extern "C"
#endif
    int __stdcall version (
        HWND   mWnd,  HWND   aWnd,
        char * data,  char * parms,
        BOOL   print, BOOL   nopause
    ) {
    sprintf(
        data, "perl4mIRC v%s by Sanko Robinson <sanko@cpan.org>", VERSION );
    return 3;
}

#ifdef __cplusplus
extern "C"
#endif
    int __stdcall perl_eval_string (
        HWND   mWnd,  HWND   aWnd,
        char * data,  char * parms,
        BOOL   print, BOOL   nopause
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

    if ( my_perl == NULL )
        return 0; /* Halt */
    char * package = form( "mIRC::eval::%d", rand( ) );
    PERL_SET_CONTEXT( my_perl );
    eval_pv( form( "{package %s;\nmy$mIRC=bless\{},'mIRC';*mIRC=*mIRC=%mIRC=$mIRC;\n#line 1 mIRC_eval\n%s}", package, data ), FALSE );
    if ( ! SvTRUE( ERRSV ) )
        return 1;
    /* TODO: make this an error message */
    mIRC_execute( form( "/echo %s", SvPVx_nolen ( ERRSV ) ) );
    return 0; /* Halt */

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

XS( XS_mIRC_execute );
XS( XS_mIRC_execute ) {
#ifdef dVAR
    dVAR;
    dXSARGS;
#else
    dXSARGS;
#endif
    if ( items != 2 )
        croak( "Usage: mIRC->testing( snippet" );
    {
        const char * snippet = ( const char * )SvPV_nolen( ST( 1 ) );
        mIRC_execute( snippet );
    }
    XSRETURN_EMPTY;
}

XS( XS_mIRC_evaluate );
XS( XS_mIRC_evaluate ) {
#ifdef dVAR
    dVAR;
    dXSARGS;
#else
    dXSARGS;
#endif
    if ( items != 2 )
        croak( "Usage: mIRC->evaluate( variable" );
    {
        const char * RETVAL;
        dXSTARG;
        const char * variable = ( const char * )SvPV_nolen( ST( 1 ) );
        RETVAL = mIRC_evaluate( variable );
        sv_setpv( TARG, RETVAL );
        XSprePUSH;
        PUSHTARG;
    }
    XSRETURN( 1 );
}

XS( XS_mIRC_VERSION );
XS( XS_mIRC_VERSION ) {
#ifdef dVAR
    dVAR;
    dXSARGS;
#else
    dXSARGS;
#endif
    {
        dXSTARG;
        sv_setpv( TARG, VERSION );
        XSprePUSH;
        PUSHTARG;
    }
    XSRETURN( 1 );
}

/*

=pod

=head1 NAME

perl4mIRC

=head1 Synopsis

  /perl print 5.6 + 465

=head1 Description

Yo, dawg! We heard you like one liners so we put perl in your mIRC so
you can write Perl while on IRC!

=head1 To Do

...plenty.

=over

=item Cache packages according to perlembed

=item bugfixes?

=item See inline TODO comments in perl4mIRC.c

=item handle $ identifiers

=back

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

=cut

*/
