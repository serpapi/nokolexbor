#include "nokolexbor.h"

extern VALUE mNokolexbor;
VALUE cNokolexborNodeSet;

lxb_status_t
lexbor_array_push_unique(lexbor_array_t *array, void *value)
{
  for (int i = 0; i < array->length; i++)
    if (array->list[i] == value)
      return LXB_STATUS_STOPPED;

  return lexbor_array_push(array, value);
}

static void
free_nl_node_set(lexbor_array_t *array)
{
  lexbor_array_destroy(array, true);
}

const rb_data_type_t nl_node_set_type = {
    "Nokolexbor::NodeSet",
    {
        0,
        free_nl_node_set,
    },
    0,
    0,
    RUBY_TYPED_FREE_IMMEDIATELY,
};

lexbor_array_t *
nl_rb_node_set_unwrap(VALUE rb_node_set)
{
  lexbor_array_t *array;
  TypedData_Get_Struct(rb_node_set, lexbor_array_t, &nl_node_set_type, array);
  return array;
}

static VALUE
nl_node_set_allocate(VALUE klass)
{
  lexbor_array_t *array = lexbor_array_create();
  lexbor_array_init(array, 1);
  return TypedData_Wrap_Struct(cNokolexborNodeSet, &nl_node_set_type, array);
}

VALUE
nl_rb_node_set_create_with_data(lexbor_array_t *array, VALUE rb_document)
{
  if (array == NULL)
  {
    array = lexbor_array_create();
    lexbor_array_init(array, 1);
  }
  VALUE ret = TypedData_Wrap_Struct(cNokolexborNodeSet, &nl_node_set_type, array);
  rb_iv_set(ret, "@document", rb_document);
  return ret;
}

static VALUE
nl_node_set_length(VALUE self)
{
  return INT2NUM(nl_rb_node_set_unwrap(self)->length);
}

static VALUE
nl_node_set_push(VALUE self, VALUE rb_node)
{
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);
  lxb_dom_node_t *node = nl_rb_node_unwrap(rb_node);

  lexbor_array_push_unique(array, node);

  return self;
}

static VALUE
nl_node_set_delete(VALUE self, VALUE rb_node)
{
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);
  lxb_dom_node_t *node = nl_rb_node_unwrap(rb_node);

  int i;
  for (i = 0; i < array->length; i++)
    if (array->list[i] == node)
    {
      break;
    }

  if (i >= array->length)
  {
    // not found
    return Qnil;
  }
  lexbor_array_delete(array, i, 1);
  return rb_node;
}

static VALUE
nl_node_set_is_include(VALUE self, VALUE rb_node)
{
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);
  lxb_dom_node_t *node = nl_rb_node_unwrap(rb_node);

  for (int i = 0; i < array->length; i++)
    if (array->list[i] == node)
    {
      return Qtrue;
    }

  return Qfalse;
}

static VALUE
nl_node_set_index_at(VALUE self, long offset)
{
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);
  if (offset >= (long)array->length || abs((int)offset) > (long)array->length)
  {
    return Qnil;
  }

  if (offset < 0)
  {
    offset += array->length;
  }

  lxb_dom_node_t *node = lexbor_array_get(array, offset);
  return nl_rb_node_create(node, nl_rb_document_get(self));
}

static VALUE
nl_node_set_subseq(VALUE self, long beg, long len)
{
  lexbor_array_t *old_array = nl_rb_node_set_unwrap(self);

  if (beg > (long)old_array->length)
  {
    return Qnil;
  }
  if (beg < 0 || len < 0)
  {
    return Qnil;
  }

  if ((beg + len) > (long)old_array->length)
  {
    len = old_array->length - beg;
  }

  lexbor_array_t *new_array = lexbor_array_create();
  lexbor_array_init(new_array, len);

  for (int j = beg; j < beg + len; ++j)
  {
    lexbor_array_push(new_array, old_array->list[j]);
  }
  return TypedData_Wrap_Struct(cNokolexborNodeSet, &nl_node_set_type, new_array);
}

