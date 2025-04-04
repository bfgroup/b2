/*
 * Copyright 1993-2002 Christopher Seiwald and Perforce Software, Inc.
 *
 * This file is part of Jam - see jam.c for Copyright information.
 */

#include "jam.h"
#include "builtins.h"

#include "compile.h"
#include "constants.h"
#include "cwd.h"
#include "debugger.h"
#include "filesys.h"
#include "frames.h"
#include "hash.h"
#include "hdrmacro.h"
#include "lists.h"
#include "make.h"
#include "md5.h"
#include "native.h"
#include "object.h"
#include "parse.h"
#include "pathsys.h"
#include "regexp.h"
#include "rules.h"
#include "jam_strings.h"
#include "startup.h"
#include "timestamp.h"
#include "variable.h"
#include "output.h"

#include <string>

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef OS_NT
#include <windows.h>
#ifndef FSCTL_GET_REPARSE_POINT
/* MinGW's version of windows.h is missing this, so we need
 * to include winioctl.h directly
 */
#include <winioctl.h>
#endif

/* With VC8 (VS2005) these are not defined:
 *   FSCTL_GET_REPARSE_POINT  (expects WINVER >= 0x0500 _WIN32_WINNT >= 0x0500 )
 *   IO_REPARSE_TAG_SYMLINK   (is part of a separate Driver SDK)
 * So define them explicitly to their expected values.
 */
#ifndef FSCTL_GET_REPARSE_POINT
# define FSCTL_GET_REPARSE_POINT 0x000900a8
#endif
#ifndef IO_REPARSE_TAG_SYMLINK
# define IO_REPARSE_TAG_SYMLINK	(0xA000000CL)
#endif

#include <io.h>
#if !defined(__BORLANDC__)
#define dup _dup
#define dup2 _dup2
#define open _open
#define close _close
#endif /* __BORLANDC__ */
#endif /* OS_NT */

#if defined(USE_EXECUNIX)
# include <sys/types.h>
# include <sys/wait.h>
#elif defined(OS_VMS)
# include <wait.h>
#else
/*
 * NT does not have wait() and associated macros and uses the system() return
 * value instead. Status code group are documented at:
 * http://msdn.microsoft.com/en-gb/library/ff565436.aspx
 */
# define WIFEXITED(w)  (((w) & 0XFFFFFF00) == 0)
# define WEXITSTATUS(w)(w)
#endif

/*
 * builtins.c - builtin jam rules
 *
 * External routines:
 *  load_builtins()               - define builtin rules
 *  unknown_rule()                - reports an unknown rule occurrence to the
 *                                  user and exits
 *
 * Internal routines:
 *  append_if_exists()            - if file exists, append it to the list
 *  builtin_calc()                - CALC rule
 *  builtin_delete_module()       - DELETE_MODULE ( MODULE ? )
 *  builtin_depends()             - DEPENDS/INCLUDES rule
 *  builtin_echo()                - ECHO rule
 *  builtin_exit()                - EXIT rule
 *  builtin_export()              - EXPORT ( MODULE ? : RULES * )
 *  builtin_flags()               - NOCARE, NOTFILE, TEMPORARY rule
 *  builtin_glob()                - GLOB rule
 *  builtin_glob_recursive()      - ???
 *  builtin_hdrmacro()            - ???
 *  builtin_import()              - IMPORT rule
 *  builtin_match()               - MATCH rule, regexp matching
 *  builtin_rebuilds()            - REBUILDS rule
 *  builtin_rulenames()           - RULENAMES ( MODULE ? )
 *  builtin_split_by_characters() - splits the given string into tokens
 *  builtin_varnames()            - VARNAMES ( MODULE ? )
 *  get_source_line()             - get a frame's file and line number
 *                                  information
 */


/*
 * compile_builtin() - define builtin rules
 */

#define P0 (PARSE *)0
#define C0 (OBJECT *)0

#if defined( OS_NT ) || defined( OS_CYGWIN )
    LIST * builtin_system_registry      ( FRAME *, int );
    LIST * builtin_system_registry_names( FRAME *, int );
#endif

int glob( char const * s, char const * c );

void backtrace        ( FRAME * );
void backtrace_line   ( FRAME * );
void print_source_line( FRAME * );


RULE * bind_builtin( char const * name_, LIST * (* f)( FRAME *, int flags ),
    int flags, char const * * args )
{
    FUNCTION * func;
    RULE * result;
    OBJECT * name = object_new( name_ );

    func = function_builtin( f, flags, args );

    result = new_rule_body( root_module(), name, func, 1 );

    function_free( func );

    object_free( name );

    return result;
}


RULE * duplicate_rule( char const * name_, RULE * other )
{
    OBJECT * name = object_new( name_ );
    RULE * result = import_rule( other, root_module(), name );
    object_free( name );
    return result;
}


/*
 *  load_builtins() - define builtin rules
 */

