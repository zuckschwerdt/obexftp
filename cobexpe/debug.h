#define COBEX_PE_DEBUG 0
#ifdef COBEX_PE_DEBUG
#define DEBUG(n, args...) if(n <= COBEX_PE_DEBUG) g_print(args)
#else
#define DEBUG(n, ...)
#endif //COBEX_PE_DEBUG
