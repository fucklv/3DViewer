#ifndef RECORDINFORMATIONTOOL_H
#define RECORDINFORMATIONTOOL_H

#include <QObject>

// qMRML includes
#include "qMRMLWidget.h"
#include "qMRMLWidgetsExport.h"

//下载信息
struct UserDownLoadInfo
{
    QString unique_hash;
    QString order;
    QString user;
    QString model_ids;

    UserDownLoadInfo() {}
    ~UserDownLoadInfo() {}

    void clear()
    {
        this->unique_hash = "";
        this->order = "";
        this->user = "";
        this->model_ids = "";
    }
};

class QMRML_WIDGETS_EXPORT RecordInformationTool : public QObject
{
    Q_OBJECT
public:
    explicit RecordInformationTool(QObject *parent = nullptr);

    static RecordInformationTool* p_Instance;
    static RecordInformationTool* getInstance();

    void setPopWidgetDICOMVisble(bool state);
    bool getPopWidgetDICOMVisble();

    void setWindowVisible(bool state);
    bool getWindowVisible();

    void setWindowLayout(int index);
    int getWindowLayout() const;

    void setWindowLayoutChange(bool state);
    bool getWindowLayoutChange();

    //打开文件的路径
    void setOpenFilePathList(const QStringList& list);
    QStringList getOpenFilePathList();
    void clearFilesList();

signals:

private:
    bool m_DICOMVisible;
    bool m_WindowVisible;
    int m_WindowLayoutIndex;


    bool m_LayoutChange;

    QStringList m_OpenFilePathList;
};

#endif // RECORDINFORMATIONTOOL_H
