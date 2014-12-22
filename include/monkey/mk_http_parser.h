/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Monkey HTTP Server
 *  ==================
 *  Copyright 2001-2014 Monkey Software LLC <eduardo@monkey.io>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef MK_HTTP_PARSER_H
#define MK_HTTP_PARSER_H

#include <ctype.h>

#include <monkey/mk_memory.h>
#include <monkey/mk_http.h>
#include <monkey/mk_http_internal.h>

/* General status */
#define MK_HTTP_PARSER_PENDING -10  /* cannot complete until more data arrives */
#define MK_HTTP_PARSER_ERROR    -1  /* found an error when parsing the string */
#define MK_HTTP_PARSER_OK        0

/* Connection header values */
#define MK_HTTP_PARSER_CONN_EMPTY    0
#define MK_HTTP_PARSER_CONN_UNKNOWN -1
#define MK_HTTP_PARSER_CONN_KA       1
#define MK_HTTP_PARSER_CONN_CLOSE    2
#define MK_HTTP_PARSER_CONN_UPGRADE  3

#define MK_HEADER_EXTRA_SIZE         8

/* Request levels
 * ==============
 *
 * 1. FIRST_LINE         : Method, URI (+ QS) + Protocol version + CRLF
 * 2. HEADERS (optional) : KEY, SEP, VALUE + CRLF
 * 3. BODY (option)      : data based on Content-Length or Chunked transfer encoding
 */

enum {
    REQ_LEVEL_FIRST    = 1,
    REQ_LEVEL_CONTINUE ,
    REQ_LEVEL_HEADERS  ,
    REQ_LEVEL_END      ,
    REQ_LEVEL_BODY
};

/* Statuses per levels */
enum {
    /* REQ_LEVEL_FIRST */
    MK_ST_REQ_METHOD        = 1,
    MK_ST_REQ_URI           ,
    MK_ST_REQ_QUERY_STRING  ,
    MK_ST_REQ_PROT_VERSION  ,
    MK_ST_FIRST_CONTINUE    ,
    MK_ST_FIRST_FINALIZING  ,    /* LEVEL_FIRST finalize the request */
    MK_ST_FIRST_COMPLETE    ,

    /* REQ_HEADERS */
    MK_ST_HEADER_KEY        ,
    MK_ST_HEADER_SEP        ,
    MK_ST_HEADER_VAL_STARTS ,
    MK_ST_HEADER_VALUE      ,
    MK_ST_HEADER_END        ,
    MK_ST_BLOCK_END
};

/* Known HTTP Methods */
enum mk_request_methods {
    MK_METHOD_GET     = 0,
    MK_METHOD_POST       ,
    MK_METHOD_HEAD       ,
    MK_METHOD_PUT        ,
    MK_METHOD_DELETE     ,
    MK_METHOD_OPTIONS    ,
    MK_METHOD_UNKNOWN    ,
    MK_METHOD_SIZEOF
};

/*
 * Define a list of known headers, they are used to perform headers
 * lookups in the parser and further Monkey core.
 */
enum mk_request_headers {
    MK_HEADER_ACCEPT             = 0,
    MK_HEADER_ACCEPT_CHARSET        ,
    MK_HEADER_ACCEPT_ENCODING       ,
    MK_HEADER_ACCEPT_LANGUAGE       ,
    MK_HEADER_AUTHORIZATION         ,
    MK_HEADER_CACHE_CONTROL         ,
    MK_HEADER_COOKIE                ,
    MK_HEADER_CONNECTION            ,
    MK_HEADER_CONTENT_LENGTH        ,
    MK_HEADER_CONTENT_RANGE         ,
    MK_HEADER_CONTENT_TYPE          ,
    MK_HEADER_HOST                  ,
    MK_HEADER_IF_MODIFIED_SINCE     ,
    MK_HEADER_LAST_MODIFIED         ,
    MK_HEADER_LAST_MODIFIED_SINCE   ,
    MK_HEADER_RANGE                 ,
    MK_HEADER_REFERER               ,
    MK_HEADER_UPGRADE               ,
    MK_HEADER_USER_AGENT            ,
    MK_HEADER_SIZEOF                ,

    /* used by the core for custom headers */
    MK_HEADER_OTHER
};

/*
 * Expected Header values that are used to take logic
 * decision.
 */
#define MK_CONN_KEEP_ALIVE     "keep-alive"
#define MK_CONN_CLOSE          "close"
#define MK_CONN_UPGRADE        "upgrade"

struct mk_http_header {
    int type;
    mk_ptr_t key;
    mk_ptr_t val;
};

/* This structure is the 'Parser Context' */
struct mk_http_parser {
    int i;
    int level;   /* request level */
    int status;  /* level status */
    int next;    /* something next after status ? */
    int length;

