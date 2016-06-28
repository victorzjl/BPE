#ifndef SERVICE_H
#define SERVICE_H

#include <QProcess>
#include <windows.h>
#include <WinUser.h>

#include "serviceconfig.h"

#define MAX_CODE_NO 8

enum EnErrorCode
{
    _NO_ERROR = 0,
    _UNKNOWN_ERROR = 1,
    _INIT_ERROR = 2,
    _CONFIG_ERROR = 3,
    _SOCKET_ERROR = 4,
    _MUTEX_ERROR = 5,
    _ENV_ERROR = 6,
    _PARAM_ERROR = 7,
    _CRASH_ERROR = 8
};

class Service : public QObject
{
    Q_OBJECT

public:
    Service();
    ~Service();

    bool Start();
    bool Stop();
    bool IsServicing() { return m_bIsServicing; }

    void SetName(const QString &strName) { m_serviceConf._strName = strName; }
    void SetProgramFile(const QString &strFile);
    void SetConfigFile(const QString &strFile);
    void SetWorkDir(const QString &strDir);
    void SetServiceConfig(const SServiceConfig &sServiceConfig);

    const QString &GetName() { return m_serviceConf._strName; }
    const QString &GetProgramFile() { return m_serviceConf._strProgramFile; }
    const QString &GetConfigFile() { return m_serviceConf._strConfFile; }
    const QString &GetWorkDir() { return m_serviceConf._strWorkDir; }
    const SServiceConfig &GetServiceConfig() { return m_serviceConf; }

    EnErrorCode GetErrorCode() { return m_enErrorCode; }
    const QString &GetErrorMessage(EnErrorCode code);

signals:
    void Started();
    void Stoped();
    void Crashed();

private:
    void HandleStarted();
    void HandleStoped();
    void HandleCrashed();

private slots:
    void OnFinished(int, QProcess::ExitStatus exitStatus);
    void OnError(QProcess::ProcessError error);
    void OnStateChanged(QProcess::ProcessState newState);

private:
    SServiceConfig m_serviceConf;
    QProcess m_qServiceInstanse;
    bool m_bIsServicing;
    QString m_strMutexName;
    HANDLE m_pNamedMutex;
    EnErrorCode m_enErrorCode;
};

#endif // SERVICE_H
