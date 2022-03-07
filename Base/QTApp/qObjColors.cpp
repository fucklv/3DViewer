#ifndef QHTTPREPLY_H
#define QHTTPREPLY_H

#include "qSlicerBaseQTAppExport.h"
#include "slicerdll.h"


enum RequestType{
	GetURLListType	= 100,	  //获取下载列表
	GetSTSType		= 101,    //获取阿里访问token
	UpLoadSuccess   = 102,	  //上传完成通知服务器
};

enum FindFileType {
	SaveNrrdFileType		= 1,
	SaveAllFileType			= 2,
	SaveMrmlFileType        = 3,
	SaveNiiFileType			= 4,
};


const QString GET_DOWNLOAD_LIST_URL = "https://zcxy-api.zc5l.com/api/client/order";
const QString GET_OSS_STS = "https://zcxy-api.zc5l.com/api/client/upload/sts";
const QString UPLOAD_SAVE_SUCCESS = "https://zcxy-api.zc5l.com/api/client/order/history";
const QString UPLOAD_OBJ_SUCCESS = "https://zcxy-api.zc5l.com/api/client/order/complete";

const QString END_POINT = "oss-cn-shenzhen.aliyuncs.com";
const QString BUCKET_NAME = "zcwl-resources";
const QString OBJECT_NAME = "build-model/";
const QString SAVE_PATH = "/Save";
const QString DOWNLOAD_PATH = "/NiiFile";


class  Q_SLICER_BASE_QTAPP_EXPORT QhttpManager : public QObject
{
	Q_OBJECT
public:
	static QhttpManager* instance();
	~QhttpManager();

	void close();
public:
	void getDownloadList(const QString& uniqueStr, const QString& user, const QString& order, const QString& model_ids);
	void UpLoadSaveFile();
	void UpLoadObjFile();

	void SetColors(QString* fileName,double colors[]);

private:
	explicit QhttpManager(QObject* parent = nullptr);
	QhttpManager(QhttpManager&) = delete;
	QhttpManager& operator=(QhttpManager m) = delete;
	
	//解析 GetURLListType json获取下载列表
	void  getURLListJson(const QByteArray& msg);
	//依次下载文件
	void  downloadNiifile();
	
	//解析 GetSTSType json获取下载列表
	void  getSTSJson(const QByteArray& msg);
	//上传保存文件
	void  uploadSaveFile();
	//通知服务器上传完成
	void  uploadSuccessReq();

	void  findFile(const qint8 type);

private slots:
	void slot_RecvMsg(QNetworkReply* reply);
	
	void slot_DownLoadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void slot_DownLoadError(QNetworkReply::NetworkError code);
	void slot_DownLoadFinish(const QString& pathName);

signals:
	void signal_DownLoadFinish();

private:
	Slicerdll*			 m_pSlicehttp;
	QVector<QString>	 m_UrlList ;			//下载列表
	//QStringList			 m_FileList;        //打开文件列表

	QStringList          m_SaveFileList;   //需要上传得工程文件
	QStringList          m_OpenFileList;   //打开的文件
	QStringList			 m_OssUrlList;     //OSS地址

	//拉起参数
	QString   m_UniqueStr;
	QString   m_UseData;
	QString   m_OrderData;
	QString   m_Model_ids;
	
	//STS
	QString   m_SecurityToken;
	QString   m_AccessKeyId;
	QString   m_AccessKeySecret;

	//新建还是历史文件
	qint8     m_SaveOrObj;
	QMap<QString*, double*> m_mapFileColor;
	QMap<QNetworkReply*, RequestType> m_TypeMap;
};
#endif
