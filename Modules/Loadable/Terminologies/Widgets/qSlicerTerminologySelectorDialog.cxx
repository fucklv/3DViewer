/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Terminologies includes
#include "qSlicerTerminologySelectorDialog.h"

#include "vtkSlicerTerminologyEntry.h"

// Qt includes
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>
#include <QLabel>
#include <QMouseEvent>
#include <QTimer>
#include <QColorDialog>
#pragma execution_character_set("utf-8")

#ifdef Q_OS_WIN
#include <qt_windows.h>
#include <dwmapi.h>

#define G_NBORDER 4             //检测鼠标与边框距离变样式

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)    ((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)    ((int)(short)HIWORD(lParam))
#endif
#endif

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Terminologies_Widgets
class qSlicerTerminologySelectorDialogPrivate : public QDialog
{
  Q_DECLARE_PUBLIC(qSlicerTerminologySelectorDialog);
protected:
  qSlicerTerminologySelectorDialog* const q_ptr;
public:
  qSlicerTerminologySelectorDialogPrivate(qSlicerTerminologySelectorDialog& object);
  ~qSlicerTerminologySelectorDialogPrivate() override;

private:
    void mousePressEvent(QMouseEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;

public:
  void init();
private:
  qSlicerTerminologyNavigatorWidget* NavigatorWidget;
  QPushButton* SelectButton;
  QPushButton* CancelButton;
  QLabel* m_pDragLabel;
  QColorDialog* ColorDialog;

  /// Terminology and other metadata (name, color, auto-generated flags) into which the selection is set
  qSlicerTerminologyNavigatorWidget::TerminologyInfoBundle TerminologyInfo;
};

//-----------------------------------------------------------------------------
qSlicerTerminologySelectorDialogPrivate::qSlicerTerminologySelectorDialogPrivate(qSlicerTerminologySelectorDialog& object)
  : q_ptr(&object)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Widget);
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(this->winId());
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);
#endif
}

//-----------------------------------------------------------------------------
qSlicerTerminologySelectorDialogPrivate::~qSlicerTerminologySelectorDialogPrivate() = default;

void qSlicerTerminologySelectorDialogPrivate::mousePressEvent(QMouseEvent* event)
{
    if (ReleaseCapture())
    {
        SendMessage(HWND(this->winId()), WM_SYSCOMMAND, SC_MOVE + HTCAPTION, 0);
        SendMessage(HWND(this->winId()), WM_LBUTTONUP, 0, 0);
    }
    event->ignore();
}

bool qSlicerTerminologySelectorDialogPrivate::nativeEvent(const QByteArray& eventType, void* message, long* result)
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
        //设置窗口最小拉伸大小
        MINMAXINFO* p = (MINMAXINFO*)(msg->lParam);
        p->ptMinTrackSize.x = 300;
        p->ptMinTrackSize.y = 400;
        p->ptMaxTrackSize.x = 900;
        p->ptMaxTrackSize.y = 400;

        //设置窗口停靠时上下左右距离边缘的距离 （setContentsMargins）
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

#endif
    return QDialog::nativeEvent(eventType, message, result);
}

//-----------------------------------------------------------------------------
void qSlicerTerminologySelectorDialogPrivate::init()
{
  Q_Q(qSlicerTerminologySelectorDialog);

  // Set up UI
  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setSpacing(4);
  layout->setContentsMargins(0, 0, 0, 4);

  ColorDialog = new QColorDialog;

  QPushButton* pCloseButton = new QPushButton;
  //设置拖拽图标
  QHBoxLayout* pCloseLayout = new QHBoxLayout();
  pCloseLayout->setSpacing(4);
  pCloseLayout->setContentsMargins(0, 0, 0, 0);
  pCloseLayout->addStretch();
  pCloseLayout->addWidget(pCloseButton);
  layout->addLayout(pCloseLayout);

  this->NavigatorWidget = new qSlicerTerminologyNavigatorWidget();
  layout->addWidget(this->NavigatorWidget);

  QHBoxLayout* buttonsLayout = new QHBoxLayout();
  buttonsLayout->setSpacing(4);
  buttonsLayout->setContentsMargins(4, 4, 4, 4);
  buttonsLayout->addStretch();

  this->SelectButton = new QPushButton("选择");
  this->SelectButton->setDefault(true);
  this->SelectButton->setEnabled(false); // Disabled until terminology selection becomes valid
  buttonsLayout->addWidget(this->SelectButton);

  this->CancelButton = new QPushButton("取消");
  buttonsLayout->addWidget(this->CancelButton);

  //设置拖拽图标
  QHBoxLayout* dragLayout = new QHBoxLayout();
  dragLayout->setSpacing(4);
  dragLayout->setContentsMargins(4, 4, 4, 4);
  dragLayout->addStretch();

  this->m_pDragLabel = new QLabel();
  dragLayout->addWidget(this->m_pDragLabel);

  this->m_pDragLabel->setStyleSheet("QLabel{min-width: 8; min-height: 8; max-width: 8; max-height: 8; border-image: url(:/Icons/dragSg.png);}");
  this->SelectButton->setStyleSheet("QPushButton{font: 12pt 'SimHei'; min-width: 68; min-height: 24; max-width: 68; max-height: 24; background: #191925; border: 1px solid #5c5c74;}"\
      "QPushButton::hover {background-color: #5C5C74; }");
  this->CancelButton->setStyleSheet("QPushButton{font: 12pt 'SimHei'; min-width: 68; min-height: 24; max-width: 68; max-height: 24; background: #191925; border: 1px solid #5c5c74;}"\
      "QPushButton::hover {background-color: #5C5C74; }");
  pCloseButton->setStyleSheet("QPushButton{min-width: 30; min-height: 30; max-width: 30; max-height: 30; border-image: url(:/Icons/closeSg.png);}"\
      "QPushButton:hover{background-color: #5C5C74;}");

  layout->addLayout(buttonsLayout);
  layout->addLayout(dragLayout);
  this->setMaximumWidth(900);

  // Make connections
  connect(this->NavigatorWidget, SIGNAL(selectionValidityChanged(bool)), q, SLOT(setSelectButtonEnabled(bool)));
  connect(this->SelectButton, SIGNAL(clicked()), this, SLOT(accept()));
  connect(this->CancelButton, SIGNAL(clicked()), this, SLOT(reject()));
  connect(pCloseButton, SIGNAL(clicked()), this, SLOT(reject()));
}

