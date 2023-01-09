#include "nokolexbor.h"

VALUE mNokolexbor;
VALUE eLexborError;
VALUE eLexborSyntaxError;

void nl_raise_lexbor_error(lxb_status_t error)
{
  switch (error) {
  case LXB_STATUS_ERROR:
    rb_raise(eLexborError, "LXB_STATUS_ERROR");
  case LXB_STATUS_ERROR_MEMORY_ALLOCATION:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_MEMORY_ALLOCATION");
  case LXB_STATUS_ERROR_OBJECT_IS_NULL:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_OBJECT_IS_NULL");
  case LXB_STATUS_ERROR_SMALL_BUFFER:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_SMALL_BUFFER");
  case LXB_STATUS_ERROR_INCOMPLETE_OBJECT:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_INCOMPLETE_OBJECT");
  case LXB_STATUS_ERROR_NO_FREE_SLOT:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_NO_FREE_SLOT");
  case LXB_STATUS_ERROR_TOO_SMALL_SIZE:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_TOO_SMALL_SIZE");
  case LXB_STATUS_ERROR_NOT_EXISTS:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_NOT_EXISTS");
  case LXB_STATUS_ERROR_WRONG_ARGS:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_WRONG_ARGS");
  case LXB_STATUS_ERROR_WRONG_STAGE:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_WRONG_STAGE");
  case LXB_STATUS_ERROR_UNEXPECTED_RESULT:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_UNEXPECTED_RESULT");
  case LXB_STATUS_ERROR_UNEXPECTED_DATA:
    rb_raise(eLexborSyntaxError, "LXB_STATUS_ERROR_UNEXPECTED_DATA");
  case LXB_STATUS_ERROR_OVERFLOW:
    rb_raise(eLexborError, "LXB_STATUS_ERROR_OVERFLOW");
  case LXB_STATUS_CONTINUE:
    rb_raise(eLexborError, "LXB_STATUS_CONTINUE");
  case LXB_STATUS_SMALL_BUFFER:
    rb_raise(eLexborError, "LXB_STATUS_SMALL_BUFFER");
  case LXB_STATUS_ABORTED:
    rb_raise(eLexborError, "LXB_STATUS_ABORTED");
  case LXB_STATUS_STOPPED:
    rb_raise(eLexborError, "LXB_STATUS_STOPPED");
  case LXB_STATUS_NEXT:
    rb_raise(eLexborError, "LXB_STATUS_NEXT");
  case LXB_STATUS_STOP:
    rb_raise(eLexborError, "LXB_STATUS_STOP");
  case LXB_STATUS_OK:
    return;
  default:
    rb_raise(eLexborError, "LXB_ERROR_UKNOWN(%d)", error);
  }
}

void Init_nokolexbor(void)
{
#ifndef NOKOLEXBOR_ASAN
  lexbor_memory_setup(ruby_xmalloc, ruby_xrealloc, ruby_xcalloc, ruby_xfree);
#endif
  mNokolexbor = rb_define_module("Nokolexbor");
  eLexborError = rb_define_class_under(mNokolexbor, "LexborError", rb_eStandardError);
  eLexborSyntaxError = rb_define_class_under(mNokolexbor, "LexborSyntaxError", eLexborError);
  Init_nl_node();
  Init_nl_document();
  Init_nl_text();
  Init_nl_comment();
  Init_nl_cdata();
  Init_nl_processing_instruction();
  Init_nl_node_set();
  Init_nl_document_fragment();
  Init_nl_xpath_context();
}
