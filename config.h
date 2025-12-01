#ifndef CONFIG_H
#define CONFIG_H

// uncomment to use verbose mode
//#define VERBOSE

#ifdef VERBOSE
	#define TRACE(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__);
#else
	#define TRACE(fmt, ...)
#endif

#define UNUSED(x) (void)(x)

#endif
