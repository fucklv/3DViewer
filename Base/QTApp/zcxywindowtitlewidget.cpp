#include "zcxywindowtitlewidget.h"
#include "ui_zcxywindowtitlewidget.h"

#include <QDebug>
#include <QDesktopWidget>

#pragma execution_character_set("utf-8")

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <dwmapi.h>

#define G_NBORDER 4

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)    ((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)    ((int)(short)HIWORD(lParam))
#endif

#endif

ZCXYWindowTitleWidget* ZCXYWindowTitleWidget::p_Instance = nullptr;

ZCXYWindowTitleWidget::ZCXYWindowTitleWidget(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ZCXYWindowTitleWidget)
{
    ui->setupUi(this);
    initWidget();
}

ZCXYWindowTitleWidget::~ZCXYWindowTitleWidget()
{
    delete ui;
}



ZCXYWindowTitleWidget* ZCXYWindowTitleWidget::getInstance()
{
    if (nullptr == p_Instance)
    {
        p_Instance = new ZCXYWindowTitleWidget();
    }
    return p_Instance;
}

/*!
 * \brief 居中显示
 */
void ZCXYWindowTitleWidget::showPosition()
{
    QDesktopWidget* desktop = qApp->desktop();
    this->move((desktop->width() - this->width()) / 2, (desktop->height() - this->height()) / 2);
    this->show();
}

/*!
 * \brief 初始化
 */
void ZCXYWindowTitleWidget::initWidget()
{
    m_reduction = false;

    setWidgetBorderless(this);
    this->setWindowTitle("中创新影");
    this->setWindowIcon(QIcon(":/Icons/Medium/DesktopIcon.png"));

    this->setAttribute(Qt::WA_DeleteOnClose, 1);

    //激活窗体为桌面的顶层窗体
    this->setWindowState(Qt::WindowActive | Qt::WindowMaximized);
    this->setMouseTracking(true);
    this->setFocus();

    //加入主窗口
    //QWidget* widget = qSlicerMainWindow::getInstance(this);
    //ui->verticalLayout->addWidget(widget);

    QImage image(":/Images/LOGO.png");
    ui->labelTitleImage->setPixmap(QPixmap::fromImage(image));
}

/*!
 * \brief 设置窗口属性
 */
void ZCXYWindowTitleWidget::setWidgetBorderless(const QWidget* widget)
{
    setWindowFlags(Qt::FramelessWindowHint);
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);
#endif
}

/*!
 * \brief 检测窗体变化并相应的处理
 */
bool ZCXYWindowTitleWidget::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
#ifdef Q_OS_WIN
    if (eventType != "windows_generic_MSG")
        return false;

    MSG* msg = static_cast<MSG*>(message);
    QWidget* widget = QWidget::find(reinterpret_cast<WId>(msg->hwnd));
    if (!widget)
        return false;

    switch (msg->message)
    {

    case WM_NCCALCSIZE:
    {
        *result = 0;
        return true;
    }

    //检测鼠标拉伸变样式
    case WM_NCHITTEST:
    {

        QPoint pos = mapFromGlobal(QPoint(LOWORD(msg->lParam), HIWORD(msg->lParam)));
        bool bHorLeft = pos.x() < G_NBORDER;
        bool bHorRight = pos.x() > width() - G_NBORDER;
        bool bVertTop = pos.y() < G_NBORDER;
        bool bVertBottom = pos.y() > height() - G_NBORDER;

        if (bHorLeft && bVertTop)
        {
            *result = HTTOPLEFT;
        }
        else if (bHorLeft && bVertBottom)
        {
            *result = HTBOTTOMLEFT;
        }
        else if (bHorRight && bVertTop)
        {
            *result = HTTOPRIGHT;
        }
        else if (bHorRight && bVertBottom)
        {
            *result = HTBOTTOMRIGHT;
        }
        else if (bHorLeft)
        {
            *result = HTLEFT;
        }
        else if (bHorRight)
        {
            *result = HTRIGHT;
        }
        else if (bVertTop)
        {
            *result = HTTOP;
        }
        else if (bVertBottom)
        {
            *result = HTBOTTOM;
        }
        else
        {
            return false;
        }
        return true;
    }


    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* p = (MINMAXINFO*)(msg->lParam);
        p->ptMinTrackSize.x = 1550;
        p->ptMinTrackSize.y = 900;

        if (::IsZoomed(msg->hwnd))
        {
            RECT frame = { 0, 0, 0, 0 };
            AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);
            frame.left = abs(frame.left);
            frame.top = abs(frame.bottom);
            widget->setContentsMargins(frame.left, frame.top, frame.right, frame.bottom);
        }
        else
        {
            widget->setContentsMargins(0, 0, 0, 0);
        }

        *result = ::DefWindowProc(msg->hwnd, msg->message, msg->wParam, msg->lParam);
        return true;
    }
    break;

    default:
        break;
    }

    //检测窗口是否最大化
    if (IsZoomed(msg->hwnd))
    {
        m_reduction = true;
        ui->Window_Maximize->setStyleSheet("QPushButton{"\
            "width : 30;"\
            "height: 30;"\
            "border-image: url(:/Images/reductionWindow.png);}"\
            "QPushButton:hover{"\
            "width : 30;"\
            "height: 30;"\
            "background-color: rgb(60, 60, 90);"\
            "border-image: url(:/Images/reductionWindow1.png);}");
    }
    else
    {
        m_reduction = false;
        ui->Window_Maximize->setStyleSheet("QPushButton{"\
            "width : 30;"\
            "height: 30;"\
            "border-image: url(:/Images/maxWindow.png);}"\
            "QPushButton:hover{"\
            "width : 30;"\
            "height: 30;"\
            "background-color: rgb(60, 60, 90);"\
            "border-image: url(:/Images/maxWindow1.png);}");
    }

/**************最小化判断******
    if (IsIconic(msg->hwnd))
    {
        this->setWindowTitle("最小化");
    }
    else
    {
        this->setWindowTitle("还原");
    }
******************************/

#endif
    return QWidget::nativeEvent(eventType, message, result);
}

/*!
 * \brief 窗口移动  窗口停靠
 */
void ZCXYWindowTitleWidget::mousePressEvent(QMouseEvent* event)
{
    if (ReleaseCapture())
        SendMessage(HWND(this->winId()), WM_SYSCOMMAND, SC_MOVE + HTCAPTION, 0);
    event->ignore();
}

/*!
 * \brief 关闭
 */
void ZCXYWindowTitleWidget::on_Window_Close_clicked()
{
    emit ZCXYWindowTitleWidget::getInstance()->signalWindowClose();
}

/*!
 * \brief 最大化
 */
void ZCXYWindowTitleWidget::on_Window_Maximize_clicked()
{
    if (!m_reduction)
    {
        this->showMaximized();

    }
    else
    {
        this->showNormal();
    }
}

/*!
 * \brief 最小化
 */
void ZCXYWindowTitleWidget::on_Window_Minimize_clicked()
{
    this->showMinimized();
}
