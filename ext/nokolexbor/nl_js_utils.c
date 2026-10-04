/*
 * Nokolexbor::JSUtils.unescape_js: a byte-identical C port of SerpApi's
 * JSUtils.unescape_js, which runs eight sequential Onigmo passes over the
 * escaped JavaScript blobs embedded in Google result pages.
 *
 * The Ruby original (app/lib/js_utils.rb):
 *
 *   string.gsub!(/\\([0-7]{3})/) { [$1.to_i(8)].pack('U*') }
 *   string.gsub!(/\\x[0-9a-fA-F]{2}/, HEX_LOOKUP)
 *   string.gsub!(/\\x([0-9a-fA-F])/) { [$1.to_i(16)].pack('U*') }
 *   if string.include?('\u')
 *     if s = string.gsub(/(?<!\\)(\\u[0-9a-fA-F]{4})+/) { |m| JSON.load(%Q("#{m}")) rescue m } and s.valid_encoding?
 *       string = s
 *     else
 *       string = string.gsub(/(\\u[0-9a-fA-F]{4})+/) { |m| JSON.load(%Q("#{m}")) rescue m }
 *     end
 *   end
 *   string.gsub!(JS_UNESCAPES_RE) { |m| JS_UNESCAPES[m[1..-1].downcase] }
 *   string.gsub!(ESCAPED_CONTROL_CHARS_RE, "")
 *   string.gsub!(CONTROL_CHARS_RE, "")
 *   string.gsub!(/\\./) { |m| UNESCAPE_CHARS[m] }
 *   string
 *
 * Four parts of that are contract, not detail:
 *
 *   - Passes cascade: a backslash produced by one pass is consumed by the next
 *     (\134 -> \ feeds the \xHH pass), so the passes cannot be fused.
 *   - The bang passes mutate the caller's string. When the \u branch runs, the
 *     method continues on a *new* string, so the caller keeps the result of the
 *     first three passes while the return value is a different object.
 *   - Onigmo raises ArgumentError on a broken byte sequence, and the \u pass can
 *     produce one (a lone low surrogate), so unescape_js can raise after having
 *     mutated its argument. A frozen argument raises FrozenError first, with or
 *     without a match.
 *   - JSON.load decodes a whole run of \uXXXX at once; json 2.6.2 pairs a high
 *     surrogate with whatever \uXXXX follows it, encodes a lone low surrogate as
 *     invalid UTF-8, and raises on a high surrogate at the end of the run, which
 *     `rescue m` turns into "keep the run verbatim".
 */
#include "nokolexbor.h"
#include <ruby/encoding.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(NOKOLEXBOR_ASAN) || defined(NOKOLEXBOR_DEBUG)
#include <assert.h>
#define NL_ASSERT(x) assert(x)
#else
#define NL_ASSERT(x) ((void)0)
#endif

static VALUE mNokolexborJSUtils;

typedef struct {
  char *data;
  size_t len;
  size_t cap;
  size_t hint; /* the argument's length: enough for any pass, see below */
} nl_buf;

typedef struct {
  const char *data;
  size_t len;
} nl_slice;

static void
nl_buf_free(nl_buf *buf)
{
  free(buf->data);
  buf->data = NULL;
  buf->len = 0;
  buf->cap = 0;
}

/* No pass can grow its input: \NNN and \xHH shrink to at most two bytes, \xH to
 * one, a \uXXXX run to at most half its length (a surrogate pair is twelve bytes
 * in and four out), \\uXXXX to at most one, the control passes to nothing, and
 * the final /\\./ keeps \u and \x and shrinks everything else. So one allocation
 * of the argument's length is always enough; growing by doubling would only hand
 * the allocator a churn of odd sizes to retain. */
static int
nl_buf_reserve(nl_buf *buf, size_t extra)
{
  size_t need = buf->len + extra;
  char *data;

  /* The bound the comment above claims, checked on every write in a debug or
   * ASAN build: no pass may ever produce more bytes than the argument had. */
  NL_ASSERT(buf->hint == 0 || need <= buf->hint);

  if (need <= buf->cap) {
    return 1;
  }
  if (need < buf->hint) {
    need = buf->hint;
  }
  data = (char *)realloc(buf->data, need);
  if (data == NULL) {
    return 0;
  }
  buf->data = data;
  buf->cap = need;
  return 1;
}