    /* lookup fields */
    int start;
    int end;
    int chars;

    long body_received;

    /* it stores the numeric value of Content-Length header */
    int  header_host_port;
    long header_content_length;

    /*
     * connection header value discovered: it can be set with
     * values:
     *
     * MK_HTTP_PARSER_CONN_EMPTY  : header not set
     * MK_HTTP_PARSER_CONN_UNKNOWN: unexpected value
     * MK_HTTP_PARSER_CONN_KA     : keep-alive
     * MK_HTTP_PARSER_CONN_CLOSE  : close
     */
    int header_connection;

    /* probable current header, fly parsing */
    int header_key;
    int header_sep;
    int header_val;
    int header_min;
    int header_max;

    int headers_extra_count;

    /* Known headers */
    struct mk_http_header headers[MK_HEADER_SIZEOF];

    /* Extra headers */
    struct mk_http_header headers_extra[MK_HEADER_EXTRA_SIZE];

} __attribute__ ((aligned (64)));

#ifdef HTTP_STANDALONE

/* ANSI Colors */

#define ANSI_RESET "\033[0m"
#define ANSI_BOLD  "\033[1m"

#define ANSI_CYAN          "\033[36m"
#define ANSI_BOLD_CYAN     ANSI_BOLD ANSI_CYAN
#define ANSI_MAGENTA       "\033[35m"
#define ANSI_BOLD_MAGENTA  ANSI_BOLD ANSI_MAGENTA
#define ANSI_RED           "\033[31m"
#define ANSI_BOLD_RED      ANSI_BOLD ANSI_RED
#define ANSI_YELLOW        "\033[33m"
#define ANSI_BOLD_YELLOW   ANSI_BOLD ANSI_YELLOW
#define ANSI_BLUE          "\033[34m"
#define ANSI_BOLD_BLUE     ANSI_BOLD ANSI_BLUE
#define ANSI_GREEN         "\033[32m"
#define ANSI_BOLD_GREEN    ANSI_BOLD ANSI_GREEN
#define ANSI_WHITE         "\033[37m"
#define ANSI_BOLD_WHITE    ANSI_BOLD ANSI_WHITE

#define TEST_OK      0
#define TEST_FAIL    1


static inline void p_field(struct mk_http_parser *req, char *buffer)
{
    int i;

    printf("'");
    for (i = req->start; i < req->end; i++) {
        printf("%c", buffer[i]);
    }
    printf("'");

}

static inline int eval_field(struct mk_http_parser *req, char *buffer)
{
    if (req->level == REQ_LEVEL_FIRST) {
        printf("[ \033[35mfirst level\033[0m ] ");
    }
    else {
        printf("[   \033[36mheaders\033[0m   ] ");
    }

    printf(" ");
    switch (req->status) {
    case MK_ST_REQ_METHOD:
        printf("MK_ST_REQ_METHOD       : ");
        break;
    case MK_ST_REQ_URI:
        printf("MK_ST_REQ_URI          : ");
        break;
    case MK_ST_REQ_QUERY_STRING:
        printf("MK_ST_REQ_QUERY_STRING : ");
        break;
    case MK_ST_REQ_PROT_VERSION:
        printf("MK_ST_REQ_PROT_VERSION : ");
        break;
    case MK_ST_HEADER_KEY:
        printf("MK_ST_HEADER_KEY       : ");
        break;
    case MK_ST_HEADER_VAL_STARTS:
        printf("MK_ST_HEADER_VAL_STARTS: ");
        break;
    case MK_ST_HEADER_VALUE:
        printf("MK_ST_HEADER_VALUE     : ");
        break;
    case MK_ST_HEADER_END:
        printf("MK_ST_HEADER_END       : ");
        break;
    default:
        printf("\033[31mUNKNOWN STATUS (%i)\033[0m     : ", req->status);
        break;
    };


    p_field(req, buffer);
    printf("\n");

    return 0;
}
#endif /* HTTP_STANDALONE */

static inline void mk_http_parser_init(struct mk_http_parser *p)
{
    memset(p, '\0', sizeof(struct mk_http_parser));

    p->level  = REQ_LEVEL_FIRST;
    p->status = MK_ST_REQ_METHOD;
    p->chars  = -1;

    /* init headers */
    p->header_key = -1;
    p->header_sep = -1;
    p->header_val = -1;
    p->header_min = -1;
    p->header_max = -1;
    p->header_content_length = -1;
    p->header_host_port = 0;
    p->headers_extra_count = 0;
}

int mk_http_parser(struct mk_http_request *req, struct mk_http_parser *p,
                   char *buffer, int len);

#endif /* MK_HTTP_H */