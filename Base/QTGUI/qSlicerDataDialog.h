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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerDataDialog_h
#define __qSlicerDataDialog_h

// CTK includes
#include <ctkPimpl.h>

// Slicer includes
#include "qSlicerFileDialog.h"
#include "qSlicerBaseQTGUIExport.h"

/// Forward declarations
class qSlicerDataDialogPrivate;
class QDropEvent;

//------------------------------------------------------------------------------
class Q_SLICER_BASE_QTGUI_EXPORT qSlicerDataDialog : public qSlicerFileDialog
{
  Q_OBJECT
public:
  typedef qSlicerFileDialog Superclass;
  qSlicerDataDialog(QObject* parent =nullptr);
  ~qSlicerDataDialog() override;

  qSlicerIO::IOFileType fileType()const override;
  QString description()const override;
  qSlicerFileDialog::IOAction action()const override;

  bool isMimeDataAccepted(const QMimeData* mimeData)const override;
  void dropEvent(QDropEvent *event) override;

  /// run the dialog to select the file/files/directory
  Q_INVOKABLE bool exec(const qSlicerIO::IOProperties& readerProperties =
                    qSlicerIO::IOProperties()) override;

  //open filepath 
  bool openFilePaths(const QStringList &list, const qSlicerIO::IOProperties& readerProperties);

public Q_SLOTS:
  /// for programmatic population of dialog
  virtual void addFile(const QString filePath);
  virtual void addDirectory(const QString directoryPath);

protected:
  QScopedPointer<qSlicerDataDialogPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerDataDialog);
  Q_DISABLE_COPY(qSlicerDataDialog);
};

#endif
