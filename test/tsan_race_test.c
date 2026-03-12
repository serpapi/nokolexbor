/*
 * TSan (ThreadSanitizer) test for thread-local css_parser/selectors/html_parser.
 *
 * Mirrors the pthread thread-local caching used in nl_node.c and nl_document.c.
 * Build with TSan to verify no data races exist in the implementation.
 *
 * Build & run:
 *   clang -fsanitize=thread -g -O1 \
 *     -I vendor/lexbor/dist/include \
 *     -o tsan_test test/tsan_race_test.c \
 *     vendor/lexbor/dist/lib/liblexbor_static.a -lpthread
 *   ./tsan_test
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lexbor/html/html.h>
#include <lexbor/css/css.h>
#include <lexbor/selectors/selectors.h>

/* ------------------------------------------------------------------ */
/* Thread-local keys — mirrors nl_node.c and nl_document.c             */
/* ------------------------------------------------------------------ */

static pthread_key_t p_key_css_parser;
static pthread_key_t p_key_selectors;
static pthread_key_t p_key_html_parser;

static void
free_css_parser(void *data)
{
  if (data != NULL)
    lxb_css_parser_destroy((lxb_css_parser_t *)data, true);
}

static void
free_selectors(void *data)
{
  if (data != NULL)
    lxb_selectors_destroy((lxb_selectors_t *)data, true);
}

static void
free_html_parser(void *data)
{
  if (data != NULL)
    lxb_html_parser_destroy((lxb_html_parser_t *)data);
}

/* Dummy callback for lxb_selectors_find */
static lxb_status_t
selector_cb(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *ctx)
{
  (void)spec;
  (*(int *)ctx)++;
  return LXB_STATUS_OK;
}

/* ------------------------------------------------------------------ */
/* Mirrors nl_node_find() — thread-local cached css_parser/selectors   */
/* ------------------------------------------------------------------ */

static lxb_status_t
nl_node_find(lxb_dom_node_t *node, const char *selector_str, size_t selector_len, int *match_count)
{
  lxb_status_t status;
  lxb_css_parser_t *css_parser = (lxb_css_parser_t *)pthread_getspecific(p_key_css_parser);
  lxb_selectors_t *selectors = (lxb_selectors_t *)pthread_getspecific(p_key_selectors);
  lxb_css_selector_list_t *list = NULL;

  if (css_parser == NULL) {
    css_parser = lxb_css_parser_create();
    status = lxb_css_parser_init(css_parser, NULL, NULL);
    if (status != LXB_STATUS_OK)
      goto init_error;
    pthread_setspecific(p_key_css_parser, css_parser);
  }

  if (selectors == NULL) {
    selectors = lxb_selectors_create();
    status = lxb_selectors_init(selectors);
    if (status != LXB_STATUS_OK)
      goto init_error;
    pthread_setspecific(p_key_selectors, selectors);
  }

  list = lxb_css_selectors_parse_relative_list(css_parser,
             (const lxb_char_t *)selector_str, selector_len);
  if (css_parser->status != LXB_STATUS_OK) {
    status = css_parser->status;
    goto cleanup;
  }

  *match_count = 0;
  status = lxb_selectors_find(selectors, node, list, selector_cb, match_count);

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
}

/* ------------------------------------------------------------------ */
/* Mirrors nl_document_parse_native() — thread-local cached html_parser*/
/* ------------------------------------------------------------------ */

static lxb_html_document_t *
parse_html(const char *html, size_t html_len)
{
  lxb_html_parser_t *html_parser = (lxb_html_parser_t *)pthread_getspecific(p_key_html_parser);

  if (html_parser == NULL) {
    html_parser = lxb_html_parser_create();
    lxb_status_t status = lxb_html_parser_init(html_parser);
    if (status != LXB_STATUS_OK) {
      lxb_html_parser_destroy(html_parser);
      return NULL;
    }
    html_parser->tree->scripting = true;
    pthread_setspecific(p_key_html_parser, html_parser);
  }

  return lxb_html_parse(html_parser, (const lxb_char_t *)html, html_len);
}

/* ------------------------------------------------------------------ */
/* Thread worker                                                       */
/* ------------------------------------------------------------------ */

#define NUM_THREADS  8
#define ITERATIONS   200

/* Manual barrier (pthread_barrier_t not available on macOS) */
static pthread_mutex_t barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  barrier_cond  = PTHREAD_COND_INITIALIZER;
static int             barrier_count = 0;

static void
barrier_wait(void)
{
  pthread_mutex_lock(&barrier_mutex);
  barrier_count++;
  if (barrier_count >= NUM_THREADS) {
    pthread_cond_broadcast(&barrier_cond);
  } else {
    while (barrier_count < NUM_THREADS)
      pthread_cond_wait(&barrier_cond, &barrier_mutex);
  }
  pthread_mutex_unlock(&barrier_mutex);
}

static const char *css_selectors[] = {
  "div", "span.c", "p > a", "#myid", "div span",
};
static const int num_selectors = sizeof(css_selectors) / sizeof(css_selectors[0]);

typedef struct {
  int thread_id;
  int errors;
} worker_args_t;

static void *
worker(void *arg)
{
  worker_args_t *args = (worker_args_t *)arg;
  barrier_wait();

  for (int i = 0; i < ITERATIONS; i++) {
    /* Parse a fresh document (exercises thread-local html_parser) */
    const char *html = "<div id='myid'><p><a href='#'>link</a></p>"
                       "<span class='c'>text</span></div>";
    lxb_html_document_t *doc = parse_html(html, strlen(html));
    if (doc == NULL) {
      fprintf(stderr, "Thread %d iter %d: parse_html failed\n",
              args->thread_id, i);
      args->errors++;
      continue;
    }

    lxb_dom_node_t *root = lxb_dom_interface_node(doc);

    /* Run CSS selectors (exercises thread-local css_parser/selectors) */
    const char *sel = css_selectors[i % num_selectors];
    int match_count = 0;
    lxb_status_t status = nl_node_find(root, sel, strlen(sel), &match_count);
    if (status != LXB_STATUS_OK) {
      fprintf(stderr, "Thread %d iter %d: nl_node_find status 0x%04x for '%s'\n",
              args->thread_id, i, (unsigned)status, sel);
      args->errors++;
    }

    lxb_html_document_destroy(doc);
  }

  return NULL;
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int
main(void)
{
  pthread_key_create(&p_key_css_parser, free_css_parser);
  pthread_key_create(&p_key_selectors, free_selectors);
  pthread_key_create(&p_key_html_parser, free_html_parser);

  printf("TSan thread-local cache test: %d threads x %d iterations\n",
         NUM_THREADS, ITERATIONS);

  pthread_t threads[NUM_THREADS];
  worker_args_t args[NUM_THREADS];
  int total_errors = 0;

  for (int i = 0; i < NUM_THREADS; i++) {
    args[i].thread_id = i;
    args[i].errors = 0;
    pthread_create(&threads[i], NULL, worker, &args[i]);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
    total_errors += args[i].errors;
  }

  /* Keys are destroyed here; destructors clean up per-thread caches */
  pthread_key_delete(p_key_css_parser);
  pthread_key_delete(p_key_selectors);
  pthread_key_delete(p_key_html_parser);

  if (total_errors > 0) {
    printf("FAIL: %d errors detected\n", total_errors);
    return EXIT_FAILURE;
  }

  printf("PASS: no errors, no TSan warnings expected\n");
  return EXIT_SUCCESS;
}