void load_builtins()
{
    duplicate_rule( "Always",
      bind_builtin( "ALWAYS",
                    builtin_flags, T_FLAG_TOUCHED, 0 ) );

    duplicate_rule( "Depends",
      bind_builtin( "DEPENDS",
                    builtin_depends, 0, 0 ) );

    duplicate_rule( "echo",
    duplicate_rule( "Echo",
      bind_builtin( "ECHO",
                    builtin_echo, 0, 0 ) ) );

    {
        char const * args[] = { "message", "*", ":", "result-value", "?", 0 };
        duplicate_rule( "exit",
        duplicate_rule( "Exit",
          bind_builtin( "EXIT",
                        builtin_exit, 0, args ) ) );
    }

    {
        char const * args[] = { "directories", "*", ":", "patterns", "*", ":",
            "case-insensitive", "?", 0 };
        duplicate_rule( "Glob",
                        bind_builtin( "GLOB", builtin_glob, 0, args ) );
    }

    {
        char const * args[] = { "patterns", "*", 0 };
        bind_builtin( "GLOB-RECURSIVELY",
                      builtin_glob_recursive, 0, args );
    }

    duplicate_rule( "Includes",
      bind_builtin( "INCLUDES",
                    builtin_depends, 1, 0 ) );

    {
        char const * args[] = { "targets", "*", ":", "targets-to-rebuild", "*",
            0 };
        bind_builtin( "REBUILDS",
                      builtin_rebuilds, 0, args );
    }

    duplicate_rule( "Leaves",
      bind_builtin( "LEAVES",
                    builtin_flags, T_FLAG_LEAVES, 0 ) );

    duplicate_rule( "Match",
      bind_builtin( "MATCH",
                    builtin_match, 0, 0 ) );

    {
        char const * args[] = { "string", ":", "delimiters", 0 };
        bind_builtin( "SPLIT_BY_CHARACTERS",
                      builtin_split_by_characters, 0, args );
    }

    duplicate_rule( "NoCare",
      bind_builtin( "NOCARE",
                    builtin_flags, T_FLAG_NOCARE, 0 ) );

    duplicate_rule( "NOTIME",
    duplicate_rule( "NotFile",
      bind_builtin( "NOTFILE",
                    builtin_flags, T_FLAG_NOTFILE, 0 ) ) );

    duplicate_rule( "NoUpdate",
      bind_builtin( "NOUPDATE",
                    builtin_flags, T_FLAG_NOUPDATE, 0 ) );

    duplicate_rule( "Temporary",
      bind_builtin( "TEMPORARY",
                    builtin_flags, T_FLAG_TEMP, 0 ) );

      bind_builtin( "ISFILE",
                    builtin_flags, T_FLAG_ISFILE, 0 );

    duplicate_rule( "HdrMacro",
      bind_builtin( "HDRMACRO",
                    builtin_hdrmacro, 0, 0 ) );

    /* FAIL_EXPECTED is used to indicate that the result of a target build
     * action should be inverted (ok <=> fail) this can be useful when
     * performing test runs from Jamfiles.
     */
    bind_builtin( "FAIL_EXPECTED",
                  builtin_flags, T_FLAG_FAIL_EXPECTED, 0 );

    bind_builtin( "RMOLD",
                  builtin_flags, T_FLAG_RMOLD, 0 );

    {
        char const * args[] = { "targets", "*", 0 };
        bind_builtin( "UPDATE",
                      builtin_update, 0, args );
    }

    {
        char const * args[] = { "targets", "*",
                            ":", "log", "?",
                            ":", "ignore-minus-n", "?",
                            ":", "ignore-minus-q", "?", 0 };
        bind_builtin( "UPDATE_NOW",
                      builtin_update_now, 0, args );
    }

    {
        char const * args[] = { "string", "pattern", "replacements", "+", 0 };
        duplicate_rule( "subst",
          bind_builtin( "SUBST",
                        builtin_subst, 0, args ) );
    }

    {
        char const * args[] = { "module", "?", 0 };
        bind_builtin( "RULENAMES",
                       builtin_rulenames, 0, args );
    }

    {
        char const * args[] = { "module", "?", 0 };
        bind_builtin( "VARNAMES",
                       builtin_varnames, 0, args );
    }

    {
        char const * args[] = { "module", "?", 0 };
        bind_builtin( "DELETE_MODULE",
                       builtin_delete_module, 0, args );
    }

    {
        char const * args[] = { "source_module", "?",
                            ":", "source_rules", "*",
                            ":", "target_module", "?",
                            ":", "target_rules", "*",
                            ":", "localize", "?", 0 };
        bind_builtin( "IMPORT",
                      builtin_import, 0, args );
    }

    {
        char const * args[] = { "module", "?", ":", "rules", "*", 0 };
        bind_builtin( "EXPORT",
                      builtin_export, 0, args );
    }

    {
        char const * args[] = { "levels", "?", 0 };
        bind_builtin( "CALLER_MODULE",
                       builtin_caller_module, 0, args );
    }

    {
        char const * args[] = { "levels", "?", 0 };
        bind_builtin( "BACKTRACE",
                      builtin_backtrace, 0, args );
    }

    {
        char const * args[] = { 0 };
        bind_builtin( "PWD",
                      builtin_pwd, 0, args );
    }

    {
        char const * args[] = { "modules_to_import", "+",
                            ":", "target_module", "?", 0 };
        bind_builtin( "IMPORT_MODULE",
                      builtin_import_module, 0, args );
    }

    {
        char const * args[] = { "module", "?", 0 };
        bind_builtin( "IMPORTED_MODULES",
                      builtin_imported_modules, 0, args );
    }

    {
        char const * args[] = { "sequence", "*", 0 };
        bind_builtin( "SORT",
                      builtin_sort, 0, args );
    }

    {
        char const * args[] = { "path_parts", "*", 0 };
        bind_builtin( "NORMALIZE_PATH",
                      builtin_normalize_path, 0, args );
    }

    {
        char const * args[] = { "args", "*", 0 };
        bind_builtin( "CALC",
                      builtin_calc, 0, args );
    }

    {
        char const * args[] = { "module", ":", "rule", 0 };
        bind_builtin( "NATIVE_RULE",
                      builtin_native_rule, 0, args );
    }

    {
        char const * args[] = { "module", ":", "rule", ":", "version", 0 };
        bind_builtin( "HAS_NATIVE_RULE",
                      builtin_has_native_rule, 0, args );
    }

    {
        char const * args[] = { "module", "*", 0 };
        bind_builtin( "USER_MODULE",
                      builtin_user_module, 0, args );
    }

    {
        char const * args[] = { 0 };
        bind_builtin( "NEAREST_USER_LOCATION",
                      builtin_nearest_user_location, 0, args );
    }

    {
        char const * args[] = { "file", 0 };
        bind_builtin( "CHECK_IF_FILE",
                      builtin_check_if_file, 0, args );
    }

# if defined( OS_NT ) || defined( OS_CYGWIN )
    {
        char const * args[] = { "key_path", ":", "data", "?", 0 };
        bind_builtin( "W32_GETREG",
                      builtin_system_registry, 0, args );
    }

    {
        char const * args[] = { "key_path", ":", "result-type", 0 };
        bind_builtin( "W32_GETREGNAMES",
                      builtin_system_registry_names, 0, args );
    }
# endif

    {
        char const * args[] = { "command", ":", "*", 0 };
        duplicate_rule( "SHELL",
          bind_builtin( "COMMAND",
                        builtin_shell, 0, args ) );
    }

    {
        char const * args[] = { "string", 0 };
        bind_builtin( "MD5",
                      builtin_md5, 0, args );
    }

    {
        char const * args[] = { "name", ":", "mode", 0 };
        bind_builtin( "FILE_OPEN",
                      builtin_file_open, 0, args );
    }

    {
        char const * args[] = { "string", ":", "width", 0 };
        bind_builtin( "PAD",
                      builtin_pad, 0, args );
    }

    {
        char const * args[] = { "targets", "*", 0 };
        bind_builtin( "PRECIOUS",
                      builtin_precious, 0, args );
    }

    {
        char const * args [] = { 0 };
        bind_builtin( "SELF_PATH", builtin_self_path, 0, args );
    }

    {
        char const * args [] = { "path", 0 };
        bind_builtin( "MAKEDIR", builtin_makedir, 0, args );
    }

    {
        const char * args [] = { "path", 0 };
        bind_builtin( "READLINK", builtin_readlink, 0, args );
    }

    {
        char const * args[] = { "archives", "*",
                                ":", "member-patterns", "*",
                                ":", "case-insensitive", "?",
                                ":", "symbol-patterns", "*", 0 };
        bind_builtin( "GLOB_ARCHIVE", builtin_glob_archive, 0, args );
    }

#ifdef JAM_DEBUGGER

	{
		const char * args[] = { "list", "*", 0 };
		bind_builtin("__DEBUG_PRINT_HELPER__", builtin_debug_print_helper, 0, args);
	}

#endif

    /* Initialize builtin modules. */
    init_property_set();
    init_sequence();
    init_order();
}


/*
 * builtin_calc() - CALC rule
 *
 * Performs simple mathematical operations on two arguments.
 */

LIST * builtin_calc( FRAME * frame, int flags )
{
    LIST * arg = lol_get( frame->args, 0 );

    LIST * result = L0;
    long lhs_value;
    long rhs_value;
    long result_value;
    char const * lhs;
    char const * op;
    char const * rhs;
    LISTITER iter = list_begin( arg );
    LISTITER const end = list_end( arg );

    if ( iter == end ) return L0;
    lhs = object_str( list_item( iter ) );

    iter = list_next( iter );
    if ( iter == end ) return L0;
    op = object_str( list_item( iter ) );

    iter = list_next( iter );
    if ( iter == end ) return L0;
    rhs = object_str( list_item( iter ) );

    lhs_value = atoi( lhs );
    rhs_value = atoi( rhs );

    if ( !strcmp( "+", op ) )
        result_value = lhs_value + rhs_value;
    else if ( !strcmp( "-", op ) )
        result_value = lhs_value - rhs_value;
    else
        return L0;

    result = list_push_back( result, b2::value::as_string(result_value) );
    return result;
}


/*
 * builtin_depends() - DEPENDS/INCLUDES rule
 *
 * The DEPENDS/INCLUDES builtin rule appends each of the listed sources on the
 * dependency/includes list of each of the listed targets. It binds both the
 * targets and sources as TARGETs.
 */

LIST * builtin_depends( FRAME * frame, int flags )
{
    LIST * const targets = lol_get( frame->args, 0 );
    LIST * const sources = lol_get( frame->args, 1 );

    LISTITER iter = list_begin( targets );
    LISTITER end = list_end( targets );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        TARGET * const t = bindtarget( list_item( iter ) );

        if ( flags )
            target_include_many( t, sources );
        else
            targetlist( t->depends, sources );
    }

    /* Enter reverse links */
    iter = list_begin( sources );
    end = list_end( sources );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        TARGET * const s = bindtarget( list_item( iter ) );
        if ( flags )
        {
            LISTITER t_iter = list_begin( targets );
            LISTITER const t_end = list_end( targets );
            for ( ; t_iter != t_end; t_iter = list_next( t_iter ) )
                targetentry( s->dependants, bindtarget(
                    list_item( t_iter ) )->includes );
        }
        else
            targetlist( s->dependants, targets );
    }

    return L0;
}


