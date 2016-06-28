#ifndef SERVICECONFIGDIALOG_H
#define SERVICECONFIGDIALOG_H

#include <QDialog>
#include <QString>

#include "serviceconfig.h"

namespace Ui {
class ServiceConfigDialog;
}

enum
{
    MB_CHOOSEN_OK = 0,
    MB_CHOOSEN_CANCEL = 1
};

class ServiceConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ServiceConfigDialog(bool bAdd = true, QWidget *parent = 0);
    ~ServiceConfigDialog();

    const SServiceConfig &GetServiceConfig() { return m_sServiceConfig; }
    void SetServiceConfig(const SServiceConfig &sServiceConfig);

    void EnableModification();
    void DisableModification();

private slots:
    void OnAccepted();
    void OnRejected();
    void OnSelectProgramFile();
    void OnSelectConfigFile();
    void OnSelectWorkDir();

private:
    Ui::ServiceConfigDialog *ui;
    bool m_bIsAdd;
    SServiceConfig m_sServiceConfig;
};

#endif // SERVICECONFIGDIALOG_H
