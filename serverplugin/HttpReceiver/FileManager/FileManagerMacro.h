#ifndef _FILE_MANAGER_MACRO_H_
#define _FILE_MANAGER_MACRO_H_

#define MAX_READ_BUFFER_SIZE 4096

#define  PATH_SEPARATOR "/"
#define GIT_DIR ".git"
#define SVN_DIR ".svn"

//////
#define SERVICE_GROUP_MARK "service"
#define SERVICE_START_MARK "$service."
#define SERVICE_REQUEST_MARK "request"
#define SERVICE_INCLUDE_MARK "include"
#define SERVICE_END_MARK ");"

#define REGEX_SERVICE_CONTENT "\"[^\"]*\""
#define REGEX_SERVICE_START "\\$service\\."
#define REGEX_SERVICE_REQUEST SERVICE_REQUEST_MARK
#define REGEX_SERVICE_INCLUDE SERVICE_INCLUDE_MARK

//////
#define SSI_GROUP_MARK "include"
#define SSI_START_MARK "<!--#include"
#define SSI_VIRTUAL_MARK "virtual"
#define SSI_END_MARK "-->"

#define REGEX_SSI_CONTENT "\"[^\"]*\""
#define REGEX_SSI_START SSI_START_MARK
#define REGEX_SSI_VIRTUAL SSI_VIRTUAL_MARK
#define REGEX_SSI_END   SSI_END_MARK

#define RS_SERVICE_REQUEST REGEX_SERVICE_START REGEX_SERVICE_REQUEST"\\(\\s*"REGEX_SERVICE_CONTENT"\\s*\\);"
#define RS_SERVICE_INCLUDE REGEX_SERVICE_START REGEX_SERVICE_INCLUDE"\\(\\s*"REGEX_SERVICE_CONTENT"\\s*\\);"
#define RS_SSI_VIRTUAL REGEX_SSI_START"\\s+"REGEX_SSI_VIRTUAL"\\s*\\=\\s*"REGEX_SSI_CONTENT"\\s*"REGEX_SSI_END

#define RS_SERVICE_PARSER "\\$([^\\.]+).([^\\(]+)\\(\\s*\"([^\"]*)\"\\s*\\);"
#define RS_SSI_PARSER "<!--#([^\\s]+)\\s+([^\\=\\s]+)\\s*\\=\\s*\"([^\"]*)\"\\s*-->"





#define SPRINTF_CONTENT_1(dist, format, arg1) \
    sprintf(dist, format, arg1);

#define SPRINTF_CONTENT_2(dist, format, arg1, arg2) \
    sprintf(dist, format, arg1, arg2);

#define SPRINTF_CONTENT_3(dist, format, arg1, arg2, arg3) \
    sprintf(dist, format, arg1, arg2, arg3);

#define SPRINTF_CONTENT_4(dist, format, arg1, arg2, arg3, arg4) \
    sprintf(dist, format, arg1, arg2, arg3, arg4)

#define SPRINTF_CONTENT_5(dist, format, arg1, arg2, arg3, arg4, arg5) \
    sprintf(dist, format, arg1, arg2, arg3, arg4, arg5)

#define SPRINTF_CONTENT_6(dist, format, arg1, arg2, arg3, arg4, arg5, arg6) \
    sprintf(dist, format, arg1, arg2, arg3, arg4, arg5, arg6)

#define SPRINTF_CONTENT_7(dist, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    sprintf(dist, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define SPRINTF_CONTENT_8(dist, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
    sprintf(dist, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)

#define SPRINTF_CONTENT_9(dist, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) \
    sprintf(dist, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)

#define SPRINTF_CONTENT_10(dist, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) \
    sprintf(dist, format, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)

//Use function 'sprintf' to construct content, and limit the number of arguments to 10
#define SPRINTF_CONTENT(dist, format, vecStr, nSize)        \
    switch (nSize)                                          \
    {                                                       \
    case 1:                                                 \
        SPRINTF_CONTENT_1(dist, format.c_str(),             \
        vecStr[0].c_str());                                 \
        break;                                              \
    case 2:                                                 \
        SPRINTF_CONTENT_2(dist, format.c_str(),             \
        vecStr[0].c_str(), vecStr[1].c_str());              \
        break;                                              \
    case 3:                                                 \
        SPRINTF_CONTENT_3(dist, format.c_str(),             \
        vecStr[0].c_str(), vecStr[1].c_str(),               \
        vecStr[2].c_str());                                 \
        break;                                              \
    case 4:                                                 \
        SPRINTF_CONTENT_4(dist, format.c_str(),             \
        vecStr[0].c_str(), vecStr[1].c_str(),               \
        vecStr[2].c_str(), vecStr[3].c_str());              \
        break;                                              \
    case 5:                                                 \
        SPRINTF_CONTENT_5(dist, format.c_str(),             \
        vecStr[0].c_str(), vecStr[1].c_str(),               \
        vecStr[2].c_str(), vecStr[3].c_str(),               \
        vecStr[4].c_str());                                 \
        break;                                              \
    case 6:                                                 \
        SPRINTF_CONTENT_6(dist, format.c_str(),             \
        vecStr[0].c_str(), vecStr[1].c_str(),               \
        vecStr[2].c_str(), vecStr[3].c_str(),               \
        vecStr[4].c_str(), vecStr[5].c_str());              \
    break;                                                  \
    case 7:                                                 \
        SPRINTF_CONTENT_7(dist, format.c_str(),             \
        vecStr[0].c_str(), vecStr[1].c_str(),               \
        vecStr[2].c_str(), vecStr[3].c_str(),               \
        vecStr[4].c_str(), vecStr[5].c_str(),               \
        vecStr[6].c_str());                                 \
    break;                                                  \
    case 8:                                                 \
        SPRINTF_CONTENT_8(dist, format.c_str(),             \
        vecStr[0].c_str(), vecStr[1].c_str(),               \
        vecStr[2].c_str(), vecStr[3].c_str(),               \
        vecStr[4].c_str(), vecStr[5].c_str(),               \
        vecStr[6].c_str(), vecStr[7].c_str());              \
    break;                                                  \
    case 9:                                                 \
        SPRINTF_CONTENT_9(dist, format.c_str(),             \
        vecStr[0].c_str(), vecStr[1].c_str(),               \
        vecStr[2].c_str(), vecStr[3].c_str(),               \
        vecStr[4].c_str(), vecStr[5].c_str(),               \
        vecStr[6].c_str(), vecStr[7].c_str(),               \
        vecStr[8].c_str());                                 \
    break;                                                  \
    case 10:                                                \
        SPRINTF_CONTENT_10(dist, format.c_str(),            \
        vecStr[0].c_str(), vecStr[1].c_str(),               \
        vecStr[2].c_str(), vecStr[3].c_str(),               \
        vecStr[4].c_str(), vecStr[5].c_str(),               \
        vecStr[6].c_str(), vecStr[7].c_str(),               \
        vecStr[8].c_str(), vecStr[9].c_str());              \
        break;                                              \
    default:                                                \
        break;                                              \
    }

//Remove the last character.
//If the string is empty, string's function 'pop_back' causes undefined behavior. 
#define REMOVE_LAST_CHARACTER(str)         \
    if (!str.empty())                      \
    {                                      \
        str = str.substr(0, str.size()-1); \
    }

#endif //_FILE_MANAGER_MACRO_H_
