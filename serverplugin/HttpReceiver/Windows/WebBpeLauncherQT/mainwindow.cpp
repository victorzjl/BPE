#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serviceconfigdialog.h"
#include "serviceconfigmanager.h"

#include <QApplication>
#include <QMessageBox>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/picture/resource/Plant_Leaf.ico"));
    setWindowTitle("WebBpeLauncher");

    m_pWidgetContainer = new WidgetContainer;
    ui->scrollArea->setWidget(m_pWidgetContainer);
    connect(ui->BtnAddService, SIGNAL(clicked(bool)), this, SLOT(OnBtnAddServiceClicked(bool)));
    connect(ui->BtnSaveAll, SIGNAL(clicked(bool)), this, SLOT(OnBtnSaveAllClicked(bool)));

    OnloadSavedService();
    CreateSystemTray();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_pWidgetContainer;
    delete m_pSysTrayIcon;
    delete m_pTrayMenu;
    delete m_pQuitAction;
    delete m_pRestoreAction;
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    hide();
    ev->ignore();
    m_pSysTrayIcon->showMessage("^_^", "躲猫猫咯，你找不到我！", QSystemTrayIcon::NoIcon, 3000);
}

void MainWindow::OnBtnAddServiceClicked(bool)
{
    ServiceConfigDialog addDialog;
    addDialog.setWindowTitle("Add Service");
    int iRet = addDialog.exec();
    if (MB_CHOOSEN_OK == iRet)
    {
        SServiceConfig sServiceConfig = addDialog.GetServiceConfig();
        m_pWidgetContainer->Add(sServiceConfig);
        ServiceConfigManager::GetInstance()->RegisterServiceConfig(sServiceConfig);
    }
}


void MainWindow::OnBtnSaveAllClicked(bool)
{
    ServiceConfigManager::GetInstance()->SaveServiceConfigToFile();
}

void MainWindow::OnTrayActived(QSystemTrayIcon::ActivationReason reason)
{
    if (QSystemTrayIcon::DoubleClick == reason)
    {
        if (this->isVisible())
        {
            this->hide();
        }
        else
        {
            this->showNormal();
        }
    }
}


void MainWindow::OnloadSavedService()
{
    const QMap<QString, SServiceConfig> &mapServiceConfig =
            ServiceConfigManager::GetInstance()->GetServiceConfigs();

    for (QMap<QString, SServiceConfig>::const_iterator iterMap = mapServiceConfig.begin();
         iterMap != mapServiceConfig.end(); ++iterMap)
    {
        m_pWidgetContainer->Add(iterMap.value());
    }
}

void MainWindow::CreateSystemTray()
{
    m_pSysTrayIcon = new QSystemTrayIcon;
    m_pSysTrayIcon->setIcon(QIcon(":/picture/resource/Plant_Leaf.ico"));
    m_pSysTrayIcon->setToolTip(tr("WebBpeLauncher"));
    connect(m_pSysTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
             this, SLOT(OnTrayActived(QSystemTrayIcon::ActivationReason)));
    m_pQuitAction = new QAction(tr("退出(&Q)"), this);
    connect(m_pQuitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    m_pRestoreAction = new QAction(tr("还原 (&R)"), this);
    connect(m_pRestoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    m_pTrayMenu = new QMenu(this);
    m_pTrayMenu->addAction(m_pRestoreAction);
    m_pTrayMenu->addAction(m_pQuitAction);
    m_pSysTrayIcon->setContextMenu(m_pTrayMenu);

    m_pSysTrayIcon->show();
}
