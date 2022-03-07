#include "QhttpManager.h"
#include <qSlicerApplication.h>
#include <qSlicerCoreIOManager.h>
#include "recordinformationtool.h"
#include "qSlicerIOManager.h"
#include "progressbarfile.h"

QhttpManager::QhttpManager(QObject* parent):QObject(parent)
{
    m_pSlicehttp = new Slicerdll(this);
    m_SaveOrObj = 0;
    m_UniqueStr = "";
    m_UseData = "";
    m_OrderData = "";
    m_Model_ids = "";

    readIPConfig();

    connect(m_pSlicehttp, &Slicerdll::signal_replyFinished, this, &QhttpManager::slot_RecvMsg);
    //下载信号
    connect(m_pSlicehttp, &Slicerdll::signal_DownloadProgress, this, &QhttpManager::slot_DownLoadProgress);
    connect(m_pSlicehttp, &Slicerdll::signal_DownloadFinished, this, &QhttpManager::slot_DownLoadFinish);
    connect(m_pSlicehttp, &Slicerdll::signal_DownloadError, this, &QhttpManager::slot_DownLoadError);

}

QhttpManager::~QhttpManager()
{
   //m_pSlicehttp->deleteLater();
}

void QhttpManager::close()
{
    m_pSlicehttp->close();
}

QhttpManager* QhttpManager::instance()
{
    static QhttpManager manager;
    return &manager;
}

void QhttpManager::getDownloadList(const QString& uniqueStr,const QString& user, const QString& order,const QString& model_ids)
{
    m_UniqueStr = uniqueStr;
    m_OrderData = order;
    m_UseData = user;
    m_Model_ids = model_ids;

    // 拼接参数
	std::initializer_list<GetParamData> reqDatas;
	GetParamData userData;
    userData.name = "user";
    userData.val = m_UseData;
    GetParamData orderData;
    orderData.name = "order";
    orderData.val = m_OrderData;
    GetParamData modelIDData;
    modelIDData.name = "model_ids";
    modelIDData.val = m_Model_ids;
    reqDatas = { userData,orderData,modelIDData};
    //清空下载目录
    QString filePath = QCoreApplication::applicationDirPath();
    filePath = filePath + DOWNLOAD_PATH;
    QDir dir(filePath);
    if (!dir.isEmpty())
    {
        dir.removeRecursively();
    }
    //发送get请求
    auto r_getlist = m_pSlicehttp->SendGetRequest(getDownLoadListURL, reqDatas);
    m_TypeMap.insert(r_getlist, GetURLListType);
}

void QhttpManager::UpLoadSaveFile()
{
    //查找文件
    findFile(UPLOAD_SAVE_FILES);
    if (m_SaveFileList.isEmpty())
    {
        QMessageBox::information(NULL, "find save file error", "savefile is null");
        return;
    }
    auto r_getsts = m_pSlicehttp->SendGetRequest(getOSSSTS);
    m_TypeMap.insert(r_getsts, GetSTSType);
}

void QhttpManager::UpLoadObjFile()
{
    //查找文件
    findFile(UPLOAD_OBJ_FILES);
    if (m_SaveFileList.isEmpty())
    {
        QMessageBox::information(NULL, "find obj file error", "obj is null");
        return;
    }
    auto r_getsts = m_pSlicehttp->SendGetRequest(getOSSSTS);
    m_TypeMap.insert(r_getsts, GetSTSType);
}

void QhttpManager::GetModelColors()
{
    nameList.clear();
    colorList.clear();
    m_vColors.clear();
    QString fileName = "";
    QDir dir;
    QStringList filters;
    filters << "*.mrml";
    QString filePath = QCoreApplication::applicationDirPath();
    filePath = filePath + SAVE_PATH;
    dir.setPath(filePath);
    dir.setNameFilters(filters);
    QDirIterator iter(dir, QDirIterator::Subdirectories);
    while (iter.hasNext())
    {
        iter.next();
        QFileInfo info = iter.fileInfo();
        if (info.isFile())
        {
            fileName = info.absoluteFilePath();
            break;
        }
    }
    if (fileName == "")
    {
        return;
    }
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError ret = doc.LoadFile(fileName.toLatin1().data());
    if (ret != 0) {
        return;
    }
    tinyxml2::XMLElement* root = doc.RootElement();
    analyticMRMlfile(root);
    for (int i = 0; i < nameList.size(); i++)
    {
        ColorData temp;
        vector<std::string> rgb;
        String_Split(colorList[i], " ", rgb);
        if (rgb.size() >= 3)
        {
            temp.color = toHexColor(rgb);
        }
        temp.name = nameList[i];
        m_vColors.push_back(temp);
    }
}