/*
 * builtin_rebuilds() - REBUILDS rule
 *
 * Appends each of the rebuild-targets listed in its second argument to the
 * rebuilds list for each of the targets listed in its first argument.
 */

LIST * builtin_rebuilds( FRAME * frame, int flags )
{
    LIST * targets = lol_get( frame->args, 0 );
    LIST * rebuilds = lol_get( frame->args, 1 );
    LISTITER iter = list_begin( targets );
    LISTITER const end = list_end( targets );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        TARGET * const t = bindtarget( list_item( iter ) );
        targetlist( t->rebuilds, rebuilds );
    }
    return L0;
}


/*
 * builtin_echo() - ECHO rule
 *
 * Echoes the targets to the user. No other actions are taken.
 */

LIST * builtin_echo( FRAME * frame, int flags )
{
    list_print( lol_get( frame->args, 0 ) );
    out_printf( "\n" );
    out_flush();
    return L0;
}


/*
 * builtin_exit() - EXIT rule
 *
 * Echoes the targets to the user and exits the program with a failure status.
 */

LIST * builtin_exit( FRAME * frame, int flags )
{
    LIST * const code = lol_get( frame->args, 1 );
    list_print( lol_get( frame->args, 0 ) );
    out_printf( "\n" );
    if ( !list_empty( code ) )
    {
        int status = atoi( object_str( list_front( code ) ) );
#ifdef OS_VMS
        switch( status )
        {
        case 0:
            status = EXITOK;
            break;
        case 1:
            status = EXITBAD;
            break;
        }
#endif
        b2::clean_exit( status );
    }
    else
        b2::clean_exit( EXITBAD );  /* yeech */
    return L0;
}


/*
 * builtin_flags() - NOCARE, NOTFILE, TEMPORARY rule
 *
 * Marks the target with the appropriate flag, for use by make0(). It binds each
 * target as a TARGET.
 */

LIST * builtin_flags( FRAME * frame, int flags )
{
    LIST * const targets = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( targets );
    LISTITER const end = list_end( targets );
    for ( ; iter != end; iter = list_next( iter ) )
        bindtarget( list_item( iter ) )->flags |= flags;
    return L0;
}


/*
 * builtin_glob() - GLOB rule
 */

struct globbing
{
    LIST * patterns;
    LIST * results;
    LIST * case_insensitive;
};


static void downcase_inplace( char * p )
{
    for ( ; *p; ++p )
        *p = tolower( *p );
}


static void builtin_glob_back( void * closure, OBJECT * file, int status,
    timestamp const * const time )
{
    PROFILE_ENTER( BUILTIN_GLOB_BACK );

    struct globbing * const globbing = (struct globbing *)closure;
    PATHNAME f;
    string buf[ 1 ];
    LISTITER iter;
    LISTITER end;

    /* Null out directory for matching. We wish we had file_dirscan() pass up a
     * PATHNAME.
     */
    path_parse( object_str( file ), &f );
    f.f_dir.len = 0;

    /* For globbing, we unconditionally ignore current and parent directory
     * items. Since these items always exist, there is no reason why caller of
     * GLOB would want to see them. We could also change file_dirscan(), but
     * then paths with embedded "." and ".." would not work anywhere.
    */
    if ( !strcmp( f.f_base.ptr, "." ) || !strcmp( f.f_base.ptr, ".." ) )
    {
        PROFILE_EXIT( BUILTIN_GLOB_BACK );
        return;
    }

    string_new( buf );
    path_build( &f, buf );

    if ( globbing->case_insensitive )
        downcase_inplace( buf->value );

    iter = list_begin( globbing->patterns );
    end = list_end( globbing->patterns );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        if ( !glob( object_str( list_item( iter ) ), buf->value ) )
        {
            globbing->results = list_push_back( globbing->results, object_copy(
                file ) );
            break;
        }
    }

    string_free( buf );

    PROFILE_EXIT( BUILTIN_GLOB_BACK );
}


static LIST * downcase_list( LIST * in )
{
    LIST * result = L0;
    LISTITER iter = list_begin( in );
    LISTITER const end = list_end( in );

    string s[ 1 ];
    string_new( s );

    for ( ; iter != end; iter = list_next( iter ) )
    {
        string_append( s, object_str( list_item( iter ) ) );
        downcase_inplace( s->value );
        result = list_push_back( result, object_new( s->value ) );
        string_truncate( s, 0 );
    }

    string_free( s );
    return result;
}


LIST * builtin_glob( FRAME * frame, int flags )
{
    LIST * const l = lol_get( frame->args, 0 );
    LIST * const r = lol_get( frame->args, 1 );

    LISTITER iter;
    LISTITER end;
    struct globbing globbing;

    globbing.results = L0;
    globbing.patterns = r;

    globbing.case_insensitive =
# if defined( OS_NT ) || defined( OS_CYGWIN ) || defined( OS_VMS )
       l;  /* Always case-insensitive if any files can be found. */
# else
       lol_get( frame->args, 2 );
# endif

    if ( globbing.case_insensitive )
        globbing.patterns = downcase_list( r );

    iter = list_begin( l );
    end = list_end( l );
    for ( ; iter != end; iter = list_next( iter ) )
        file_dirscan( list_item( iter ), builtin_glob_back, &globbing );

    if ( globbing.case_insensitive )
        list_free( globbing.patterns );

    return globbing.results;
}


static int has_wildcards( char const * const str )
{
    return str[ strcspn( str, "[]*?" ) ] ? 1 : 0;
}


/*
 * append_if_exists() - if file exists, append it to the list
 */

static LIST * append_if_exists( LIST * list, OBJECT * file )
{
    file_info_t * info = file_query( file );
    return info
        ? list_push_back( list, object_copy( info->name ) )
        : list ;
}


LIST * glob1( OBJECT * dirname, OBJECT * pattern )
{
    LIST * const plist = list_new( object_copy( pattern ) );
    struct globbing globbing;

    globbing.results = L0;
    globbing.patterns = plist;

    globbing.case_insensitive
# if defined( OS_NT ) || defined( OS_CYGWIN ) || defined( OS_VMS )
       = plist;  /* always case-insensitive if any files can be found */
# else
       = L0;
# endif

    if ( globbing.case_insensitive )
        globbing.patterns = downcase_list( plist );

    file_dirscan( dirname, builtin_glob_back, &globbing );

    if ( globbing.case_insensitive )
        list_free( globbing.patterns );

    list_free( plist );

    return globbing.results;
}


