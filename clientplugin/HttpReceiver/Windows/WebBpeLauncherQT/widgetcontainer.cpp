#include "widgetcontainer.h"
#include "service.h"
#include "serviceconfigmanager.h"

#include <QMessageBox>

#define SPACE 4

WidgetContainer::WidgetContainer(QWidget *parent) :
    QWidget(parent),
    m_iNo(0)
{
    setFixedSize(665, 4);
    setContentsMargins(0, 0, 0, 0);
    m_pMainLayoutH = new QHBoxLayout;
    m_pSWLayoutV = new QVBoxLayout;
    m_pDBLayoutV = new QVBoxLayout;

    m_pMainLayoutH->setSpacing(0);
    m_pMainLayoutH->setContentsMargins(0, 2, 2, 2);
    m_pSWLayoutV->setSpacing(SPACE);
    m_pSWLayoutV->setContentsMargins(0, 0, 0, 0);
    m_pDBLayoutV->setSpacing(70+SPACE);
    m_pDBLayoutV->setContentsMargins(0, 35, 0, 35);

    m_pMainLayoutH->addLayout(m_pSWLayoutV);
    m_pMainLayoutH->addLayout(m_pDBLayoutV);
    setLayout(m_pMainLayoutH);
}

WidgetContainer::~WidgetContainer()
{
    for (QMap<int, DelButton *>::iterator iterBtn = m_mapDelBtn.begin();
         iterBtn != m_mapDelBtn.end(); ++iterBtn)
    {
        delete iterBtn.value();
    }
    m_mapDelBtn.clear();


    for (QMap<int, ServiceWidget *>::iterator iterWidget = m_mapServiceWidget.begin();
         iterWidget != m_mapServiceWidget.end(); ++iterWidget)
    {
        ServiceWidget *pServiceWidget = iterWidget.value();
        Service *pService = pServiceWidget->GetAssociatedService();
        if (NULL != pService)
        {
            pServiceWidget->DetachService();
            delete pService;
            pService = NULL;
        }
        delete iterWidget.value();
    }
    m_mapServiceWidget.clear();


    for (QVector<SUnusedWidgetPair>::iterator iterPair = m_vecUnusedWidget.begin();
         iterPair != m_vecUnusedWidget.end(); ++iterPair)
    {
        delete (*iterPair)._pDelBtn;
        delete (*iterPair)._pServiceWidget;
    }
    m_vecUnusedWidget.clear();
}

void WidgetContainer::Add(const SServiceConfig &sServiceConfig)
{
    unsigned int nNum = m_mapDelBtn.size() == 0? 0:1;
    int iNo = m_iNo++;
    while (m_mapDelBtn.end() != m_mapDelBtn.find(iNo))
    {
        iNo = m_iNo++;
    }

    ServiceWidget *pServiceWidget = NULL;
    DelButton *pDelBtn = NULL;
    if (m_vecUnusedWidget.empty())
    {
        pServiceWidget = new ServiceWidget();
        pDelBtn = new DelButton();
        connect(pDelBtn, SIGNAL(DelClicked(int)), this, SLOT(OnDelBtnClicked(int)));
        connect(pServiceWidget, SIGNAL(ServiceStarted(int)), this, SLOT(OnServiceStarted(int)));
        connect(pServiceWidget, SIGNAL(ServiceStoped(int)), this, SLOT(OnServiceStoped(int)));
    }
    else
    {
        pServiceWidget = m_vecUnusedWidget.first()._pServiceWidget;
        pDelBtn = m_vecUnusedWidget.first()._pDelBtn;
        m_vecUnusedWidget.pop_front();
    }

    Service *pService = new Service;
    pService->SetServiceConfig(sServiceConfig);
    pServiceWidget->AssociateWithService(pService);

    pServiceWidget->SetNo(iNo);
    pDelBtn->SetNo(iNo);
    m_mapServiceWidget[iNo] = pServiceWidget;
    m_mapDelBtn[iNo] = pDelBtn;
    setUpdatesEnabled(false);
    setFixedHeight(height() + pServiceWidget->height() + nNum*SPACE);
    m_pSWLayoutV->addWidget(pServiceWidget);
    m_pDBLayoutV->addWidget(pDelBtn);
    setUpdatesEnabled(true);
}

void WidgetContainer::OnServiceStarted(int iNo)
{
    QMap<int, DelButton *>::iterator iterDelBtn = m_mapDelBtn.find(iNo);
    if (iterDelBtn != m_mapDelBtn.end())
    {
        iterDelBtn.value()->setEnabled(false);
        iterDelBtn.value()->setStyleSheet("background-color: rgba(0, 0, 0, 0);");
    }
}

void WidgetContainer::OnServiceStoped(int iNo)
{
    QMap<int, DelButton *>::iterator iterDelBtn = m_mapDelBtn.find(iNo);
    if (iterDelBtn != m_mapDelBtn.end())
    {
        iterDelBtn.value()->setEnabled(true);
        iterDelBtn.value()->setStyleSheet("border: 1px solid black; background-image: url(:/picture/resource/delete.png);");
    }
}

void WidgetContainer::OnDelBtnClicked(int iNo)
{
    QMap<int, ServiceWidget *>::iterator iterServiceWidget = m_mapServiceWidget.find(iNo);
    if (iterServiceWidget != m_mapServiceWidget.end())
    {
        ServiceWidget *pServiceWidget = iterServiceWidget.value();
        Service *pService = pServiceWidget->GetAssociatedService();
        if (NULL != pService)
        {
            if (pService->IsServicing())
            {
                QMessageBox::about(this, "Error", "The service is running, can't delete it.");
                return;
            }
        }

        unsigned int nNum = m_mapDelBtn.size() == 1? 0:1;

        ServiceConfigManager::GetInstance()->UnregisterServiceConfig(pService->GetName());
        pServiceWidget->DetachService();
        delete pService;

        QMap<int, DelButton *>::iterator iterDelBtn = m_mapDelBtn.find(iNo);
        DelButton *pDelBtn = iterDelBtn.value();
        setUpdatesEnabled(false);
        setFixedHeight(height() - pServiceWidget->height() - nNum*SPACE);
        m_pSWLayoutV->removeWidget(pServiceWidget);
        m_pDBLayoutV->removeWidget(pDelBtn);
        pServiceWidget->hide();
        pDelBtn->hide();
        setUpdatesEnabled(true);
    }
}