void QhttpManager::slot_RecvMsg(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::information(NULL, reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString(), reply->errorString() + reply->readAll());
    }
    else {
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()>299) 
        {
            QMessageBox::information(NULL, reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString(), reply->errorString());
            return;
        }
        //获取响应信息
        const QByteArray reply_data = reply->readAll(); 
        RequestType reType = m_TypeMap.value(reply);
        switch (reType)
        {
        case GetURLListType:
            getURLListJson(reply_data);
            downloadNiifile();
            break;
        case GetSTSType:
            getSTSJson(reply_data);
            uploadSaveFile();
            break;
        case UpLoadSuccess:
            //成功
            QMessageBox::information(NULL, "upload file","upload success");
            ProgressbarFile::getInstance()->close();
            break;
        default:
            break;
        }
        
    }
}


void QhttpManager::readIPConfig()
{
    tinyxml2::XMLDocument doc;
    QString filePath = QCoreApplication::applicationDirPath();
    filePath = filePath + "/config/ServerIP.xml";
    tinyxml2::XMLError ret = doc.LoadFile(filePath.toLatin1().data());
    if (ret != 0) {
        QMessageBox::information(NULL, "error", "can't load config file",
            QMessageBox::Yes);
        return;
    }
    tinyxml2::XMLElement* root = doc.RootElement();
    getDownLoadListURL = root->FirstChildElement("GetDownLoadListURL")->FirstAttribute()->Value();
    getOSSSTS = root->FirstChildElement("GetOSSSTS")->FirstAttribute()->Value();
    uploadSaveSuccess = root->FirstChildElement("UploadSaveSuccess")->FirstAttribute()->Value();
    uploadObjSuccess = root->FirstChildElement("UploadObjSuccess")->FirstAttribute()->Value();
    endPoint = root->FirstChildElement("EndPoint")->FirstAttribute()->Value();
    bucketName = root->FirstChildElement("BucketName")->FirstAttribute()->Value();
    objectName = root->FirstChildElement("ObjectName")->FirstAttribute()->Value();
}

void QhttpManager::getURLListJson(const QByteArray & msg)
{
        m_UrlList.clear();
        QJsonParseError json_error;
        QJsonDocument doucment = QJsonDocument::fromJson(msg, &json_error);
        if (json_error.error == QJsonParseError::NoError) {
            if (doucment.isObject()) {
                const QJsonObject obj = doucment.object();
                if (obj.contains("data")) {
                    auto dataObject = obj.value("data").toObject();             
                    auto hisList = dataObject.value("history");
                    if (hisList.isArray())
                    {
                        auto downlist = hisList.toArray();
                        for (int i = 0; i < downlist.count(); i++)
                        {
                            QJsonValue value = downlist.at(i);
                            if (value.isObject())
                            {
                                auto data = value.toObject();
                                if (data.contains("url"))
                                {
                                    m_UrlList.push_back(data.value("url").toString());
                                }
                            }
                        }
                    }
                    if (1)
                    {
                        auto niisList = dataObject.value("niis");
                        if (niisList.isArray())
                        {
                            auto downlist = niisList.toArray();
                            for (int i = 0; i < downlist.count(); i++)
                            {
                                QJsonValue value = downlist.at(i);
                                if (value.isString())
                                {                           
                                    m_UrlList.push_back(value.toString());
                                }
                            }
                        }

                    }
                }
            }
        }
        else {
            QMessageBox::information(NULL, " GetURLListType json error", json_error.errorString());
        }
}

void QhttpManager::downloadNiifile()
{
    if (m_UrlList.size() < 1)
    {
        QMessageBox::information(NULL, "down", "urls is null");
        return;
    }

    //检查是否有下载目
    QString filePath = QCoreApplication::applicationDirPath();
    filePath = filePath + DOWNLOAD_PATH;
    QDir dir(filePath);
    if (!dir.exists())
    {
        dir.mkdir(filePath);
    }
    //创建iin文件目录
    filePath = filePath + "/" + m_UniqueStr;
    QDir Uniquedir(filePath);
    bool IsRemove = false;
    if (!Uniquedir.exists())
    {
        Uniquedir.mkdir(filePath);
    }
    //设置下载目录
    m_pSlicehttp->setDownLoadPath(filePath+"/"); 
    m_pSlicehttp->startDownFile(m_UrlList[0]);
}

