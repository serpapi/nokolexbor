#include <ruby.h>
#include "lexbor.h"
#include "libxml.h"
#include "libxml/globals.h"
#include "libxml/xpath.h"
#include "libxml/xpathInternals.h"
#include "libxml/parserInternals.h"

extern VALUE mLexbor;
extern VALUE cLexborNodeSet;
VALUE cXpathContext;

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
rb_xml_xpath_context_register_ns(VALUE self, VALUE prefix, VALUE uri)
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
rb_xml_xpath_context_register_variable(VALUE self, VALUE name, VALUE value)
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
    rb_retval = rb_utf8_str_new_cstr(c_xpath_object->stringval);
    xmlFree(c_xpath_object->stringval);
    return rb_retval;

  case XPATH_NODESET:
    rb_retval = rb_ary_new();
    for (int i = 0; i < c_xpath_object->nodesetval->nodeNr; i++)
    {
      rb_ary_push(rb_retval, lexbor_rb_node_create(c_xpath_object->nodesetval->nodeTab[i], rb_document));
    }
    return rb_retval;

  case XPATH_NUMBER:
    return rb_float_new(c_xpath_object->floatval);

  case XPATH_BOOLEAN:
    return (c_xpath_object->boolval == 1) ? Qtrue : Qfalse;

  default:
    return Qundef;
  }
}

/*
 * call-seq:
 *  evaluate(search_path, handler = nil)
 *
 * Evaluate the +search_path+ returning an XML::XPath object.
 */
static VALUE
rb_xml_xpath_context_evaluate(int argc, VALUE *argv, VALUE self)
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

  // xmlSetStructuredErrorFunc((void *)errors, Nokogiri_error_array_pusher);
  // xmlSetGenericErrorFunc((void *)errors, generic_exception_pusher);

  xpath = xmlXPathEvalExpression(query, ctx);

  // xmlSetStructuredErrorFunc(NULL, NULL);
  // xmlSetGenericErrorFunc(NULL, NULL);

  if (xpath == NULL)
  {
    rb_exc_raise(rb_ary_entry(errors, 0));
  }

  retval = xpath2ruby(xpath, ctx, rb_iv_get(self, "@document"));
  if (retval == Qundef) {
    retval = rb_funcall(cLexborNodeSet, rb_intern("new"), 1, rb_ary_new());
  }

  // xmlXPathFreeNodeSetList(xpath);

  return retval;
}

/*
 * call-seq:
 *  new(node)
 *
 * Create a new XPathContext with +node+ as the reference point.
 */
static VALUE
rb_xml_xpath_context_new(VALUE klass, VALUE nodeobj)
{
  xmlXPathContextPtr ctx;
  VALUE self;

  lexbor_node_t *node = lexbor_rb_node_unwrap(nodeobj);

  ctx = xmlXPathNewContext(node->node->owner_document);
  ctx->node = node->node;

  self = Data_Wrap_Struct(klass, 0, free_xml_xpath_context, ctx);
  rb_iv_set(self, "@document", node->rb_document);
  return self;
}

void Init_lexbor_xpath_context(void)
{
  cXpathContext = rb_define_class_under(mLexbor, "XPathContext", rb_cObject);

  rb_undef_alloc_func(cXpathContext);

  rb_define_singleton_method(cXpathContext, "new", rb_xml_xpath_context_new, 1);

  rb_define_method(cXpathContext, "evaluate", rb_xml_xpath_context_evaluate, -1);
  rb_define_method(cXpathContext, "register_variable", rb_xml_xpath_context_register_variable, 2);
  rb_define_method(cXpathContext, "register_ns", rb_xml_xpath_context_register_ns, 2);
}
