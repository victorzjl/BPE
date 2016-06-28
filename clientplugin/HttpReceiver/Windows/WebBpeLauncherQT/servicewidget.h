#ifndef SERVICEWIDGET_H
#define SERVICEWIDGET_H

#include <QFrame>

#include "service.h"

namespace Ui {
class ServiceWidget;
}

class ServiceWidget : public QFrame
{
    Q_OBJECT

public:
    explicit ServiceWidget(QWidget *parent = 0);
    ~ServiceWidget();

    void SetNo(int iNo) { m_iNo = iNo; }
    int GetNo() { return m_iNo; }

    void AssociateWithService(Service *pService);
    Service *GetAssociatedService() { return m_pService; }
    void DetachService();

signals:
    void ServiceStarted(int);
    void ServiceStoped(int);

private slots:
    void OnServiceStarted();
    void OnServiceStoped();
    void OnServiceCrashed();
    void OnControlBtnClicked(bool);
    void OnConfBtnClicked(bool);

private:
    void ShowMessage(const QString &strTitle, const QString &strMsg);
    void SetLabelText(const SServiceConfig &sServiceConfig);

private:
    Ui::ServiceWidget *ui;
    int m_iNo;
    Service *m_pService;
};

#endif // SERVICEWIDGET_H