//-----------------------------------------------------------------------------
// qSlicerTerminologySelectorDialog methods

//-----------------------------------------------------------------------------
qSlicerTerminologySelectorDialog::qSlicerTerminologySelectorDialog(QObject* parent)
  : QObject(parent)
  , d_ptr(new qSlicerTerminologySelectorDialogPrivate(*this))
{
  Q_D(qSlicerTerminologySelectorDialog);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerTerminologySelectorDialog::qSlicerTerminologySelectorDialog(
  qSlicerTerminologyNavigatorWidget::TerminologyInfoBundle &initialTerminologyInfo, QObject* parent)
  : QObject(parent)
  , d_ptr(new qSlicerTerminologySelectorDialogPrivate(*this))
{
  Q_D(qSlicerTerminologySelectorDialog);
  d->TerminologyInfo = initialTerminologyInfo;
  
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerTerminologySelectorDialog::~qSlicerTerminologySelectorDialog() = default;

//-----------------------------------------------------------------------------
bool qSlicerTerminologySelectorDialog::exec()
{
  Q_D(qSlicerTerminologySelectorDialog);

  // Initialize dialog
  d->NavigatorWidget->setTerminologyInfo(d->TerminologyInfo);

  // Show dialog
  bool result = false;

  if (d->ColorDialog->exec() != QDialog::Accepted)
    {
    return result;
    }
  
  // Save selection after clean exit
  QColor newColor;
  newColor = d->ColorDialog->selectedColor();
    
  d->NavigatorWidget->setColorRGB(newColor);
  d->NavigatorWidget->terminologyInfo(d->TerminologyInfo);
  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerTerminologySelectorDialog::getTerminology(
  qSlicerTerminologyNavigatorWidget::TerminologyInfoBundle &terminologyInfo, QObject* parent)
{
  // Open terminology dialog and store result
  qSlicerTerminologySelectorDialog dialog(terminologyInfo, parent);
  bool result = dialog.exec();
  dialog.terminologyInfo(terminologyInfo);
  return result;
}

//-----------------------------------------------------------------------------
bool qSlicerTerminologySelectorDialog::getTerminology(vtkSlicerTerminologyEntry* terminologyEntry, QObject* parent)
{
  qSlicerTerminologyNavigatorWidget::TerminologyInfoBundle terminologyInfo;
  terminologyInfo.GetTerminologyEntry()->Copy(terminologyEntry);
  // Open terminology dialog and store result
  qSlicerTerminologySelectorDialog dialog(terminologyInfo, parent);
  dialog.setOverrideSectionVisible(false);
  if (!dialog.exec())
    {
    return false;
    }
  dialog.terminologyInfo(terminologyInfo);
  terminologyEntry->Copy(terminologyInfo.GetTerminologyEntry());
  return true;
}

//-----------------------------------------------------------------------------
void qSlicerTerminologySelectorDialog::terminologyInfo(
  qSlicerTerminologyNavigatorWidget::TerminologyInfoBundle &terminologyInfo )
{
  Q_D(qSlicerTerminologySelectorDialog);
  terminologyInfo = d->TerminologyInfo;
}

//-----------------------------------------------------------------------------
void qSlicerTerminologySelectorDialog::setSelectButtonEnabled(bool enabled)
{
  Q_D(qSlicerTerminologySelectorDialog);
  d->SelectButton->setEnabled(enabled);
}

//-----------------------------------------------------------------------------
bool qSlicerTerminologySelectorDialog::overrideSectionVisible() const
{
  Q_D(const qSlicerTerminologySelectorDialog);
  return d->NavigatorWidget->overrideSectionVisible();
}

//-----------------------------------------------------------------------------
void qSlicerTerminologySelectorDialog::setOverrideSectionVisible(bool visible)
{
  Q_D(qSlicerTerminologySelectorDialog);
  d->NavigatorWidget->setOverrideSectionVisible(visible);
}
