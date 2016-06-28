#ifndef _COMMON_MACRO_H_
#define _COMMON_MACRO_H_

#if defined(_WIN32) || defined(_WIN64)
#ifndef snprintf
#define snprintf _snprintf
#endif

#ifndef strncasecmp
#define strncasecmp strnicmp
#endif

#endif

#endif
