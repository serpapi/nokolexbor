#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lexbor/css/css.h>
#include <lexbor/html/html.h>
#include <lexbor/selectors/selectors.h>

typedef struct {
    lxb_selectors_t *selectors;
    size_t matches;
    int stale;
} recovery_context_t;

static lxb_status_t
interrupt_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec,
                   void *context)
{
    (void) node;
    (void) spec;
    longjmp(*(jmp_buf *) context, 1);
    return LXB_STATUS_ERROR;
}

static lxb_status_t
verify_callback(lxb_dom_node_t *node, lxb_css_selector_specificity_t *spec,
                void *context)
{
    recovery_context_t *recovery = context;
    (void) node;
    (void) spec;

    if (recovery->selectors->index != NULL) {
        recovery->stale = 1;
        return LXB_STATUS_ERROR;
    }

    recovery->matches++;
    return LXB_STATUS_OK;
}

int
main(void)
{
    static const char element[] = "<div class='hit'></div>";
    static const lxb_char_t query[] = ".hit, .missing, [data-missing]";
    char html[8192] = "<main>";
    size_t length = strlen(html);
    jmp_buf interruption;
    lxb_status_t status;
    recovery_context_t recovery = {0};

    for (size_t i = 0; i < 160; i++) {
        if (length + sizeof(element) + sizeof("</main>") > sizeof(html)) {
            return EXIT_FAILURE;
        }
        memcpy(html + length, element, sizeof(element) - 1);
        length += sizeof(element) - 1;
    }
    memcpy(html + length, "</main>", sizeof("</main>"));

    lxb_html_document_t *first = lxb_html_document_create();
    lxb_html_document_t *second = lxb_html_document_create();
    lxb_css_parser_t *parser = lxb_css_parser_create();
    lxb_selectors_t *selectors = lxb_selectors_create();
    if (first == NULL || second == NULL || parser == NULL || selectors == NULL
        || lxb_html_document_parse(first, (const lxb_char_t *) html, strlen(html)) != LXB_STATUS_OK
        || lxb_html_document_parse(second, (const lxb_char_t *) html, strlen(html)) != LXB_STATUS_OK
        || lxb_css_parser_init(parser, NULL, NULL) != LXB_STATUS_OK
        || lxb_selectors_init(selectors) != LXB_STATUS_OK)
    {
        return EXIT_FAILURE;
    }

    lxb_css_selector_list_t *list = lxb_css_selectors_parse_relative_list(
        parser, query, sizeof(query) - 1);
    if (parser->status != LXB_STATUS_OK || list == NULL) {
        return EXIT_FAILURE;
    }

    selectors->use_index = true;
    if (setjmp(interruption) == 0) {
        (void) lxb_selectors_find(selectors, lxb_dom_interface_node(first),
                                  list, interrupt_callback, &interruption);
        fprintf(stderr, "selector callback did not interrupt the search\n");
        return EXIT_FAILURE;
    }
    if (selectors->index == NULL) {
        fprintf(stderr, "the interrupted search never built an index\n");
        return EXIT_FAILURE;
    }

    recovery.selectors = selectors;
    selectors->use_index = false;
    status = lxb_selectors_find(selectors, lxb_dom_interface_node(second),
                                list, verify_callback, &recovery);
    if (status != LXB_STATUS_OK || recovery.stale || recovery.matches != 160) {
        fprintf(stderr, "stale index after interruption: status=%d matches=%zu stale=%d\n",
                status, recovery.matches, recovery.stale);
        return EXIT_FAILURE;
    }

    lxb_selectors_destroy(selectors, true);
    lxb_css_selector_list_destroy_memory(list);
    lxb_css_parser_destroy(parser, true);
    lxb_html_document_destroy(first);
    lxb_html_document_destroy(second);
    puts("selector index recovered after interrupted search");
    return EXIT_SUCCESS;
}
