struct NaClExceptionContext;
#if 0
/*
  uint32_t prog_ctr;
  uint32_t stack_ptr;
  uint32_t frame_ptr;
  /*
   * Pad this up to an even multiple of 8 bytes so this struct will add
   * a predictable amount of space to the various ExceptionFrame structures
   * used on each platform. This allows us to verify stack layout with dead
   * reckoning, without access to the ExceptionFrame structure used to set up
   * the call stack.
   */
  uint32_t pad;
};
*/
#endif


#define NACL_IRT_DEV_EXCEPTION_HANDLING_v0_1 \
  "nacl-irt-dev-exception-handling-0.1"
typedef void (*NaClExceptionHandler)(struct NaClExceptionContext *context);
struct nacl_irt_dev_exception_handling {
  int (*exception_handler)(NaClExceptionHandler handler,
                           NaClExceptionHandler *old_handler);
  int (*exception_stack)(void *stack, size_t size);
  int (*exception_clear_flag)(void);
};

#define NACL_PRIuPTR "u"

size_t nacl_interface_query(const char *interface_ident,
                            void *table, size_t tablesize);
                            