#include "servicewidget.h"
#include "ui_servicewidget.h"
#include "serviceconfigdialog.h"
#include "serviceconfigmanager.h"
#include <QMessageBox>


ServiceWidget::ServiceWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ServiceWidget),
    m_iNo(-1),
    m_pService(NULL)
{
    ui->setupUi(this);
    this->setFrameShadow(QFrame::Raised);
    this->setFrameShape(QFrame::Panel);
    ui->BtnControl->setStyleSheet("border: 1px solid black; background-image: url(:/picture/resource/start.png);");
    ui->BtnConf->setStyleSheet("border: 1px solid black; background-image: url(:/picture/resource/config.png);");
    ui->LName->setStyleSheet("border: 1px solid gray;");
    ui->LProgramFile->setStyleSheet("border: 1px solid gray;");
    ui->LConfigFile->setStyleSheet("border: 1px solid gray;");
    ui->LWorkDir->setStyleSheet("border: 1px solid gray;");

    connect(ui->BtnControl, SIGNAL(clicked(bool)), this, SLOT(OnControlBtnClicked(bool)));
    connect(ui->BtnConf, SIGNAL(clicked(bool)), this, SLOT(OnConfBtnClicked(bool)));
}

ServiceWidget::~ServiceWidget()
{
    delete ui;

    DetachService();
}


void ServiceWidget::AssociateWithService(Service *pService)
{
    if (NULL != pService)
    {
        DetachService();

        m_pService = pService;
        connect(m_pService, SIGNAL(Started()), this, SLOT(OnServiceStarted()));
        connect(m_pService, SIGNAL(Stoped()), this, SLOT(OnServiceStoped()));
        connect(m_pService, SIGNAL(Crashed()), this, SLOT(OnServiceCrashed()));

        SetLabelText(m_pService->GetServiceConfig());
    }
}

void ServiceWidget::DetachService()
{
    if (NULL != m_pService)
    {
        m_pService->Stop();
        m_pService = NULL;

        SServiceConfig sServiceConfig;
        SetLabelText(sServiceConfig);
    }
}

void ServiceWidget::OnServiceStarted()
{
    emit ServiceStarted(m_iNo);
    ui->BtnControl->setStyleSheet("border: 1px solid black; background-image: url(:/picture/resource/stop.png);");
    ShowMessage("Info", QString("The service '") + m_pService->GetName() + "' starts successfully.");
}

void ServiceWidget::OnServiceStoped()
{
    emit ServiceStoped(m_iNo);
    ui->BtnControl->setStyleSheet("border: 1px solid black; background-image: url(:/picture/resource/start.png);");
    ShowMessage("debug", QString("The service '") + m_pService->GetName() + "' stops successfully.");
}

void ServiceWidget::OnServiceCrashed()
{
    emit ServiceStoped(m_iNo);
    ui->BtnControl->setStyleSheet("border: 1px solid black; background-image: url(:/picture/resource/start.png);");
    ShowMessage("debug", QString("The service '") + m_pService->GetName() + "' crashed.");
}

void ServiceWidget::OnControlBtnClicked(bool)
{
    if (NULL != m_pService)
    {
        if (m_pService->IsServicing())
        {
            m_pService->Stop();
        }
        else
        {
            if (!m_pService->Start())
            {
                ShowMessage("debug", m_pService->GetErrorMessage(m_pService->GetErrorCode()));
            }
        }
    }
}


void ServiceWidget::OnConfBtnClicked(bool)
{
    if (NULL != m_pService)
    {
        ServiceConfigDialog addDialog(false);
        addDialog.setWindowTitle("Service Config");
        addDialog.SetServiceConfig(m_pService->GetServiceConfig());
        if (m_pService->IsServicing())
        {
            addDialog.DisableModification();
        }
        int iRet = addDialog.exec();
        if (MB_CHOOSEN_OK == iRet)
        {
            const SServiceConfig &sNewServiceConfig = addDialog.GetServiceConfig();
            const SServiceConfig &sOldServiceConfig = m_pService->GetServiceConfig();
            if (sNewServiceConfig != sOldServiceConfig)
            {
                ServiceConfigManager::GetInstance()->UnregisterServiceConfig(sOldServiceConfig._strName);
                m_pService->SetServiceConfig(sNewServiceConfig);
                SetLabelText(sNewServiceConfig);
                ServiceConfigManager::GetInstance()->RegisterServiceConfig(sNewServiceConfig);
            }
        }
    }
}

void ServiceWidget::ShowMessage(const QString &strTitle, const QString &strMsg)
{
    QMessageBox qMsgBox(QMessageBox::NoIcon, strTitle, strMsg, QMessageBox::NoButton, this);
    qMsgBox.exec();
}

void ServiceWidget::SetLabelText(const SServiceConfig &sServiceConfig)
{
    ui->LName->setText(sServiceConfig._strName);
    ui->LProgramFile->setText(sServiceConfig._strProgramFile);
    ui->LConfigFile->setText(sServiceConfig._strConfFile);
    ui->LWorkDir->setText(sServiceConfig._strWorkDir);

    ui->LName->setToolTip(sServiceConfig._strName);
    ui->LProgramFile->setToolTip(sServiceConfig._strProgramFile);
    ui->LConfigFile->setToolTip(sServiceConfig._strConfFile);
    ui->LWorkDir->setToolTip(sServiceConfig._strWorkDir);
}
