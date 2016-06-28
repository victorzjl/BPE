#include "serviceconfigmanager.h"
#include <QFile>
#include <QMessageBox>
ServiceConfigManager *ServiceConfigManager::s_pServiceConfigManager = NULL;

ServiceConfigManager *ServiceConfigManager::GetInstance()
{
    if (NULL == s_pServiceConfigManager)
    {
        s_pServiceConfigManager = new ServiceConfigManager;
    }

    return s_pServiceConfigManager;
}

void ServiceConfigManager::DeleteInstance()
{
    if (NULL != s_pServiceConfigManager)
    {
        delete s_pServiceConfigManager;
        s_pServiceConfigManager = NULL;
    }
}

ServiceConfigManager::ServiceConfigManager()
{
    LoadServiceConfigFromFile();
}

ServiceConfigManager::~ServiceConfigManager()
{
    SaveServiceConfigToFile();
}

bool ServiceConfigManager::IsServiceNameRegistered(const QString &strName)
{
    return (m_mapServiceConfig.find(strName) != m_mapServiceConfig.end());
}

bool ServiceConfigManager::RegisterServiceConfig(const SServiceConfig &sServiceConfig)
{
    if (m_mapServiceConfig.end() != m_mapServiceConfig.find(sServiceConfig._strName))
    {
        return false;
    }
    else
    {
        m_mapServiceConfig[sServiceConfig._strName] = sServiceConfig;
        return true;
    }
}

bool ServiceConfigManager::UnregisterServiceConfig(const QString &strName)
{
    QMap<QString, SServiceConfig>::iterator iterFind = m_mapServiceConfig.find(strName);
    if (m_mapServiceConfig.end() == iterFind)
    {
        return false;
    }
    else
    {
        m_mapServiceConfig.erase(iterFind);
        return true;
    }
}

void ServiceConfigManager::LoadServiceConfigFromFile()
{
    QFile qConfFile("ServiceConfig.conf");
    if (qConfFile.open(QIODevice::ReadOnly))
    {
        char szBuf[1024] = {0};
        qint64 lineLength = qConfFile.readLine(szBuf, 1024);
        QRegExp qRex("([^\\|]+)\\|([^\\|]+)\\|([^\\|]+)\\|([^\\s]+)");
        while (-1 != lineLength)
        {
            if (-1 != qRex.indexIn(szBuf))
            {
                SServiceConfig sServerConfig;
                sServerConfig._strName = qRex.cap(1);
                sServerConfig._strProgramFile = qRex.cap(2);
                sServerConfig._strConfFile = qRex.cap(3);
                sServerConfig._strWorkDir = qRex.cap(4);
                m_mapServiceConfig[sServerConfig._strName] = sServerConfig;
            }
            lineLength = qConfFile.readLine(szBuf, 1024);
        }

        qConfFile.close();
    }
}

void ServiceConfigManager::SaveServiceConfigToFile()
{
    QFile qConfFile("ServiceConfig.conf");

    if (qConfFile.open(QIODevice::WriteOnly))
    {
        QString strLine;
        for (QMap<QString, SServiceConfig>::iterator iterMap = m_mapServiceConfig.begin();
             iterMap != m_mapServiceConfig.end(); ++iterMap)
        {
            strLine += iterMap.value()._strName
                    + "|" + iterMap.value()._strProgramFile
                    + "|" + iterMap.value()._strConfFile
                    + "|" + iterMap.value()._strWorkDir + "\n";
        }
        qConfFile.write(strLine.toStdString().c_str(), strLine.size());
        qConfFile.flush();
        qConfFile.close();
    }
}
