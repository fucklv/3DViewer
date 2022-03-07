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
	GetURLListType	= 100,	  //获取下载列表
	GetSTSType		= 101,    //获取阿里访问token
	UpLoadSuccess   = 102,	  //上传完成通知服务器
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
	
	//读取服务器IP配置
	void readIPConfig();

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

	//获取颜色
	void  GetModelColors();
	//解析文件
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
	QVector<QString>	 m_UrlList ;			//下载列表
	//QStringList			 m_FileList;        //打开文件列表

	QStringList          m_SaveFileList;   //需要上传得工程文件
	QStringList          m_OpenFileList;   //打开的文件
	QStringList			 m_OssUrlList;     //OSS地址

	//IP配置
	QString getDownLoadListURL;
	QString getOSSSTS;
	QString uploadSaveSuccess;
	QString uploadObjSuccess;
	QString endPoint;
	QString bucketName;
	QString objectName;


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



	QMap<QNetworkReply*, RequestType> m_TypeMap;
};
#endif
