#include "nokolexbor.h"

VALUE eLexborError;
VALUE eLexborMemoryAllocationError;
VALUE eLexborSmallBufferError;
VALUE eLexborObjectIsNullError;
VALUE eLexborIncompleteObjectError;
VALUE eLexborNoFreeSlotError;
VALUE eLexborTooSmallSizeError;
VALUE eLexborNotExistsError;
VALUE eLexborWrongArgsError;
VALUE eLexborWrongStageError;
VALUE eLexborUnexpectedResultError;
VALUE eLexborUnexpectedDataError;
VALUE eLexborOverflowError;
VALUE eLexborContinueStatus;
VALUE eLexborSmallBufferStatus;
VALUE eLexborAbortedStatus;
VALUE eLexborStoppedStatus;
VALUE eLexborNextStatus;
VALUE eLexborStopStatus;
extern VALUE mNokolexbor;

void nl_raise_lexbor_error(lxb_status_t error)
{
  switch (error) {
  case LXB_STATUS_ERROR:
    rb_exc_raise(eLexborError);
  case LXB_STATUS_ERROR_MEMORY_ALLOCATION:
    rb_exc_raise(eLexborMemoryAllocationError);
  case LXB_STATUS_ERROR_OBJECT_IS_NULL:
    rb_exc_raise(eLexborObjectIsNullError);
  case LXB_STATUS_ERROR_SMALL_BUFFER:
    rb_exc_raise(eLexborSmallBufferError);
  case LXB_STATUS_ERROR_INCOMPLETE_OBJECT:
    rb_exc_raise(eLexborIncompleteObjectError);
  case LXB_STATUS_ERROR_NO_FREE_SLOT:
    rb_exc_raise(eLexborNoFreeSlotError);
  case LXB_STATUS_ERROR_TOO_SMALL_SIZE:
    rb_exc_raise(eLexborTooSmallSizeError);
  case LXB_STATUS_ERROR_NOT_EXISTS:
    rb_exc_raise(eLexborNotExistsError);
  case LXB_STATUS_ERROR_WRONG_ARGS:
    rb_exc_raise(eLexborWrongArgsError);
  case LXB_STATUS_ERROR_WRONG_STAGE:
    rb_exc_raise(eLexborWrongStageError);
  case LXB_STATUS_ERROR_UNEXPECTED_RESULT:
    rb_exc_raise(eLexborUnexpectedResultError);
  case LXB_STATUS_ERROR_UNEXPECTED_DATA:
    rb_raise(eLexborUnexpectedDataError, "Invalid syntax");
  case LXB_STATUS_ERROR_OVERFLOW:
    rb_exc_raise(eLexborOverflowError);
  case LXB_STATUS_CONTINUE:
    rb_exc_raise(eLexborContinueStatus);
  case LXB_STATUS_SMALL_BUFFER:
    rb_exc_raise(eLexborSmallBufferStatus);
  case LXB_STATUS_ABORTED:
    rb_exc_raise(eLexborAbortedStatus);
  case LXB_STATUS_STOPPED:
    rb_exc_raise(eLexborStoppedStatus);
  case LXB_STATUS_NEXT:
    rb_exc_raise(eLexborNextStatus);
  case LXB_STATUS_STOP:
    rb_exc_raise(eLexborStopStatus);
  case LXB_STATUS_OK:
    return;
  default:
    rb_raise(eLexborError, "Unknown error (%d)", error);
  }
}

void Init_nl_error(void)
{
  eLexborError = rb_define_class_under(mNokolexbor, "LexborError", rb_eStandardError);
  eLexborMemoryAllocationError = rb_define_class_under(mNokolexbor, "LexborMemoryAllocationError", eLexborError);
  eLexborSmallBufferError = rb_define_class_under(mNokolexbor, "LexborSmallBufferError", eLexborError);
  eLexborObjectIsNullError = rb_define_class_under(mNokolexbor, "LexborObjectIsNullError", eLexborError);
  eLexborIncompleteObjectError = rb_define_class_under(mNokolexbor, "LexborIncompleteObjectError", eLexborError);
  eLexborNoFreeSlotError = rb_define_class_under(mNokolexbor, "LexborNoFreeSlotError", eLexborError);
  eLexborTooSmallSizeError = rb_define_class_under(mNokolexbor, "LexborTooSmallSizeError", eLexborError);
  eLexborNotExistsError = rb_define_class_under(mNokolexbor, "LexborNotExistsError", eLexborError);
  eLexborWrongArgsError = rb_define_class_under(mNokolexbor, "LexborWrongArgsError", eLexborError);
  eLexborWrongStageError = rb_define_class_under(mNokolexbor, "LexborWrongStageError", eLexborError);
  eLexborUnexpectedResultError = rb_define_class_under(mNokolexbor, "LexborUnexpectedResultError", eLexborError);
  eLexborUnexpectedDataError = rb_define_class_under(mNokolexbor, "LexborUnexpectedDataError", eLexborError);
  eLexborOverflowError = rb_define_class_under(mNokolexbor, "LexborOverflowError", eLexborError);
  eLexborContinueStatus = rb_define_class_under(mNokolexbor, "LexborContinueStatus", eLexborError);
  eLexborSmallBufferStatus = rb_define_class_under(mNokolexbor, "LexborSmallBufferStatus", eLexborError);
  eLexborAbortedStatus = rb_define_class_under(mNokolexbor, "LexborAbortedStatus", eLexborError);
  eLexborStoppedStatus = rb_define_class_under(mNokolexbor, "LexborStoppedStatus", eLexborError);
  eLexborNextStatus = rb_define_class_under(mNokolexbor, "LexborNextStatus", eLexborError);
  eLexborStopStatus = rb_define_class_under(mNokolexbor, "LexborStopStatus", eLexborError);
}
