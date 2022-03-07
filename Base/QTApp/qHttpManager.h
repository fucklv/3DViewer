#ifndef QHTTPREPLY_H
#define QHTTPREPLY_H

#include <vector>
#include <string>
#include <atlstr.h>
#include <sstream>

#include "qSlicerBaseQTAppExport.h"
#include "slicerdll.h"
#include "tinyxml2.h"

enum RequestType{
	GetURLListType	= 100,	  //��ȡ�����б�
	GetSTSType		= 101,    //��ȡ�������token
	UpLoadSuccess   = 102,	  //�ϴ����֪ͨ������
};

enum FindFileType {
	UPLOAD_SAVE_FILES		= 1,
	UPLOAD_OBJ_FILES		= 2,
	OPEN_MRML_FILE          = 3,
};


//const QString getDownLoadListURL = "https://zcxy-api.zc5l.com/api/client/order";
//const QString getOSSSTS = "https://zcxy-api.zc5l.com/api/client/upload/sts";
//const QString uploadSaveSuccess = "https://zcxy-api.zc5l.com/api/client/order/history";
//const QString uploadObjSuccess = "https://zcxy-api.zc5l.com/api/client/order/complete";
//
//const QString endPoint = "oss-cn-shenzhen.aliyuncs.com";
//const QString bucketName = "zcwl-resources";
//const QString objectName = "build-model/";
const QString SAVE_PATH = "/Save";
const QString DOWNLOAD_PATH = "/NiiFile";

/*const QString getDownLoadListURL = "https://dev-api.zc5l.com/api/client/order";
const QString getOSSSTS = "https://dev-api.zc5l.com/api/client/upload/sts";
const QString uploadSaveSuccess = "https://dev-api.zc5l.com/api/client/order/history";
const QString uploadObjSuccess = "https://dev-api.zc5l.com/api/client/order/complete";

const QString endPoint = "oss-cn-shenzhen.aliyuncs.com";
const QString bucketName = "zcwl-test";
const QString objectName = "build-model/";
const QString SAVE_PATH = "/Save";
const QString DOWNLOAD_PATH = "/NiiFile"*/;

struct ColorData
{
	std::string name;
	std::string color;
};

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

private:
	explicit QhttpManager(QObject* parent = nullptr);
	QhttpManager(QhttpManager&) = delete;
	QhttpManager& operator=(QhttpManager m) = delete;
	
	//��ȡ������IP����
	void readIPConfig();

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

	//��ȡ��ɫ
	void  GetModelColors();
	//�����ļ�
	void analyticMRMlfile(tinyxml2::XMLElement* element);
	void String_Split(std::string s, std::string delim, vector<std::string>& ans);
	std::string toHexColor(vector<std::string>& colors);

private slots:
	void slot_RecvMsg(QNetworkReply* reply);
	
	void slot_DownLoadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void slot_DownLoadError(QNetworkReply::NetworkError code);
	void slot_DownLoadFinish(const QString& pathName);

signals:
	void signal_DownLoadFinish();

private:
	vector<std::string>     nameList;
	vector<std::string>     colorList;
	vector<ColorData>       m_vColors;

	Slicerdll*			 m_pSlicehttp;
	QVector<QString>	 m_UrlList ;			//�����б�
	//QStringList			 m_FileList;        //���ļ��б�

	QStringList          m_SaveFileList;   //��Ҫ�ϴ��ù����ļ�
	QStringList          m_OpenFileList;   //�򿪵��ļ�
	QStringList			 m_OssUrlList;     //OSS��ַ

	//IP����
	QString getDownLoadListURL;
	QString getOSSSTS;
	QString uploadSaveSuccess;
	QString uploadObjSuccess;
	QString endPoint;
	QString bucketName;
	QString objectName;


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



	QMap<QNetworkReply*, RequestType> m_TypeMap;
};
#endif
