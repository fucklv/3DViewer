#ifndef SLICERDLL_H
#define SLICERDLL_H

#include <QtCore>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include <initializer_list>
#include <qfile.h>
#include <qfileinfo.h>
#include <qurlquery.h>
#include <qhttpmultipart.h>
#include <QMessageBox>
#include "alibabacloud/oss/OssClient.h"


#if defined(SLICERDLL_LIBRARY)
#  define SLICERDLL_EXPORT Q_DECL_EXPORT
#else
#  define SLICERDLL_EXPORT Q_DECL_IMPORT
#endif


#define DOWNLOAD_FILE_SUFFIX "_temp"


struct   GetParamData
{
	QString name;
	QString val;
};


class  SLICERDLL_EXPORT Slicerdll :public QObject
{
	Q_OBJECT
public:
	explicit Slicerdll(QObject* parent = nullptr);
	~Slicerdll();
public:
	//static Slicerdll* instance();
	QNetworkReply* SendGetRequest(const QString& url, const std::initializer_list<GetParamData>& param = {});
	QNetworkReply* SendPostRequest(const QString& url, const QByteArray& msg);

	//文件下载
	void setDownLoadPath(const QString& path);
	void startDownFile(const QString& url);

	//文件上传
	bool uploadFileToOss(const QString& accessID,const QString& accessKey,const QString& endPoint,const QString& securityToken,const QString& bucketName,const QString& objectName,const QString& localName);
	void startUploadFile(const QString& path, const QString& url);

	//关闭
	void  close();
	
private slots:
	void onUploadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void onUploadFinished();
	void onUploadError(QNetworkReply::NetworkError code);


private slots:
	void replyFinished(QNetworkReply* reply);
	void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void onReadyRead();
	void onError(QNetworkReply::NetworkError code);
	void onFinished();
	void onGetFileInfo();


private:
	Slicerdll(Slicerdll&) = delete;
	Slicerdll& operator=(Slicerdll nwu) = delete;
	void stopWork();
	void reset();
	void closeDownload();
	void removeFile(const QString& fileName);
	void downLoadFile();

signals:
	//消息返回
	void signal_replyFinished(QNetworkReply* reply);
	//下载文件信号
	void signal_DownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
	void signal_DownloadFinished(const QString& fileName);
	void signal_DownloadError(QNetworkReply::NetworkError code);


private:
	AlibabaCloud::OSS::OssClient* m_pOssclient();
	//class Private;
	//friend class Private;
	//Private* m_pSelf;
	QNetworkAccessManager* m_pManager;
	QUrl     m_url;
	QHttpMultiPart* m_multiPart;
	QNetworkReply*	m_pHeadReply;
	QNetworkReply*	m_UploadReply;
	QNetworkReply*	m_DownLoadReply;

	//文件
	QFile*		m_pFile;
	QString     m_RealName;
	QString     m_fileName;
	QString		m_StoragePath;
	qint64		m_fileSize;
	qint64		m_bytesRecv;
	qint64      m_bytesTotoal;
	qint64      m_bytesCurrentRecv;
	bool        m_isStop;
};

#endif // SLICERDLL_H