static int
nl_buf_cat(nl_buf *buf, const char *src, size_t len)
{
  if (len == 0) {
    return 1;
  }
  if (!nl_buf_reserve(buf, len)) {
    return 0;
  }
  memcpy(buf->data + buf->len, src, len);
  buf->len += len;
  return 1;
}

static int
nl_buf_putc(nl_buf *buf, char c)
{
  if (!nl_buf_reserve(buf, 1)) {
    return 0;
  }
  buf->data[buf->len++] = c;
  return 1;
}

static const signed char nl_hex_value[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

#define NL_HEX(c) (nl_hex_value[(unsigned char)(c)] >= 0)
#define NL_OCT(c) ((c) >= '0' && (c) <= '7')

static const char nl_control_char[256] = {
    [0x03] = 1, [0x0b] = 1, [0x19] = 1, [0x1c] = 1, [0x1d] = 1,
};

/* json 2.6.2's convert_UTF32_to_UTF8, surrogate-blind on purpose. */
static int
nl_utf32_to_utf8(char *buf, uint32_t ch)
{
  int len = 1;
  if (ch <= 0x7F) {
    buf[0] = (char)ch;
  } else if (ch <= 0x07FF) {
    buf[0] = (char)((ch >> 6) | 0xC0);
    buf[1] = (char)((ch & 0x3F) | 0x80);
    len++;
  } else if (ch <= 0xFFFF) {
    buf[0] = (char)((ch >> 12) | 0xE0);
    buf[1] = (char)(((ch >> 6) & 0x3F) | 0x80);
    buf[2] = (char)((ch & 0x3F) | 0x80);
    len += 2;
  } else if (ch <= 0x1fffff) {
    buf[0] = (char)((ch >> 18) | 0xF0);
    buf[1] = (char)(((ch >> 12) & 0x3F) | 0x80);
    buf[2] = (char)(((ch >> 6) & 0x3F) | 0x80);
    buf[3] = (char)((ch & 0x3F) | 0x80);
    len += 3;
  } else {
    buf[0] = '?';
  }
  return len;
}

/* [codepoint].pack("U*") for the 0..0x1FF the octal and \xHH passes can reach. */
static int
nl_pack_u(char *buf, uint32_t cp)
{
  if (cp <= 0x7F) {
    buf[0] = (char)cp;
    return 1;
  }
  buf[0] = (char)((cp >> 6) | 0xC0);
  buf[1] = (char)((cp & 0x3F) | 0x80);
  return 2;
}

static uint32_t
nl_hex4(const char *p)
{
  return ((uint32_t)nl_hex_value[(unsigned char)p[0]] << 12) |
         ((uint32_t)nl_hex_value[(unsigned char)p[1]] << 8) |
         ((uint32_t)nl_hex_value[(unsigned char)p[2]] << 4) |
         (uint32_t)nl_hex_value[(unsigned char)p[3]];
}

/* Copies [i, next backslash after i) and leaves i on that backslash, or copies
 * the rest. Only called once position i is known not to start a match, so the
 * next candidate can only be a later backslash. */
#define NL_SKIP_TO_NEXT_BACKSLASH(s, n, i, out)                              \
  do {                                                                       \
    const char *nl_hit_ = (i) + 1 < (n)                                      \
                              ? (const char *)memchr((s) + (i) + 1, '\\', (n) - (i) - 1) \
                              : NULL;                                        \
    if (nl_hit_ == NULL) {                                                   \
      nl_buf_cat((out), (s) + (i), (n) - (i));                               \
      (i) = (n);                                                             \
    } else {                                                                 \
      nl_buf_cat((out), (s) + (i), (size_t)(nl_hit_ - ((s) + (i))));         \
      (i) = (size_t)(nl_hit_ - (s));                                         \
    }                                                                        \
  } while (0)

/* Finds the first backslash at or after i that starts a match, or n. */
static size_t
nl_first_match(const char *s, size_t n, size_t i, int (*matches)(const char *, size_t, size_t, void *), void *ctx)
{
  while (i < n) {
    const char *hit = (const char *)memchr(s + i, '\\', n - i);
    if (hit == NULL) {
      return n;
    }
    i = (size_t)(hit - s);
    if (matches(s, n, i, ctx)) {
      return i;
    }
    i++;
  }
  return n;
}

/* Each pass returns 1 when it changed its input (the result is in `out`) and 0
 * when it matched nothing, in which case the caller keeps the source. */

/* /\\([0-7]{3})/ { [$1.to_i(8)].pack('U*') } */
static int
nl_octal_at(const char *s, size_t n, size_t i, void *ctx)
{
  (void)ctx;
  return i + 3 < n && NL_OCT(s[i + 1]) && NL_OCT(s[i + 2]) && NL_OCT(s[i + 3]);
}

static int
nl_pass_octal(const char *s, size_t n, nl_buf *out)
{
  size_t i = nl_first_match(s, n, 0, nl_octal_at, NULL);
  char tmp[4];

  if (i >= n) {
    return 0;
  }
  nl_buf_cat(out, s, i);
  while (i < n) {
    if (s[i] == '\\' && nl_octal_at(s, n, i, NULL)) {
      uint32_t cp = (uint32_t)(((s[i + 1] - '0') << 6) | ((s[i + 2] - '0') << 3) | (s[i + 3] - '0'));
      nl_buf_cat(out, tmp, (size_t)nl_pack_u(tmp, cp));
      i += 4;
    } else {
      NL_SKIP_TO_NEXT_BACKSLASH(s, n, i, out);
    }
  }
  return 1;
}

/* /\\x[0-9a-fA-F]{2}/ with HEX_LOOKUP and /\\x([0-9a-fA-F])/ with pack('U*')
 * both map the hex value to its UTF-8 character; `digits` picks the pass. */
static int
nl_hex_at(const char *s, size_t n, size_t i, void *ctx)
{
  int digits = *(int *)ctx;
  return i + 1 + (size_t)digits < n && s[i + 1] == 'x' && NL_HEX(s[i + 2]) &&
         (digits == 1 || NL_HEX(s[i + 3]));
}

static int
nl_pass_hex(const char *s, size_t n, nl_buf *out, int digits)
{
  size_t i = nl_first_match(s, n, 0, nl_hex_at, &digits);
  char tmp[4];

  if (i >= n) {
    return 0;
  }
  nl_buf_cat(out, s, i);
  while (i < n) {
    if (s[i] == '\\' && nl_hex_at(s, n, i, &digits)) {
      uint32_t cp = (uint32_t)nl_hex_value[(unsigned char)s[i + 2]];
      if (digits == 2) {
        cp = (cp << 4) | (uint32_t)nl_hex_value[(unsigned char)s[i + 3]];
      }
      nl_buf_cat(out, tmp, (size_t)nl_pack_u(tmp, cp));
      i += 2 + (size_t)digits;
    } else {
      NL_SKIP_TO_NEXT_BACKSLASH(s, n, i, out);
    }
  }
  return 1;
}

/* JSON.load(%Q("#{run}")) for a run of \uXXXX escapes, mirroring json 2.6.2's
 * json_string_unescape. Returns 0 where json raises (an incomplete surrogate
 * pair), which the Ruby code turns into `rescue m`. */
static int
nl_decode_u_run(const char *run, size_t len, nl_buf *out)
{
  const char *p = run;
  const char *end = run + len;
  char buf[4];

  while (p < end) {
    const char *pe = p + 1; /* at 'u' */
    uint32_t ch;

    if (pe > end - 4) {
      return 0;
    }
    ch = nl_hex4(pe + 1);
    pe += 4; /* at the last hex digit */

    if ((ch & 0xFC00) == 0xD800) {
      pe++;
      if (pe > end - 6) {
        return 0;
      }
      if (pe[0] == '\\' && pe[1] == 'u') {
        uint32_t sur = nl_hex4(pe + 2);
        ch = (((ch & 0x3F) << 10) | ((((ch >> 6) & 0xF) + 1) << 16) | (sur & 0x3FF));
        pe += 5;
      } else {
        nl_buf_putc(out, '?');
        p = pe + 1;
        continue;
      }
    }
    nl_buf_cat(out, buf, (size_t)nl_utf32_to_utf8(buf, ch));
    p = pe + 1;
  }
  return 1;
}

/* /(?<!\\)(\\u[0-9a-fA-F]{4})+/ (lookbehind = 1) or /(\\u[0-9a-fA-F]{4})+/ */
static int
nl_u_escape_at(const char *s, size_t n, size_t i)
{
  return i + 5 < n && s[i + 1] == 'u' && NL_HEX(s[i + 2]) && NL_HEX(s[i + 3]) &&
         NL_HEX(s[i + 4]) && NL_HEX(s[i + 5]);
}

static int
nl_u_match_at(const char *s, size_t n, size_t i, void *ctx)
{
  int lookbehind = *(int *)ctx;
  return nl_u_escape_at(s, n, i) && !(lookbehind && i > 0 && s[i - 1] == '\\');
}

static int
nl_pass_u(const char *s, size_t n, nl_buf *out, int lookbehind)
{
  size_t i = nl_first_match(s, n, 0, nl_u_match_at, &lookbehind);

  if (i >= n) {
    return 0;
  }
  nl_buf_cat(out, s, i);
  while (i < n) {
    if (s[i] == '\\' && nl_u_match_at(s, n, i, &lookbehind)) {
      size_t j = i;
      size_t mark = out->len;

      while (j < n && s[j] == '\\' && nl_u_escape_at(s, n, j)) {
        j += 6;
      }
      if (!nl_decode_u_run(s + i, j - i, out)) {
        out->len = mark;
        nl_buf_cat(out, s + i, j - i);
      }
      i = j;
    } else {
      NL_SKIP_TO_NEXT_BACKSLASH(s, n, i, out);
    }
  }
  return 1;
}

/* JS_UNESCAPES_RE and ESCAPED_CONTROL_CHARS_RE share one shape:
 * /(?<!\\)(\\\\u[0-9a-fA-F]{4})/i over a fixed set of code points. The /i and
 * the downcase before the hash lookup make a numeric comparison exact. */
static const char *
nl_double_u_replacement(uint32_t code, size_t *repl_len, int escaped_control)
{
  *repl_len = 1;
  if (escaped_control) {
    switch (code) {
      case 0x0003:
      case 0x000b:
      case 0x0019:
      case 0x001c:
      case 0x001d:
        *repl_len = 0;
        return "";
      default:
        return NULL;
    }
  }
  switch (code) {
    case 0x005c: return "\\";
    case 0x0027: return "'";
    case 0x0022: return "\"";
    case 0x003e: return ">";
    case 0x003c: return "<";
    case 0x0026: return "&";
    case 0x003d: return "=";
    case 0x002d: return "-";
    case 0x003b: return ";";
    case 0x0060: return "`";
    default: return NULL;
  }
}

static int
nl_double_u_at(const char *s, size_t n, size_t i, void *ctx)
{
  int escaped_control = *(int *)ctx;
  size_t repl_len;

  if (i + 6 >= n || s[i + 1] != '\\' || (s[i + 2] | 0x20) != 'u') {
    return 0;
  }
  if (!NL_HEX(s[i + 3]) || !NL_HEX(s[i + 4]) || !NL_HEX(s[i + 5]) || !NL_HEX(s[i + 6])) {
    return 0;
  }
  if (i > 0 && s[i - 1] == '\\') {
    return 0;
  }
  return nl_double_u_replacement(nl_hex4(s + i + 3), &repl_len, escaped_control) != NULL;
}

static int
nl_pass_double_u(const char *s, size_t n, nl_buf *out, int escaped_control)
{
  size_t i = nl_first_match(s, n, 0, nl_double_u_at, &escaped_control);

  if (i >= n) {
    return 0;
  }
  nl_buf_cat(out, s, i);
  while (i < n) {
    if (s[i] == '\\' && nl_double_u_at(s, n, i, &escaped_control)) {
      size_t repl_len;
      const char *repl = nl_double_u_replacement(nl_hex4(s + i + 3), &repl_len, escaped_control);
      nl_buf_cat(out, repl, repl_len);
      i += 7;
    } else {
      NL_SKIP_TO_NEXT_BACKSLASH(s, n, i, out);
    }
  }
  return 1;
}

/* CONTROL_CHARS_RE: /(\x03|\x0b|\x19|\x1d|\x1c)/ -> "" */
static int
nl_pass_control(const char *s, size_t n, nl_buf *out)
{
  size_t i = 0;

  while (i < n && !nl_control_char[(unsigned char)s[i]]) {
    i++;
  }
  if (i >= n) {
    return 0;
  }
  nl_buf_cat(out, s, i);
  while (i < n) {
    size_t run;
    while (i < n && nl_control_char[(unsigned char)s[i]]) {
      i++;
    }
    run = i;
    while (run < n && !nl_control_char[(unsigned char)s[run]]) {
      run++;
    }
    nl_buf_cat(out, s + i, run - i);
    i = run;
  }
  return 1;
}

static size_t
nl_utf8_char_len(unsigned char c)
{
  if (c < 0x80) {
    return 1;
  }
  if ((c & 0xE0) == 0xC0) {
    return 2;
  }
  if ((c & 0xF0) == 0xE0) {
    return 3;
  }
  if ((c & 0xF8) == 0xF0) {
    return 4;
  }
  return 1;
}

/* /\\./ { |m| UNESCAPE_CHARS[m] }: "." never matches a newline, and the default
 * proc returns the character after the backslash. */
static int
nl_backslash_dot_at(const char *s, size_t n, size_t i, void *ctx)
{
  (void)ctx;
  return i + 1 < n && s[i + 1] != '\n';
}

static int
nl_pass_backslash_dot(const char *s, size_t n, nl_buf *out)
{
  size_t i = nl_first_match(s, n, 0, nl_backslash_dot_at, NULL);

  if (i >= n) {
    return 0;
  }
  nl_buf_cat(out, s, i);
  while (i < n) {
    if (s[i] == '\\' && i + 1 < n && s[i + 1] != '\n') {
      size_t clen = nl_utf8_char_len((unsigned char)s[i + 1]);
      if (i + 1 + clen > n) {
        clen = n - (i + 1);
      }
      if (clen == 1) {
        switch (s[i + 1]) {
          case '0': nl_buf_putc(out, '\0'); break;
          case 'a': nl_buf_putc(out, '\a'); break;
          case 'b': nl_buf_putc(out, '\b'); break;
          case 't': nl_buf_putc(out, '\t'); break;
          case 'n': nl_buf_putc(out, '\n'); break;
          case 'v': nl_buf_putc(out, '\v'); break;
          case 'f': nl_buf_putc(out, '\f'); break;
          case 'r': nl_buf_putc(out, '\r'); break;
          case 'u': nl_buf_cat(out, "\\u", 2); break;
          case 'x': nl_buf_cat(out, "\\x", 2); break;
          default: nl_buf_putc(out, s[i + 1]); break;
        }
      } else {
        nl_buf_cat(out, s + i + 1, clen);
      }
      i += 1 + clen;
    } else {
      NL_SKIP_TO_NEXT_BACKSLASH(s, n, i, out);
    }
  }
  return 1;
}

static int
nl_utf8_valid(const char *s, size_t n)
{
  size_t i = 0;

  while (i < n) {
    unsigned char c = (unsigned char)s[i];
    size_t len, k;
    uint32_t cp;

    if (c < 0x80) {
      i++;
      continue;
    }
    if ((c & 0xE0) == 0xC0) {
      len = 2;
      cp = c & 0x1Fu;
    } else if ((c & 0xF0) == 0xE0) {
      len = 3;
      cp = c & 0x0Fu;
    } else if ((c & 0xF8) == 0xF0) {
      len = 4;
      cp = c & 0x07u;
    } else {
      return 0;
    }
    if (i + len > n) {
      return 0;
    }
    for (k = 1; k < len; k++) {
      unsigned char cc = (unsigned char)s[i + k];
      if ((cc & 0xC0) != 0x80) {
        return 0;
      }
      cp = (cp << 6) | (cc & 0x3Fu);
    }
    if (len == 2 ? cp < 0x80 : (len == 3 ? cp < 0x800 : cp < 0x10000)) {
      return 0;
    }
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
      return 0;
    }
    i += len;
  }
  return 1;
}

