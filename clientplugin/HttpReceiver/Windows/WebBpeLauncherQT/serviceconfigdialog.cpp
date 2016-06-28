#include "serviceconfigdialog.h"
#include "ui_serviceconfigdialog.h"
#include "serviceconfigmanager.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

ServiceConfigDialog::ServiceConfigDialog(bool bAdd, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ServiceConfigDialog),
    m_bIsAdd(bAdd)
{   
    ui->setupUi(this);
    ui->LeName->setText("WebBpe");
    ui->LeConfigFile->setText("config.xml");
    ui->LeProgramFile->setText("BpeSimulator.exe");
    ui->LeWorkDir->setText(".");

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(OnAccepted()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(OnRejected()));
    connect(ui->BtnSelectProgramFile, SIGNAL(clicked(bool)), this, SLOT(OnSelectProgramFile()));
    connect(ui->BtnSelectConfigFile, SIGNAL(clicked(bool)), this, SLOT(OnSelectConfigFile()));
    connect(ui->BtnSelectWorkDir, SIGNAL(clicked(bool)), this, SLOT(OnSelectWorkDir()));
}

ServiceConfigDialog::~ServiceConfigDialog()
{
    delete ui;
}

void ServiceConfigDialog::SetServiceConfig(const SServiceConfig &sServiceConfig)
{
    m_sServiceConfig = sServiceConfig;
    ui->LeName->setText(sServiceConfig._strName);
    ui->LeProgramFile->setText(sServiceConfig._strProgramFile);
    ui->LeConfigFile->setText(sServiceConfig._strConfFile);
    ui->LeWorkDir->setText(sServiceConfig._strWorkDir);
}

void ServiceConfigDialog::EnableModification()
{
    ui->LeName->setEnabled(true);
    ui->LeProgramFile->setEnabled(true);
    ui->LeConfigFile->setEnabled(true);
    ui->LeWorkDir->setEnabled(true);
    ui->BtnSelectConfigFile->setEnabled(true);
    ui->BtnSelectProgramFile->setEnabled(true);
}

void ServiceConfigDialog::DisableModification()
{
    ui->LeName->setEnabled(false);
    ui->LeProgramFile->setEnabled(false);
    ui->LeConfigFile->setEnabled(false);
    ui->LeWorkDir->setEnabled(false);
    ui->BtnSelectConfigFile->setEnabled(false);
    ui->BtnSelectProgramFile->setEnabled(false);
}

void ServiceConfigDialog::OnAccepted()
{
    QString strServiceName = ui->LeName->text();
    QString strProgramFile = ui->LeProgramFile->text();
    QString strConfigFile = ui->LeConfigFile->text();
    QString strWorkDir = ui->LeWorkDir->text();

    //check empty
    if (strServiceName.isEmpty())
    {
        QMessageBox::about(this, "Error", QString("Name can't be empty"));
        return;
    }
    else if (strProgramFile.isEmpty())
    {
        QMessageBox::about(this, "Error", QString("Program file path can't be empty"));
        return;
    }
    else if (strConfigFile.isEmpty())
    {
        QMessageBox::about(this, "Error", QString("Config file path can't be empty"));
        return;
    }
    else if (strWorkDir.isEmpty())
    {
        QMessageBox::about(this, "Error", QString("Work directory can't be empty"));
        return;
    }

    //check service name
    bool bIsRegistered = false;
    if (m_bIsAdd)
    {
        bIsRegistered = ServiceConfigManager::GetInstance()->IsServiceNameRegistered(strServiceName);
    }
    else
    {
        if (strServiceName == m_sServiceConfig._strName)
        {
            bIsRegistered = false;
        }
        else
        {
            bIsRegistered = ServiceConfigManager::GetInstance()->IsServiceNameRegistered(strServiceName);
        }
    }

    if (!bIsRegistered)
    {
        //check whether file exists
        if (!QFile::exists(strProgramFile))
        {
            QMessageBox::about(this, "Error", QString("Program file '") + strProgramFile + "' does not exist");
            return;
        }
        else if (!QFile::exists(strConfigFile))
        {
            QMessageBox::about(this, "Error", QString("Config file '") + strConfigFile + "' does not exist");
            return;
        }
        else if (!QFile::exists(strWorkDir))
        {
            QMessageBox::about(this, "Error", QString("Work directory '") + strConfigFile + "' does not exist");
            return;
        }
        else
        {
            m_sServiceConfig._strName = strServiceName;
            m_sServiceConfig._strProgramFile = strProgramFile;
            m_sServiceConfig._strConfFile = strConfigFile;
            m_sServiceConfig._strWorkDir = strWorkDir;
        }

        done(MB_CHOOSEN_OK);
    }
    else
    {
        QMessageBox::about(this, "Error", QString("The service name '") + strServiceName + "' exists");
    }
}

void ServiceConfigDialog::OnRejected()
{
    done(MB_CHOOSEN_CANCEL);
}

void ServiceConfigDialog::OnSelectProgramFile()
{
    QString strStartDir = ui->LeProgramFile->text();
    if (strStartDir.isEmpty())
    {
        strStartDir = ui->LeWorkDir->text();
    }
    else
    {
        strStartDir = QFileInfo(strStartDir).filePath();
    }

    if (strStartDir.isEmpty())
    {
        strStartDir = ".";
    }

    QString strFile = QFileDialog::getOpenFileName(this,
        tr("Open File"), strStartDir, tr("Exec Files (*.exe)"));
    if (!strFile.isEmpty())
    {
        ui->LeProgramFile->setText(strFile);
    }
}

void ServiceConfigDialog::OnSelectConfigFile()
{
    QString strStartDir = ui->LeConfigFile->text();
    if (strStartDir.isEmpty())
    {
        strStartDir = ui->LeWorkDir->text();
    }
    else
    {
        strStartDir = QFileInfo(strStartDir).filePath();
    }

    if (strStartDir.isEmpty())
    {
        strStartDir = ".";
    }

    QString strFile = QFileDialog::getOpenFileName(this,
        tr("Open File"), strStartDir, tr("All Files(*.*)"));
    if (!strFile.isEmpty())
    {
        ui->LeConfigFile->setText(strFile);
    }
}

void ServiceConfigDialog::OnSelectWorkDir()
{
    QString strStartDir = ui->LeWorkDir->text();
    if (strStartDir.isEmpty())
    {
        strStartDir = ".";
    }

    QString strDir = QFileDialog::getExistingDirectory(this,
        tr("Open Dir"), strStartDir);
    if (!strDir.isEmpty())
    {
        ui->LeWorkDir->setText(strDir);
    }
}