LIST * glob_recursive( char const * pattern )
{
    LIST * result = L0;

    /* Check if there's metacharacters in pattern */
    if ( !has_wildcards( pattern ) )
    {
        /* No metacharacters. Check if the path exists. */
        OBJECT * p = object_new( pattern );
        result = append_if_exists( result, p );
        object_free( p );
    }
    else
    {
        /* Have metacharacters in the pattern. Split into dir/name. */
        PATHNAME path[ 1 ];
        path_parse( pattern, path );

        if ( path->f_dir.ptr )
        {
            LIST * dirs = L0;
            string dirname[ 1 ];
            string basename[ 1 ];
            string_new( dirname );
            string_new( basename );

            string_append_range( dirname, path->f_dir.ptr,
                                path->f_dir.ptr + path->f_dir.len );

            path->f_grist.ptr = 0;
            path->f_grist.len = 0;
            path->f_dir.ptr = 0;
            path->f_dir.len = 0;
            path_build( path, basename );

            dirs =  has_wildcards( dirname->value )
                ? glob_recursive( dirname->value )
                : list_push_back( dirs, object_new( dirname->value ) );

            if ( has_wildcards( basename->value ) )
            {
                OBJECT * b = object_new( basename->value );
                LISTITER iter = list_begin( dirs );
                LISTITER const end = list_end( dirs );
                for ( ; iter != end; iter = list_next( iter ) )
                    result = list_append( result, glob1( list_item( iter ), b )
                        );
                object_free( b );
            }
            else
            {
                LISTITER iter = list_begin( dirs );
                LISTITER const end = list_end( dirs );
                string file_string[ 1 ];
                string_new( file_string );

                /* No wildcard in basename. */
                for ( ; iter != end; iter = list_next( iter ) )
                {
                    OBJECT * p;
                    path->f_dir.ptr = object_str( list_item( iter ) );
                    path->f_dir.len = int32_t(strlen( object_str( list_item( iter ) ) ));
                    path_build( path, file_string );

                    p = object_new( file_string->value );

                    result = append_if_exists( result, p );

                    object_free( p );

                    string_truncate( file_string, 0 );
                }

                string_free( file_string );
            }

            string_free( dirname );
            string_free( basename );

            list_free( dirs );
        }
        else
        {
            /* No directory, just a pattern. */
            OBJECT * p = object_new( pattern );
            result = list_append( result, glob1( constant_dot, p ) );
            object_free( p );
        }
    }

    return result;
}


/*
 * builtin_glob_recursive() - ???
 */

LIST * builtin_glob_recursive( FRAME * frame, int flags )
{
    LIST * result = L0;
    LIST * const l = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( l );
    LISTITER const end = list_end( l );
    for ( ; iter != end; iter = list_next( iter ) )
        result = list_append( result, glob_recursive( object_str( list_item(
            iter ) ) ) );
    return result;
}


LIST * builtin_subst( FRAME * frame, int flags )
{
    LIST * result = L0;
    LIST * const arg1 = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( arg1 );
    LISTITER const end = list_end( arg1 );

    if ( iter != end && list_next( iter ) != end && list_next( list_next( iter )
        ) != end )
    {
        char const * const source = object_str( list_item( iter ) );
        b2::regex::program re( list_item( list_next( iter ) )->str() );

        if ( auto re_i = re.search(source) )
        {
            LISTITER subst = list_next( iter );

            while ( ( subst = list_next( subst ) ) != end )
            {
#define BUFLEN 4096
                char buf[ BUFLEN + 1 ];
                char const * in = object_str( list_item( subst ) );
                char * out = buf;

                for ( ; *in && out < buf + BUFLEN; ++in )
                {
                    if ( *in == '\\' || *in == '$' )
                    {
                        ++in;
                        if ( *in == 0 )
                            break;
                        if ( *in >= '0' && *in <= '9' )
                        {
                            unsigned int const n = *in - '0';
                            size_t const srclen = re_i[n].size();
                            size_t const remaining = buf + BUFLEN - out;
                            size_t const len = srclen < remaining
                                ? srclen
                                : remaining;
                            memcpy( out, re_i[ n ].begin(), len );
                            out += len;
                            continue;
                        }
                        /* fall through and copy the next character */
                    }
                    *out++ = *in;
                }
                *out = 0;

                result = list_push_back( result, object_new( buf ) );
#undef BUFLEN
            }
        }
    }

    return result;
}


/*
 * builtin_match() - MATCH rule, regexp matching
 */

LIST * builtin_match( FRAME * frame, int flags )
{
    b2::list_ref result;

    string buf[ 1 ];
    string_new( buf );

    /* For each pattern, compile regex and search through strings. */
    b2::list_cref patterns( lol_get( frame->args, 0 ) );
    for ( auto pattern : patterns )
    {
        b2::regex::program re( pattern->str() );

        /* For each text string to match against. */
        b2::list_cref texts( lol_get( frame->args, 1 ) );
        for ( auto text : texts )
        {
            if ( auto re_i = re.search( text->str() ) )
            {
                /* Find highest parameter */
                int top = NSUBEXP-1;
                while ( !re_i[top].begin() ) top -= 1;
                /* And add all parameters up to highest onto list. */
                /* Must have parameters to have results! */
                for ( int i = 1; i <= top ; ++i )
                {
                    string_append_range( buf, re_i[i].begin(), re_i[i].end() );
                    result.push_back( object_new( buf->value ) );
                    string_truncate( buf, 0 );
                }
            }
        }
    }

    string_free( buf );
    return result.release();
}


/*
 * builtin_split_by_characters() - splits the given string into tokens
 */

LIST * builtin_split_by_characters( FRAME * frame, int flags )
{
    LIST * l1 = lol_get( frame->args, 0 );
    LIST * l2 = lol_get( frame->args, 1 );

    LIST * result = L0;

    string buf[ 1 ];

    char const * delimiters = object_str( list_front( l2 ) );
    char * t;

    string_copy( buf, object_str( list_front( l1 ) ) );

    t = strtok( buf->value, delimiters );
    while ( t )
    {
        result = list_push_back( result, object_new( t ) );
        t = strtok( NULL, delimiters );
    }

    string_free( buf );

    return result;
}


/*
 * builtin_hdrmacro() - ???
 */

LIST * builtin_hdrmacro( FRAME * frame, int flags )
{
    LIST * const l = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( l );
    LISTITER const end = list_end( l );

    for ( ; iter != end; iter = list_next( iter ) )
    {
        TARGET * const t = bindtarget( list_item( iter ) );

        /* Scan file for header filename macro definitions. */
        if ( is_debug_header() )
            out_printf( "scanning '%s' for header file macro definitions\n",
                object_str( list_item( iter ) ) );

        macro_headers( t );
    }

    return L0;
}


/*
 * builtin_rulenames() - RULENAMES ( MODULE ? )
 *
 * Returns a list of the non-local rule names in the given MODULE. If MODULE is
 * not supplied, returns the list of rule names in the global module.
 */

static void add_rule_name( void * r_, void * result_ )
{
    RULE * const r = (RULE *)r_;
    LIST * * const result = (LIST * *)result_;
    if ( r->exported )
        *result = list_push_back( *result, object_copy( r->name ) );
}


LIST * builtin_rulenames( FRAME * frame, int flags )
{
    LIST * arg0 = lol_get( frame->args, 0 );
    LIST * result = L0;
    module_t * const source_module = bindmodule( list_empty( arg0 )
        ? 0
        : list_front( arg0 ) );

    if ( source_module->rules )
        hashenumerate( source_module->rules, add_rule_name, &result );
    return result;
}


/*
 * builtin_varnames() - VARNAMES ( MODULE ? )
 *
 * Returns a list of the variable names in the given MODULE. If MODULE is not
 * supplied, returns the list of variable names in the global module.
 */

/* helper function for builtin_varnames(), below. Used with hashenumerate, will
 * prepend the key of each element to the list
 */
static void add_hash_key( void * np, void * result_ )
{
    LIST * * result = (LIST * *)result_;
    *result = list_push_back( *result, object_copy( *(OBJECT * *)np ) );
}


LIST * builtin_varnames( FRAME * frame, int flags )
{
    LIST * arg0 = lol_get( frame->args, 0 );
    LIST * result = L0;
    module_t * source_module = bindmodule( list_empty( arg0 )
        ? 0
        : list_front( arg0 ) );

    struct hash * const vars = source_module->variables;
    if ( vars )
        hashenumerate( vars, add_hash_key, &result );
    return result;
}


/*
 * builtin_delete_module() - DELETE_MODULE ( MODULE ? )
 *
 * Clears all rules and variables from the given module.
 */

LIST * builtin_delete_module( FRAME * frame, int flags )
{
    LIST * const arg0 = lol_get( frame->args, 0 );
    module_t * const source_module = bindmodule( list_empty( arg0 ) ? 0 :
        list_front( arg0 ) );
    delete_module( source_module );
    return L0;
}


/*
 * unknown_rule() - reports an unknown rule occurrence to the user and exits
 */