static VALUE
nl_node_set_slice(int argc, VALUE *argv, VALUE self)
{
  VALUE arg;
  long beg, len;

  lexbor_array_t *array = nl_rb_node_set_unwrap(self);

  if (argc == 2)
  {
    beg = NUM2LONG(argv[0]);
    len = NUM2LONG(argv[1]);
    if (beg < 0)
    {
      beg += array->length;
    }
    return nl_node_set_subseq(self, beg, len);
  }

  if (argc != 1)
  {
    rb_scan_args(argc, argv, "11", NULL, NULL);
  }
  arg = argv[0];

  if (FIXNUM_P(arg))
  {
    return nl_node_set_index_at(self, FIX2LONG(arg));
  }

  /* if arg is Range */
  switch (rb_range_beg_len(arg, &beg, &len, array->length, 0))
  {
  case Qfalse:
    break;
  case Qnil:
    return Qnil;
  default:
    return nl_node_set_subseq(self, beg, len);
  }

  return nl_node_set_index_at(self, NUM2LONG(arg));
}

static VALUE
nl_node_set_to_array(VALUE self)
{
  lexbor_array_t *array = nl_rb_node_set_unwrap(self);

  VALUE list = rb_ary_new2(array->length);
  VALUE doc = nl_rb_document_get(self);
  for (int i = 0; i < array->length; i++)
  {
    lxb_dom_node_t *node = (lxb_dom_node_t *)array->list[i];
    VALUE rb_node = nl_rb_node_create(node, doc);
    rb_ary_push(list, rb_node);
  }

  return list;
}

static VALUE
nl_node_set_union(VALUE self, VALUE other)
{
  if (!rb_obj_is_kind_of(other, cNokolexborNodeSet))
  {
    rb_raise(rb_eArgError, "parameter must be a Nokolexbor::NodeSet");
  }

  lexbor_array_t *self_array = nl_rb_node_set_unwrap(self);
  lexbor_array_t *other_array = nl_rb_node_set_unwrap(other);

  lexbor_array_t *new_array = lexbor_array_create();
  lexbor_array_init(new_array, self_array->length + other_array->length);
  for (int i = 0; i < self_array->length; i++)
  {
    new_array->list[i] = self_array->list[i];
  }
  new_array->length = self_array->length;

  for (int i = 0; i < other_array->length; i++)
  {
    lexbor_array_push_unique(new_array, other_array->list[i]);
  }

  return nl_rb_node_set_create_with_data(new_array, nl_rb_document_get(self));
}

void Init_nl_node_set(void)
{
  cNokolexborNodeSet = rb_define_class_under(mNokolexbor, "NodeSet", rb_cObject);

  rb_define_alloc_func(cNokolexborNodeSet, nl_node_set_allocate);

  rb_define_method(cNokolexborNodeSet, "length", nl_node_set_length, 0);
  rb_define_method(cNokolexborNodeSet, "[]", nl_node_set_slice, -1);
  rb_define_method(cNokolexborNodeSet, "slice", nl_node_set_slice, -1);
  rb_define_method(cNokolexborNodeSet, "push", nl_node_set_push, 1);
  rb_define_method(cNokolexborNodeSet, "|", nl_node_set_union, 1);
  rb_define_method(cNokolexborNodeSet, "to_a", nl_node_set_to_array, 0);
  rb_define_method(cNokolexborNodeSet, "delete", nl_node_set_delete, 1);
  rb_define_method(cNokolexborNodeSet, "include?", nl_node_set_is_include, 1);

  rb_define_alias(cNokolexborNodeSet, "<<", "push");
  rb_define_alias(cNokolexborNodeSet, "size", "length");
  rb_define_alias(cNokolexborNodeSet, "+", "|");
}