void QhttpManager::getSTSJson(const QByteArray& msg)
{
    QJsonParseError json_error;
    QJsonDocument doucment = QJsonDocument::fromJson(msg, &json_error);
    if (json_error.error == QJsonParseError::NoError) {
        if (doucment.isObject()) {
            const QJsonObject obj = doucment.object();
            if (obj.contains("data")) {
                auto dataObject = obj.value("data").toObject();
                m_SecurityToken = dataObject.value("SecurityToken").toString();
                m_AccessKeyId = dataObject.value("AccessKeyId").toString();
                m_AccessKeySecret = dataObject.value("AccessKeySecret").toString();
            }
        }
    }
    else
    {
        QMessageBox::information(NULL, " getSTSJson json error", json_error.errorString());
    }
}

void QhttpManager::uploadSaveFile()
{
    if (!m_AccessKeyId.isEmpty() && !m_AccessKeySecret.isEmpty() && !m_SecurityToken.isEmpty())
    {  
        if (!m_SaveFileList.isEmpty())
        {
            m_UrlList.clear();
            for (size_t i = 0; i < m_SaveFileList.size(); i++)
            {
                QFileInfo* info = new QFileInfo(m_SaveFileList[i]);
                auto name = info->fileName();
                auto objName = objectName + m_UniqueStr + "/" + name;
                if (!m_pSlicehttp->uploadFileToOss(m_AccessKeyId, m_AccessKeySecret, endPoint,m_SecurityToken,bucketName, objName, m_SaveFileList[i]))
                {
                    //上传失败
                    break;
                }
                QString url = QString("%1.%2/%3").arg(bucketName).arg(endPoint).arg(objName);
                m_UrlList.append(url);
            }

            if (m_SaveFileList.size() == m_UrlList.size())
            {
                //上传完成通知应用服务器
                uploadSuccessReq();
            }
        }
        else
        {
            QMessageBox::information(NULL, " upload error", "not have progorma file");
        }
    }

    ProgressbarFile::getInstance()->close();
}

void QhttpManager::uploadSuccessReq()
{
   
    //生成json发送POST请求
    if (!m_UrlList.isEmpty())
    {
        // 构建 Json 数组 
        QJsonArray urls;
        for (auto data : m_UrlList)
        {
            urls.append(data);
        }
        GetModelColors();
        // 构建 Json 对象
        QJsonObject json;
        json.insert("file_urls", QJsonValue(urls));
        json.insert("order", m_OrderData);
        json.insert("user", m_UseData);
        json.insert("model_ids", m_Model_ids);
        QJsonArray configJson;
        for (int i = 0; i < m_vColors.size(); ++i)
        {
            QJsonObject colorJson;
            colorJson.insert("filename", QString::fromStdString(m_vColors[i].name+".obj"));
            colorJson.insert("color", QString::fromStdString(m_vColors[i].color));
            configJson.append(colorJson);
        }
        json.insert("config_json", configJson);
       
        // 构建 Json 文档
        QJsonDocument document;
        document.setObject(json);
        QByteArray byteArray = document.toJson(QJsonDocument::Compact);
        QNetworkReply* reply;
        if (m_SaveOrObj == UPLOAD_SAVE_FILES) {
            reply = m_pSlicehttp->SendPostRequest(uploadSaveSuccess, byteArray);
        }
        else if (m_SaveOrObj == UPLOAD_OBJ_FILES)
        {
            reply = m_pSlicehttp->SendPostRequest(uploadObjSuccess, byteArray);
        }
        
        if (reply != nullptr)
        {
            m_TypeMap.insert(reply, UpLoadSuccess);
        }
    }
}

