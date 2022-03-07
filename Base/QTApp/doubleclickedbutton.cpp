#include "doubleClickedButton.h"
#include <QDebug>

DoubleClickedButton::DoubleClickedButton(QWidget *parent) : QPushButton(parent)
{

}

DoubleClickedButton::~DoubleClickedButton()
{

}

void DoubleClickedButton::mouseDoubleClickEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        emit signalsDoubleClicked();
    }
}
