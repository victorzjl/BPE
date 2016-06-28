#include "service.h"
#include <QUuid>
#include <QDir>
#include <QMessageBox>

#define EXIT_CODE_NO_ERROR     ((DWORD)0x00000000L)
#define EXIT_CODE_CONFIG_ERROR ((DWORD)0xE0000001L)
#define EXIT_CODE_SOCKET_ERROR ((DWORD)0xE0000002L)
#define EXIT_CODE_MUTEX_ERROR  ((DWORD)0xE0000003L)
#define EXIT_CODE_ENV_ERROR    ((DWORD)0xE0000004L)
#define EXIT_CODE_PARAM_ERROR  ((DWORD)0xE0000005L)

static const QString sc_strErrorMessage[] =
                {
                    "Success to start",
                    "Unknown Error",
                    "Fail to wait for service initializing.",
                    "The configure is wrong.",
                    "Can't bind port.",
                    "Can't initialize service.",
                    "The service program requires Windows NT.",
                    "The common line is incorrect.",
                    "Service crashed."
                };


Service::Service() :
    m_bIsServicing(false),
    m_pNamedMutex(NULL)
{
    m_strMutexName = QUuid::createUuid().toString();
    m_pNamedMutex = CreateMutex(NULL, false, m_strMutexName.toStdWString().c_str());

    QStringList listStrArg;
    listStrArg.push_back(QString("--mutex=")+m_strMutexName);
    m_qServiceInstanse.setArguments(listStrArg);

    connect(&m_qServiceInstanse, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(OnFinished(int, QProcess::ExitStatus)));
    connect(&m_qServiceInstanse, SIGNAL(error(QProcess::ProcessError)),
            this, SLOT(OnError(QProcess::ProcessError)));
    connect(&m_qServiceInstanse, SIGNAL(stateChanged(QProcess::ProcessState)),
            this, SLOT(OnStateChanged(QProcess::ProcessState)));
}


Service::~Service()
{
    Stop();
    CloseHandle(m_pNamedMutex);
}

//void ErrorExit()
//{
//    // Retrieve the system error message for the last-error code

//    LPVOID lpMsgBuf;
//    DWORD dw = GetLastError();

//    FormatMessage(
//        FORMAT_MESSAGE_ALLOCATE_BUFFER |
//        FORMAT_MESSAGE_FROM_SYSTEM |
//        FORMAT_MESSAGE_IGNORE_INSERTS,
//        NULL,
//        dw,
//        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
//        (LPTSTR) &lpMsgBuf,
//        0, NULL );

//    // Display the error message and exit the process

//    QMessageBox::about(NULL, "info", QString::fromStdWString((LPTSTR)lpMsgBuf));

//    LocalFree(lpMsgBuf);
//}

bool Service::Start()
{
    bool bRet = m_bIsServicing;
    if (!m_bIsServicing)
    {
        m_qServiceInstanse.start();
        _PROCESS_INFORMATION *pProcessInfo = m_qServiceInstanse.pid();
        WaitForInputIdle(pProcessInfo->hProcess, INFINITE);
        DWORD dwRet = WaitForSingleObject(m_pNamedMutex, INFINITE);
        if (WAIT_OBJECT_0 != dwRet)
        {
            m_enErrorCode = _INIT_ERROR;
        }
        else
        {
            DWORD dwExitCode;

            if (GetExitCodeProcess(pProcessInfo->hProcess, &dwExitCode))
            {
                switch(dwExitCode)
                {
                case STILL_ACTIVE:
                    HandleStarted();
                    bRet = true;
                    m_enErrorCode = _NO_ERROR;
                    break;
                case EXIT_CODE_CONFIG_ERROR:
                    m_enErrorCode = _CONFIG_ERROR;
                    break;
                case EXIT_CODE_SOCKET_ERROR:
                    m_enErrorCode = _SOCKET_ERROR;
                    break;
                case EXIT_CODE_MUTEX_ERROR:
                    m_enErrorCode = _MUTEX_ERROR;
                    break;
                case EXIT_CODE_ENV_ERROR:
                    m_enErrorCode = _ENV_ERROR;
                    break;
                case EXIT_CODE_PARAM_ERROR:
                    m_enErrorCode = _PARAM_ERROR;
                    break;
                default:;
                    m_enErrorCode = _UNKNOWN_ERROR;
                    break;
                }
            }
            else
            {
                m_enErrorCode = _UNKNOWN_ERROR;
            }
            ReleaseMutex(m_pNamedMutex);
        }
    }

    return bRet;
}

bool Service::Stop()
{
    if (m_bIsServicing)
    {
        m_qServiceInstanse.kill();
        HandleStoped();
    }
    return true;
}

void Service::SetProgramFile(const QString &strFile)
{
    m_serviceConf._strProgramFile = strFile;
    m_qServiceInstanse.setProgram(strFile);
}

void Service::SetConfigFile(const QString &strFile)
{
    m_serviceConf._strConfFile = strFile;
    QStringList listStrArg = m_qServiceInstanse.arguments();
    for (QStringList::iterator iterList = listStrArg.begin(); iterList != listStrArg.end(); ++iterList)
    {
        if (iterList->startsWith("--config="))
        {
           listStrArg.erase(iterList);
           break;
        }
    }

    listStrArg.push_back(QString("--config=") + strFile);
    m_qServiceInstanse.setArguments(listStrArg);
}

void Service::SetWorkDir(const QString &strDir)
{
    m_serviceConf._strWorkDir = strDir;
    m_qServiceInstanse.setWorkingDirectory(strDir);
}

void Service::SetServiceConfig(const SServiceConfig &sServiceConfig)
{
    SetName(sServiceConfig._strName);
    SetProgramFile(sServiceConfig._strProgramFile);
    SetConfigFile(sServiceConfig._strConfFile);
    SetWorkDir(sServiceConfig._strWorkDir);
}

const QString &Service::GetErrorMessage(EnErrorCode code)
{
    if (MAX_CODE_NO < code)
    {
        return sc_strErrorMessage[_UNKNOWN_ERROR];
    }
    else
    {
        return sc_strErrorMessage[code];
    }
}

void Service::HandleStarted()
{
    m_bIsServicing = true;
    emit Started();
}

void Service::HandleStoped()
{
    m_bIsServicing = false;
    emit Stoped();
}

void Service::HandleCrashed()
{
    m_bIsServicing = false;
    emit Crashed();
}

void Service::OnFinished(int, QProcess::ExitStatus exitStatus)
{
    if (m_bIsServicing)
    {
        if (QProcess::CrashExit == exitStatus)
        {
            HandleCrashed();
        }
        else
        {
            HandleStoped();
        }
    }
}

void Service::OnError(QProcess::ProcessError error)
{
    if (m_bIsServicing)
    {
        switch(error)
        {
        case QProcess::Crashed:
            HandleCrashed();
            break;
        case QProcess::UnknownError:
            if (QProcess::NotRunning == m_qServiceInstanse.state())
            {
                HandleCrashed();
            }
            break;
        default:
            break;
        }
    }
}

void Service::OnStateChanged(QProcess::ProcessState newState)
{
    if ((newState == QProcess::NotRunning) && m_bIsServicing)
    {
        HandleCrashed();
    }
}



