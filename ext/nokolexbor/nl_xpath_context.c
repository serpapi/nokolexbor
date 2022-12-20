#include <ruby.h>
#include <ruby/util.h>
#include "nokolexbor.h"
#include "libxml.h"
#include "libxml/globals.h"
#include "libxml/xpath.h"
#include "libxml/xpathInternals.h"
#include "libxml/parserInternals.h"

#define RBSTR_OR_QNIL(_str) (_str ? rb_utf8_str_new_cstr(_str) : Qnil)

extern VALUE mNokolexbor;
extern VALUE cNokolexborNodeSet;
VALUE cNokolexborXpathContext;
VALUE mNokolexborXpath;
VALUE cNokolexborXpathSyntaxError;

static void
free_xml_xpath_context(xmlXPathContextPtr ctx)
{
  xmlXPathFreeContext(ctx);
}

/*
 * call-seq:
 *  register_ns(prefix, uri)
 *
 * Register the namespace with +prefix+ and +uri+.
 */
static VALUE
nl_xpath_context_register_ns(VALUE self, VALUE prefix, VALUE uri)
{
  xmlXPathContextPtr ctx;
  Data_Get_Struct(self, xmlXPathContext, ctx);

  xmlXPathRegisterNs(ctx,
                     (const xmlChar *)StringValueCStr(prefix),
                     (const xmlChar *)StringValueCStr(uri));
  return self;
}

/*
 * call-seq:
 *  register_variable(name, value)
 *
 * Register the variable +name+ with +value+.
 */
static VALUE
nl_xpath_context_register_variable(VALUE self, VALUE name, VALUE value)
{
  xmlXPathContextPtr ctx;
  xmlXPathObjectPtr xmlValue;
  Data_Get_Struct(self, xmlXPathContext, ctx);

  xmlValue = xmlXPathNewCString(StringValueCStr(value));

  xmlXPathRegisterVariable(ctx,
                           (const xmlChar *)StringValueCStr(name),
                           xmlValue);

  return self;
}

/*
 *  convert an XPath object into a Ruby object of the appropriate type.
 *  returns Qundef if no conversion was possible.
 */
static VALUE
xpath2ruby(xmlXPathObjectPtr c_xpath_object, xmlXPathContextPtr ctx, VALUE rb_document)
{
  VALUE rb_retval;

  switch (c_xpath_object->type)
  {
  case XPATH_STRING:
    rb_retval = rb_utf8_str_new_cstr((const char *)c_xpath_object->stringval);
    xmlFree(c_xpath_object->stringval);
    return rb_retval;

  case XPATH_NODESET:
  {
    if (c_xpath_object->nodesetval == NULL)
    {
      return nl_rb_node_set_create_with_data(NULL, rb_document);
    }
    if (c_xpath_object->nodesetval->nodeNr == 0)
    {
      return nl_rb_node_set_create_with_data(NULL, rb_document);
    }

    lexbor_array_t *array = lexbor_array_create();
    lxb_status_t status = lexbor_array_init(array, c_xpath_object->nodesetval->nodeNr);
    if (status != LXB_STATUS_OK)
    {
      nl_raise_lexbor_error(status);
    }
    memcpy(array->list, c_xpath_object->nodesetval->nodeTab, sizeof(lxb_dom_node_t *) * c_xpath_object->nodesetval->nodeNr);
    array->length = c_xpath_object->nodesetval->nodeNr;
    return nl_rb_node_set_create_with_data(array, rb_document);
  }

  case XPATH_NUMBER:
    return rb_float_new(c_xpath_object->floatval);

  case XPATH_BOOLEAN:
    return (c_xpath_object->boolval == 1) ? Qtrue : Qfalse;

  default:
    return Qundef;
  }
}

static VALUE
nl_xpath_wrap_syntax_error(xmlErrorPtr error)
{
  VALUE msg, e;

  msg = (error && error->message) ? rb_utf8_str_new_cstr(error->message) : Qnil;

  e = rb_class_new_instance(
      1,
      &msg,
      cNokolexborXpathSyntaxError);

  if (error)
  {
    rb_iv_set(e, "@domain", INT2NUM(error->domain));
    rb_iv_set(e, "@code", INT2NUM(error->code));
    rb_iv_set(e, "@level", INT2NUM((short)error->level));
    rb_iv_set(e, "@file", RBSTR_OR_QNIL(error->file));
    rb_iv_set(e, "@line", INT2NUM(error->line));
    rb_iv_set(e, "@str1", RBSTR_OR_QNIL(error->str1));
    rb_iv_set(e, "@str2", RBSTR_OR_QNIL(error->str2));
    rb_iv_set(e, "@str3", RBSTR_OR_QNIL(error->str3));
    rb_iv_set(e, "@int1", INT2NUM(error->int1));
    rb_iv_set(e, "@column", INT2NUM(error->int2));
  }

  return e;
}