static int
nl_includes_backslash_u(const char *s, size_t n)
{
  size_t i = 0;

  while (i + 1 < n) {
    const char *hit = (const char *)memchr(s + i, '\\', n - i - 1);
    if (hit == NULL) {
      return 0;
    }
    i = (size_t)(hit - s);
    if (s[i + 1] == 'u') {
      return 1;
    }
    i++;
  }
  return 0;
}

typedef struct {
  nl_buf buf[2];     /* ping-pong for the in-place passes */
  nl_buf ubuf;       /* the \u pass writes here so stage3 survives */
  nl_buf stage3;     /* the caller's string keeps this when the \u branch runs */
  nl_slice cur;
  nl_slice stage3_slice;
  int reassigned;    /* the \u branch produced a new string */
  int broken;        /* Onigmo raises ArgumentError on the \u result */
} nl_js_state;

static void
nl_js_state_free(nl_js_state *st)
{
  nl_buf_free(&st->buf[0]);
  nl_buf_free(&st->buf[1]);
  nl_buf_free(&st->ubuf);
  nl_buf_free(&st->stage3);
}

/* Scratch buffers reused across calls. Allocating three buffers per call, each
 * the size of that call's argument, hands the allocator a different size class
 * every time and it keeps the regions: 83,226 real inputs left 29.7 MB of RSS
 * behind that the Ruby passes did not. Reusing one set of buffers makes the
 * steady state a handful of allocations instead of a quarter of a million.
 *
 * One global set is enough because the passes never release the GVL, so two
 * Ruby threads cannot be inside them at once; a Ractor would break that, as it
 * would break reading the argument in place. The largest input a Google page
 * has produced is 375 KB, so NL_SCRATCH_KEEP bounds the idle cost at 4 MB and
 * still returns anything pathological instead of pinning it.
 */
