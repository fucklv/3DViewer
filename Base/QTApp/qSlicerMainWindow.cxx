/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#include "qSlicerMainWindow_p.h"

// Qt includes
#include <QAction>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QQueue>
#include <QSettings>
#include <QShowEvent>
#include <QSignalMapper>
#include <QStyle>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QMenuBar>
#include <QButtonGroup>

// CTK includes
#include <ctkErrorLogWidget.h>
#include <ctkMessageBox.h>
#ifdef Slicer_USE_PYTHONQT
# include <ctkPythonConsole.h>
#endif
#ifdef Slicer_USE_QtTesting
#include <ctkQtTestingUtility.h>
#endif
#include <ctkSettingsDialog.h>
#include <ctkVTKSliceView.h>
#include <ctkVTKWidgetsUtils.h>

// SlicerApp includes
#include "qSlicerAboutDialog.h"
#include "qSlicerActionsDialog.h"
#include "qSlicerApplication.h"
#include "qSlicerAbstractModule.h"
#if defined Slicer_USE_QtTesting && defined Slicer_BUILD_CLI_SUPPORT
#include "qSlicerCLIModuleWidgetEventPlayer.h"
#endif
#include "qSlicerCommandOptions.h"
#include "qSlicerCoreCommandOptions.h"
#include "qSlicerErrorReportDialog.h"
#include "qSlicerLayoutManager.h"
#include "qSlicerModuleManager.h"
#include "qSlicerModulesMenu.h"
#include "qSlicerModuleSelectorToolBar.h"
#include "qSlicerIOManager.h"
#include "qSlicerMouseModeToolBar.h"
#include "qSlicerViewersToolBar.h"
#include "recordinformationtool.h"
#include "progressbarfile.h"

// qMRML includes
#include <qMRMLSliceWidget.h>

// MRMLLogic includes
#include <vtkMRMLSliceLogic.h>
#include <vtkMRMLSliceLayerLogic.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLLayoutNode.h>

// VTK includes
#include <vtkCollection.h>
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

namespace
{

//-----------------------------------------------------------------------------
void setThemeIcon(QAction* action, const QString& name)
{
  action->setIcon(QIcon::fromTheme(name, action->icon()));
}

} // end of anonymous namespace

//-----------------------------------------------------------------------------
// qSlicerMainWindowPrivate methods

qSlicerMainWindowPrivate::qSlicerMainWindowPrivate(qSlicerMainWindow& object)
  : q_ptr(&object)
{
#ifdef Slicer_USE_PYTHONQT
  this->PythonConsoleDockWidget = nullptr;
  this->PythonConsoleToggleViewAction = nullptr;
#endif
  this->ErrorLogWidget = nullptr;
  this->ErrorLogToolButton = nullptr;
  this->ModuleSelectorToolBar = nullptr;
  this->LayoutManager = nullptr;
  this->WindowInitialShowCompleted = false;
  this->IsClosing = false;

  this->MouseModeToolBar = nullptr;
  this->ViewersToolBar = nullptr;

  this->LayoutMenu = nullptr;
  this->menuConventionalQuantitative = nullptr;
  this->menuFourUpQuantitative = nullptr;
  this->menuOneUpQuantitative = nullptr;
  this->menuThreeOverThreeQuantitative = nullptr;
  this->menuOneUpQuantitative = nullptr;
}