void unknown_rule( FRAME * frame, char const * key, module_t * module,
    OBJECT * rule_name )
{
    backtrace_line( frame->prev );
    if ( key )
        out_printf("%s error", key);
    else
        out_printf("ERROR");
    out_printf( ": rule \"%s\" unknown in ", object_str( rule_name ) );
    if ( module->name )
        out_printf( "module \"%s\".\n", object_str( module->name ) );
    else
        out_printf( "root module.\n" );
    backtrace( frame->prev );
    b2::clean_exit( EXITBAD );
}


/*
 * builtin_import() - IMPORT rule
 *
 * IMPORT
 * (
 *     SOURCE_MODULE ? :
 *     SOURCE_RULES  * :
 *     TARGET_MODULE ? :
 *     TARGET_RULES  * :
 *     LOCALIZE      ?
 * )
 *
 * Imports rules from the SOURCE_MODULE into the TARGET_MODULE as local rules.
 * If either SOURCE_MODULE or TARGET_MODULE is not supplied, it refers to the
 * global module. SOURCE_RULES specifies which rules from the SOURCE_MODULE to
 * import; TARGET_RULES specifies the names to give those rules in
 * TARGET_MODULE. If SOURCE_RULES contains a name that does not correspond to
 * a rule in SOURCE_MODULE, or if it contains a different number of items than
 * TARGET_RULES, an error is issued. If LOCALIZE is specified, the rules will be
 * executed in TARGET_MODULE, with corresponding access to its module local
 * variables.
 */

LIST * builtin_import( FRAME * frame, int flags )
{
    LIST * source_module_list = lol_get( frame->args, 0 );
    LIST * source_rules       = lol_get( frame->args, 1 );
    LIST * target_module_list = lol_get( frame->args, 2 );
    LIST * target_rules       = lol_get( frame->args, 3 );
    LIST * localize           = lol_get( frame->args, 4 );

    module_t * target_module = bindmodule( list_empty( target_module_list )
        ? 0
        : list_front( target_module_list ) );
    module_t * source_module = bindmodule( list_empty( source_module_list )
        ? 0
        : list_front( source_module_list ) );

    LISTITER source_iter = list_begin( source_rules );
    LISTITER const source_end = list_end( source_rules );
    LISTITER target_iter = list_begin( target_rules );
    LISTITER const target_end = list_end( target_rules );

    for ( ;
          source_iter != source_end && target_iter != target_end;
          source_iter = list_next( source_iter ),
          target_iter = list_next( target_iter ) )
    {
        RULE * r = nullptr;
        RULE * imported = nullptr;

        if ( !source_module->rules || !(r = (RULE *)hash_find(
            source_module->rules, list_item( source_iter ) ) ) )
        {
            unknown_rule( frame, "IMPORT", source_module, list_item( source_iter
                ) );
        }

        imported = import_rule( r, target_module, list_item( target_iter ) );
        if ( !list_empty( localize ) )
            rule_localize( imported, target_module );
        /* This rule is really part of some other module. Just refer to it here,
         * but do not let it out.
         */
        imported->exported = 0;
    }

    if ( source_iter != source_end || target_iter != target_end )
    {
        backtrace_line( frame->prev );
        out_printf( "import error: length of source and target rule name lists "
            "don't match!\n" );
        out_printf( "    source: " );
        list_print( source_rules );
        out_printf( "\n    target: " );
        list_print( target_rules );
        out_printf( "\n" );
        backtrace( frame->prev );
        b2::clean_exit( EXITBAD );
    }

    return L0;
}


/*
 * builtin_export() - EXPORT ( MODULE ? : RULES * )
 *
 * The EXPORT rule marks RULES from the SOURCE_MODULE as non-local (and thus
 * exportable). If an element of RULES does not name a rule in MODULE, an error
 * is issued.
 */

LIST * builtin_export( FRAME * frame, int flags )
{
    LIST * const module_list = lol_get( frame->args, 0 );
    LIST * const rules = lol_get( frame->args, 1 );
    module_t * const m = bindmodule( list_empty( module_list ) ? 0 : list_front(
        module_list ) );

    LISTITER iter = list_begin( rules );
    LISTITER const end = list_end( rules );
    for ( ; iter != end; iter = list_next( iter ) )
    {
        RULE * r = nullptr;
        if ( !m->rules || !( r = (RULE *)hash_find( m->rules, list_item( iter )
            ) ) )
        {
            unknown_rule( frame, "EXPORT", m, list_item( iter ) );
        }
        r->exported = 1;
    }
    return L0;
}


/*
 * get_source_line() - get a frame's file and line number information
 *
 * This is the execution traceback information to be indicated for in debug
 * output or an error backtrace.
 */

static void get_source_line( FRAME * frame, char const * * file, int * line )
{
    if ( frame->file )
    {
        char const * f = object_str( frame->file );
        int l = frame->line;
        *file = f;
        *line = l;
    }
    else
    {
        *file = "(builtin)";
        *line = -1;
    }
}


void print_source_line( FRAME * frame )
{
    char const * file;
    int line;
    get_source_line( frame, &file, &line );
    if ( line < 0 )
        out_printf( "(builtin):" );
    else
        out_printf( "%s:%d:", file, line );
}


/*
 * backtrace_line() - print a single line of error backtrace for the given
 * frame.
 */

void backtrace_line( FRAME * frame )
{
    if ( frame == 0 )
    {
        out_printf( "(no frame):" );
    }
    else
    {
        print_source_line( frame );
        out_printf( " in %s\n", frame->rulename );
    }
}


/*
 * backtrace() - Print the entire backtrace from the given frame to the Jambase
 * which invoked it.
 */

void backtrace( FRAME * frame )
{
    if ( !frame ) return;
    while ( ( frame = frame->prev ) )
        backtrace_line( frame );
}


/*
 * builtin_backtrace() - A Jam version of the backtrace function, taking no
 * arguments and returning a list of quadruples: FILENAME LINE MODULE. RULENAME
 * describing each frame. Note that the module-name is always followed by a
 * period.
 */

LIST * builtin_backtrace( FRAME * frame, int flags )
{
    LIST * const levels_arg = lol_get( frame->args, 0 );
    int levels = list_empty( levels_arg )
        ? (int)( (unsigned int)(-1) >> 1 )
        : atoi( object_str( list_front( levels_arg ) ) );

    LIST * result = L0;
    for ( ; ( frame = frame->prev ) && levels; --levels )
    {
        char const * file;
        int line;
        string module_name[ 1 ];
        get_source_line( frame, &file, &line );
        string_new( module_name );
        if ( frame->module->name )
        {
            string_append( module_name, object_str( frame->module->name ) );
            string_append( module_name, "." );
        }
        result = list_push_back( result, object_new( file ) );
        result = list_push_back( result, b2::value::as_string(line) );
        result = list_push_back( result, object_new( module_name->value ) );
        result = list_push_back( result, object_new( frame->rulename ) );
        string_free( module_name );
    }
    return result;
}


/*
 * builtin_caller_module() - CALLER_MODULE ( levels ? )
 *
 * If levels is not supplied, returns the name of the module of the rule which
 * called the one calling this one. If levels is supplied, it is interpreted as
 * an integer specifying a number of additional levels of call stack to traverse
 * in order to locate the module in question. If no such module exists, returns
 * the empty list. Also returns the empty list when the module in question is
 * the global module. This rule is needed for implementing module import
 * behavior.
 */

LIST * builtin_caller_module( FRAME * frame, int flags )
{
    LIST * const levels_arg = lol_get( frame->args, 0 );
    int const levels = list_empty( levels_arg )
        ? 0
        : atoi( object_str( list_front( levels_arg ) ) );

    int i;
    for ( i = 0; ( i < levels + 2 ) && frame->prev; ++i )
        frame = frame->prev;

    return frame->module == root_module()
        ? L0
        : list_new( object_copy( frame->module->name ) );
}


/*
 * Return the current working directory.
 *
 * Usage: pwd = [ PWD ] ;
 */

LIST * builtin_pwd( FRAME * frame, int flags )
{
    return list_new( object_copy( cwd() ) );
}


