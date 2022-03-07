#ifndef DOUBLECLICKEDBUTTON_H
#define DOUBLECLICKEDBUTTON_H

#include <QMouseEvent>
#include <QPushButton>
#include "qSlicerBaseQTAppExport.h"


class Q_SLICER_BASE_QTAPP_EXPORT DoubleClickedButton : public QPushButton
{
    Q_OBJECT

public:
    explicit DoubleClickedButton(QWidget *parent = nullptr);
    ~DoubleClickedButton();

    virtual void mouseDoubleClickEvent(QMouseEvent *event);

signals:
    //双击按钮
    void signalsDoubleClicked();
};

#endif // DOUBLECLICKEDBUTTON_H