#define NL_SCRATCH_KEEP (1024 * 1024)

static nl_js_state nl_scratch;
static int nl_scratch_busy;

static void
nl_js_state_release(nl_js_state *st)
{
  if (st != &nl_scratch) {
    nl_js_state_free(st);
    return;
  }
  if (st->buf[0].cap > NL_SCRATCH_KEEP) {
    nl_buf_free(&st->buf[0]);
  }
  if (st->buf[1].cap > NL_SCRATCH_KEEP) {
    nl_buf_free(&st->buf[1]);
  }
  if (st->ubuf.cap > NL_SCRATCH_KEEP) {
    nl_buf_free(&st->ubuf);
  }
  if (st->stage3.cap > NL_SCRATCH_KEEP) {
    nl_buf_free(&st->stage3);
  }
  nl_scratch_busy = 0;
}

static void
nl_unescape_js_run(nl_js_state *st, const char *src, size_t src_len)
{
  int which = 0;
  int lookbehind;

#define NL_RUN(call)                                 \
  do {                                               \
    nl_buf *out_ = &st->buf[which];                  \
    out_->len = 0;                                   \
    if (call) {                                      \
      st->cur.data = out_->data;                     \
      st->cur.len = out_->len;                       \
      which ^= 1;                                    \
    }                                                \
  } while (0)

  st->cur.data = src;
  st->cur.len = src_len;

  NL_RUN(nl_pass_octal(st->cur.data, st->cur.len, out_));
  NL_RUN(nl_pass_hex(st->cur.data, st->cur.len, out_, 2));
  NL_RUN(nl_pass_hex(st->cur.data, st->cur.len, out_, 1));

  if (nl_includes_backslash_u(st->cur.data, st->cur.len)) {
    nl_slice stage = st->cur;

    st->reassigned = 1;
    /* The argument keeps `stage`. When no pass touched it, that is still the
     * argument's own buffer, so there is nothing to copy and nothing to write
     * back; otherwise copy it out of the ping-pong buffers before reusing them. */
    if (stage.data == src) {
      st->stage3_slice = stage;
    } else {
      nl_buf_cat(&st->stage3, stage.data, stage.len);
      st->stage3_slice.data = st->stage3.data;
      st->stage3_slice.len = st->stage3.len;
    }

    lookbehind = 1;
    if (nl_pass_u(stage.data, stage.len, &st->ubuf, lookbehind)) {
      st->cur.data = st->ubuf.data;
      st->cur.len = st->ubuf.len;
    }
    if (!nl_utf8_valid(st->cur.data, st->cur.len)) {
      lookbehind = 0;
      st->ubuf.len = 0;
      if (nl_pass_u(stage.data, stage.len, &st->ubuf, lookbehind)) {
        st->cur.data = st->ubuf.data;
        st->cur.len = st->ubuf.len;
      } else {
        st->cur = st->stage3_slice;
      }
      if (!nl_utf8_valid(st->cur.data, st->cur.len)) {
        st->broken = 1;
        return;
      }
    }
    if (st->cur.data != st->ubuf.data) {
      st->cur = st->stage3_slice;
    }
    st->buf[0].len = 0;
    st->buf[1].len = 0;
    which = 0;
  }

  NL_RUN(nl_pass_double_u(st->cur.data, st->cur.len, out_, 0));
  NL_RUN(nl_pass_double_u(st->cur.data, st->cur.len, out_, 1));
  NL_RUN(nl_pass_control(st->cur.data, st->cur.len, out_));
  NL_RUN(nl_pass_backslash_dot(st->cur.data, st->cur.len, out_));
#undef NL_RUN
}