/*
 * Adds targets to the list of target that jam will attempt to update.
 */

LIST * builtin_update( FRAME * frame, int flags )
{
    LIST * result = list_copy( targets_to_update() );
    LIST * arg1 = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( arg1 ), end = list_end( arg1 );
    clear_targets_to_update();
    for ( ; iter != end; iter = list_next( iter ) )
        mark_target_for_updating( object_copy( list_item( iter ) ) );
    return result;
}

int last_update_now_status;

/* Takes a list of target names and immediately updates them.
 *
 * Parameters:
 *  1. Target list.
 *  2. Optional file descriptor (converted to a string) for a log file where all
 *     the related build output should be redirected.
 *  3. If specified, makes the build temporarily disable the -n option, i.e.
 *     forces all needed out-of-date targets to be rebuilt.
 *  4. If specified, makes the build temporarily disable the -q option, i.e.
 *     forces the build to continue even if one of the targets fails to build.
 */
LIST * builtin_update_now( FRAME * frame, int flags )
{
    LIST * targets = lol_get( frame->args, 0 );
    LIST * log = lol_get( frame->args, 1 );
    LIST * force = lol_get( frame->args, 2 );
    LIST * continue_ = lol_get( frame->args, 3 );
    int status;
    int original_stdout = 0;
    int original_stderr = 0;
    bool original_noexec = false;
    bool original_quitquick = false;

    if ( !list_empty( log ) )
    {
        /* Temporarily redirect stdout and stderr to the given log file. */
        int const fd = atoi( object_str( list_front( log ) ) );
        original_stdout = dup( 0 );
        original_stderr = dup( 1 );
        dup2( fd, 0 );
        dup2( fd, 1 );
    }

    if ( !list_empty( force ) )
    {
        original_noexec = globs.noexec;
        globs.noexec = false;
    }

    if ( !list_empty( continue_ ) )
    {
        original_quitquick = globs.quitquick;
        globs.quitquick = false;
    }

    status = make( targets, globs.anyhow );

    if ( !list_empty( force ) )
    {
        globs.noexec = original_noexec;
    }

    if ( !list_empty( continue_ ) )
    {
        globs.quitquick = original_quitquick;
    }

    if ( !list_empty( log ) )
    {
        /* Flush whatever stdio might have buffered, while descriptions 0 and 1
         * still refer to the log file.
         */
        out_flush( );
        err_flush( );
        dup2( original_stdout, 0 );
        dup2( original_stderr, 1 );
        close( original_stdout );
        close( original_stderr );
    }

    last_update_now_status = status;

    return status ? L0 : list_new( object_copy( constant_ok ) );
}


LIST * builtin_import_module( FRAME * frame, int flags )
{
    LIST * const arg1 = lol_get( frame->args, 0 );
    LIST * const arg2 = lol_get( frame->args, 1 );
    module_t * const m = list_empty( arg2 )
        ? root_module()
        : bindmodule( list_front( arg2 ) );
    import_module( arg1, m );
    return L0;
}


LIST * builtin_imported_modules( FRAME * frame, int flags )
{
    LIST * const arg0 = lol_get( frame->args, 0 );
    OBJECT * const module = list_empty( arg0 ) ? 0 : list_front( arg0 );
    return imported_modules( bindmodule( module ) );
}


LIST * builtin_sort( FRAME * frame, int flags )
{
    return list_sort( lol_get( frame->args, 0 ) );
}


namespace
{
    template <class S>
    void replace_all(S &str, const S &from, const S &to)
    {
        const auto from_len = from.length();
        const auto to_len = to.length();
        auto pos = str.find(from, 0);
        while (pos != S::npos)
        {
            str.replace(pos, from_len, to);
            pos += to_len;
            pos = str.find(from, pos);
        }
    }
}


LIST * builtin_normalize_path( FRAME * frame, int flags )
{
    LIST * arg = lol_get( frame->args, 0 );
    LISTITER arg_iter = list_begin( arg );
    LISTITER arg_end = list_end( arg );
    std::string in;
    for ( ; arg_iter != arg_end; arg_iter = list_next( arg_iter ) )
    {
        auto arg_str = object_str( list_item( arg_iter ) );
        if (arg_str[ 0 ] == '\0') continue;
        if (!in.empty()) in += "/";
        in += arg_str;
    }
    std::string out = b2::paths::normalize(in);

    if (out.empty()) return L0;
    else return list_new(object_new(out.c_str()));
}


LIST * builtin_native_rule( FRAME * frame, int flags )
{
    LIST * module_name = lol_get( frame->args, 0 );
    LIST * rule_name = lol_get( frame->args, 1 );

    module_t * module = bindmodule( list_front( module_name ) );

    native_rule_t * np;
    if ( module->native_rules && (np = (native_rule_t *)hash_find(
        module->native_rules, list_front( rule_name ) ) ) )
    {
        new_rule_body( module, np->name, np->procedure, 1 );
    }
    else
    {
        backtrace_line( frame->prev );
        out_printf( "error: no native rule \"%s\" defined in module \"%s.\"\n",
            object_str( list_front( rule_name ) ), object_str( module->name ) );
        backtrace( frame->prev );
        b2::clean_exit( EXITBAD );
    }
    return L0;
}


LIST * builtin_has_native_rule( FRAME * frame, int flags )
{
    LIST * module_name = lol_get( frame->args, 0 );
    LIST * rule_name   = lol_get( frame->args, 1 );
    LIST * version     = lol_get( frame->args, 2 );

    module_t * module = bindmodule( list_front( module_name ) );

    native_rule_t * np;
    if ( module->native_rules && (np = (native_rule_t *)hash_find(
        module->native_rules, list_front( rule_name ) ) ) )
    {
        int expected_version = atoi( object_str( list_front( version ) ) );
        if ( np->version == expected_version )
            return list_new( object_copy( constant_true ) );
    }
    return L0;
}


LIST * builtin_user_module( FRAME * frame, int flags )
{
    LIST * const module_name = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( module_name );
    LISTITER const end = list_end( module_name );
    for ( ; iter != end; iter = list_next( iter ) )
        bindmodule( list_item( iter ) )->user_module = 1;
    return L0;
}


LIST * builtin_nearest_user_location( FRAME * frame, int flags )
{
    FRAME * const nearest_user_frame = frame->module->user_module
        ? frame
        : frame->prev_user;
    if ( !nearest_user_frame )
        return L0;

    {
        LIST * result = L0;
        char const * file;
        int line;

        get_source_line( nearest_user_frame, &file, &line );
        result = list_push_back( result, object_new( file ) );
        result = list_push_back( result, b2::value::as_string(line) );
        return result;
    }
}


LIST * builtin_check_if_file( FRAME * frame, int flags )
{
    LIST * const name = lol_get( frame->args, 0 );
    return file_is_file( list_front( name ) ) == 1
        ? list_new( object_copy( constant_true ) )
        : L0;
}


LIST * builtin_md5( FRAME * frame, int flags )
{
    LIST * l = lol_get( frame->args, 0 );
    char const * s = object_str( list_front( l ) );

    md5_state_t state;
    md5_byte_t digest[ 16 ];
    char hex_output[ 16 * 2 + 1 ];

    int di;

    md5_init( &state );
    md5_append( &state, (md5_byte_t const *)s, strlen( s ) );
    md5_finish( &state, digest );

    static const char hex_digit[] = "0123456789abcdef";
    for ( di = 0; di < 16; ++di )
    {
        hex_output[di*2+0] = hex_digit[digest[di]>>4];
        hex_output[di*2+1] = hex_digit[digest[di]&0xF];
    }
    hex_output[16*2] = '\0';

    return list_new( object_new( hex_output ) );
}


