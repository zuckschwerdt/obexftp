#define COBEX_BFB_DEBUG 0
#ifdef COBEX_BFB_DEBUG
#define DEBUG(n, args...) if(n <= COBEX_BFB_DEBUG) g_print(args)
#else
#define DEBUG(n, ...)
#endif //COBEX_BFB_DEBUG
