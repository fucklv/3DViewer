#ifndef QHTTPREPLY_H
#define QHTTPREPLY_H

#include "qSlicerBaseQTAppExport.h"
#include "slicerdll.h"


enum RequestType{
	GetURLListType	= 100,	  //��ȡ�����б�
	GetSTSType		= 101,    //��ȡ�������token
	UpLoadSuccess   = 102,	  //�ϴ����֪ͨ������
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
	
	//���� GetURLListType json��ȡ�����б�
	void  getURLListJson(const QByteArray& msg);
	//���������ļ�
	void  downloadNiifile();
	
	//���� GetSTSType json��ȡ�����б�
	void  getSTSJson(const QByteArray& msg);
	//�ϴ������ļ�
	void  uploadSaveFile();
	//֪ͨ�������ϴ����
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
	QVector<QString>	 m_UrlList ;			//�����б�
	//QStringList			 m_FileList;        //���ļ��б�

	QStringList          m_SaveFileList;   //��Ҫ�ϴ��ù����ļ�
	QStringList          m_OpenFileList;   //�򿪵��ļ�
	QStringList			 m_OssUrlList;     //OSS��ַ

	//�������
	QString   m_UniqueStr;
	QString   m_UseData;
	QString   m_OrderData;
	QString   m_Model_ids;
	
	//STS
	QString   m_SecurityToken;
	QString   m_AccessKeyId;
	QString   m_AccessKeySecret;

	//�½�������ʷ�ļ�
	qint8     m_SaveOrObj;
	QMap<QString*, double*> m_mapFileColor;
	QMap<QNetworkReply*, RequestType> m_TypeMap;
};
#endif