LIST * builtin_file_open( FRAME * frame, int flags )
{
    char const * name = object_str( list_front( lol_get( frame->args, 0 ) ) );
    char const * mode = object_str( list_front( lol_get( frame->args, 1 ) ) );
    int fd;

    if ( strcmp(mode, "t") == 0 )
    {
        FILE* f = fopen( name, "r" );
        if ( !f ) return L0;
        char buf[ 1025 ];
        std::string text;
        size_t c = 0;
        do
        {
            c = fread( buf, sizeof(char), 1024, f );
            if ( c > 0 )
            {
                buf[c] = 0;
                text += buf;
            }
        }
        while( c > 0 );
        fclose( f );
        return list_new( object_new( text.c_str() ) );
    }

    if ( strcmp(mode, "w") == 0 )
        fd = open( name, O_WRONLY|O_CREAT|O_TRUNC, 0666 );
    else
        fd = open( name, O_RDONLY );

    if ( fd != -1 )
    {
        return list_new( b2::value::as_string(fd) );
    }
    return L0;
}


LIST * builtin_pad( FRAME * frame, int flags )
{
    OBJECT * string = list_front( lol_get( frame->args, 0 ) );
    char const * width_s = object_str( list_front( lol_get( frame->args, 1 ) ) );

    int32_t current = int32_t(strlen( object_str( string ) ));
    int32_t desired = atoi( width_s );
    if ( current >= desired )
        return list_new( object_copy( string ) );
    else
    {
        char * buffer = (char *)BJAM_MALLOC( desired + 1 );
        int32_t i;
        LIST * result;

        strcpy( buffer, object_str( string ) );
        for ( i = current; i < desired; ++i )
            buffer[ i ] = ' ';
        buffer[ desired ] = '\0';
        result = list_new( object_new( buffer ) );
        BJAM_FREE( buffer );
        return result;
    }
}


LIST * builtin_precious( FRAME * frame, int flags )
{
    LIST * targets = lol_get( frame->args, 0 );
    LISTITER iter = list_begin( targets );
    LISTITER const end = list_end( targets );
    for ( ; iter != end; iter = list_next( iter ) )
        bindtarget( list_item( iter ) )->flags |= T_FLAG_PRECIOUS;
    return L0;
}


LIST * builtin_self_path( FRAME * frame, int flags )
{
    extern char const * saved_argv0;
    char * p = executable_path( saved_argv0 );
    if ( p )
    {
        LIST * const result = list_new( object_new( p ) );
        free( p );
        return result;
    }
    return L0;
}


LIST * builtin_makedir( FRAME * frame, int flags )
{
    LIST * const path = lol_get( frame->args, 0 );
    return file_mkdir( object_str( list_front( path ) ) )
        ? L0
        : list_new( object_copy( list_front( path ) ) );
}

LIST *builtin_readlink( FRAME * frame, int flags )
{
    const char * path = object_str( list_front( lol_get( frame->args, 0 ) ) );
#ifdef OS_NT

    /* This struct is declared in ntifs.h which is
     * part of the Windows Driver Kit.
     */
    typedef struct _REPARSE_DATA_BUFFER {
        ULONG ReparseTag;
        USHORT ReparseDataLength;
        USHORT Reserved;
        union {
            struct {
                USHORT SubstituteNameOffset;
                USHORT SubstituteNameLength;
                USHORT PrintNameOffset;
                USHORT PrintNameLength;
                ULONG Flags;
                WCHAR PathBuffer[ 1 ];
            } SymbolicLinkReparseBuffer;
            struct {
                USHORT SubstituteNameOffset;
                USHORT SubstituteNameLength;
                USHORT PrintNameOffset;
                USHORT PrintNameLength;
                WCHAR PathBuffer[ 1 ];
            } MountPointReparseBuffer;
            struct {
                UCHAR DataBuffer[ 1 ];
            } GenericReparseBuffer;
        };
    } REPARSE_DATA_BUFFER;

    HANDLE hLink = CreateFileA( path, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL );
    DWORD n;
    union {
        REPARSE_DATA_BUFFER reparse;
        char data[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    } buf;
    int okay = DeviceIoControl(hLink, FSCTL_GET_REPARSE_POINT, NULL, 0, &buf, sizeof(buf), &n, NULL);

    CloseHandle( hLink );

    if (okay && buf.reparse.ReparseTag == IO_REPARSE_TAG_SYMLINK )
    {
        int index = buf.reparse.SymbolicLinkReparseBuffer.SubstituteNameOffset / 2;
        int length = buf.reparse.SymbolicLinkReparseBuffer.SubstituteNameLength / 2;
        char cbuf[MAX_PATH + 1];
        int numchars = WideCharToMultiByte( CP_ACP, 0, buf.reparse.SymbolicLinkReparseBuffer.PathBuffer + index, length, cbuf, sizeof(cbuf), NULL, NULL );
        if( numchars >= int(sizeof(cbuf)) )
        {
            return 0;
        }
        cbuf[numchars] = '\0';
        return list_new( object_new( cbuf ) );
    }
    else if( okay && buf.reparse.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT )
    {
        int index = buf.reparse.MountPointReparseBuffer.SubstituteNameOffset / 2;
        int length = buf.reparse.MountPointReparseBuffer.SubstituteNameLength / 2;
        char cbuf[MAX_PATH + 1];
        const char * result;
        int numchars = WideCharToMultiByte( CP_ACP, 0, buf.reparse.MountPointReparseBuffer.PathBuffer + index, length, cbuf, sizeof(cbuf), NULL, NULL );
        if( numchars >= int(sizeof(cbuf)) )
        {
            return 0;
        }
        cbuf[numchars] = '\0';
        /* strip off the leading "\??\" */
        result = cbuf;
        if ( cbuf[ 0 ] == '\\' && cbuf[ 1 ] == '?' &&
            cbuf[ 2 ] == '?' && cbuf[ 3 ] == '\\' &&
            cbuf[ 4 ] != '\0' && cbuf[ 5 ] == ':' )
        {
            result += 4;
        }
        return list_new( object_new( result ) );
    }
    return 0;
#else
    char static_buf[256];
    char * buf = static_buf;
    int32_t bufsize = 256;
    LIST * result = 0;
    while (1) {
        ssize_t len = readlink( path, buf, bufsize );
        if ( len < 0 )
        {
            break;
        }
        else if ( int32_t(len) < bufsize )
        {
            buf[ len ] = '\0';
            result = list_new( object_new( buf ) );
            break;
        }
        if ( buf != static_buf )
            BJAM_FREE( buf );
        bufsize *= 2;
        buf = (char *)BJAM_MALLOC( bufsize );
    }

    if ( buf != static_buf )
        BJAM_FREE( buf );

    return result;
#endif
}

#ifdef JAM_DEBUGGER

LIST *builtin_debug_print_helper( FRAME * frame, int flags )
{
    debug_print_result.reset( list_copy( lol_get( frame->args, 0 ) ) );
    return L0;
}

#endif


#ifdef HAVE_POPEN

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__MINGW64__) || defined(__MINGW32__)
    #undef popen
    #define popen windows_popen_wrapper
    #undef pclose
    #define pclose _pclose

    /*
     * This wrapper is a workaround for a funny _popen() feature on Windows
     * where it eats external quotes in some cases. The bug seems to be related
     * to the quote stripping functionality used by the Windows cmd.exe
     * interpreter when its /S is not specified.
     *
     * Cleaned up quote from the cmd.exe help screen as displayed on Windows XP
     * SP3:
     *
     *   1. If all of the following conditions are met, then quote characters on
     *      the command line are preserved:
     *
     *       - no /S switch
     *       - exactly two quote characters
     *       - no special characters between the two quote characters, where
     *         special is one of: &<>()@^|
     *       - there are one or more whitespace characters between the two quote
     *         characters
     *       - the string between the two quote characters is the name of an
     *         executable file.
     *
     *   2. Otherwise, old behavior is to see if the first character is a quote
     *      character and if so, strip the leading character and remove the last
     *      quote character on the command line, preserving any text after the
     *      last quote character.
     *
     * This causes some commands containing quotes not to be executed correctly.
     * For example:
     *
     *   "\Long folder name\aaa.exe" --name="Jurko" --no-surname
     *
     * would get its outermost quotes stripped and would be executed as:
     *
     *   \Long folder name\aaa.exe" --name="Jurko --no-surname
     *
     * which would report an error about '\Long' not being a valid command.
     *
     * cmd.exe help seems to indicate it would be enough to add an extra space
     * character in front of the command to avoid this but this does not work,
     * most likely due to the shell first stripping all leading whitespace
     * characters from the command.
     *
     * Solution implemented here is to quote the whole command in case it
     * contains any quote characters. Note thought this will not work correctly
     * should Windows ever 'fix' this feature.
     *                                               (03.06.2008.) (Jurko)
     */
    static FILE * windows_popen_wrapper( char const * command,
        char const * mode )
    {
        int const extra_command_quotes_needed = !!strchr( command, '"' );
        string quoted_command;
        FILE * result;

        if ( extra_command_quotes_needed )
        {
            string_new( &quoted_command );
            string_append( &quoted_command, "\"" );
            string_append( &quoted_command, command );
            string_append( &quoted_command, "\"" );
            command = quoted_command.value;
        }

        result = _popen( command, "r" );

        if ( extra_command_quotes_needed )
            string_free( &quoted_command );

        return result;
    }
