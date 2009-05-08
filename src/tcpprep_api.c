/* $Id$ */

/*
 * Copyright (c) 2009 Aaron Turner.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright owners nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "defines.h"
#include "common.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "tcpprep_api.h"

#ifdef USE_AUTOOPTS
#include "tcpprep_opts.h"
#endif

extern void print_comment(const char *);
extern void print_info(const char *);
extern void print_stats(const char *);


/**
 * \brief Initialize a new tcpprep context
 *
 * Allocates memory and stuff like that.  Always returns a buffer or completely
 * fails by calling exit() on malloc failure.
 */
tcpprep_t *
tcpprep_init()
{
    tcpprep_t *ctx;
    int i;
    
    ctx = safe_malloc(sizeof(tcpprep_t));
    ctx->options = safe_malloc(sizeof(tcpprep_opt_t));

    ctx->options->bpf.optimize = BPF_OPTIMIZE;

    for (i = DEFAULT_LOW_SERVER_PORT; i <= DEFAULT_HIGH_SERVER_PORT; i++) {
        ctx->options->services.tcp[i] = 1;
        ctx->options->services.udp[i] = 1;
    }
    
    return ctx;
}

void 
tcpprep_close(tcpprep_t *ctx)
{
    assert(ctx);
    errx(1, "%s", "not defined");
}


int 
tcpprep_set_pcap_file(tcpprep_t *ctx, char *value)
{
    assert(ctx);
    assert(value);
    ctx->pcapfile = safe_strdup(value);
    return 0;
}

int 
tcpprep_set_output_file(tcpprep_t *ctx, char *value)
{
    assert(ctx);
    assert(value);
    ctx->outfile = safe_strdup(value);
    return 0;
}

int 
tcpprep_set_comment(tcpprep_t *ctx, char *value)
{
    assert(ctx);
    assert(value);
    ctx->options->comment = safe_strdup(value);
    return 0;
}

int 
tcpprep_set_nocomment(tcpprep_t *ctx, bool value)
{
    assert(ctx);
    ctx->options->nocomment = value;
    return 0;
}

int 
tcpprep_set_mode(tcpprep_t *ctx, tcpprep_mode_t value)
{
    assert(ctx);
    ctx->options->mode = value;
    return 0;
}

int 
tcpprep_set_automode(tcpprep_t *ctx, tcpprep_mode_t value)
{
    assert(ctx);
    ctx->options->automode = value;
    return 0;
}

int 
tcpprep_set_min_mask(tcpprep_t *ctx, int value)
{
    return 0;
}

int 
tcpprep_set_max_mask(tcpprep_t *ctx, int value)
{
    return 0;
}

int 
tcpprep_set_ratio(tcpprep_t *ctx, double value)
{
    return 0;
}

int 
tcpprep_set_regex(tcpprep_t *ctx, char *value)
{
    return 0;
}

int 
tcpprep_set_nonip_is_secondary(tcpprep_t *ctx, bool value)
{
    return 0;
}

#ifdef ENABLE_VERBOSE
int 
tcpprep_set_verbose(tcpprep_t *ctx, bool value)
{
    return 0;
}

int 
tcpprep_set_tcpdump_args(tcpprep_t *ctx, char *value)
{
    return 0;
}

int 
tcpprep_set_tcpdump(tcpprep_t *ctx, tcpdump_t *value)
{
    return 0;
}
#endif

/**
 * \brief Returns a string describing the last error.  
 *
 * Value when the last call does not result in an error is undefined 
 * (may be NULL, may be garbage)
 */
char *
tcpprep_geterr(tcpprep_t *ctx)
{
    assert(ctx);
    return(ctx->errstr);    
}

/**
 * \brief Returns a string describing the last warning.  
 *
 * Value when the last call does not result in an warning is undefined 
 * (may be NULL, may be garbage)
 */