static void nl_xpath_error_array_pusher(void *ctx, xmlErrorPtr error)
{
  VALUE list = (VALUE)ctx;
  Check_Type(list, T_ARRAY);
  rb_ary_push(list, nl_xpath_wrap_syntax_error(error));
}

static void
nl_xpath_generic_exception_pusher(void *ctx, const char *msg, ...)
{
  VALUE rb_errors = (VALUE)ctx;
  VALUE rb_message;
  VALUE rb_exception;

  Check_Type(rb_errors, T_ARRAY);

  va_list args;
  va_start(args, msg);
  rb_message = rb_vsprintf(msg, args);
  va_end(args);

  rb_exception = rb_exc_new_str(cNokolexborXpathSyntaxError, rb_message);
  rb_ary_push(rb_errors, rb_exception);
}

/*
 * call-seq:
 *  evaluate(search_path, handler = nil)
 *
 * Evaluate the +search_path+ returning an XML::XPath object.
 */
static VALUE
nl_xpath_context_evaluate(int argc, VALUE *argv, VALUE self)
{
  VALUE search_path, xpath_handler;
  VALUE retval = Qnil;
  xmlXPathContextPtr ctx;
  xmlXPathObjectPtr xpath;
  xmlChar *query;
  VALUE errors = rb_ary_new();

  Data_Get_Struct(self, xmlXPathContext, ctx);

  if (rb_scan_args(argc, argv, "11", &search_path, &xpath_handler) == 1)
  {
    xpath_handler = Qnil;
  }

  query = (xmlChar *)StringValueCStr(search_path);

  // if (Qnil != xpath_handler) {
  //   /* FIXME: not sure if this is the correct place to shove private data. */
  //   ctx->userData = (void *)xpath_handler;
  //   xmlXPathRegisterFuncLookup(ctx, handler_lookup, (void *)xpath_handler);
  // }

  xmlSetStructuredErrorFunc((void *)errors, nl_xpath_error_array_pusher);
  xmlSetGenericErrorFunc((void *)errors, nl_xpath_generic_exception_pusher);

  xpath = xmlXPathEvalExpression(query, ctx);

  xmlSetStructuredErrorFunc(NULL, NULL);
  xmlSetGenericErrorFunc(NULL, NULL);

  if (xpath == NULL)
  {
    xmlXPathFreeNodeSetList(xpath);
    rb_exc_raise(rb_ary_entry(errors, 0));
  }

  retval = xpath2ruby(xpath, ctx, nl_rb_document_get(self));
  if (retval == Qundef)
  {
    retval = rb_funcall(cNokolexborNodeSet, rb_intern("new"), 1, rb_ary_new());
  }

  xmlXPathFreeNodeSetList(xpath);

  return retval;
}

/*
 * call-seq:
 *  new(node)
 *
 * Create a new XPathContext with +node+ as the reference point.
 */
static VALUE
nl_xpath_context_new(VALUE klass, VALUE rb_node)
{
  xmlXPathContextPtr ctx;
  VALUE self;

  lxb_dom_node_t *node = nl_rb_node_unwrap(rb_node);

  ctx = xmlXPathNewContext(node->owner_document);
  ctx->node = node;

  self = Data_Wrap_Struct(klass, 0, free_xml_xpath_context, ctx);
  rb_iv_set(self, "@document", nl_rb_document_get(rb_node));

  return self;
}

void Init_nl_xpath_context(void)
{
#ifndef NOKOLEXBOR_ASAN
  xmlMemSetup((xmlFreeFunc)ruby_xfree, (xmlMallocFunc)ruby_xmalloc, (xmlReallocFunc)ruby_xrealloc, ruby_strdup);
#else
  xmlMemSetup((xmlFreeFunc)free, (xmlMallocFunc)malloc, (xmlReallocFunc)realloc, strdup);
#endif

  cNokolexborXpathContext = rb_define_class_under(mNokolexbor, "XPathContext", rb_cObject);
  mNokolexborXpath = rb_define_module_under(mNokolexbor, "XPath");
  cNokolexborXpathSyntaxError = rb_define_class_under(mNokolexborXpath, "SyntaxError", rb_eStandardError);

  rb_undef_alloc_func(cNokolexborXpathContext);

  rb_define_singleton_method(cNokolexborXpathContext, "new", nl_xpath_context_new, 1);

  rb_define_method(cNokolexborXpathContext, "evaluate", nl_xpath_context_evaluate, -1);
  rb_define_method(cNokolexborXpathContext, "register_variable", nl_xpath_context_register_variable, 2);
  rb_define_method(cNokolexborXpathContext, "register_ns", nl_xpath_context_register_ns, 2);
}
