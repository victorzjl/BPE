#ifndef SERVICECONFIG
#define SERVICECONFIG

#include <QString>

typedef struct stServiceConfig
{
    QString _strName;
    QString _strProgramFile;
    QString _strConfFile;
    QString _strWorkDir;

    bool operator==(const struct stServiceConfig &other) const { return (other._strName == _strName)
                                                          &&(other._strProgramFile == _strProgramFile)
                                                          &&(other._strConfFile == _strConfFile)
                                                          &&(other._strWorkDir == _strWorkDir);}
    bool operator!=(const struct stServiceConfig &other) const { return !operator==(other); }
}SServiceConfig;


#endif // SERVICECONFIG

