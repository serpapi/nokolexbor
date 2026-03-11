/*
 * TSan (ThreadSanitizer) race condition test
 *
 * Proves that the original static css_parser/selectors singletons in
 * nl_node_find() are subject to a data race when called concurrently.
 *
 * Build and run (buggy):
 *   clang -fsanitize=thread -g -O1 \
 *     -I../../vendor/lexbor/dist/include \
 *     -L../../vendor/lexbor/dist/lib \
 *     -llexbor_static \
 *     -lpthread \
 *     -o tsan_race_test tsan_race_test.c && ./tsan_race_test
 *
 * Build and run (fixed):
 *   Same command, but with -DFIXED
 *   TSan should report no races.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lexbor/html/html.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

/* ------------------------------------------------------------------ */
/* Shared state — mirrors the original buggy nl_node_find() exactly    */
/* ------------------------------------------------------------------ */

#ifndef FIXED
/* BUGGY: process-global singletons, no locking — mirrors original code */
static lxb_css_parser_t *css_parser = NULL;
static lxb_selectors_t  *selectors  = NULL;
#else
/* FIXED: thread-local storage via pthread keys */
static pthread_key_t p_key_css_parser;
static pthread_key_t p_key_selectors;
#endif

/* Manual barrier (pthread_barrier_t not available on macOS) */
static pthread_mutex_t barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  barrier_cond  = PTHREAD_COND_INITIALIZER;
static int             barrier_count = 0;
static int             barrier_total = 0;

static void
barrier_init(int n)
{
    pthread_mutex_lock(&barrier_mutex);
    barrier_count = 0;
    barrier_total = n;
    pthread_mutex_unlock(&barrier_mutex);
}

static void
barrier_wait(void)
{
    pthread_mutex_lock(&barrier_mutex);
    barrier_count++;
    if (barrier_count >= barrier_total) {
        pthread_cond_broadcast(&barrier_cond);
    } else {
        while (barrier_count < barrier_total)
            pthread_cond_wait(&barrier_cond, &barrier_mutex);
    }
    pthread_mutex_unlock(&barrier_mutex);
}


/* Dummy callback for lxb_selectors_find (must not be NULL) */
static lxb_status_t
selector_cb(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *ctx)
{
    return LXB_STATUS_OK;
}
/* ------------------------------------------------------------------ */
/* The function under test                                             */
/* ------------------------------------------------------------------ */

static lxb_status_t
nl_node_find_buggy(lxb_dom_node_t *node,
                   const char *selector_str, size_t selector_len)
{
#ifndef FIXED
    lxb_status_t status;
    lxb_css_selector_list_t *list = NULL;

    /* CSS parser — lazy init, shared across all threads (THE BUG) */
    if (css_parser == NULL) {
        css_parser = lxb_css_parser_create();
        status = lxb_css_parser_init(css_parser, NULL, NULL);
        if (status != LXB_STATUS_OK) {
            goto init_error;
        }
    }

    /* Selectors — lazy init, shared across all threads (THE BUG) */
    if (selectors == NULL) {
        selectors = lxb_selectors_create();
        status = lxb_selectors_init(selectors);
        if (status != LXB_STATUS_OK) {
            goto init_error;
        }
    }

    /* Parse — css_parser->stage is written here: LXB_CSS_PARSER_RUN */
    list = lxb_css_selectors_parse_relative_list(css_parser,
               (const lxb_char_t *)selector_str, selector_len);
    if (css_parser->status != LXB_STATUS_OK) {
        status = css_parser->status;
        goto cleanup;
    }

    /* Find */
    status = lxb_selectors_find(selectors, node, list, selector_cb, NULL);
    if (status != LXB_STATUS_OK) {
        goto cleanup;
    }

cleanup:
    lxb_css_selector_list_destroy_memory(list);
    return status;

init_error:
    lxb_selectors_destroy(selectors, true);
    selectors = NULL;
    lxb_css_parser_destroy(css_parser, true);
    css_parser = NULL;
    lxb_css_selector_list_destroy_memory(list);
    return status;

#else
    /* FIXED: thread-local cached allocation via pthread keys */
    lxb_status_t status;
    lxb_css_parser_t *css_parser = (lxb_css_parser_t *)pthread_getspecific(p_key_css_parser);
    lxb_selectors_t  *selectors  = (lxb_selectors_t *)pthread_getspecific(p_key_selectors);
    lxb_css_selector_list_t *list = NULL;

    if (css_parser == NULL) {
        css_parser = lxb_css_parser_create();
        status = lxb_css_parser_init(css_parser, NULL, NULL);
        if (status != LXB_STATUS_OK) {
            goto init_error;
        }
        pthread_setspecific(p_key_css_parser, css_parser);
    }

    if (selectors == NULL) {
        selectors = lxb_selectors_create();
        status = lxb_selectors_init(selectors);
        if (status != LXB_STATUS_OK) {
            goto init_error;
        }
        pthread_setspecific(p_key_selectors, selectors);
    }

    list = lxb_css_selectors_parse_relative_list(css_parser,
               (const lxb_char_t *)selector_str, selector_len);
    if (css_parser->status != LXB_STATUS_OK) {
        status = css_parser->status;
        goto cleanup;
    }

    status = lxb_selectors_find(selectors, node, list, selector_cb, NULL);

cleanup:
    lxb_css_selector_list_destroy_memory(list);
    return status;

init_error:
    lxb_selectors_destroy(selectors, true);
    selectors = NULL;
    lxb_css_parser_destroy(css_parser, true);
    css_parser = NULL;
    pthread_setspecific(p_key_css_parser, NULL);
    pthread_setspecific(p_key_selectors, NULL);
    return status;
#endif
}

