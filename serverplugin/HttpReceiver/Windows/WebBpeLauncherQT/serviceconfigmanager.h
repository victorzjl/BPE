#ifndef SERVICECONFIGMANAGER_H
#define SERVICECONFIGMANAGER_H

#include <QString>
#include <QMap>
#include "serviceconfig.h"

class ServiceConfigManager
{
public:
    static ServiceConfigManager *GetInstance();
    static void DeleteInstance();

public:
    ~ServiceConfigManager();
    const QMap<QString, SServiceConfig> &GetServiceConfigs() { return m_mapServiceConfig; }
    bool IsServiceNameRegistered(const QString &strName);
    bool RegisterServiceConfig(const SServiceConfig &sServiceConfig);
    bool UnregisterServiceConfig(const QString &strName);
    void LoadServiceConfigFromFile();
    void SaveServiceConfigToFile();

private:
    ServiceConfigManager();

private:
    static ServiceConfigManager *s_pServiceConfigManager;
    QMap<QString, SServiceConfig> m_mapServiceConfig;
};

#endif // SERVICECONFIGMANAGER_H