//-----------------------------------------------------------------------------
qSlicerMainWindowPrivate::~qSlicerMainWindowPrivate()
{
  delete this->ErrorLogWidget;
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::init()
{
  Q_Q(qSlicerMainWindow);
  this->setupUi(q);
  this->setupStatusBar();
  q->setSlicerWindowUi();
  q->initWidget();
  q->setupMenuActions();
  this->StartupState = q->saveState();
  this->readSettings();
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::setupUi(QMainWindow * mainWindow)
{
  Q_Q(qSlicerMainWindow);

  this->Ui_qSlicerMainWindow::setupUi(mainWindow);

  q->setFixedSize(1550, 900);
  qSlicerApplication * app = qSlicerApplication::application();

  //----------------------------------------------------------------------------
  // Load data shortcuts for backward compatibility
  //----------------------------------------------------------------------------
  //QList<QKeySequence> addShortcuts = this->FileAddDataAction->shortcuts();
  //addShortcuts << QKeySequence(Qt::CTRL + Qt::Key_A);
  //this->FileAddDataAction->setShortcuts(addShortcuts);

  //----------------------------------------------------------------------------
  // Recently loaded files
  //----------------------------------------------------------------------------
  QObject::connect(app->coreIOManager(), SIGNAL(newFileLoaded(qSlicerIO::IOProperties)),
                   q, SLOT(onNewFileLoaded(qSlicerIO::IOProperties)));

  //----------------------------------------------------------------------------
  // Load DICOM
  //----------------------------------------------------------------------------
#ifndef Slicer_BUILD_DICOM_SUPPORT
  this->LoadDICOMAction->setVisible(false);
#endif

  //----------------------------------------------------------------------------
  // ModulePanel
  //----------------------------------------------------------------------------
  this->PanelDockWidget->toggleViewAction()->setText(qSlicerMainWindow::tr("&Module Panel"));
  //this->PanelDockWidget->toggleViewAction()->setToolTip(
  //  qSlicerMainWindow::tr("Collapse/Expand the GUI panel and allows Slicer's viewers to occupy "
  //        "the entire application window"));
  this->PanelDockWidget->toggleViewAction()->setShortcut(QKeySequence("Ctrl+5"));
  //this->ViewMenu->insertAction(this->WindowToolBarsMenu->menuAction(),
                               //this->PanelDockWidget->toggleViewAction());

  //屏蔽右键菜单
  mainWindow->setContextMenuPolicy(Qt::NoContextMenu);
  //----------------------------------------------------------------------------
  // ModuleManager
  //----------------------------------------------------------------------------
  // Update the list of modules when they are loaded
  qSlicerModuleManager * moduleManager = qSlicerApplication::application()->moduleManager();
  if (!moduleManager)
    {
    qWarning() << "No module manager is created.";
    }

  QObject::connect(moduleManager,SIGNAL(moduleLoaded(QString)),
                   q, SLOT(onModuleLoaded(QString)));

  QObject::connect(moduleManager, SIGNAL(moduleAboutToBeUnloaded(QString)),
                   q, SLOT(onModuleAboutToBeUnloaded(QString)));

  //----------------------------------------------------------------------------
  // ModuleSelector ToolBar
  //----------------------------------------------------------------------------
  // Create a Module selector
  this->ModuleSelectorToolBar = new qSlicerModuleSelectorToolBar("Module Selection",this->moduleSelectorTool);
  this->ModuleSelectorToolBar->setObjectName(QString::fromUtf8("ModuleSelectorToolBar"));
  this->ModuleSelectorToolBar->setAllowedAreas(Qt::NoToolBarArea);
  this->ModuleSelectorToolBar->setOrientation(Qt::Vertical);
  this->ModuleSelectorToolBar->setMovable(false);
  this->ModuleSelectorToolBar->setModuleManager(moduleManager);
  //q->insertToolBar(this->ModuleToolBar, this->ModuleSelectorToolBar);
  //this->ModuleSelectorToolBar->stackUnder(this->ModuleToolBar);
  this->moduleSelectorTool->setVisible(false);
  this->interactiveLobeSegmentationButton->setVisible(false);

  // Connect the selector with the module panel
  this->ModulePanel->setModuleManager(moduleManager);
  QObject::connect(this->ModuleSelectorToolBar, SIGNAL(moduleSelected(QString)),
                   this->ModulePanel, SLOT(setModule(QString)));

  // Ensure the panel dock widget is visible
  QObject::connect(this->ModuleSelectorToolBar, SIGNAL(moduleSelected(QString)),
                   this->PanelDockWidget, SLOT(show()));

  //----------------------------------------------------------------------------
  // MouseMode ToolBar
  //----------------------------------------------------------------------------
  // MouseMode toolBar should listen the MRML scene
  //QWidget* MouseModeToolBarWidget = new QWidget();
  QHBoxLayout *layout = new QHBoxLayout(this->ToolBarWidgets);
  this->ToolBarWidgets->setLayout(layout);
  layout->setContentsMargins(0,0,0,0);
  layout->setSpacing(4);

  //kyo
  this->toolbarWidget->hide();

  QToolBar* openFileButton = new QToolBar(mainWindow);
  auto openFileAction = new QAction(openFileButton);
  openFileAction->setObjectName("OpenFilesBnt");
  openFileAction->setIcon(QIcon(":/Images/OpenFile.png"));
  openFileAction->setCheckable(true);
  openFileButton->addAction(openFileAction);
  openFileButton->setToolTip("打开文件");
  //QLabel* label1 = new QLabel;
  //label1->setFixedWidth(1);
  //openFileButton->addWidget(label1);
  layout->addWidget(openFileButton);
  QObject::connect(openFileButton, SIGNAL(actionTriggered(QAction*)),
      q, SLOT(on_openfileButton_clicked(QAction*)));
  
  //保存
  QToolBar* savebtn = new QToolBar(mainWindow);
  auto saveAction = new QAction(savebtn);
  saveAction->setObjectName("markbtn");
  saveAction->setIcon(QIcon(":/Images/save.png"));
  saveAction->setCheckable(true);
  savebtn->addAction(saveAction);
  layout->addWidget(savebtn);
  savebtn->setToolTip("保存");
  QObject::connect(savebtn, SIGNAL(actionTriggered(QAction*)),
      q, SLOT(on_savebtnButton_clicked(QAction*)));

  QFrame* line1 = new QFrame();
  line1->setFrameShape(QFrame::VLine);
  line1->setFrameShadow(QFrame::Plain);
  line1->setFixedHeight(19);
  line1->setStyleSheet("color:#272739");
  line1->raise();
  line1->setFocusPolicy(Qt::NoFocus);
  layout->addWidget(line1);

  //鼠标和窗宽窗位
  this->ToolBarWidgets->setContentsMargins(0, 0, 0, 0);
  this->MouseModeToolBar = new qSlicerMouseModeToolBar("mouseModeTool", mainWindow);
  this->MouseModeToolBar->setObjectName(QString::fromUtf8("MouseModeToolBar"));
  this->MouseModeToolBar->setAllowedAreas(Qt::NoToolBarArea);
  this->MouseModeToolBar->setOrientation(Qt::Horizontal);
  layout->addWidget(this->MouseModeToolBar);
  this->MouseModeToolBar->setApplicationLogic(
    qSlicerApplication::application()->applicationLogic());
  this->MouseModeToolBar->setMRMLScene(qSlicerApplication::application()->mrmlScene());
  QObject::connect(qSlicerApplication::application(),
                   SIGNAL(mrmlSceneChanged(vtkMRMLScene*)),
                   this->MouseModeToolBar,
                   SLOT(setMRMLScene(vtkMRMLScene*)));
  
  //----------------------------------------------------------------------------
  // Capture tool bar
  //----------------------------------------------------------------------------
  // Capture bar needs to listen to the MRML scene and the layout
  //this->CaptureToolBar->setMRMLScene(qSlicerApplication::application()->mrmlScene());
  //QObject::connect(qSlicerApplication::application(),
  //                 SIGNAL(mrmlSceneChanged(vtkMRMLScene*)),
  //                 this->CaptureToolBar,
  //                 SLOT(setMRMLScene(vtkMRMLScene*)));
  //this->CaptureToolBar->setMRMLScene(
  //    qSlicerApplication::application()->mrmlScene());

  //QObject::connect(this->CaptureToolBar,
  //                 SIGNAL(screenshotButtonClicked()),
  //                 qSlicerApplication::application()->ioManager(),
  //                 SLOT(openScreenshotDialog()));

  // to get the scene views module dialog to pop up when a button is pressed
  // in the capture tool bar, we emit a signal, and the
  // io manager will deal with the sceneviews module
  //QObject::connect(this->CaptureToolBar,
  //                 SIGNAL(sceneViewButtonClicked()),
  //                 qSlicerApplication::application()->ioManager(),
  //                 SLOT(openSceneViewsDialog()));

  // if testing is enabled on the application level, add a time out to the pop ups
  //if (qSlicerApplication::application()->testAttribute(qSlicerCoreApplication::AA_EnableTesting))
  //  {
  //  this->CaptureToolBar->setPopupsTimeOut(true);
  //  }

  //QList<QAction*> toolBarActions;
  //toolBarActions << this->MainToolBar->toggleViewAction();
  //toolBarActions << this->UndoRedoToolBar->toggleViewAction();
  //toolBarActions << this->ModuleSelectorToolBar->toggleViewAction();
  //toolBarActions << this->ModuleToolBar->toggleViewAction();
  //toolBarActions << this->ViewToolBar->toggleViewAction();
  //toolBarActions << this->LayoutToolBar->toggleViewAction();
  //toolBarActions << this->MouseModeToolBar->toggleViewAction();
  //toolBarActions << this->CaptureToolBar->toggleViewAction();
  //toolBarActions << this->ViewersToolBar->toggleViewAction();
  //toolBarActions << this->DialogToolBar->toggleViewAction();
  //this->WindowToolBarsMenu->addActions(toolBarActions);

  //----------------------------------------------------------------------------
  // Hide toolbars by default
  //----------------------------------------------------------------------------
  // Hide the Layout toolbar by default
  // The setVisibility slot of the toolbar is connected to the
  // QAction::triggered signal.
  // It's done for a long list of reasons. If you change this behavior, make sure
  // minimizing the application and restore it doesn't hide the module panel. check
  // also the geometry and the state of the menu qactions are correctly restored when
  // loading slicer.
  //this->UndoRedoToolBar->toggleViewAction()->trigger();
  //this->LayoutToolBar->toggleViewAction()->trigger();
  //q->removeToolBar(this->UndoRedoToolBar);
  //q->removeToolBar(this->LayoutToolBar);
  //delete this->UndoRedoToolBar;
  //this->UndoRedoToolBar = nullptr;
  //delete this->LayoutToolBar;
  //this->LayoutToolBar = nullptr;

  // Color of the spacing between views:
  QFrame* layoutFrame = new QFrame(this->CentralWidget);
  layoutFrame->setObjectName("CentralWidgetLayoutFrame");
  QHBoxLayout* centralLayout = new QHBoxLayout(this->CentralWidget);
  centralLayout->setContentsMargins(0, 0, 0, 0);
  centralLayout->addWidget(layoutFrame);

  QColor windowColor = this->CentralWidget->palette().color(QPalette::Window);
  QPalette centralPalette = this->CentralWidget->palette();
  centralPalette.setColor(QPalette::Window, QColor(95, 95, 113));
  this->CentralWidget->setAutoFillBackground(true);
  this->CentralWidget->setPalette(centralPalette);

  // Restore the palette for the children
  centralPalette.setColor(QPalette::Window, windowColor);
  layoutFrame->setPalette(centralPalette);
  layoutFrame->setAttribute(Qt::WA_NoSystemBackground, true);
  layoutFrame->setAttribute(Qt::WA_TranslucentBackground, true);


  //----------------------------------------------------------------------------
  // Layout Manager
  //----------------------------------------------------------------------------
  // Instantiate and assign the layout manager to the slicer application
  this->LayoutManager = new qSlicerLayoutManager(layoutFrame);
  this->LayoutManager->setScriptedDisplayableManagerDirectory(
      qSlicerApplication::application()->slicerHome() + "/bin/Python/mrmlDisplayableManager");
  qSlicerApplication::application()->setLayoutManager(this->LayoutManager);
#ifdef Slicer_USE_QtTesting
  // we store this layout manager to the Object state property for QtTesting
  qSlicerApplication::application()->testingUtility()->addObjectStateProperty(
      qSlicerApplication::application()->layoutManager(), QString("layout"));
  qSlicerApplication::application()->testingUtility()->addObjectStateProperty(
      this->ModuleSelectorToolBar->modulesMenu(), QString("currentModule"));
#endif
  // Layout manager should also listen the MRML scene
  // Note: This creates the OpenGL context for each view, so things like
  // multisampling should probably be configured before this line is executed.
  this->LayoutManager->setMRMLScene(qSlicerApplication::application()->mrmlScene());
  QObject::connect(qSlicerApplication::application(),
                   SIGNAL(mrmlSceneChanged(vtkMRMLScene*)),
                   this->LayoutManager,
                   SLOT(setMRMLScene(vtkMRMLScene*)));
  QObject::connect(this->LayoutManager, SIGNAL(layoutChanged(int)),
                   q, SLOT(onLayoutChanged(int)));

  // TODO: When module will be managed by the layoutManager, this should be
  //       revisited.
  QObject::connect(this->LayoutManager, SIGNAL(selectModule(QString)),
                   this->ModuleSelectorToolBar, SLOT(selectModule(QString)));

  // Add menus for configuring compare view
  QMenu *compareMenu = new QMenu(qSlicerMainWindow::tr("Select number of viewers..."), mainWindow);
  compareMenu->setObjectName("CompareMenuView");
  compareMenu->addAction(this->ViewLayoutCompare_2_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_3_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_4_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_5_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_6_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_7_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompare_8_viewersAction);
  this->ViewLayoutCompareAction->setMenu(compareMenu);
  QObject::connect(compareMenu, SIGNAL(triggered(QAction*)),
                   q, SLOT(onLayoutCompareActionTriggered(QAction*)));
  compareMenu->setStyleSheet("QMenu::item{color: #FFFFFF;}QMenu::item:selected{background: #6987d5;}");

  // ... and for widescreen version of compare view as well
  compareMenu = new QMenu(qSlicerMainWindow::tr("Select number of viewers..."), mainWindow);
  compareMenu->setObjectName("CompareMenuWideScreen");
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_2_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_3_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_4_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_5_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_6_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_7_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareWidescreen_8_viewersAction);
  this->ViewLayoutCompareWidescreenAction->setMenu(compareMenu);
  QObject::connect(compareMenu, SIGNAL(triggered(QAction*)),
                   q, SLOT(onLayoutCompareWidescreenActionTriggered(QAction*)));
  compareMenu->setStyleSheet("QMenu::item{color: #FFFFFF;}QMenu::item:selected{background: #6987d5;}");

  // ... and for the grid version of the compare views
  compareMenu = new QMenu(qSlicerMainWindow::tr("Select number of viewers..."), mainWindow);
  compareMenu->setObjectName("CompareMenuGrid");
  compareMenu->addAction(this->ViewLayoutCompareGrid_2x2_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareGrid_3x3_viewersAction);
  compareMenu->addAction(this->ViewLayoutCompareGrid_4x4_viewersAction);
  this->ViewLayoutCompareGridAction->setMenu(compareMenu);
  QObject::connect(compareMenu, SIGNAL(triggered(QAction*)),
                   q, SLOT(onLayoutCompareGridActionTriggered(QAction*)));
  compareMenu->setStyleSheet("QMenu::item{color: #FFFFFF;}QMenu::item:selected{background: #6987d5;}");

  // Capture tool bar needs to listen to the layout manager
  //QObject::connect(this->LayoutManager,
  //                 SIGNAL(activeMRMLThreeDViewNodeChanged(vtkMRMLViewNode*)),
  //                 this->CaptureToolBar,
  //                 SLOT(setActiveMRMLThreeDViewNode(vtkMRMLViewNode*)));
  //this->CaptureToolBar->setActiveMRMLThreeDViewNode(
  //    this->LayoutManager->activeMRMLThreeDViewNode());

  // Authorize Drops action from outside
  q->setAcceptDrops(true);

  //----------------------------------------------------------------------------
  // View Toolbar
  //----------------------------------------------------------------------------
  // Populate the View ToolBar with all the layouts of the layout manager
  QToolButton* layoutButton = new QToolButton(mainWindow);
  layout->addWidget(layoutButton);

  this->LayoutMenu = new QMenu(mainWindow);
  this->menuConventionalQuantitative = new QMenu(mainWindow);
  this->menuFourUpQuantitative = new QMenu(mainWindow);
  this->menuOneUpQuantitative = new QMenu(mainWindow);
  this->menuThreeOverThreeQuantitative = new QMenu(mainWindow);
  
  this->LayoutMenu->setIcon(QIcon(":/Icons/LayoutChooseView.png"));
  this->menuConventionalQuantitative->setIcon(QIcon(":/Icons/LayoutConvetionalQuantitativeView.png"));
  this->menuFourUpQuantitative->setIcon(QIcon(":/Icons/LayoutFourUpQuantitativeView.png"));
  this->menuOneUpQuantitative->setIcon(QIcon(":/Icons/LayoutOneUpQuantitativeView.png"));
  this->menuThreeOverThreeQuantitative->setIcon(QIcon(":/Icons/Icons/LayoutWindow.png"));

  this->LayoutMenu->setTitle(QObject::tr("Layout"));
  this->menuConventionalQuantitative->setTitle(QObject::tr("Conventional Quantitative"));
  this->menuFourUpQuantitative->setTitle(QObject::tr("Four-Up Quantitative"));
  this->menuOneUpQuantitative->setTitle(QObject::tr("One-Up Quantitative"));
  this->menuThreeOverThreeQuantitative->setTitle(QObject::tr("Three over three Quantitative"));

  this->LayoutMenu->setStyleSheet("QMenu::item{color: #FFFFFF;}QMenu::item:selected{background: #6987d5;}");
  this->menuConventionalQuantitative->setStyleSheet("QMenu::item{color: #FFFFFF;}QMenu::item:selected{background: #6987d5;}");
  this->menuFourUpQuantitative->setStyleSheet("QMenu::item{color: #FFFFFF;}QMenu::item:selected{background: #6987d5;}");
  this->menuOneUpQuantitative->setStyleSheet("QMenu::item{color: #FFFFFF;}QMenu::item:selected{background: #6987d5;}");
  this->menuThreeOverThreeQuantitative->setStyleSheet("QMenu::item{color: #FFFFFF;}QMenu::item:selected{background: #6987d5;}");


  layoutButton->setText(qSlicerMainWindow::tr("Layout"));
  layoutButton->setMenu(this->LayoutMenu);
  layoutButton->setPopupMode(QToolButton::InstantPopup);
  
  layoutButton->setDefaultAction(this->ViewLayoutFourUpAction);
  
  QObject::connect(this->LayoutMenu, SIGNAL(triggered(QAction*)),
                   layoutButton, SLOT(setDefaultAction(QAction*)));
  QObject::connect(this->LayoutMenu, SIGNAL(triggered(QAction*)),
                   q, SLOT(onLayoutActionTriggered(QAction*)));
  

  QObject::connect(this->menuConventionalQuantitative, SIGNAL(triggered(QAction*)),
                   layoutButton, SLOT(setDefaultAction(QAction*)));
  QObject::connect(this->menuConventionalQuantitative, SIGNAL(triggered(QAction*)),
                   q, SLOT(onLayoutActionTriggered(QAction*)));
  
  QObject::connect(this->menuFourUpQuantitative, SIGNAL(triggered(QAction*)),
                   layoutButton, SLOT(setDefaultAction(QAction*)));
  QObject::connect(this->menuFourUpQuantitative, SIGNAL(triggered(QAction*)),
                   q, SLOT(onLayoutActionTriggered(QAction*)));
  
  QObject::connect(this->menuOneUpQuantitative, SIGNAL(triggered(QAction*)),
                   layoutButton, SLOT(setDefaultAction(QAction*)));
  QObject::connect(this->menuOneUpQuantitative, SIGNAL(triggered(QAction*)),
                   q, SLOT(onLayoutActionTriggered(QAction*)));
  
  QObject::connect(this->menuThreeOverThreeQuantitative, SIGNAL(triggered(QAction*)),
                   layoutButton, SLOT(setDefaultAction(QAction*)));
  QObject::connect(this->menuThreeOverThreeQuantitative, SIGNAL(triggered(QAction*)),
                   q, SLOT(onLayoutActionTriggered(QAction*)));
  

  //this->ViewToolBar->addWidget(layoutButton);
  //QObject::connect(this->ViewToolBar,
  //                SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
  //                layoutButton, SLOT(setToolButtonStyle(Qt::ToolButtonStyle)));

  //----------------------------------------------------------------------------
  // Viewers Toolbar
  //----------------------------------------------------------------------------
  // Viewers toolBar should listen the MRML scene
  this->ViewersToolBar = new qSlicerViewersToolBar("ViewersToolBarS", mainWindow);
  this->ViewersToolBar->setObjectName(QString::fromUtf8("ViewersToolBar"));
  this->ViewersToolBar->setAllowedAreas(Qt::NoToolBarArea);
  this->ViewersToolBar->setOrientation(Qt::Horizontal);
  this->ViewersToolBar->setApplicationLogic(
    qSlicerApplication::application()->applicationLogic());
  this->ViewersToolBar->setMRMLScene(qSlicerApplication::application()->mrmlScene());
  this->ViewersToolBar->setToolTip("联动");
  QObject::connect(qSlicerApplication::application(),
                   SIGNAL(mrmlSceneChanged(vtkMRMLScene*)),
                   this->ViewersToolBar,
                   SLOT(setMRMLScene(vtkMRMLScene*)));
  layout->addWidget(this->ViewersToolBar);



  //测量
  QToolBar* markbtn = new QToolBar(mainWindow);
  auto markbtnAction = new QAction(markbtn);
  markbtnAction->setObjectName("markbtn");
  markbtnAction->setIcon(QIcon(":/Images/marker.png"));
  markbtnAction->setCheckable(true);
  markbtn->addAction(markbtnAction);
 /* QLabel* labelmark = new QLabel;
  labelmark->setFixedWidth(4);
  markbtn->addWidget(labelmark);*/
  layout->addWidget(markbtn);
  markbtn->setToolTip("测量");
  QObject::connect(markbtn, SIGNAL(actionTriggered(QAction*)),
      q, SLOT(on_markupsButton_clicked(QAction*)));
  
  //裁剪
  QToolBar* scissorsBtn = new QToolBar(mainWindow);
  auto scissorsAction = new QAction(scissorsBtn);
  scissorsAction->setObjectName("srceenShotBnt");
  scissorsAction->setIcon(QIcon(":/Images/scissors.png"));
  scissorsAction->setCheckable(true);
  scissorsBtn->addAction(scissorsAction);
  
  scissorsBtn->setToolTip("裁剪");
  layout->addWidget(scissorsBtn);
  /* QObject::connect(srceenShot, &QToolBar::actionTriggered, [&]() {
       qSlicerApplication::application()->ioManager()->openScreenshotDialog();
       });*/
  QObject::connect(scissorsBtn, SIGNAL(actionTriggered(QAction*)),
      q, SLOT(on_scissorsButton_clicked(QAction*)));


  //截图
  QToolBar* srceenShot = new QToolBar(mainWindow);
  auto srceenShotAction = new QAction(srceenShot);
  srceenShotAction->setObjectName("srceenShotBnt");
  srceenShotAction->setIcon(QIcon(":/Images/screenshot.png"));
  srceenShotAction->setCheckable(true);
  srceenShot->addAction(srceenShotAction);
  /*QLabel* label2 = new QLabel;
  label2->setFixedWidth(4);
  srceenShot->addWidget(label2);*/
  srceenShot->setToolTip("截图");
  layout->addWidget(srceenShot);
 /* QObject::connect(srceenShot, &QToolBar::actionTriggered, [&]() {
      qSlicerApplication::application()->ioManager()->openScreenshotDialog();
      });*/
  QObject::connect(srceenShot, SIGNAL(actionTriggered(QAction*)),
      q, SLOT(on_srceenshotButton_clicked(QAction*)));


  QFrame* line = new QFrame();
  line->setFrameShape(QFrame::VLine);
  line->setFrameShadow(QFrame::Plain);
  line->setFixedHeight(19);
  line->setStyleSheet("color:#272739");
  line->raise();
  line->setFocusPolicy(Qt::NoFocus);
  layout->addWidget(line);

  //帮助文档
  QToolBar* helpDoc = new QToolBar(mainWindow);
  auto helpDocAction = new QAction(helpDoc);
  helpDocAction->setObjectName("srceenShotBnt");
  helpDocAction->setIcon(QIcon(":/Images/HelpDoc.png"));
  helpDocAction->setCheckable(true);
  helpDoc->addAction(helpDocAction);
 /* QLabel* labelHelp = new QLabel;
  labelHelp->setFixedWidth(4);
  helpDoc->addWidget(labelHelp);*/
  helpDoc->setToolTip("帮助文档");
  layout->addWidget(helpDoc);
  /* QObject::connect(srceenShot, &QToolBar::actionTriggered, [&]() {
       qSlicerApplication::application()->ioManager()->openScreenshotDialog();
       });*/
  QObject::connect(helpDoc, SIGNAL(actionTriggered(QAction*)),
      q, SLOT(on_HelpDocButton_clicked(QAction*)));


  //----------------------------------------------------------------------------
  // Undo/Redo Toolbar
  //----------------------------------------------------------------------------
  // Listen to the scene to enable/disable the undo/redo toolbuttons
  //q->qvtkConnect(qSlicerApplication::application()->mrmlScene(), vtkCommand::ModifiedEvent,
  //               q, SLOT(onMRMLSceneModified(vtkObject*)));
  //q->onMRMLSceneModified(qSlicerApplication::application()->mrmlScene());

  //----------------------------------------------------------------------------
  // Icons in the menu
  //----------------------------------------------------------------------------
  // Customize QAction icons with standard pixmaps

  //this->CutAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  //this->CopyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  //this->PasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  //
  //setThemeIcon(this->FileExitAction, "application-exit");
  //setThemeIcon(this->EditUndoAction, "edit-undo");
  //setThemeIcon(this->EditRedoAction, "edit-redo");
  //setThemeIcon(this->CutAction, "edit-cut");
  //setThemeIcon(this->CopyAction, "edit-copy");
  //setThemeIcon(this->PasteAction, "edit-paste");
  //setThemeIcon(this->EditApplicationSettingsAction, "preferences-system");
  //setThemeIcon(this->ModuleHomeAction, "go-home");

  //----------------------------------------------------------------------------
  // Error log widget
  //----------------------------------------------------------------------------
  this->ErrorLogWidget = new ctkErrorLogWidget;
  this->ErrorLogWidget->setErrorLogModel(
    qSlicerApplication::application()->errorLogModel());

  //----------------------------------------------------------------------------
  // Python console
  //----------------------------------------------------------------------------
#ifdef Slicer_USE_PYTHONQT
  if (q->pythonConsole())
    {
    if (QSettings().value("Python/DockableWindow").toBool())
      {
      this->PythonConsoleDockWidget = new QDockWidget(qSlicerMainWindow::tr("Python Interactor"));
      this->PythonConsoleDockWidget->setObjectName("PythonConsoleDockWidget");
      this->PythonConsoleDockWidget->setAllowedAreas(Qt::AllDockWidgetAreas);
      this->PythonConsoleDockWidget->setWidget(q->pythonConsole());
      this->PythonConsoleToggleViewAction = this->PythonConsoleDockWidget->toggleViewAction();
      // Set default state
      q->addDockWidget(Qt::BottomDockWidgetArea, this->PythonConsoleDockWidget);
      this->PythonConsoleDockWidget->hide();
      }
    else
      {
      ctkPythonConsole* pythonConsole = q->pythonConsole();
      pythonConsole->setWindowTitle("Slicer Python Interactor");
      pythonConsole->resize(600, 280);
      pythonConsole->hide();
      //this->PythonConsoleToggleViewAction = new QAction("", this->ViewMenu);
      this->PythonConsoleToggleViewAction->setCheckable(true);
      }
    q->pythonConsole()->setScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->updatePythonConsolePalette();
    QObject::connect(q->pythonConsole(), SIGNAL(aboutToExecute(const QString&)),
      q, SLOT(onPythonConsoleUserInput(const QString&)));
    // Set up show/hide action
    this->PythonConsoleToggleViewAction->setText(qSlicerMainWindow::tr("&Python Interactor"));
    //this->PythonConsoleToggleViewAction->setToolTip(qSlicerMainWindow::tr(
    //  "Show Python Interactor window for controlling the application's data, user interface, and internals"));
    this->PythonConsoleToggleViewAction->setShortcut(QKeySequence("Ctrl+3"));
    QObject::connect(this->PythonConsoleToggleViewAction, SIGNAL(toggled(bool)),
      q, SLOT(onPythonConsoleToggled(bool)));
    //this->ViewMenu->insertAction(this->WindowToolBarsMenu->menuAction(), this->PythonConsoleToggleViewAction);
    //this->PythonConsoleToggleViewAction->setIcon(QIcon(":/python-icon.png"));
    //this->DialogToolBar->addAction(this->PythonConsoleToggleViewAction);
    }
  else
    {
    qWarning("qSlicerMainWindowPrivate::setupUi: Failed to create Python console");
    }
#endif
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::updatePythonConsolePalette()
{
  Q_Q(qSlicerMainWindow);
#ifdef Slicer_USE_PYTHONQT
  ctkPythonConsole* pythonConsole = q->pythonConsole();
  if (!pythonConsole)
    {
    return;
    }
  QPalette palette = qSlicerApplication::application()->palette();

  // pythonConsole->setBackgroundColor is not called, because by default
  // the background color of the current palette is used, which is good.

  // Color of the >> prompt. Blue in both bright and dark styles (brighter in dark style).
  pythonConsole->setPromptColor(palette.color(QPalette::Highlight));

  // Color of text that user types in the console. Blue in both bright and dark styles (brighter in dark style).
  pythonConsole->setCommandTextColor(palette.color(QPalette::Link));

  // Color of output of commands. Black in bright style, white in dark style.
  pythonConsole->setOutputTextColor(palette.color(QPalette::WindowText));

  // Color of error messages. Red in both bright and dark styles.
  pythonConsole->setErrorTextColor(palette.color(QPalette::BrightText));

  // Color of welcome message (printed when the terminal is reset)
  // and "user input" (this does not seem to be used in Slicer).
  // Gray in both bright and dark styles.
  pythonConsole->setStdinTextColor(palette.color(QPalette::Disabled, QPalette::WindowText));   // gray
  pythonConsole->setWelcomeTextColor(palette.color(QPalette::Disabled, QPalette::WindowText)); // gray
#endif
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::readSettings()
{
  Q_Q(qSlicerMainWindow);
  //QSettings settings;
  //settings.beginGroup("MainWindow");
  //q->setToolButtonStyle(settings.value("ShowToolButtonText").toBool()
  //                      ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
  //bool restore = settings.value("RestoreGeometry", false).toBool();
  //if (restore)
  //  {
  //  q->restoreGeometry(settings.value("geometry").toByteArray());
  //  q->restoreState(settings.value("windowState").toByteArray());
  //  this->LayoutManager->setLayout(settings.value("layout").toInt());
  //  }
  //settings.endGroup();
  //this->FavoriteModules << settings.value("Modules/FavoriteModules").toStringList();
  //
  //foreach(const qSlicerIO::IOProperties& fileProperty, Self::readRecentlyLoadedFiles())
  //  {
  //  this->RecentlyLoadedFileProperties.enqueue(fileProperty);
  //  }
  //this->filterRecentlyLoadedFileProperties();
  //this->setupRecentlyLoadedMenu(this->RecentlyLoadedFileProperties);
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::writeSettings()
{
  Q_Q(qSlicerMainWindow);
  QSettings settings;
  settings.beginGroup("MainWindow");
  bool restore = settings.value("RestoreGeometry", false).toBool();
  if (restore)
    {
    settings.setValue("geometry", q->saveGeometry());
    settings.setValue("windowState", q->saveState());
    settings.setValue("layout", this->LayoutManager->layout());
    }
  settings.endGroup();
  QString strPath = settings.fileName();
  Self::writeRecentlyLoadedFiles(this->RecentlyLoadedFileProperties);

  //cxc 2020-9-18 每次退出清理ini文件
  QFileInfo FileInfo(strPath);
  if(FileInfo.isFile())
  {
      bool state = QFile::remove(strPath);
      qDebug() << state;
  }
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::setupRecentlyLoadedMenu(const QList<qSlicerIO::IOProperties>& fileProperties)
{
  Q_Q(qSlicerMainWindow);

  //this->RecentlyLoadedMenu->setEnabled(fileProperties.size() > 0);
  //this->RecentlyLoadedMenu->clear();

  QListIterator<qSlicerIO::IOProperties> iterator(fileProperties);
  iterator.toBack();
  while (iterator.hasPrevious())
    {
    qSlicerIO::IOProperties filePropertie = iterator.previous();
    QString fileName = filePropertie.value("fileName").toString();
    if (fileName.isEmpty())
      {
      continue;
      }
    //QAction * action = this->RecentlyLoadedMenu->addAction(
    //  fileName, q, SLOT(onFileRecentLoadedActionTriggered()));
    //action->setProperty("fileParameters", filePropertie);
    //action->setEnabled(QFile::exists(fileName));
    }

  // Add separator and clear action
  //this->RecentlyLoadedMenu->addSeparator();
  //QAction * clearAction = this->RecentlyLoadedMenu->addAction(
  //  "Clear History", q, SLOT(onFileRecentLoadedActionTriggered()));
  //clearAction->setProperty("clearMenu", QVariant(true));
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::filterRecentlyLoadedFileProperties()
{
  int numberOfFilesToKeep = QSettings().value("RecentlyLoadedFiles/NumberToKeep").toInt();

  // Remove extra element
  while (this->RecentlyLoadedFileProperties.size() > numberOfFilesToKeep)
    {
    this->RecentlyLoadedFileProperties.dequeue();
    }
}

//-----------------------------------------------------------------------------
QList<qSlicerIO::IOProperties> qSlicerMainWindowPrivate::readRecentlyLoadedFiles()
{
  QList<qSlicerIO::IOProperties> fileProperties;

  QSettings settings;
  int size = settings.beginReadArray("RecentlyLoadedFiles/RecentFiles");
  for(int i = 0; i < size; ++i)
    {
    settings.setArrayIndex(i);
    QVariant file = settings.value("file");
    fileProperties << file.toMap();
    }
  settings.endArray();

  return fileProperties;
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::writeRecentlyLoadedFiles(const QList<qSlicerIO::IOProperties>& fileProperties)
{
  QSettings settings;
  settings.beginWriteArray("RecentlyLoadedFiles/RecentFiles", fileProperties.size());
  for (int i = 0; i < fileProperties.size(); ++i)
    {
    settings.setArrayIndex(i);
    settings.setValue("file", fileProperties.at(i));
    }
  settings.endArray();
}

//-----------------------------------------------------------------------------
bool qSlicerMainWindowPrivate::confirmCloseApplication()
{
  Q_Q(qSlicerMainWindow);
  vtkMRMLScene* scene = qSlicerApplication::application()->mrmlScene();
  QString question = qSlicerMainWindow::tr("退出软件");
  if (scene->GetStorableNodesModifiedSinceRead())
    {
    //question = qSlicerMainWindow::tr("一些数据已被修改。是否要在退出前保存它们？");
    }
  else if (scene->GetModifiedSinceRead())
    {
    //question = qSlicerMainWindow::tr("场景已被修改。退出前是否保存？");
    }
  bool close = false;
  if (!question.isEmpty())
  {
      QMessageBox* messageBox = new QMessageBox(QMessageBox::Warning, qSlicerMainWindow::tr("退出"), question, QMessageBox::NoButton);
      //QAbstractButton* saveButton =
         //messageBox->addButton(qSlicerMainWindow::tr("保存"), QMessageBox::ActionRole);
      QAbstractButton* exitButton =
         messageBox->addButton(qSlicerMainWindow::tr("退出"), QMessageBox::ActionRole);
      QAbstractButton* cancelButton =
          messageBox->addButton(qSlicerMainWindow::tr("取消退出"), QMessageBox::ActionRole);
      Q_UNUSED(cancelButton);
      messageBox->exec();
      //  if (messageBox->clickedButton() == saveButton)
      //    {
      //    // \todo Check if the save data dialog was "applied" and close the
      //    // app in that case
      //      qSlicerApplication::application()->ioManager()->openSaveDataDialog();
      //    }
      //  else 
       if (messageBox->clickedButton() == exitButton)
          {
          close = true;
          messageBox->deleteLater();
       }
       else if (messageBox->clickedButton() == exitButton) {
           messageBox->deleteLater();
       }
      //  }
      //else
      //  {
      //  close = ctkMessageBox::confirmExit("MainWindow/DontConfirmExit", q);
      //  }
  }
   
  return close;
}

//-----------------------------------------------------------------------------
bool qSlicerMainWindowPrivate::confirmCloseScene()
{
  Q_Q(qSlicerMainWindow);
  vtkMRMLScene* scene = qSlicerApplication::application()->mrmlScene();
  QString question;
  if (scene->GetStorableNodesModifiedSinceRead())
    {
    question = qSlicerMainWindow::tr("一些数据已被修改。在关闭场景之前是否要保存它们？");
    }
  else if (scene->GetModifiedSinceRead())
    {
    question = qSlicerMainWindow::tr("场景已被修改。在关闭场景之前是否要保存它？");
    }
  else
    {
    // no unsaved changes, no need to ask confirmation
    return true;
    }

  ctkMessageBox* confirmCloseMsgBox = new ctkMessageBox(q);
  confirmCloseMsgBox->setAttribute(Qt::WA_DeleteOnClose);
  confirmCloseMsgBox->setWindowTitle(qSlicerMainWindow::tr("在关闭场景之前保存？"));
  confirmCloseMsgBox->setText(question);

  // Use AcceptRole&RejectRole instead of Save&Discard because we would
  // like discard changes to be the default behavior.
  confirmCloseMsgBox->addButton(qSlicerMainWindow::tr("关闭场景（放弃修改）"), QMessageBox::AcceptRole);
  confirmCloseMsgBox->addButton(qSlicerMainWindow::tr("保存场景"), QMessageBox::RejectRole);
  confirmCloseMsgBox->addButton(QMessageBox::Cancel);
  confirmCloseMsgBox->setButtonText(QMessageBox::Cancel, QString("取 消"));

  confirmCloseMsgBox->setDontShowAgainVisible(true);
  confirmCloseMsgBox->setDontShowAgainSettingsKey("MainWindow/DontConfirmSceneClose");
  confirmCloseMsgBox->setIcon(QMessageBox::Question);
  int resultCode = confirmCloseMsgBox->exec();
  if (resultCode == QMessageBox::Cancel)
    {
    return false;
    }
  if (resultCode != QMessageBox::AcceptRole)
    {
    if (!qSlicerApplication::application()->ioManager()->openSaveDataDialog())
      {
      return false;
      }
    }
  return true;
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::setupStatusBar()
{
  Q_Q(qSlicerMainWindow);
  //this->ErrorLogToolButton = new QToolButton();
  //this->ErrorLogToolButton->setDefaultAction(this->WindowErrorLogAction);
  //q->statusBar()->addPermanentWidget(this->ErrorLogToolButton);

  //QObject::connect(qSlicerApplication::application()->errorLogModel(),
  //                 SIGNAL(entryAdded(ctkErrorLogLevel::LogLevel)),
  //                 q, SLOT(onWarningsOrErrorsOccurred(ctkErrorLogLevel::LogLevel)));
}

//-----------------------------------------------------------------------------
void qSlicerMainWindowPrivate::setErrorLogIconHighlighted(bool highlighted)
{
  Q_Q(qSlicerMainWindow);
  QIcon defaultIcon = q->style()->standardIcon(QStyle::SP_MessageBoxCritical);
  QIcon icon = defaultIcon;
  if(!highlighted)
    {
    QIcon disabledIcon;
    disabledIcon.addPixmap(
          defaultIcon.pixmap(QSize(32, 32), QIcon::Disabled, QIcon::On), QIcon::Active, QIcon::On);
    icon = disabledIcon;
    }
  //this->WindowErrorLogAction->setIcon(icon);
}

//-----------------------------------------------------------------------------
// qSlicerMainWindow methods

//-----------------------------------------------------------------------------
qSlicerMainWindow::qSlicerMainWindow(QWidget *_parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerMainWindowPrivate(*this))
{
  Q_D(qSlicerMainWindow);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerMainWindow::qSlicerMainWindow(qSlicerMainWindowPrivate* pimpl,
                                     QWidget* windowParent)
  : Superclass(windowParent)
  , d_ptr(pimpl)
{
  // init() is called by derived class.
}

/*!
 * \brief 自定义菜单栏
 */
void qSlicerMainWindow::setSlicerWindowUi()
{
    Q_D(qSlicerMainWindow);

    ////菜单文件
    //QMenu* fileMenu = new QMenu(this);
    //QAction* actOpenFile = new QAction(QString("  打开文件"));
    //QAction* actSaveFile = new QAction(QString("  保存文件"));
    //QAction* actUploadFile = new QAction(QString("  上传文件"));
    //QAction* actSubmitFile = new QAction(QString("  提交文件"));
    //QAction* actCloseScene = new QAction(QString("  关闭本次任务"));
    //QAction* actExit = new QAction(QString("  退出"));
    //connect(actOpenFile, SIGNAL(triggered()), this, SLOT(on_FileAddDataAction_triggered()));
    //connect(actSaveFile, SIGNAL(triggered()), this, SLOT(on_FileSaveSceneAction_triggered()));
    //connect(actCloseScene, SIGNAL(triggered()), this, SLOT(on_FileCloseSceneAction_triggered()));
    //connect(actExit, SIGNAL(triggered()), this, SLOT(on_FileExitAction_triggered()));
    //connect(actUploadFile, SIGNAL(triggered()), this, SLOT(slotUploadFile()));
    //connect(actSubmitFile, SIGNAL(triggered()), this, SLOT(slotSubmitFile()));
    //
    //fileMenu->addAction(actOpenFile);
    //fileMenu->addAction(actSaveFile);
    //fileMenu->addAction(actUploadFile);
    //fileMenu->addAction(actSubmitFile);
    //fileMenu->addAction(actCloseScene);
    //fileMenu->addAction(actExit);

    //fileMenu->setStyleSheet(
    //    "QMenu{"\
    //    "width: 150;"\
    //    "color: #FFFFFF;"\
    //    "background: #3c3c5a;}"\
    //    "QMenu::item{"\
    //    "font: 10pt 'SimHei';"\
    //    "color: #FFFFFF;"\
    //    "width: 150;"\
    //    "height:24;}"\
    //    "QMenu::item:selected{"\
    //    "background: #6987d5;}"\
    //);
    //d->menuFilePushButton->setMenu(fileMenu);

    //菜单文件
    QMenu* DataGroupMenu = new QMenu(this);
    QAction* actData = new QAction(QString("  全局数据管理"));
    //QAction* actDICOM = new QAction(QString("  Dicom管理"));
    QAction* actSegmentions = new QAction(QString("  分割管理"));
    QAction* actModul = new QAction(QString("  模型管理"));
    connect(actData, SIGNAL(triggered()), this, SLOT(on_dataButton_clicked()));
    //connect(actDICOM, SIGNAL(triggered()), this, SLOT(on_DICOMButton_clicked()));
    connect(actSegmentions, SIGNAL(triggered()), this, SLOT(on_segmentationsButton_clicked()));
    connect(actModul, SIGNAL(triggered()), this, SLOT(on_modelsButton_clicked()));

    DataGroupMenu->addAction(actData);
    //DataGroupMenu->addAction(actDICOM);
    DataGroupMenu->addAction(actSegmentions);
    DataGroupMenu->addAction(actModul);

    DataGroupMenu->setStyleSheet(
        "QMenu{"\
        "width: 150;"\
        "color: #FFFFFF;"\
        "background: #3c3c5a;}"\
        "QMenu::item{"\
        "font: 10pt 'SimHei';"\
        "color: #FFFFFF;"\
        "width: 150;"\
        "height:24;}"\
        "QMenu::item:selected{"\
        "background: #6987d5;}"\
    );
    //d->dataGroupButton->setMenu(DataGroupMenu);
}

void qSlicerMainWindow::initWidget()
{
    Q_D(qSlicerMainWindow);

    d->m_reduction = false;
    setWidgetBorderless(this);
    //this->setAttribute(Qt::WA_DeleteOnClose, true);

    QImage image(":/Images/LOGO.png");
    d->labelIconImage->setPixmap(QPixmap::fromImage(image));

    d->centralwidget->setFocusPolicy(Qt::NoFocus);
    d->PanelDockWidget->installEventFilter(this);
    d->WindowTitleWidget->installEventFilter(this);  
    d->LogoLabel->setVisible(false);
    d->fileLoadDataPushButton->setVisible(false);
    d->fileSaveScenePushButton->setVisible(false);
    d->DoubleclickPushButton->setVisible(false);
    d->annotationsButton->setVisible(false);
    d->volumesButton->setVisible(false);
    d->dataButton->setVisible(false);
    d->DICOMButton->setVisible(false);
    d->segmentationsButton->setVisible(false);
    d->modelsButton->setVisible(false);
    d->previousPushButton->setVisible(false);
    d->nextPushButton->setVisible(false);

    QButtonGroup * buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);
    buttonGroup->addButton(d->fileLoadDataPushButton);
    buttonGroup->addButton(d->fileSaveScenePushButton);
    buttonGroup->addButton(d->annotationsButton);
    buttonGroup->addButton(d->slicerElastixButton);
    buttonGroup->addButton(d->transformsButton);
    buttonGroup->addButton(d->gradientADButton);
    buttonGroup->addButton(d->volumeRenderingButton);
    buttonGroup->addButton(d->segmentEditorButton);
    buttonGroup->addButton(d->markupsButton);
    buttonGroup->addButton(d->dataButton);
    buttonGroup->addButton(d->DICOMButton);
    buttonGroup->addButton(d->decimationButton);
    buttonGroup->addButton(d->modelsButton);
    buttonGroup->addButton(d->segmentationsButton);
    buttonGroup->addButton(d->volumesButton);
    buttonGroup->addButton(d->interactiveLobeSegmentationButton);

    //去除PanelDockWidget的标题栏
    QWidget* dockOldTitleBar = d->PanelDockWidget->titleBarWidget();
    QWidget* dockNewTitleBar = new QWidget();
    d->PanelDockWidget->setTitleBarWidget(dockNewTitleBar);
    delete dockOldTitleBar;
    dockOldTitleBar = nullptr;

    connect(d->fileLoadDataPushButton, SIGNAL(clicked()), this, SLOT(on_FileLoadDataAction_triggered()));
    connect(d->fileSaveScenePushButton, SIGNAL(clicked()), this, SLOT(on_FileSaveSceneAction_triggered()));
    connect(d->DoubleclickPushButton, &DoubleClickedButton::signalsDoubleClicked, this, &qSlicerMainWindow::slotDoubleClickedPushButton);
}

void qSlicerMainWindow::setWidgetBorderless(const QWidget* widget)
{
    setWindowFlags(Qt::FramelessWindowHint);
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    DWORD style = GetWindowLong(hwnd, GWL_STYLE);
    SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);
#endif
}

bool qSlicerMainWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    Q_D(qSlicerMainWindow);
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
        setmaxPushButtonState(true);
    }
    else
    {
        setmaxPushButtonState(false);
    }

    if (IsIconic(msg->hwnd))
    {
        RecordInformationTool::getInstance()->setWindowVisible(true);
    }
    else
    {
        RecordInformationTool::getInstance()->setWindowVisible(false);
        RecordInformationTool::getInstance()->setWindowLayoutChange(false);
    }

#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}

void qSlicerMainWindow::mousePressEvent(QMouseEvent* event)
{
    if (ReleaseCapture())
    {
        SendMessage(HWND(this->winId()), WM_SYSCOMMAND, SC_MOVE + HTCAPTION, 0);
        SendMessage(HWND(this->winId()), WM_LBUTTONUP, 0, 0);
    }
    //event->ignore();
}

void qSlicerMainWindow::setmaxPushButtonState(bool state)
{
    Q_D(qSlicerMainWindow);
    //检测窗口是否最大化
    if (state)
    {
        d->m_reduction = true;
        d->maxPushButton->setStyleSheet("QPushButton{"\
            "width : 30;"\
            "height: 30;"\
            "border-image: url(:/Images/reductionWindow.png);}"\
            "QPushButton:hover{"\
            "width : 30;"\
            "height: 30;"\
            "background-color: rgb(60, 60, 90);"\
            "border-image: url(:/Images/reductionWindow1.png);}"\
            "QPushButton:pressed {"\
            "background - color: rgb(60, 60, 90);}");
    }
    else
    {
        d->m_reduction = false;
        d->maxPushButton->setStyleSheet("QPushButton{"\
            "width : 30;"\
            "height: 30;"\
            "border-image: url(:/Images/maxWindow.png);}"\
            "QPushButton:hover{"\
            "width : 30;"\
            "height: 30;"\
            "background-color: rgb(60, 60, 90);"\
            "border-image: url(:/Images/maxWindow1.png);}"\
            "QPushButton:pressed {"\
            "background - color: rgb(60, 60, 90);}");
    }
}

//-----------------------------------------------------------------------------
qSlicerMainWindow::~qSlicerMainWindow()
{
  Q_D(qSlicerMainWindow);
  // When quitting the application, the modules are unloaded (~qSlicerCoreApplication)
  // in particular the Colors module which deletes vtkMRMLColorLogic and removes
  // all the color nodes from the scene. If a volume was loaded in the views,
  // it would then try to render it with no color node and generate warnings.
  // There is no need to render anything so remove the volumes from the views.
  // It is maybe not the best place to do that but I couldn't think of anywhere
  // else.
  vtkCollection* sliceLogics = d->LayoutManager ? d->LayoutManager->mrmlSliceLogics() : nullptr;
  for (int i = 0; i < sliceLogics->GetNumberOfItems(); ++i)
    {
    vtkMRMLSliceLogic* sliceLogic = vtkMRMLSliceLogic::SafeDownCast(sliceLogics->GetItemAsObject(i));
    if (!sliceLogic)
      {
      continue;
      }
    sliceLogic->GetSliceCompositeNode()->SetReferenceBackgroundVolumeID(nullptr);
    sliceLogic->GetSliceCompositeNode()->SetReferenceForegroundVolumeID(nullptr);
    sliceLogic->GetSliceCompositeNode()->SetReferenceLabelVolumeID(nullptr);
    }
}

//-----------------------------------------------------------------------------
qSlicerModuleSelectorToolBar* qSlicerMainWindow::moduleSelector()const
{
  Q_D(const qSlicerMainWindow);
  return d->ModuleSelectorToolBar;
}

#ifdef Slicer_USE_PYTHONQT
//---------------------------------------------------------------------------
ctkPythonConsole* qSlicerMainWindow::pythonConsole()const
{
  return qSlicerCoreApplication::application()->pythonConsole();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onPythonConsoleUserInput(const QString& cmd)
{
  if (!cmd.isEmpty())
    {
    qDebug("Python console user input: %s", qPrintable(cmd));
    }
}
#endif

//---------------------------------------------------------------------------
ctkErrorLogWidget* qSlicerMainWindow::errorLogWidget()const
{
  Q_D(const qSlicerMainWindow);
  return d->ErrorLogWidget;
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileAddDataAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openAddDataDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileLoadDataAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openAddDataDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileImportSceneAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openAddSceneDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileLoadSceneAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openLoadSceneDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileAddVolumeAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openAddVolumesDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileAddTransformAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openAddTransformDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileSaveSceneAction_triggered()
{
  qSlicerApplication::application()->ioManager()->openSaveDataDialog();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileExitAction_triggered()
{
  this->close();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_SDBSaveToDirectoryAction_triggered()
{
  // Q_D(qSlicerMainWindow);
  // open a file dialog to let the user choose where to save
  QString tempDir = qSlicerCoreApplication::application()->temporaryPath();
  QString saveDirName = QFileDialog::getExistingDirectory(
    this, tr("Slicer Data Bundle Directory (Select Empty Directory)"),
    tempDir, QFileDialog::ShowDirsOnly);
  if (saveDirName.isEmpty())
    {
    std::cout << "No directory name chosen!" << std::endl;
    return;
    }
  // pass in a screen shot
  QWidget* widget = qSlicerApplication::application()->layoutManager()->viewport();
  QImage screenShot = ctk::grabVTKWidget(widget);
  qSlicerIO::IOProperties properties;
  properties["fileName"] = saveDirName;
  properties["screenShot"] = screenShot;
  qSlicerCoreApplication::application()->coreIOManager()
    ->saveNodes(QString("SceneFile"), properties);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_SDBSaveToMRBAction_triggered()
{
  //
  // open a file dialog to let the user choose where to save
  // make sure it was selected and add a .mrb to it if needed
  //
  QString fileName = QFileDialog::getSaveFileName(
    this, tr("Save Data Bundle File"),
    "", tr("Medical Reality Bundle (*.mrb)"));

  if (fileName.isEmpty())
    {
    std::cout << "No directory name chosen!" << std::endl;
    return;
    }

  if ( !fileName.endsWith(".mrb") )
    {
    fileName += QString(".mrb");
    }
  qSlicerIO::IOProperties properties;
  properties["fileName"] = fileName;
  qSlicerCoreApplication::application()->coreIOManager()
    ->saveNodes(QString("SceneFile"), properties);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_FileCloseSceneAction_triggered()
{
  Q_D(qSlicerMainWindow);
  if (d->confirmCloseScene())
    {
    qSlicerCoreApplication::application()->mrmlScene()->Clear(false);
    // Make sure we don't remember the last scene's filename to prevent
    // accidentally overwriting the scene.
    qSlicerCoreApplication::application()->mrmlScene()->SetURL("");
    // Set default scene file format to .mrml
    qSlicerCoreIOManager* coreIOManager = qSlicerCoreApplication::application()->coreIOManager();
    coreIOManager->setDefaultSceneFileType("MRML Scene (.mrml)");
    }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_EditRecordMacroAction_triggered()
{
#ifdef Slicer_USE_QtTesting
  qSlicerApplication::application()->testingUtility()->recordTestsBySuffix(QString("xml"));
#endif
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_EditPlayMacroAction_triggered()
{
#ifdef Slicer_USE_QtTesting
  qSlicerApplication::application()->testingUtility()->openPlayerDialog();
#endif
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_EditUndoAction_triggered()
{
  qSlicerApplication::application()->mrmlScene()->Undo();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_EditRedoAction_triggered()
{
  qSlicerApplication::application()->mrmlScene()->Redo();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_ModuleHomeAction_triggered()
{
  this->setHomeModuleCurrent();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::setLayout(int layout)
{
  qSlicerApplication::application()->layoutManager()->setLayout(layout);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::setLayoutNumberOfCompareViewRows(int num)
{
  qSlicerApplication::application()->layoutManager()->setLayoutNumberOfCompareViewRows(num);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::setLayoutNumberOfCompareViewColumns(int num)
{
  qSlicerApplication::application()->layoutManager()->setLayoutNumberOfCompareViewColumns(num);
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::on_WindowErrorLogAction_triggered()
{
  Q_D(qSlicerMainWindow);
  d->ErrorLogWidget->show();
  d->ErrorLogWidget->activateWindow();
  d->ErrorLogWidget->raise();
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::onPythonConsoleToggled(bool toggled)
{
  Q_D(qSlicerMainWindow);
#ifdef Slicer_USE_PYTHONQT
  ctkPythonConsole* pythonConsole = this->pythonConsole();
  if (!pythonConsole)
    {
    qCritical() << Q_FUNC_INFO << " failed: python console is not available";
    return;
    }
  if (d->PythonConsoleDockWidget)
    {
    // Dockable Python console
    if (toggled)
      {
      d->PythonConsoleDockWidget->activateWindow();
      QTextEdit* textEditWidget = pythonConsole->findChild<QTextEdit*>();
      if (textEditWidget)
        {
        textEditWidget->setFocus();
        }
      }
    }
  else
    {
    // Independent Python console
    if (toggled)
      {
      pythonConsole->show();
      pythonConsole->activateWindow();
      pythonConsole->raise();
      }
    else
      {
      pythonConsole->hide();
      }
    }
#else
  Q_UNUSED(toggled);
#endif
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_WindowToolbarsResetToDefaultAction_triggered()
{
  Q_D(qSlicerMainWindow);
  this->restoreState(d->StartupState);
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::onFileRecentLoadedActionTriggered()
{
  Q_D(qSlicerMainWindow);

  QAction* loadRecentFileAction = qobject_cast<QAction*>(this->sender());
  Q_ASSERT(loadRecentFileAction);

  // Clear menu if it applies
  if (loadRecentFileAction->property("clearMenu").isValid())
    {
    d->RecentlyLoadedFileProperties.clear();
    d->setupRecentlyLoadedMenu(d->RecentlyLoadedFileProperties);
    return;
    }

  QVariant fileParameters = loadRecentFileAction->property("fileParameters");
  Q_ASSERT(fileParameters.isValid());

  qSlicerIO::IOProperties fileProperties = fileParameters.toMap();
  qSlicerIO::IOFileType fileType =
      static_cast<qSlicerIO::IOFileType>(
        fileProperties.find("fileType").value().toString());

  qSlicerApplication* app = qSlicerApplication::application();
  app->coreIOManager()->loadNodes(fileType, fileProperties);
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::closeEvent(QCloseEvent *event)
{
  Q_D(qSlicerMainWindow);

  // This is necessary because of a Qt bug on MacOS.
  // (https://bugreports.qt.io/browse/QTBUG-43344).
  // This flag prevents a second close event to be handled.
  if (d->IsClosing)
    {
    return;
    }
  d->IsClosing = true;

  if (d->confirmCloseApplication())
    {
    // Proceed with closing the application

    // Exit current module to leave it a chance to change the UI (e.g. layout)
    // before writing settings.
    d->ModuleSelectorToolBar->selectModule("");

    d->writeSettings();
    event->accept();

    QTimer::singleShot(0, qApp, SLOT(closeAllWindows()));
    }
  else
    {
    // Request is cancelled, application will not be closed
    event->ignore();
    d->IsClosing = false;
    }
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::showEvent(QShowEvent *event)
{
  Q_D(qSlicerMainWindow);
  this->Superclass::showEvent(event);
  if (!d->WindowInitialShowCompleted)
    {
    d->WindowInitialShowCompleted = true;
    this->disclaimer();
    this->pythonConsoleInitialDisplay();
#ifdef Slicer_BUILD_EXTENSIONMANAGER_SUPPORT
    // See https://issues.slicer.org/view.php?id=4641
    // qSlicerApplication::application()->gatherExtensionsHistoryInformationOnStartup();
#endif
    emit initialWindowShown();
    }
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::pythonConsoleInitialDisplay()
{
  Q_D(qSlicerMainWindow);
#ifdef Slicer_USE_PYTHONQT
  qSlicerApplication * app = qSlicerApplication::application();
  if (qSlicerCoreApplication::testAttribute(qSlicerCoreApplication::AA_DisablePython))
    {
    return;
    }
  if (app->commandOptions()->showPythonInteractor() && d->PythonConsoleDockWidget)
    {
    d->PythonConsoleDockWidget->show();
    }
#endif
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::disclaimer()
{
  qSlicerCoreApplication * app = qSlicerCoreApplication::application();
  if (app->testAttribute(qSlicerCoreApplication::AA_EnableTesting) ||
      !app->coreCommandOptions()->pythonCode().isEmpty() ||
      !app->coreCommandOptions()->pythonScript().isEmpty())
    {
    return;
    }

  QString message = QString(Slicer_DISCLAIMER_AT_STARTUP);
  if (message.isEmpty())
    {
    // No disclaimer message to show, skip the popup
    return;
    }

  // Replace "%1" in the text by the name and version of the application
  //message = message.arg(app->applicationName() + " " + app->applicationVersion());

  //登录进入显示的message
  //ctkMessageBox* disclaimerMessage = new ctkMessageBox(this);
  //disclaimerMessage->setAttribute( Qt::WA_DeleteOnClose, true);
  //disclaimerMessage->setText(message);
  //disclaimerMessage->setIcon(QMessageBox::Information);
  //disclaimerMessage->setDontShowAgainSettingsKey("MainWindow/DontShowDisclaimerMessage");
  //QTimer::singleShot(0, disclaimerMessage, SLOT(exec()));
}

//-----------------------------------------------------------------------------
void qSlicerMainWindow::setupMenuActions()
{
  Q_D(qSlicerMainWindow);

  //d->LayoutMenu->addAction(d->ViewLayoutConventionalAction);
  d->LayoutMenu->addAction(d->ViewLayoutConventionalWidescreenAction);
  //d->LayoutMenu->addMenu(d->menuConventionalQuantitative);
  //d->LayoutMenu->addAction(d->ViewLayoutFourUpAction);
  //d->LayoutMenu->addAction(d->ViewLayoutFourUpTableAction);
  //d->LayoutMenu->addMenu(d->menuFourUpQuantitative);
  //d->LayoutMenu->addAction(d->ViewLayoutDual3DAction);
  //d->LayoutMenu->addAction(d->ViewLayoutTriple3DAction);
  d->LayoutMenu->addAction(d->ViewLayoutOneUp3DAction);
  //d->LayoutMenu->addAction(d->ViewLayout3DTableAction);
  //d->LayoutMenu->addMenu(d->menuOneUpQuantitative);
  d->LayoutMenu->addAction(d->ViewLayoutOneUpRedSliceAction);
  d->LayoutMenu->addAction(d->ViewLayoutOneUpYellowSliceAction);
  d->LayoutMenu->addAction(d->ViewLayoutOneUpGreenSliceAction);
  //d->LayoutMenu->addAction(d->ViewLayoutTabbed3DAction);
  //d->LayoutMenu->addAction(d->ViewLayoutTabbedSliceAction);
  //d->LayoutMenu->addAction(d->ViewLayoutCompareAction);
  //d->LayoutMenu->addAction(d->ViewLayoutCompareWidescreenAction);
  //d->LayoutMenu->addAction(d->ViewLayoutCompareGridAction);
  //d->LayoutMenu->addAction(d->ViewLayoutThreeOverThreeAction);
  //d->LayoutMenu->addMenu(d->menuThreeOverThreeQuantitative);
  //d->LayoutMenu->addAction(d->ViewLayoutFourOverFourAction);
  //d->LayoutMenu->addAction(d->ViewLayoutTwoOverTwoAction);
  //d->LayoutMenu->addAction(d->ViewLayoutSideBySideAction);
  //d->LayoutMenu->addAction(d->ViewLayoutFourByThreeSliceAction);
  //d->LayoutMenu->addAction(d->ViewLayoutFourByTwoSliceAction);
  //d->LayoutMenu->addAction(d->ViewLayoutThreeByThreeSliceAction);

  d->menuConventionalQuantitative->addAction(d->ViewLayoutConventionalQuantitativeAction);
  d->menuConventionalQuantitative->addAction(d->ViewLayoutConventionalPlotAction);
  d->menuFourUpQuantitative->addAction(d->ViewLayoutFourUpQuantitativeAction);
  d->menuFourUpQuantitative->addAction(d->ViewLayoutFourUpPlotAction);
  d->menuFourUpQuantitative->addAction(d->ViewLayoutFourUpPlotTableAction);
  d->menuOneUpQuantitative->addAction(d->ViewLayoutOneUpQuantitativeAction);
  d->menuOneUpQuantitative->addAction(d->ViewLayoutOneUpPlotAction);
  d->menuThreeOverThreeQuantitative->addAction(d->ViewLayoutThreeOverThreeQuantitativeAction);
  d->menuThreeOverThreeQuantitative->addAction(d->ViewLayoutThreeOverThreePlotAction);

  d->ViewLayoutConventionalAction->setData(vtkMRMLLayoutNode::SlicerLayoutConventionalView);
  d->ViewLayoutConventionalWidescreenAction->setData(vtkMRMLLayoutNode::SlicerLayoutConventionalWidescreenView);
  d->ViewLayoutConventionalQuantitativeAction->setData(vtkMRMLLayoutNode::SlicerLayoutConventionalQuantitativeView);
  d->ViewLayoutConventionalPlotAction->setData(vtkMRMLLayoutNode::SlicerLayoutConventionalPlotView);
  d->ViewLayoutFourUpAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourUpView);
  d->ViewLayoutFourUpQuantitativeAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourUpQuantitativeView);
  d->ViewLayoutFourUpPlotAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourUpPlotView);
  d->ViewLayoutFourUpPlotTableAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourUpPlotTableView);
  d->ViewLayoutFourUpTableAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourUpTableView);
  d->ViewLayoutDual3DAction->setData(vtkMRMLLayoutNode::SlicerLayoutDual3DView);
  d->ViewLayoutTriple3DAction->setData(vtkMRMLLayoutNode::SlicerLayoutTriple3DEndoscopyView);
  d->ViewLayoutOneUp3DAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUp3DView);
  d->ViewLayout3DTableAction->setData(vtkMRMLLayoutNode::SlicerLayout3DTableView);
  d->ViewLayoutOneUpQuantitativeAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUpQuantitativeView);
  d->ViewLayoutOneUpPlotAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUpPlotView);
  d->ViewLayoutOneUpRedSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUpRedSliceView);
  d->ViewLayoutOneUpYellowSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUpYellowSliceView);
  d->ViewLayoutOneUpGreenSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutOneUpGreenSliceView);
  d->ViewLayoutTabbed3DAction->setData(vtkMRMLLayoutNode::SlicerLayoutTabbed3DView);
  d->ViewLayoutTabbedSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutTabbedSliceView);
  d->ViewLayoutCompareAction->setData(vtkMRMLLayoutNode::SlicerLayoutCompareView);
  d->ViewLayoutCompareWidescreenAction->setData(vtkMRMLLayoutNode::SlicerLayoutCompareWidescreenView);
  d->ViewLayoutCompareGridAction->setData(vtkMRMLLayoutNode::SlicerLayoutCompareGridView);
  d->ViewLayoutThreeOverThreeAction->setData(vtkMRMLLayoutNode::SlicerLayoutThreeOverThreeView);
  d->ViewLayoutThreeOverThreeQuantitativeAction->setData(vtkMRMLLayoutNode::SlicerLayoutThreeOverThreeQuantitativeView);
  d->ViewLayoutThreeOverThreePlotAction->setData(vtkMRMLLayoutNode::SlicerLayoutThreeOverThreePlotView);
  d->ViewLayoutFourOverFourAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourOverFourView);
  d->ViewLayoutTwoOverTwoAction->setData(vtkMRMLLayoutNode::SlicerLayoutTwoOverTwoView);
  d->ViewLayoutSideBySideAction->setData(vtkMRMLLayoutNode::SlicerLayoutSideBySideView);
  d->ViewLayoutFourByThreeSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourByThreeSliceView);
  d->ViewLayoutFourByTwoSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutFourByTwoSliceView);
  d->ViewLayoutFiveByTwoSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutFiveByTwoSliceView);
  d->ViewLayoutThreeByThreeSliceAction->setData(vtkMRMLLayoutNode::SlicerLayoutThreeByThreeSliceView);
  
  d->ViewLayoutCompare_2_viewersAction->setData(2);
  d->ViewLayoutCompare_3_viewersAction->setData(3);
  d->ViewLayoutCompare_4_viewersAction->setData(4);
  d->ViewLayoutCompare_5_viewersAction->setData(5);
  d->ViewLayoutCompare_6_viewersAction->setData(6);
  d->ViewLayoutCompare_7_viewersAction->setData(7);
  d->ViewLayoutCompare_8_viewersAction->setData(8);
  
  d->ViewLayoutCompareWidescreen_2_viewersAction->setData(2);
  d->ViewLayoutCompareWidescreen_3_viewersAction->setData(3);
  d->ViewLayoutCompareWidescreen_4_viewersAction->setData(4);
  d->ViewLayoutCompareWidescreen_5_viewersAction->setData(5);
  d->ViewLayoutCompareWidescreen_6_viewersAction->setData(6);
  d->ViewLayoutCompareWidescreen_7_viewersAction->setData(7);
  d->ViewLayoutCompareWidescreen_8_viewersAction->setData(8);
  
  d->ViewLayoutCompareGrid_2x2_viewersAction->setData(2);
  d->ViewLayoutCompareGrid_3x3_viewersAction->setData(3);
  d->ViewLayoutCompareGrid_4x4_viewersAction->setData(4);
  
  //d->WindowErrorLogAction->setIcon(
  //  this->style()->standardIcon(QStyle::SP_MessageBoxCritical));

  d->ViewLayoutConventionalWidescreenAction->setText("全部视窗");
  d->ViewLayoutOneUp3DAction->setText("只显示3D窗口");
  d->ViewLayoutOneUpRedSliceAction->setText("只显示红色窗口");
  d->ViewLayoutOneUpYellowSliceAction->setText("只显示黄色窗口");
  d->ViewLayoutOneUpGreenSliceAction->setText("只显示绿色窗口");

  
  d->ViewLayoutConventionalWidescreenAction->setToolTip("全部视窗");
  d->ViewLayoutOneUp3DAction->setToolTip("只显示3D窗口");
  d->ViewLayoutOneUpRedSliceAction->setToolTip("只显示红色窗口");
  d->ViewLayoutOneUpYellowSliceAction->setToolTip("只显示黄色窗口");
  d->ViewLayoutOneUpGreenSliceAction->setToolTip("只显示绿色窗口");


  if (this->errorLogWidget())
    {
    d->setErrorLogIconHighlighted(false);
    this->errorLogWidget()->installEventFilter(this);
    }
#ifdef Slicer_USE_PYTHONQT
  if (this->pythonConsole())
    {
    this->pythonConsole()->installEventFilter(this);
    }
#endif

  qSlicerApplication * app = qSlicerApplication::application();

#ifdef Slicer_BUILD_EXTENSIONMANAGER_SUPPORT
  // d->ViewExtensionsManagerAction->setVisible(
  //  app->revisionUserSettings()->value("Extensions/ManagerEnabled").toBool());
#else
  d->ViewExtensionsManagerAction->setVisible(false);
  d->WindowToolBarsMenu->removeAction(d->ViewExtensionsManagerAction);
#endif

#if defined Slicer_USE_QtTesting && defined Slicer_BUILD_CLI_SUPPORT
  if (app->commandOptions()->enableQtTesting() ||
      app->userSettings()->value("QtTesting/Enabled").toBool())
    {
  //  d->EditPlayMacroAction->setVisible(true);
  //  d->EditRecordMacroAction->setVisible(true);
    app->testingUtility()->addPlayer(new qSlicerCLIModuleWidgetEventPlayer());
    }
#endif
  Q_UNUSED(app);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_LoadDICOMAction_triggered()
{
  qSlicerLayoutManager * layoutManager = qSlicerApplication::application()->layoutManager();

  if (!layoutManager)
    {
    return;
    }
  layoutManager->setCurrentModule("DICOM");
  RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(false);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onWarningsOrErrorsOccurred(ctkErrorLogLevel::LogLevel logLevel)
{
  Q_D(qSlicerMainWindow);
  if(logLevel > ctkErrorLogLevel::Info)
    {
    d->setErrorLogIconHighlighted(true);
    }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_EditApplicationSettingsAction_triggered()
{
  ctkSettingsDialog* const settingsDialog =
    qSlicerApplication::application()->settingsDialog();

  // Reload settings to apply any changes that have been made outside of the
  // dialog (e.g. changes to module paths due to installing extensions). See
  // http://na-mic.org/Mantis/view.php?id=3658.
  settingsDialog->reloadSettings();

  // Now show the dialog
  settingsDialog->exec();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onNewFileLoaded(const qSlicerIO::IOProperties& fileProperties)
{
  Q_D(qSlicerMainWindow);

  d->RecentlyLoadedFileProperties.removeAll(fileProperties);

  d->RecentlyLoadedFileProperties.enqueue(fileProperties);

  d->filterRecentlyLoadedFileProperties();

  d->setupRecentlyLoadedMenu(d->RecentlyLoadedFileProperties);

  // Keep the settings up-to-date
  qSlicerMainWindowPrivate::writeRecentlyLoadedFiles(d->RecentlyLoadedFileProperties);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_CopyAction_triggered()
{
  QWidget* focused = QApplication::focusWidget();
  if (focused != nullptr)
    {
    QApplication::postEvent(focused,
                            new QKeyEvent( QEvent::KeyPress,
                                           Qt::Key_C,
                                           Qt::ControlModifier));
    QApplication::postEvent(focused,
                            new QKeyEvent( QEvent::KeyRelease,
                                           Qt::Key_C,
                                           Qt::ControlModifier));
    }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_PasteAction_triggered()
{
  QWidget* focused = QApplication::focusWidget();
  if (focused != nullptr)
    {
    QApplication::postEvent(focused,
                            new QKeyEvent( QEvent::KeyPress,
                                           Qt::Key_V,
                                           Qt::ControlModifier));
    QApplication::postEvent(focused,
                            new QKeyEvent( QEvent::KeyRelease,
                                           Qt::Key_V,
                                           Qt::ControlModifier));
    }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_CutAction_triggered()
{
  QWidget* focused = QApplication::focusWidget();
  if (focused != nullptr)
    {
    QApplication::postEvent(focused,
                            new QKeyEvent( QEvent::KeyPress,
                                           Qt::Key_X,
                                           Qt::ControlModifier));
    QApplication::postEvent(focused,
                            new QKeyEvent( QEvent::KeyRelease,
                                           Qt::Key_X,
                                           Qt::ControlModifier));
    }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::on_ViewExtensionsManagerAction_triggered()
{
#ifdef Slicer_BUILD_EXTENSIONMANAGER_SUPPORT
  qSlicerApplication * app = qSlicerApplication::application();
  app->openExtensionsManagerDialog();
#endif
}

/*!
 * \brief 最大化 cxc
 */
void qSlicerMainWindow::on_maxPushButton_clicked()
{
    Q_D(qSlicerMainWindow);
    if (!d->m_reduction)
    {
        this->showMaximized();
    }
    else
    {
        this->showNormal();
    }
}

/*!
 * \brief 关闭 cxc
 */
void qSlicerMainWindow::on_closePushButton_clicked()
{
    this->close();
}

/*!
 * \brief 最小化 cxc
 */
void qSlicerMainWindow::on_minPushButton_clicked()
{
    this->showMinimized();
}

/*!
 * \brief segmentEditor cxc
 */
void qSlicerMainWindow::on_segmentEditorButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButton("SegmentEditor");

    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

/*!
 * \brief Annotations cxc
 */
void qSlicerMainWindow::on_annotationsButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButton("Annotations");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

/*!
 * \brief Data cxc
 */
void qSlicerMainWindow::on_dataButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButton("Data");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

/*!
 * \brief DICOM cxc
 */
void qSlicerMainWindow::on_DICOMButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButton("DICOM");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(false);
}

/*!
 * \brief Markups cxc
 */
void qSlicerMainWindow::on_markupsButton_clicked(QAction* a)
{
    Q_D(qSlicerMainWindow);
    auto b = a->isChecked();
    if (b)
    {
        d->ModuleSelectorToolBar->currentModuleToolButton("Markups");
        if (d->PanelDockWidget->isHidden())
        {
            d->PanelDockWidget->show();
        }
        RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
    }
    else
    {
        d->ModuleSelectorToolBar->currentModuleToolButton("SegmentEditor");
        if (d->PanelDockWidget->isHidden())
        {
            d->PanelDockWidget->show();
        }
        RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
    }

   
}

void qSlicerMainWindow::on_openfileButton_clicked(QAction* a)
{
    Q_D(qSlicerMainWindow);
    auto b = a->isChecked();
    if (b)
    {
        a->setChecked(false);
    }
 
    qSlicerApplication::application()->ioManager()->openAddDataDialog();
}

void qSlicerMainWindow::on_savebtnButton_clicked(QAction* a)
{
    Q_D(qSlicerMainWindow);
    auto b = a->isChecked();
    if (b)
    {
        a->setChecked(false);
    }
    qSlicerApplication::application()->ioManager()->openSaveDataDialog();
}

void qSlicerMainWindow::on_srceenshotButton_clicked(QAction* a)
{
    Q_D(qSlicerMainWindow);
    auto b = a->isChecked();
    if (b)
    {
        a->setChecked(false);
    }
    qSlicerApplication::application()->ioManager()->openScreenshotDialog();
}

void qSlicerMainWindow::on_scissorsButton_clicked(QAction* a)
{
    auto b = a->isChecked();
    emit cool(&b);
}

void qSlicerMainWindow::on_HelpDocButton_clicked(QAction* a)
{
    Q_D(qSlicerMainWindow);
    auto b = a->isChecked();
    if (b)
    {
        a->setChecked(false);
    }
    QProcess process(this);
    process.start("tasklist");
    process.waitForFinished();

    QByteArray result = process.readAllStandardOutput();
    if (QString(result).contains("hh.exe"))
    {
        QString cmdStr = QString("taskkill /im %1 /f").arg("hh.exe");
        //process.execute("taskkill /im MedProViewModel.exe /f");
        process.execute(cmdStr);
        process.close();
    }
    QString chmPath = QCoreApplication::applicationDirPath() + "/中创新影3D Viewer操作说明.chm";
    QDesktopServices::openUrl(QUrl::fromLocalFile(chmPath));
}

/*!
 * \brief Models cxc
 */
void qSlicerMainWindow::on_modelsButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButton("Models");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

/*!
 * \brief Segmentations cxc
 */
void qSlicerMainWindow::on_segmentationsButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButton("Segmentations");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

/*!
 * \brief Transforms cxc
 */
void qSlicerMainWindow::on_transformsButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButton("Transforms");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

/*!
 * \brief VolumeRendering cxc
 */
void qSlicerMainWindow::on_volumeRenderingButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButton("VolumeRendering");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

/*!
 * \brief Volumes cxc
 */
void qSlicerMainWindow::on_volumesButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButton("Volumes");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

void qSlicerMainWindow::on_nextPushButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->selectNextModule();
}

void qSlicerMainWindow::on_previousPushButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->selectPreviousModule();
}

/*!
 * \brief 降噪 Gradient Anisotropic Diffusion
 */
void qSlicerMainWindow::on_gradientADButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButtonTitle("Gradient Anisotropic Diffusion");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

/*!
 * \brief Elastix
 */
void qSlicerMainWindow::on_slicerElastixButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButtonTitle("General Registration (Elastix)");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

void qSlicerMainWindow::on_decimationButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButton("Decimation");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

/*!
 * \brief Chest Imaging Platfrom  --Interactive Lobe SegmentationButton
 */
void qSlicerMainWindow::on_interactiveLobeSegmentationButton_clicked()
{
    Q_D(qSlicerMainWindow);
    d->ModuleSelectorToolBar->currentModuleToolButtonTitle("Interactive Lobe Segmentation");
    if (d->PanelDockWidget->isHidden())
    {
        d->PanelDockWidget->show();
    }
    RecordInformationTool::getInstance()->setPopWidgetDICOMVisble(true);
}

void qSlicerMainWindow::slotDoubleClickedPushButton()
{
    Q_D(qSlicerMainWindow);
    if (!d->m_reduction)
    {
        this->showMaximized();
    }
    else
    {
        this->showNormal();
    }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::slotUploadFile()
{
    //上传mrml nrrd
    ProgressbarFile::getInstance()->setProgressBarValue(61);
    ProgressbarFile::getInstance()->setProgressbarText("上传数据", "数据正在上传，请稍后...");
    ProgressbarFile::getInstance()->open();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::slotSubmitFile()
{
    //上传mrml nrrd obj
    ProgressbarFile::getInstance()->setProgressBarValue(61);
    ProgressbarFile::getInstance()->setProgressbarText("提交数据", "数据正在提交，请稍后...");
    ProgressbarFile::getInstance()->open();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onModuleLoaded(const QString& moduleName)
{
  Q_D(qSlicerMainWindow);

  qSlicerAbstractCoreModule* coreModule =
    qSlicerApplication::application()->moduleManager()->module(moduleName);
  qSlicerAbstractModule* module = qobject_cast<qSlicerAbstractModule*>(coreModule);
  if (!module)
    {
    return;
    }

  // Module ToolBar
  QAction * action = module->action();
  if (!action || action->icon().isNull())
    {
    return;
    }

  Q_ASSERT(action->data().toString() == module->name());
  Q_ASSERT(action->text() == module->title());

  // Add action to ToolBar if it's an "allowed" action
  int index = d->FavoriteModules.indexOf(module->name());
  if (index != -1)
    {
    // find the location of where to add the action.
    // Note: FavoriteModules is sorted
    //QAction* beforeAction = nullptr; // 0 means insert at end
    //foreach(QAction* toolBarAction, d->ModuleToolBar->actions())
    //  {
    //  bool isActionAFavoriteModule =
    //    (d->FavoriteModules.indexOf(toolBarAction->data().toString()) != -1);
    //  if ( isActionAFavoriteModule &&
    //      d->FavoriteModules.indexOf(toolBarAction->data().toString()) > index)
    //    {
    //    beforeAction = toolBarAction;
    //    break;
    //    }
    //  }
    //d->ModuleToolBar->insertAction(beforeAction, action);
    //action->setParent(d->ModuleToolBar);
    }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onModuleAboutToBeUnloaded(const QString& moduleName)
{
  Q_D(qSlicerMainWindow);

  if (d->ModuleSelectorToolBar->selectedModule() == moduleName)
    {
    d->ModuleSelectorToolBar->selectModule("");
    }

  //foreach(QAction* action, d->ModuleToolBar->actions())
  //  {
  //  if (action->data().toString() == moduleName)
  //    {
  //    d->ModuleToolBar->removeAction(action);
  //    return;
  //    }
  //  }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onMRMLSceneModified(vtkObject* sender)
{
  Q_UNUSED(sender);
  //Q_D(qSlicerMainWindow);
  //
  //vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sender);
  //if (scene && scene->IsBatchProcessing())
  //  {
  //  return;
  //  }
  //d->EditUndoAction->setEnabled(scene && scene->GetNumberOfUndoLevels());
  //d->EditRedoAction->setEnabled(scene && scene->GetNumberOfRedoLevels());
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onLayoutActionTriggered(QAction* action)
{
  Q_D(qSlicerMainWindow);
  bool found = false;
  // std::cerr << "onLayoutActionTriggered: " << action->text().toStdString() << std::endl;
  foreach(QAction* maction, d->LayoutMenu->actions())
    {
    if (action->text() == maction->text())
      {
      found = true;
      break;
      }
    }
  
  foreach(QAction* maction, d->menuConventionalQuantitative->actions())
    {
    if (action->text() == maction->text())
      {
      found = true;
      break;
      }
    }
  
  foreach(QAction* maction, d->menuFourUpQuantitative->actions())
    {
    if (action->text() == maction->text())
      {
      found = true;
      break;
      }
    }
  
  foreach(QAction* maction, d->menuOneUpQuantitative->actions())
    {
    if (action->text() == maction->text())
      {
      found = true;
      break;
      }
    }
  
  foreach(QAction* maction, d->menuThreeOverThreeQuantitative->actions())
    {
    if (action->text() == maction->text())
      {
      found = true;
      break;
      }
    }
  
  if (found && !action->data().isNull())
    {
      int index = action->data().toInt();
      
      if (index == RecordInformationTool::getInstance()->getWindowLayout())
      {
          return;
      }
      RecordInformationTool::getInstance()->setWindowLayout(index);
      this->setLayout(index);
    }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onLayoutCompareActionTriggered(QAction* action)
{
  Q_D(qSlicerMainWindow);

  // std::cerr << "onLayoutCompareActionTriggered: " << action->text().toStdString() << std::endl;

  // we need to communicate both the layout change and the number of viewers.
  this->setLayout(d->ViewLayoutCompareAction->data().toInt());
  this->setLayoutNumberOfCompareViewRows(action->data().toInt());
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onLayoutCompareWidescreenActionTriggered(QAction* action)
{
  Q_D(qSlicerMainWindow);

  // std::cerr << "onLayoutCompareWidescreenActionTriggered: " << action->text().toStdString() << std::endl;

  // we need to communicate both the layout change and the number of viewers.
  this->setLayout(d->ViewLayoutCompareWidescreenAction->data().toInt());
  this->setLayoutNumberOfCompareViewColumns(action->data().toInt());
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::onLayoutCompareGridActionTriggered(QAction* action)
{
  Q_D(qSlicerMainWindow);

  // std::cerr << "onLayoutCompareGridActionTriggered: " << action->text().toStdString() << std::endl;

  // we need to communicate both the layout change and the number of viewers.
  this->setLayout(d->ViewLayoutCompareGridAction->data().toInt());
  this->setLayoutNumberOfCompareViewRows(action->data().toInt());
  this->setLayoutNumberOfCompareViewColumns(action->data().toInt());
}


//---------------------------------------------------------------------------
void qSlicerMainWindow::onLayoutChanged(int layout)
{
  Q_D(qSlicerMainWindow);
  // Trigger the action associated with the new layout
  foreach(QAction* action, d->LayoutMenu->actions())
    {
    if (action->data().toInt() == layout)
      {
      action->trigger();
      }
    }
  
  foreach(QAction* action, d->menuConventionalQuantitative->actions())
    {
    if (action->data().toInt() == layout)
      {
      action->trigger();
      }
    }
  
  foreach(QAction* action, d->menuFourUpQuantitative->actions())
    {
    if (action->data().toInt() == layout)
      {
      action->trigger();
      }
    }
  
  foreach(QAction* action, d->menuOneUpQuantitative->actions())
    {
    if (action->data().toInt() == layout)
      {
      action->trigger();
      }
    }
  
  foreach(QAction* action, d->menuThreeOverThreeQuantitative->actions())
    {
    if (action->data().toInt() == layout)
      {
      action->trigger();
      }
    }
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    //屏蔽拖拽事件
  //qSlicerApplication::application()->ioManager()->dragEnterEvent(event);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::dropEvent(QDropEvent *event)
{
    //屏蔽拖拽事件
  //qSlicerApplication::application()->ioManager()->dropEvent(event);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::setHomeModuleCurrent()
{
  Q_D(qSlicerMainWindow);
  QSettings settings;
  QString homeModule = settings.value("Modules/HomeModule").toString();
  homeModule = "SegmentEditor";
  d->ModuleSelectorToolBar->selectModule(homeModule);

  //初始化关闭
  //d->PanelDockWidget->close();
}


//kyo
void qSlicerMainWindow::setViewLayout()
{
    onLayoutActionTriggered(d_ptr->ViewLayoutConventionalWidescreenAction);
    ProgressbarFile::getInstance()->setOrientationMarkerTypeTT();
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::restoreToolbars()
{
  Q_D(qSlicerMainWindow);
  this->restoreState(d->StartupState);
}

//---------------------------------------------------------------------------
bool qSlicerMainWindow::eventFilter(QObject* object, QEvent* event)
{
  Q_D(qSlicerMainWindow);
  if (object == this->errorLogWidget())
    {
    if (event->type() == QEvent::ActivationChange
        && this->errorLogWidget()->isActiveWindow())
      {
      d->setErrorLogIconHighlighted(false);
      }
    }

  if (object == d->PanelDockWidget)
  {
      if (event->type() == QEvent::Resize)
      {
          int index = 0;
          index = d->PanelDockWidget->width() - 170;
          //d->label->setFixedWidth(index);
      }
  }

  if (object == d->WindowTitleWidget)
  {
      if (event->type() == QEvent::MouseButtonDblClick)
      {
          if (!d->m_reduction)
          {
              this->showMaximized();
          }
          else
          {
              this->showNormal();
          }
      }
  }
#ifdef Slicer_USE_PYTHONQT
  if (object == this->pythonConsole())
    {
    if (event->type() == QEvent::Hide)
      {
      bool wasBlocked = d->PythonConsoleToggleViewAction->blockSignals(true);
      d->PythonConsoleToggleViewAction->setChecked(false);
      d->PythonConsoleToggleViewAction->blockSignals(wasBlocked);
      }
    }
#endif
  return this->Superclass::eventFilter(object, event);
}

//---------------------------------------------------------------------------
void qSlicerMainWindow::changeEvent(QEvent* event)
{
  Q_D(qSlicerMainWindow);
  switch (event->type())
    {
    case QEvent::PaletteChange:
      {
      d->updatePythonConsolePalette();
      break;
      }
    default:
      break;
    }
}
