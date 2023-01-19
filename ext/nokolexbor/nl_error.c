#include "nokolexbor.h"

VALUE mLexbor;
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
  mLexbor = rb_define_module_under(mNokolexbor, "Lexbor");
  eLexborError = rb_define_class_under(mLexbor, "Error", rb_eStandardError);
  eLexborMemoryAllocationError = rb_define_class_under(mLexbor, "MemoryAllocationError", eLexborError);
  eLexborSmallBufferError = rb_define_class_under(mLexbor, "SmallBufferError", eLexborError);
  eLexborObjectIsNullError = rb_define_class_under(mLexbor, "ObjectIsNullError", eLexborError);
  eLexborIncompleteObjectError = rb_define_class_under(mLexbor, "IncompleteObjectError", eLexborError);
  eLexborNoFreeSlotError = rb_define_class_under(mLexbor, "NoFreeSlotError", eLexborError);
  eLexborTooSmallSizeError = rb_define_class_under(mLexbor, "TooSmallSizeError", eLexborError);
  eLexborNotExistsError = rb_define_class_under(mLexbor, "NotExistsError", eLexborError);
  eLexborWrongArgsError = rb_define_class_under(mLexbor, "WrongArgsError", eLexborError);
  eLexborWrongStageError = rb_define_class_under(mLexbor, "WrongStageError", eLexborError);
  eLexborUnexpectedResultError = rb_define_class_under(mLexbor, "UnexpectedResultError", eLexborError);
  eLexborUnexpectedDataError = rb_define_class_under(mLexbor, "UnexpectedDataError", eLexborError);
  eLexborOverflowError = rb_define_class_under(mLexbor, "OverflowError", eLexborError);
  eLexborContinueStatus = rb_define_class_under(mLexbor, "ContinueStatus", eLexborError);
  eLexborSmallBufferStatus = rb_define_class_under(mLexbor, "SmallBufferStatus", eLexborError);
  eLexborAbortedStatus = rb_define_class_under(mLexbor, "AbortedStatus", eLexborError);
  eLexborStoppedStatus = rb_define_class_under(mLexbor, "StoppedStatus", eLexborError);
  eLexborNextStatus = rb_define_class_under(mLexbor, "NextStatus", eLexborError);
  eLexborStopStatus = rb_define_class_under(mLexbor, "StopStatus", eLexborError);
}
