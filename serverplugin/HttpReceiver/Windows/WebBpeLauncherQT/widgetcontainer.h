#ifndef WIDGETCONTAINER_H
#define WIDGETCONTAINER_H

#include <QMap>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "delbutton.h"
#include "servicewidget.h"

class WidgetContainer : public QWidget
{
    Q_OBJECT
public:
    explicit WidgetContainer(QWidget *parent = 0);
    ~WidgetContainer();

    void Add(const SServiceConfig &sServiceConfig);

private slots:
    void OnServiceStarted(int iNo);
    void OnServiceStoped(int iNo);
    void OnDelBtnClicked(int iNo);

private:
    QHBoxLayout *m_pMainLayoutH;
    QVBoxLayout *m_pSWLayoutV;
    QVBoxLayout *m_pDBLayoutV;

    typedef struct stUnusedWidgetPair
    {
        DelButton *_pDelBtn;
        ServiceWidget *_pServiceWidget;
    }SUnusedWidgetPair;

    int m_iNo;
    QMap<int, DelButton *> m_mapDelBtn;
    QMap<int, ServiceWidget *> m_mapServiceWidget;
    QVector<SUnusedWidgetPair> m_vecUnusedWidget;
};

#endif // WIDGETCONTAINER_H
