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

#include <stdint.h>

static pthread_key_t p_key_css_parser;
static pthread_key_t p_key_selectors;
static pthread_key_t p_key_selector_cache;
static pthread_key_t p_key_html_parser;

#define NL_SELECTOR_CACHE_CAP 512
#define NL_SELECTOR_HASH_BUCKETS 1024

typedef struct nl_selector_cache_entry {
  char *str;
  size_t len;
  uint64_t hash;
  lxb_css_selector_list_t *list;
  struct nl_selector_cache_entry *prev;
  struct nl_selector_cache_entry *next;
  struct nl_selector_cache_entry *hash_next;
} nl_selector_cache_entry_t;

typedef struct {
  nl_selector_cache_entry_t *head;
  nl_selector_cache_entry_t *tail;
  size_t count;
  nl_selector_cache_entry_t *buckets[NL_SELECTOR_HASH_BUCKETS];
} nl_selector_cache_t;

static inline uint64_t
nl_fnv1a_hash(const char *str, size_t len)
{
  uint64_t hash = 14695981039346656037ULL;
  for (size_t i = 0; i < len; i++) {
    hash ^= (uint8_t)str[i];
    hash *= 1099511628211ULL;
  }
  return hash;
}

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
free_selector_cache(void *data)
{
  if (data != NULL) {
    nl_selector_cache_t *cache = (nl_selector_cache_t *)data;
    nl_selector_cache_entry_t *entry = cache->head;
    while (entry != NULL) {
      nl_selector_cache_entry_t *next = entry->next;
      if (entry->list != NULL) {
        lxb_css_selector_list_destroy_memory(entry->list);
      }
      free(entry->str);
      free(entry);
      entry = next;
    }
    free(cache);
  }
}

static void
free_html_parser(void *data)
{
  if (data != NULL)
    lxb_html_parser_destroy((lxb_html_parser_t *)data);
}

static void
nl_cache_detach_lru(nl_selector_cache_t *cache, nl_selector_cache_entry_t *entry)
{
  if (entry->prev != NULL) {
    entry->prev->next = entry->next;
  } else {
    cache->head = entry->next;
  }
  if (entry->next != NULL) {
    entry->next->prev = entry->prev;
  } else {
    cache->tail = entry->prev;
  }
  entry->prev = NULL;
  entry->next = NULL;
}

static void
nl_cache_attach_front(nl_selector_cache_t *cache, nl_selector_cache_entry_t *entry)
{
  entry->prev = NULL;
  entry->next = cache->head;
  if (cache->head != NULL) {
    cache->head->prev = entry;
  } else {
    cache->tail = entry;
  }
  cache->head = entry;
}

static lxb_css_selector_list_t *
nl_cache_get(nl_selector_cache_t *cache, const char *str, size_t len, uint64_t hash)
{
  size_t bucket = (size_t)(hash & (NL_SELECTOR_HASH_BUCKETS - 1));
  nl_selector_cache_entry_t *entry = cache->buckets[bucket];
  while (entry != NULL) {
    if (entry->hash == hash && entry->len == len && memcmp(entry->str, str, len) == 0) {
      if (cache->head != entry) {
        nl_cache_detach_lru(cache, entry);
        nl_cache_attach_front(cache, entry);
      }
      return entry->list;
    }
    entry = entry->hash_next;
  }
  return NULL;
}