static void
nl_str_set(VALUE str, const char *data, size_t len)
{
  rb_str_modify(str);
  rb_str_resize(str, (long)len);
  if (len > 0) {
    memcpy(RSTRING_PTR(str), data, len);
  }
  rb_enc_associate(str, rb_utf8_encoding());
  ENC_CODERANGE_CLEAR(str);
}

/*
 * call-seq: Nokolexbor::JSUtils.unescape_js(string) -> String
 *
 * Byte-identical replacement for SerpApi's JSUtils.unescape_js, including its
 * in-place mutation of +string+, its object identity and its exceptions.
 */
static VALUE
nl_js_utils_unescape_js(VALUE self, VALUE rb_str)
{
  nl_js_state local;
  nl_js_state *st;
  VALUE result;
  const char *src;
  size_t src_len;

  Check_Type(rb_str, T_STRING);
  rb_check_frozen(rb_str);
  if (rb_enc_get(rb_str) != rb_utf8_encoding()) {
    rb_raise(rb_eArgError, "unescape_js needs a UTF-8 string, got %s", rb_enc_name(rb_enc_get(rb_str)));
  }
  if (rb_enc_str_coderange(rb_str) == ENC_CODERANGE_BROKEN) {
    rb_raise(rb_eArgError, "invalid byte sequence in UTF-8");
  }

  if (nl_scratch_busy) {
    st = &local;
    memset(st, 0, sizeof(*st));
  } else {
    st = &nl_scratch;
    nl_scratch_busy = 1;
    st->buf[0].len = 0;
    st->buf[1].len = 0;
    st->ubuf.len = 0;
    st->stage3.len = 0;
    st->cur.data = NULL;
    st->cur.len = 0;
    st->stage3_slice.data = NULL;
    st->stage3_slice.len = 0;
    st->reassigned = 0;
    st->broken = 0;
  }

  /* The passes call no Ruby API, so nothing can move or free the argument's
   * buffer while they run and they can read it in place. Reinstate a copy here
   * before ever releasing the GVL. */
  src = RSTRING_PTR(rb_str);
  src_len = (size_t)RSTRING_LEN(rb_str);
  st->buf[0].hint = src_len;
  st->buf[1].hint = src_len;
  st->ubuf.hint = src_len;
  st->stage3.hint = src_len;

  nl_unescape_js_run(st, src, src_len);

  if (st->broken) {
    if (st->stage3_slice.data != src) {
      nl_str_set(rb_str, st->stage3_slice.data, st->stage3_slice.len);
    }
    nl_js_state_release(st);
    rb_raise(rb_eArgError, "invalid byte sequence in UTF-8");
  }

  if (st->reassigned) {
    result = rb_utf8_str_new(st->cur.data, (long)st->cur.len);
    if (st->stage3_slice.data != src) {
      nl_str_set(rb_str, st->stage3_slice.data, st->stage3_slice.len);
    }
  } else {
    /* gsub! leaves the string alone when nothing matched, coderange included. */
    if (st->cur.data != src) {
      nl_str_set(rb_str, st->cur.data, st->cur.len);
    }
    result = rb_str;
  }

  nl_js_state_release(st);
  return result;
}

void Init_nl_js_utils(void)
{
  mNokolexborJSUtils = rb_define_module_under(mNokolexbor, "JSUtils");
  rb_define_singleton_method(mNokolexborJSUtils, "unescape_js", nl_js_utils_unescape_js, 1);
}
