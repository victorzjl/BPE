#include "delbutton.h"

DelButton::DelButton():
    m_iNo(-1)
{
    setStyleSheet("border: 1px solid black; background-image: url(:/picture/resource/delete.png);");
    setFixedSize(25, 25);
    connect(this, SIGNAL(clicked(bool)), this, SLOT(OnClicked(bool)));
}

DelButton::~DelButton()
{

}

void DelButton::OnClicked(bool)
{
    emit DelClicked(m_iNo);
}

