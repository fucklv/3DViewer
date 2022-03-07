#include "progressbarfile.h"
#include "qMRMLThreeDViewControllerWidget.h"

#include <QVBoxLayout>
#pragma execution_character_set("utf-8")

ProgressbarFile *ProgressbarFile::p_Instance = nullptr;
ProgressbarFile::ProgressbarFile(QWidget *parent) : QDialog(parent)
{
    this->setFixedSize(QSize(350, 100));

    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinimizeButtonHint);
    p_Label = new QLabel(this);
    p_Label->setText("");

    p_Progressbar = new QProgressBar(this);
    p_Progressbar->setFixedHeight(5);
    p_Progressbar->setValue(23);

    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->addStretch();
    vLayout->addWidget(p_Label);
    vLayout->addWidget(p_Progressbar);
    vLayout->addStretch();
    vLayout->setSpacing(10);
    this->setLayout(vLayout);
    this->setContentsMargins(9, 0, 9, 0);
}

ProgressbarFile *ProgressbarFile::getInstance()
{
    if(nullptr == p_Instance)
    {
        p_Instance = new ProgressbarFile();
    }

    return p_Instance;
}

void ProgressbarFile::setDialogParent(QWidget* parent)
{
    this->setParent(parent);
}

void ProgressbarFile::setProgressbarText(const QString& title, const QString &text)
{
    this->setWindowTitle(title);
    p_Label->setText(text);
}

void ProgressbarFile::setProgressBarValue(int value)
{
    p_Progressbar->setValue(value);
}

void ProgressbarFile::setOrientationMarkerTypeTT()
{
    emit test(3);
}
