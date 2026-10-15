/*
 * Nokolexbor::JSUtils.unescape_js: decodes the escape sequences of the body of
 * a JavaScript string literal in one pass (ECMAScript string literal escapes,
 * including Annex B legacy octal escapes). See the method documentation below
 * for the exact rules.
 */
#include "nokolexbor.h"
#include <ruby/encoding.h>
#include <string.h>
#include <stdint.h>

static VALUE mNokolexborJSUtils;

static int
nl_hex_digit(unsigned char c)
{
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

/* Reads exactly `count` hex digits at p; -1 if any is missing. */
static long
nl_hex_value(const unsigned char *p, const unsigned char *end, int count)
{
  long value = 0;

  if (end - p < count) {
    return -1;
  }
  for (int i = 0; i < count; i++) {
    int digit = nl_hex_digit(p[i]);
    if (digit < 0) {
      return -1;
    }
    value = (value << 4) | digit;
  }
  return value;
}

static unsigned char *
nl_put_utf8(unsigned char *out, uint32_t cp)
{
  if (cp >= 0xD800 && cp <= 0xDFFF) {
    cp = 0xFFFD;
  }
  if (cp < 0x80) {
    *out++ = (unsigned char)cp;
  } else if (cp < 0x800) {
    *out++ = (unsigned char)(0xC0 | (cp >> 6));
    *out++ = (unsigned char)(0x80 | (cp & 0x3F));
  } else if (cp < 0x10000) {
    *out++ = (unsigned char)(0xE0 | (cp >> 12));
    *out++ = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
    *out++ = (unsigned char)(0x80 | (cp & 0x3F));
  } else {
    *out++ = (unsigned char)(0xF0 | (cp >> 18));
    *out++ = (unsigned char)(0x80 | ((cp >> 12) & 0x3F));
    *out++ = (unsigned char)(0x80 | ((cp >> 6) & 0x3F));
    *out++ = (unsigned char)(0x80 | (cp & 0x3F));
  }
  return out;
}

/*
 * Decodes the escape at p (just after a backslash) into out. Returns the input
 * position after the escape, or NULL when the escape is malformed and has to
 * stay as written. Every escape decodes to at most as many bytes as it spans,
 * so the output never outgrows the input.
 */
static const unsigned char *
nl_unescape_one(const unsigned char *p, const unsigned char *end, unsigned char **out)
{
  unsigned char c = *p;
  long value;

  switch (c) {
    case 'b': *(*out)++ = '\b'; return p + 1;
    case 'f': *(*out)++ = '\f'; return p + 1;
    case 'n': *(*out)++ = '\n'; return p + 1;
    case 'r': *(*out)++ = '\r'; return p + 1;
    case 't': *(*out)++ = '\t'; return p + 1;
    case 'v': *(*out)++ = '\v'; return p + 1;
    case '\n': return p + 1;
    case '\r': return (p + 1 < end && p[1] == '\n') ? p + 2 : p + 1;
    case 'x':
      value = nl_hex_value(p + 1, end, 2);
      if (value < 0) {
        return NULL;
      }
      *out = nl_put_utf8(*out, (uint32_t)value);
      return p + 3;
    case 'u':
      if (p + 1 < end && p[1] == '{') {
        const unsigned char *q = p + 2;
        uint32_t cp = 0;
        while (q < end && nl_hex_digit(*q) >= 0) {
          cp = (cp << 4) | (uint32_t)nl_hex_digit(*q);
          if (cp > 0x10FFFF) {
            return NULL;
          }
          q++;
        }
        if (q == p + 2 || q >= end || *q != '}') {
          return NULL;
        }
        *out = nl_put_utf8(*out, cp);
        return q + 1;
      }
      value = nl_hex_value(p + 1, end, 4);
      if (value < 0) {
        return NULL;
      }
      if (value >= 0xD800 && value <= 0xDBFF && end - p >= 11 && p[5] == '\\' && p[6] == 'u') {
        long low = nl_hex_value(p + 7, end, 4);
        if (low >= 0xDC00 && low <= 0xDFFF) {
          *out = nl_put_utf8(*out, (uint32_t)(0x10000 + ((value - 0xD800) << 10) + (low - 0xDC00)));
          return p + 11;
        }
      }
      *out = nl_put_utf8(*out, (uint32_t)value);
      return p + 5;
    case '8':
    case '9':
      *(*out)++ = c;
      return p + 1;
    default:
      break;
  }

  if (c >= '0' && c <= '7') {
    /* Legacy octal: \0 not followed by a digit is NUL; otherwise up to three
     * digits when the first is 0-3, up to two when it is 4-7. */
    int max_digits = c <= '3' ? 3 : 2;
    const unsigned char *q = p;
    uint32_t cp = 0;
    while (q < end && q - p < max_digits && *q >= '0' && *q <= '7') {
      cp = (cp << 3) | (uint32_t)(*q - '0');
      q++;
    }
    *out = nl_put_utf8(*out, cp);
    return q;
  }

  /* U+2028 and U+2029 are line terminators: the escape is a line continuation. */
  if (c == 0xE2 && end - p >= 3 && p[1] == 0x80 && (p[2] == 0xA8 || p[2] == 0xA9)) {
    return p + 3;
  }

  /* Any other character escapes to itself, multibyte characters included. */
  {
    size_t len = 1;
    if (c >= 0xF0) {
      len = 4;
    } else if (c >= 0xE0) {
      len = 3;
    } else if (c >= 0xC0) {
      len = 2;
    }
    if ((size_t)(end - p) < len) {
      len = (size_t)(end - p);
    }
    memcpy(*out, p, len);
    *out += len;
    return p + len;
  }
}

/*
 * call-seq: Nokolexbor::JSUtils.unescape_js(string) -> String
 *
 * Decodes the escape sequences of a JavaScript string literal's body and
 * returns a new UTF-8 String; +string+ itself is not modified.
 *
 * - <tt>\b \f \n \r \t \v</tt> become U+0008, U+000C, U+000A, U+000D, U+0009, U+000B.
 * - <tt>\xHH</tt> becomes U+00HH.
 * - <tt>\uHHHH</tt> is a UTF-16 code unit: a high surrogate followed by a low
 *   surrogate escape becomes one character, an unpaired surrogate becomes U+FFFD.
 * - <tt>\u{H...}</tt> becomes that code point (up to U+10FFFF; surrogates become U+FFFD).
 * - Legacy octal escapes: <tt>\0</tt> not followed by a digit is U+0000;
 *   <tt>\1</tt>-<tt>\377</tt> take up to three octal digits when the first is
 *   0-3 and up to two when it is 4-7; <tt>\8</tt> and <tt>\9</tt> are "8" and "9".
 * - A backslash before a line terminator (LF, CR, CRLF, U+2028, U+2029) is a
 *   line continuation: both are removed.
 * - A backslash before any other character yields that character.
 * - Where JavaScript would raise a SyntaxError (<tt>\x</tt> or <tt>\u</tt>
 *   without enough hex digits, a malformed <tt>\u{}</tt>, a trailing
 *   backslash) the text is kept as written.
 *
 * Raises ArgumentError unless +string+ is valid UTF-8, US-ASCII, or ASCII-only
 * in another ASCII-compatible encoding.
 */
static VALUE
nl_js_utils_unescape_js(VALUE self, VALUE rb_str)
{
  Check_Type(rb_str, T_STRING);

  rb_encoding *enc = rb_enc_get(rb_str);
  int coderange = rb_enc_str_coderange(rb_str);
  if (coderange == ENC_CODERANGE_BROKEN) {
    rb_raise(rb_eArgError, "invalid byte sequence in %s", rb_enc_name(enc));
  }
  if (coderange != ENC_CODERANGE_7BIT && enc != rb_utf8_encoding() && enc != rb_usascii_encoding()) {
    rb_raise(rb_eArgError, "unescape_js needs a UTF-8 string, got %s", rb_enc_name(enc));
  }

  long len = RSTRING_LEN(rb_str);
  VALUE result = rb_str_buf_new(len);

  /* No Ruby calls from here on, so neither buffer can move or be freed. */
  const unsigned char *src = (const unsigned char *)RSTRING_PTR(rb_str);
  const unsigned char *p = src;
  const unsigned char *end = src + len;
  unsigned char *start = (unsigned char *)RSTRING_PTR(result);
  unsigned char *out = start;

  while (p < end) {
    const unsigned char *slash = memchr(p, '\\', (size_t)(end - p));
    if (slash == NULL) {
      memcpy(out, p, (size_t)(end - p));
      out += end - p;
      break;
    }
    memcpy(out, p, (size_t)(slash - p));
    out += slash - p;
    if (slash + 1 == end) {
      *out++ = '\\';
      break;
    }
    const unsigned char *next = nl_unescape_one(slash + 1, end, &out);
    if (next == NULL) {
      *out++ = '\\';
      next = slash + 1;
    }
    p = next;
  }

  rb_str_set_len(result, (long)(out - start));
  rb_enc_associate(result, rb_utf8_encoding());
  RB_GC_GUARD(rb_str);
  return result;
}

void Init_nl_js_utils(void)
{
  mNokolexborJSUtils = rb_define_module_under(mNokolexbor, "JSUtils");
  rb_define_singleton_method(mNokolexborJSUtils, "unescape_js", nl_js_utils_unescape_js, 1);
}