#endif  /* defined(_MSC_VER) || defined(__BORLANDC__) */


LIST * builtin_shell( FRAME * frame, int flags )
{
    LIST   * command = lol_get( frame->args, 0 );
    LIST   * result = L0;
    string   s;
    int32_t ret;
    char     buffer[ 1024 ];
    FILE   * p = NULL;
    int      exit_status = -1;
    int      exit_status_opt = 0;
    int      no_output_opt = 0;
    int      strip_eol_opt = 0;

    /* Process the variable args options. */
    {
        int a = 1;
        LIST * arg = lol_get( frame->args, a );
        for ( ; !list_empty( arg ); arg = lol_get( frame->args, ++a ) )
        {
            if ( !strcmp( "exit-status", object_str( list_front( arg ) ) ) )
                exit_status_opt = 1;
            else if ( !strcmp( "no-output", object_str( list_front( arg ) ) ) )
                no_output_opt = 1;
            else if ( !strcmp("strip-eol", object_str( list_front( arg ) ) ) )
                strip_eol_opt = 1;
        }
    }

    /* The following fflush() call seems to be indicated as a workaround for a
     * popen() bug on POSIX implementations related to synhronizing input
     * stream positions for the called and the calling process.
     */
    fflush( NULL );

    p = popen( object_str( list_front( command ) ), "r" );
    if ( p == NULL )
        return L0;

    string_new( &s );

    while ( ( ret = int32_t(fread( buffer, sizeof( char ), sizeof( buffer ) - 1, p )) ) >
        0 )
    {
        buffer[ ret ] = 0;
        if ( !no_output_opt )
        {
            string_append( &s, buffer );
        }

        /* Explicit EOF check for systems with broken fread */
        if ( feof( p ) ) break;
    }

    if ( strip_eol_opt )
        string_rtrim( &s );

    exit_status = pclose( p );

    /* The command output is returned first. */
    result = list_new( object_new( s.value ) );
    string_free( &s );

    /* The command exit result next. */
    if ( exit_status_opt )
    {
        if ( WIFEXITED( exit_status ) )
            exit_status = WEXITSTATUS( exit_status );
        else
            exit_status = -1;

#ifdef OS_VMS
        /* Harmonize VMS success status with POSIX */
        if ( exit_status == 1 ) exit_status = EXIT_SUCCESS;
#endif
        result = list_push_back( result, b2::value::as_string(exit_status) );
    }

    return result;
}

#else  /* #ifdef HAVE_POPEN */

LIST * builtin_shell( FRAME * frame, int flags )
{
    return L0;
}

#endif  /* #ifdef HAVE_POPEN */


/*
 * builtin_glob_archive() - GLOB_ARCHIVE rule
 */

struct globbing2
{
    LIST * patterns[ 2 ];
    LIST * results;
    LIST * case_insensitive;
};


static void builtin_glob_archive_back( void * closure, OBJECT * member,
    LIST * symbols, int status, timestamp const * const time )
{
    PROFILE_ENTER( BUILTIN_GLOB_ARCHIVE_BACK );

    struct globbing2 * const globbing = (struct globbing2 *)closure;
    PATHNAME f;
    string buf[ 1 ];
    LISTITER iter;
    LISTITER end;
    LISTITER iter_symbols;
    LISTITER end_symbols;
    int matched = 0;

    /* Match member name.
     */
    path_parse( object_str( member ), &f );

    if ( !strcmp( f.f_member.ptr, "" ) )
    {
        PROFILE_EXIT( BUILTIN_GLOB_ARCHIVE_BACK );
        return;
    }

    string_new( buf );
    string_append_range( buf, f.f_member.ptr, f.f_member.ptr + f.f_member.len );

    if ( globbing->case_insensitive )
        downcase_inplace( buf->value );

    /* Glob with member patterns. If not matched, then match symbols.
     */
    matched = 0;
    iter = list_begin( globbing->patterns[ 0 ] );
    end = list_end( globbing->patterns[ 0 ] );
    for ( ; !matched && iter != end;
            iter = list_next( iter ) )
    {
        const char * pattern = object_str( list_item( iter ) );
        int match_exact = ( !has_wildcards( pattern ) );
        matched = ( match_exact ?
            ( !strcmp( pattern, buf->value ) ) :
            ( !glob( pattern, buf->value ) ) );
    }


    /* Glob with symbol patterns, if requested.
     */
    iter = list_begin( globbing->patterns[ 1 ] );
    end = list_end( globbing->patterns[ 1 ] );

    if ( iter != end ) matched = 0;

    for ( ; !matched && iter != end;
            iter = list_next( iter ) )
    {
        const char * pattern = object_str( list_item( iter ) );
        int match_exact = ( !has_wildcards( pattern ) );

        iter_symbols = list_begin( symbols );
        end_symbols = list_end( symbols );

        for ( ; !matched && iter_symbols != end_symbols;
                iter_symbols = list_next( iter_symbols ) )
        {
            const char * symbol = object_str( list_item( iter_symbols ) );

            string_copy( buf, symbol );
            if ( globbing->case_insensitive )
                downcase_inplace( buf->value );

            matched = ( match_exact ?
                ( !strcmp( pattern, buf->value ) ) :
                ( !glob( pattern, buf->value ) ) );
        }
    }

    if ( matched )
    {
        globbing->results = list_push_back( globbing->results,
            object_copy( member ) );
    }

    string_free( buf );

    PROFILE_EXIT( BUILTIN_GLOB_ARCHIVE_BACK );
}


LIST * builtin_glob_archive( FRAME * frame, int flags )
{
    LIST * const l = lol_get( frame->args, 0 );
    LIST * const r1 = lol_get( frame->args, 1 );
    LIST * const r3 = lol_get( frame->args, 3 );

    LISTITER iter;
    LISTITER end;
    struct globbing2 globbing;

    globbing.results = L0;
    globbing.patterns[ 0 ] = r1;
    globbing.patterns[ 1 ] = r3;

    globbing.case_insensitive =
# if defined( OS_NT ) || defined( OS_CYGWIN ) || defined( OS_VMS )
       l;  /* Always case-insensitive. */
# else
       lol_get( frame->args, 2 ); // r2
# endif

    if ( globbing.case_insensitive )
    {
        globbing.patterns[ 0 ] = downcase_list( globbing.patterns[ 0 ] );
        globbing.patterns[ 1 ] = downcase_list( globbing.patterns[ 1 ] );
    }

    iter = list_begin( l );
    end = list_end( l );
    for ( ; iter != end; iter = list_next( iter ) )
        file_archivescan( list_item( iter ), builtin_glob_archive_back, &globbing );

    if ( globbing.case_insensitive )
    {
        list_free( globbing.patterns[ 0 ] );
        list_free( globbing.patterns[ 1 ] );
    }

    return globbing.results;
}
