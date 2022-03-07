#ifndef PROGRESSBARFILE_H
#define PROGRESSBARFILE_H

#include <QWidget>
#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include "qMRMLWidgetsExport.h"

class QMRML_WIDGETS_EXPORT ProgressbarFile : public QDialog
{
    Q_OBJECT
public:
    explicit ProgressbarFile(QWidget *parent = nullptr);

    static ProgressbarFile *p_Instance;
    static ProgressbarFile *getInstance();

    //设置父窗口
    void setDialogParent(QWidget *parent);
    //设置标题内容
    void setProgressbarText(const QString& title, const QString &text);
    //设置进度条进度
    void setProgressBarValue(int value);

    void setOrientationMarkerTypeTT();

private:
    QLabel* p_Label;
    QProgressBar* p_Progressbar;
signals:
    void test(int);
};

#endif // PROGRESSBARFILE_H
