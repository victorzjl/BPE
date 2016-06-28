#ifndef QDELBUTTON_H
#define QDELBUTTON_H

#include <QPushButton>

class DelButton : public QPushButton
{
    Q_OBJECT

public:
    DelButton();
    ~DelButton();

    void SetNo(int iNo) { m_iNo = iNo; }
    int GetNo() { return m_iNo; }

signals:
    void DelClicked(int);

private:
    void Init();

private slots:
    void OnClicked(bool);

private:
    int m_iNo;
};

#endif // QDELBUTTON_H
