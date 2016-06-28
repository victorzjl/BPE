#ifndef _COMMON_MACRO_H_
#define _COMMON_MACRO_H_

#ifdef WIN32
#   ifndef snprintf
#   define snprintf _snprintf
#   endif

#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

#ifndef snprintf
#define snprintf _snprintf
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#endif


#ifdef WIN32
#define CST_PATH_SEP	"\\"

#define PluginPtr HINSTANCE
#define LOAD_LIBRARY(name) LoadLibrary(name)
#define CLOSE_LIBRARY(handle) FreeLibrary(handle)
#define GET_ERROR_INFO(default_info) (default_info)
#define GET_FUNC_PTR(handle, func_name) GetProcAddress(handle, func_name)
#else
#define CST_PATH_SEP	"/"	

#define PluginPtr void*
#define LOAD_LIBRARY(name) dlopen(name, RTLD_LAZY)
#define CLOSE_LIBRARY(handle) dlclose(handle)
#define GET_ERROR_INFO(default_info) dlerror()
#define GET_FUNC_PTR(handle, func_name) dlsym(handle, func_name)
#endif

#endif
