#include "mainwindow.h"
#include "serviceconfigmanager.h"
#include <QApplication>
#include <QSharedMemory>
#include <QMessageBox>
#include <QSystemSemaphore>

#define INSTANCE_SEMAPHORE_NAME "WebBpeLauncher_Semaphore"
#define INSTANCE_SHARE_MEM_NAME "WebBpeLauncher_ShareMem"



int main(int argc, char *argv[])
{
    QApplication::addLibraryPath("./QtPlugins");
    QApplication a(argc, argv);

    //check whether instance exists
    QSystemSemaphore qSysSemaphore(INSTANCE_SEMAPHORE_NAME, 1, QSystemSemaphore::Open);
    QSharedMemory qShareMem(INSTANCE_SHARE_MEM_NAME);
    if (qSysSemaphore.acquire())
    {
        if(!qShareMem.create(1))
        {
            QMessageBox::information(0, "Info", "An instance has already been running.");
            qSysSemaphore.release();
            return 0;
        }
        qSysSemaphore.release();
    }
    else
    {
        QMessageBox::information(0, "Info", "Fail to start. Error[Can't Lock Semaphore]");
        return 0;
    }



    //launch
    MainWindow w;
    w.show();
    int iRet = a.exec();

    //clean
    ServiceConfigManager::DeleteInstance();

    return iRet;
}