void QhttpManager::findFile(const qint8 type)
{
    m_SaveOrObj = type;
    m_SaveFileList.clear();
    m_OpenFileList.clear();
    QDir dir;
    QStringList filters;

    switch (type)
    {
    case UPLOAD_SAVE_FILES:
        filters << "*.nrrd" << "*.mrml"<< "*.nii"<<"*.vp"<<"*.acsv";
        break;
    case UPLOAD_OBJ_FILES:
        filters << "*.obj";
        break;
    case OPEN_MRML_FILE:
        filters << "*.mrml"<<"*.nii";
        break;
    default:
        break;
    }
    QString filePath = QCoreApplication::applicationDirPath();
    if (type == OPEN_MRML_FILE){
        filePath = filePath + DOWNLOAD_PATH + "/" + m_UniqueStr;
    }
    else{
        filePath = filePath + SAVE_PATH;
    }

    //dir.setPath(filePath);
    //dir.setNameFilters(filters);
    //QDirIterator iter(dir, QDirIterator::Subdirectories);
    QDirIterator iter(filePath, filters, QDir::Files);
    while (iter.hasNext())
    {
        iter.next();
        QFileInfo info = iter.fileInfo();
        auto filetime  = info.birthTime();
        if (info.isFile())
        {
            m_SaveFileList.append(info.absoluteFilePath());
        }
    }

    // 检查有OBJ文件
    if (type == UPLOAD_OBJ_FILES)
    {
        if (m_SaveFileList.size() != 0)
        {
            filters.clear();
            filters << "*.nrrd" << "*.mrml" << "*.nii" << "*.vp" << "*.acsv";
            dir.setNameFilters(filters);
            QDirIterator iter(dir, QDirIterator::Subdirectories);
            while (iter.hasNext())
            {
                iter.next();
                QFileInfo info = iter.fileInfo();
                if (info.isFile())
                {
                    m_SaveFileList.append(info.absoluteFilePath());
                }
            }
        }
    }
}

void QhttpManager::analyticMRMlfile(tinyxml2::XMLElement *element)
{
    ColorData temp;
    if (element->FirstChildElement() == NULL && strcmp(element->Name(), "Model") == 0)
    {
        temp.name = element->Attribute("name");
    }
    if (element->FirstChildElement() == NULL && strcmp(element->Name(), "ModelDisplay") == 0)
    {
        temp.color = element->Attribute("color");
    }
    if (temp.color != "")
    {
        colorList.push_back(temp.color);
    }
    if (temp.name != "")
    {
        nameList.push_back(temp.name);
    }
    for (tinyxml2::XMLElement* ptrElement = element->FirstChildElement(); ptrElement; ptrElement = ptrElement->NextSiblingElement())
    {
        analyticMRMlfile(ptrElement);
    }
    return;
}

void QhttpManager::String_Split(std::string s, std::string delim, vector<std::string>& ans)
{
    std::string::size_type pos_1, pos_2 = 0;
    while (pos_2 != s.npos) {
        pos_1 = s.find_first_not_of(delim, pos_2);
        if (pos_1 == s.npos) break;
        pos_2 = s.find_first_of(delim, pos_1);
        ans.push_back(s.substr(pos_1, pos_2 - pos_1 + 1));
    }
}

std::string QhttpManager::toHexColor(vector<std::string>& colors)
{
    double r = atof(colors[0].c_str()) * 255.0;
    double g = atof(colors[1].c_str()) * 255.0;
    double b = atof(colors[2].c_str()) * 255.0;
    auto R = static_cast<unsigned int>(r);
    auto G = static_cast<unsigned int>(g);
    auto B = static_cast<unsigned int>(b);
    std::stringstream ss;
    ss << "#";
    ss << std::hex << (R << 16 | G << 8 | B);
    return ss.str();

}

void QhttpManager::slot_DownLoadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    qDebug() << "slot_DownLoadProgress" << bytesReceived<< bytesTotal;
}

void QhttpManager::slot_DownLoadError(QNetworkReply::NetworkError code)
{
    QMessageBox::information(NULL, " download error", "error code:"+code);
}

void QhttpManager::slot_DownLoadFinish(const QString& pathName)
{
    //当前下载如果还有继续下载
    m_UrlList.pop_front();
    if (m_UrlList.size()>0)
    {
        downloadNiifile();
    }
    else
    {
        //下载完成查找有没有meml文件
        findFile(OPEN_MRML_FILE);   
        if (!m_SaveFileList.isEmpty())
        {
            //判断打开nii 还是mrml
            for ( int i = 0; i < m_SaveFileList.size(); i++)
            {
                if (m_SaveFileList[i].contains(".mrml"))
                {
                    m_OpenFileList.clear();
                    m_OpenFileList.append(m_SaveFileList[i]);
                }
            }
            if (m_OpenFileList.isEmpty())
            {
                m_OpenFileList = m_SaveFileList;
            }
        }
        RecordInformationTool::getInstance()->setOpenFilePathList(m_OpenFileList);
        qSlicerApplication::application()->ioManager()->openAddDataDialog();
        emit signal_DownLoadFinish();
    }
}