static bool
nl_cache_put(nl_selector_cache_t *cache, const char *str, size_t len, uint64_t hash, lxb_css_selector_list_t *list)
{
  if (cache->count >= NL_SELECTOR_CACHE_CAP && cache->tail != NULL) {
    nl_selector_cache_entry_t *victim = cache->tail;
    nl_cache_detach_lru(cache, victim);

    size_t v_bucket = (size_t)(victim->hash & (NL_SELECTOR_HASH_BUCKETS - 1));
    nl_selector_cache_entry_t **pp = &cache->buckets[v_bucket];
    while (*pp != NULL) {
      if (*pp == victim) {
        *pp = victim->hash_next;
        break;
      }
      pp = &((*pp)->hash_next);
    }

    if (victim->list != NULL) {
      lxb_css_selector_list_destroy_memory(victim->list);
    }
    free(victim->str);
    free(victim);
    cache->count--;
  }

  nl_selector_cache_entry_t *entry = (nl_selector_cache_entry_t *)malloc(sizeof(nl_selector_cache_entry_t));
  if (entry == NULL) return false;

  entry->str = (char *)malloc(len + 1);
  if (entry->str == NULL) {
    free(entry);
    return false;
  }
  memcpy(entry->str, str, len);
  entry->str[len] = '\0';
  entry->len = len;
  entry->hash = hash;
  entry->list = list;
  entry->prev = NULL;
  entry->next = NULL;

  size_t bucket = (size_t)(hash & (NL_SELECTOR_HASH_BUCKETS - 1));
  entry->hash_next = cache->buckets[bucket];
  cache->buckets[bucket] = entry;

  nl_cache_attach_front(cache, entry);
  cache->count++;
  return true;
}

/* Dummy callback for lxb_selectors_find */
static lxb_status_t
selector_cb(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec, void *ctx)
{
  (void)node;
  (void)spec;
  (*(int *)ctx)++;
  return LXB_STATUS_OK;
}

/* ------------------------------------------------------------------ */
/* Mirrors nl_node_find() — thread-local cached css_parser/selectors   */
/* and thread-local LRU selector cache                                 */
/* ------------------------------------------------------------------ */

static lxb_status_t
nl_node_find(lxb_dom_node_t *node, const char *selector_str, size_t selector_len, int *match_count)
{
  lxb_status_t status;
  lxb_css_parser_t *css_parser = (lxb_css_parser_t *)pthread_getspecific(p_key_css_parser);
  lxb_selectors_t *selectors = (lxb_selectors_t *)pthread_getspecific(p_key_selectors);
  nl_selector_cache_t *cache = (nl_selector_cache_t *)pthread_getspecific(p_key_selector_cache);
  lxb_css_selector_list_t *list = NULL;
  bool destroy_list_on_exit = false;

  if (selectors == NULL) {
    selectors = lxb_selectors_create();
    status = lxb_selectors_init(selectors);
    if (status != LXB_STATUS_OK)
      goto init_error;
    pthread_setspecific(p_key_selectors, selectors);
  }

  if (cache == NULL) {
    cache = (nl_selector_cache_t *)calloc(1, sizeof(nl_selector_cache_t));
    if (cache != NULL) {
      pthread_setspecific(p_key_selector_cache, cache);
    }
  }

  uint64_t hash = nl_fnv1a_hash(selector_str, selector_len);
  if (cache != NULL) {
    list = nl_cache_get(cache, selector_str, selector_len, hash);
  }

  if (list == NULL) {
    if (css_parser == NULL) {
      css_parser = lxb_css_parser_create();
      status = lxb_css_parser_init(css_parser, NULL, NULL);
      if (status != LXB_STATUS_OK)
        goto init_error;
      pthread_setspecific(p_key_css_parser, css_parser);
    }

    list = lxb_css_selectors_parse_relative_list(css_parser,
               (const lxb_char_t *)selector_str, selector_len);
    if (css_parser->status != LXB_STATUS_OK) {
      status = css_parser->status;
      destroy_list_on_exit = true;
      goto cleanup;
    }

    if (cache == NULL || !nl_cache_put(cache, selector_str, selector_len, hash, list)) {
      destroy_list_on_exit = true;
    }
  }

  *match_count = 0;
  status = lxb_selectors_find(selectors, node, list, selector_cb, match_count);

cleanup:
  if (destroy_list_on_exit && list != NULL) {
    lxb_css_selector_list_destroy_memory(list);
  }
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
  pthread_key_create(&p_key_selector_cache, free_selector_cache);
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
  pthread_key_delete(p_key_selector_cache);
  pthread_key_delete(p_key_html_parser);

  if (total_errors > 0) {
    printf("FAIL: %d errors detected\n", total_errors);
    return EXIT_FAILURE;
  }

  printf("PASS: no errors, no TSan warnings expected\n");
  return EXIT_SUCCESS;
}
