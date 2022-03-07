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

// Slicer includes
#include "qSlicerApplication.h"
#include "qSlicerApplicationHelper.h"
#include "qSlicerStyle.h"
#include <QTranslator>
#include <QTextCodec>
#include <QWindow>
#include <QToolTip>
#include <QUrlQuery>
#include <QMessagebox>

// SlicerApp includes
#include "qSlicerAppMainWindow.h"
#include "qSlicerCoreApplication.h"
#include "recordinformationtool.h"
#include "progressbarfile.h"

#pragma execution_character_set("utf-8")

// DownLoad file
UserDownLoadInfo userInfo;

void parseWebArguments()
{
    // 获取参数
    QStringList arguments = QCoreApplication::arguments();
    qDebug() << "Arguments : " << arguments;
    
    if (arguments.count() < 2)
    {
        return;
    }


    QString strData = arguments.at(1);
    qDebug() << "数据 : " << strData;

    //QMessageBox::information(nullptr, "data", strData);

    strData = strData.replace("zcxyshell://", "");
    qDebug() << "数据 : " << strData;

    //QMessageBox::information(nullptr, "data::::", strData);

    // 处理字符串
    QUrlQuery query;
    query.setQuery(strData);
  
    userInfo.unique_hash = query.queryItemValue("unique_hash");
    userInfo.order       = query.queryItemValue("order");
    userInfo.user        = query.queryItemValue("user");
    userInfo.model_ids   = query.queryItemValue("model_ids");

    if (userInfo.unique_hash.isEmpty())
    {
        
        if (strData.startsWith("unique_hash="))
        {
            userInfo.unique_hash = strData.mid(12, 32);
        }
    }

    if (userInfo.model_ids.contains("="))
    {
        int nb = strData.indexOf("model_ids");
        userInfo.model_ids = strData.mid(nb + 10, strData.size() - nb -11);
    }
    

    query.clear();
}

namespace
{

//----------------------------------------------------------------------------
int SlicerAppMain(int argc, char* argv[])
{
  typedef qSlicerAppMainWindow SlicerMainWindowType;
  typedef qSlicerStyle SlicerAppStyle;

  qSlicerApplicationHelper::preInitializeApplication(argv[0], new SlicerAppStyle);

  qSlicerApplication app(argc, argv);
  if (app.returnCode() != -1)
    {
    return app.returnCode();
    }

  //网页唤起exe获取web携带参数
  parseWebArguments();

  //加载外部插件
  qSlicerCoreApplication* appCore = qSlicerCoreApplication::application();
  appCore->addSegmentEditorExtraPlugIn();

  QTextCodec* textCodec = QTextCodec::codecForName("utf8");
  QTextCodec::setCodecForLocale(textCodec);
  QTranslator translator;
  translator.load(":/qt_zh_CN.qm");
  app.installTranslator(&translator);

  //设置全局文字大小
  QFont font = app.font();
  font.setFamily("SimHei");
  app.setFont(font);

  QScopedPointer<SlicerMainWindowType> window;
  QScopedPointer<QSplashScreen> splashScreen;

  int exitCode = qSlicerApplicationHelper::postInitializeApplication<SlicerMainWindowType>(
        app, splashScreen, window);
  if (exitCode != 0)
    {
    return exitCode;
    }
  
  if (!window.isNull())
    {
    //QString windowTitle = QString("%1 %2").arg(window->windowTitle()).arg(Slicer_VERSION_FULL);
        //QString windowTitle = QString("中创新影");
        //window->setWindowTitle(windowTitle);
        //window->setWindowIcon(QIcon(":/Slicer.ico"));
    }

  return app.exec();
}

} // end of anonymous namespace

#include "qSlicerApplicationMainWrapper.cxx"