/* ------------------------------------------------------------------ */
/* Thread worker                                                       */
/* ------------------------------------------------------------------ */

#define NUM_THREADS  4
#define ITERATIONS   50

typedef struct {
    lxb_dom_node_t *root;
    int             thread_id;
    int             errors;
} worker_args_t;

/* Barrier to maximise contention — all threads start simultaneously */


static const char *selectors_list[] = {
    "div",
    "span.valid",
    "p > a",
    "#myid",
    "div span",
};
static const int num_selectors =
    (int)(sizeof(selectors_list) / sizeof(selectors_list[0]));

static void *
worker(void *arg)
{
    worker_args_t *args = (worker_args_t *)arg;

    /* Wait for all threads to be ready, then all start at once */
    barrier_wait();

    for (int i = 0; i < ITERATIONS; i++) {
        const char *sel = selectors_list[i % num_selectors];
        lxb_status_t status = nl_node_find_buggy(args->root, sel, strlen(sel));

        /*
         * LXB_STATUS_OK and LXB_STATUS_ERROR_NOT_EXISTS are both fine
         * (the latter just means no nodes matched).
         * LXB_STATUS_ERROR_WRONG_ARGS means the static parser was in
         * LXB_CSS_PARSER_RUN state — the race was triggered.
         */
        if (status != LXB_STATUS_OK &&
            status != LXB_STATUS_ERROR_NOT_EXISTS &&
            status != 0x0017 /* LXB_STATUS_STOP */)
        {
            fprintf(stderr,
                "Thread %d iter %d: unexpected status 0x%04x for '%s'\n",
                args->thread_id, i, (unsigned)status, sel);
            args->errors++;
        }
    }

    return NULL;
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int
main(void)
{
    /* Build a simple HTML document to query against */
    static const lxb_char_t html[] =
        "<div id='myid'>"
        "  <p><a href='#'>link</a></p>"
        "  <span class='valid'>text</span>"
        "  <span class='other'>other</span>"
        "</div>";

    lxb_html_document_t *document = lxb_html_document_create();
    if (document == NULL) {
        fprintf(stderr, "Failed to create HTML document\n");
        return EXIT_FAILURE;
    }

    lxb_status_t status = lxb_html_document_parse(document, html,
                              sizeof(html) / sizeof(lxb_char_t) - 1);
    if (status != LXB_STATUS_OK) {
        fprintf(stderr, "Failed to parse HTML\n");
        return EXIT_FAILURE;
    }

    lxb_dom_node_t *root = lxb_dom_interface_node(document);

#ifdef FIXED
    printf("Running FIXED version (thread-local storage)...\n");

    pthread_key_create(&p_key_css_parser, NULL);
    pthread_key_create(&p_key_selectors, NULL);
#else
    printf("Running BUGGY version (static singletons)...\n");
#endif
    printf("%d threads x %d iterations each\n\n", NUM_THREADS, ITERATIONS);

    barrier_init(NUM_THREADS);

    pthread_t    threads[NUM_THREADS];
    worker_args_t args[NUM_THREADS];
    int total_errors = 0;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].root      = root;
        args[i].thread_id = i;
        args[i].errors    = 0;
        pthread_create(&threads[i], NULL, worker, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_errors += args[i].errors;
    }


    lxb_html_document_destroy(document);

    if (total_errors > 0) {
        printf("RUNTIME ERRORS: %d (race triggered at application level)\n",
               total_errors);
    } else {
        printf("No runtime errors detected.\n");
        printf("(TSan instrumentation above is the definitive proof of races)\n");
    }

    return (total_errors > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
