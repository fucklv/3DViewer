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

    //���ø�����
    void setDialogParent(QWidget *parent);
    //���ñ�������
    void setProgressbarText(const QString& title, const QString &text);
    //���ý���������
    void setProgressBarValue(int value);

    void setOrientationMarkerTypeTT();

private:
    QLabel* p_Label;
    QProgressBar* p_Progressbar;
signals:
    void test(int);
};

#endif // PROGRESSBARFILE_H