char *
tcpprep_getwarn(tcpprep_t *ctx)
{
    assert(ctx);
    return(ctx->warnstr);    
}


void 
__tcpprep_seterr(tcpprep_t *ctx, const char *func, const int line, const char *file, const char *fmt, ...)
{
}

void 
tcpprep_setwarn(tcpprep_t *ctx, const char *fmt, ...)
{
}



#ifdef USE_AUTOOPTS
int 
tcpprep_post_args(tcpprep_t *ctx, int argc, char *argv[])
{
    char myargs[MYARGS_LEN];
    int i, bufsize;
    char *tempstr;

    memset(myargs, 0, MYARGS_LEN);

    /* print_comment and print_info don't return */
    if (HAVE_OPT(PRINT_COMMENT))
        print_comment(OPT_ARG(PRINT_COMMENT));

    if (HAVE_OPT(PRINT_INFO))
        print_info(OPT_ARG(PRINT_INFO));

    if (HAVE_OPT(PRINT_STATS))
        print_stats(OPT_ARG(PRINT_STATS));
        
    if (! HAVE_OPT(CACHEFILE) && ! HAVE_OPT(PCAP))
        err(-1, "Must specify an output cachefile (-o) and input pcap (-i)");
    
    if (! ctx->options->mode)
        err(-1, "Must specify a processing mode: -a, -c, -r, -p");

#ifdef DEBUG
    if (HAVE_OPT(DBUG))
        debug = OPT_VALUE_DBUG;
#endif

#ifdef ENABLE_VERBOSE
    if (HAVE_OPT(VERBOSE)) {
        ctx->options->verbose = 1;
    }

    if (HAVE_OPT(DECODE))
        ctx->tcpdump.args = safe_strdup(OPT_ARG(DECODE));
   
    /*
     * put the open after decode options so they are passed to tcpdump
     */
#endif


    /* 
     * if we are to include the cli args, then prep it for the
     * cache file header
     */
    if (! ctx->options->nocomment) {
        /* copy all of our args to myargs */
        for (i = 1; i < argc; i ++) {
            /* skip the -C <comment> */
            if (strcmp(argv[i], "-C") == 0) {
                i += 2;
                continue;
            }
            
            strlcat(myargs, argv[i], MYARGS_LEN);
            strlcat(myargs, " ", MYARGS_LEN);
        }

        /* remove trailing space */
        myargs[strlen(myargs) - 1] = 0;

        dbgx(1, "Comment args length: %zu", strlen(myargs));
    }

    /* setup or options.comment buffer so that that we get args\ncomment */
    if (ctx->options->comment != NULL) {
        strlcat(myargs, "\n", MYARGS_LEN);
        bufsize = strlen(ctx->options->comment) + strlen(myargs) + 1;
        ctx->options->comment = (char *)safe_realloc(ctx->options->comment, 
            bufsize);
        
        tempstr = strdup(ctx->options->comment);
        strlcpy(ctx->options->comment, myargs, bufsize);
        strlcat(ctx->options->comment, tempstr, bufsize);
    } else {
        bufsize = strlen(myargs) + 1;
        ctx->options->comment = (char *)safe_malloc(bufsize);
        strlcpy(ctx->options->comment, myargs, bufsize);
    }
        
    dbgx(1, "Final comment length: %zu", strlen(ctx->options->comment));

    /* copy over our min/max mask */
    ctx->options->min_mask = OPT_VALUE_MINMASK;
    
    ctx->options->max_mask = OPT_VALUE_MAXMASK;
    
    if (! ctx->options->min_mask > ctx->options->max_mask)
        errx(-1, "Min network mask len (%d) must be less then max network mask len (%d)",
        ctx->options->min_mask, ctx->options->max_mask);

    ctx->options->ratio = atof(OPT_ARG(RATIO));
    if (ctx->options->ratio < 0)
        err(-1, "Ratio must be a non-negative number.");
}
#endif /* USE_AUTOOPTS */