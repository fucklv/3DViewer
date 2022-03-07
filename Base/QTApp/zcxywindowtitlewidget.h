#ifndef ZCXYWINDOWTITLEWIDGET_H
#define ZCXYWINDOWTITLEWIDGET_H

#include <QMainWindow>
#include <QMouseEvent>
#include "qSlicerBaseQTAppExport.h"


namespace Ui {
class ZCXYWindowTitleWidget;
}

class Q_SLICER_BASE_QTAPP_EXPORT ZCXYWindowTitleWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit ZCXYWindowTitleWidget(QWidget *parent = nullptr);
    ~ZCXYWindowTitleWidget();


    static ZCXYWindowTitleWidget* p_Instance;
    static ZCXYWindowTitleWidget* getInstance();

    //居中显示
    void showPosition();

signals:
    //关闭主窗口
    void signalWindowClose();

private:
    //初始化
    void initWidget();
    //设置窗口属性
    void setWidgetBorderless(const QWidget* widget);
    //检测窗体变化并相应的处理
    bool nativeEvent(const QByteArray& eventType, void* message, long* result);
    //窗口移动  窗口停靠
    void mousePressEvent(QMouseEvent* event);

private slots:
    //关闭
    void on_Window_Close_clicked();
    //最大化
    void on_Window_Maximize_clicked();
    //最小化
    void on_Window_Minimize_clicked();

private:
    Ui::ZCXYWindowTitleWidget *ui;

    bool m_reduction;        //窗口最大化变化
};

#endif // ZCXYWINDOWTITLEWIDGET_H
