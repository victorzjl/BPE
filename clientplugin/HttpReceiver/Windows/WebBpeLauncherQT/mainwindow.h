#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
#include <QSystemTrayIcon>

#include "widgetcontainer.h"
#include "service.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public:
    void closeEvent(QCloseEvent *ev);

private slots:
    void OnBtnAddServiceClicked(bool);
    void OnBtnSaveAllClicked(bool);
    void OnTrayActived(QSystemTrayIcon::ActivationReason reason);

private:
    void OnloadSavedService();
    void CreateSystemTray();

private:
    Ui::MainWindow *ui;
    WidgetContainer *m_pWidgetContainer;
    QSystemTrayIcon *m_pSysTrayIcon;
    QMenu *m_pTrayMenu;
    QAction *m_pQuitAction;
    QAction *m_pRestoreAction;
};

#endif // MAINWINDOW_H
