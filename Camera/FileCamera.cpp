#include "FileCamera.h"
#include <iostream>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include "../JsonWork/ParamManage.h"
#include <dirent.h>
#include <ctime>
#include <sys/stat.h>
#include <QDebug>
#include <Analysis/checkresult.h>
#include <vector>
#include "../ServiceManager/ServiceManager.h"


/**
 * @brief Constructor of FileCamera
 */
FileCamera::FileCamera(QObject* parent) :
        Camera(parent),
        _dir(QString::fromStdString(ParamManage::getInstance().model()->paramStruct().camera.path)),
        _dirList(_dir.entryList(QDir::Files)),
        _fileNum(-1),
        _yoloAlg(new AlgYolov5s()){setDir();}

/**
 * @brief Destructor of FileCamera
 */
FileCamera::~FileCamera() {

}

/**
 * @brief Acquire an image from camera and sen it out if needed.
 * @param dir : the direction of getting image
 *          @arg : true : next image
 *          @arg : false : previous image
 */
void FileCamera::acquireImage(bool dir, QTableWidget* widget, ResultModel* model) {
    ParamManage paramManage = ParamManage::getInstance();
    if(_dirList.empty())
        return;
    //the file number in the folder
    QString FileName;
    if(dir) {
        _fileNum++;
        _fileNum %= _dirList.size();
    } else if(!dir && _fileNum > 0) {
        _fileNum--;
    } else if(!dir && _fileNum == 0) {
        _fileNum = _dirList.size() - 1;
    }


    //get the particular image file name from the file list
    FileName = QString::fromStdString(paramManage.model()->paramStruct().camera.path) +"/raw/" + _dirList[_fileNum];
    qDebug() << "image from file:" << FileName << endl;

    //read the image
    cv::Mat imageDisplay;
    cv::Mat image = cv::imread(FileName.toStdString());
    imageDisplay = cv::imread(FileName.toStdString(),cv::IMREAD_UNCHANGED);
    std::cout << "read image";

    QImage sendimage(QSize(2432, 896), QImage::Format::Format_RGB888);

    if(widget) {
//        //convert the image from bayer to rgb
//        cvtColor(image, image, cv::COLOR_BGR2RGB);
        //convert the image from cv::Mat in 16bits to QImage in 8bits for display


        quint16* img16 = (quint16*)image.data;
        std::cout << "handle image";
        for(int i = 0; i < 2432*896*3; i++) {
            sendimage.bits()[i] = img16[i] >> 4;

        }

//        cvtColor(image, image, cv::COLOR_BGR2RGB);
        std::vector<std::pair<int, double>> detectRes;
        std::vector<double> max_box;

        std::cout<<"Ready to enter Yolo"<<std::endl;
        _yoloAlg->handleImage(image, imageDisplay, detectRes,  max_box, _dirList[_fileNum].left(_dirList[_fileNum].size() - 4).toStdString(), paramManage.model()->paramStruct().capture.savePath +
                              "/res/ylabel/");


        QString mlabel = QString::fromStdString(paramManage.model()->paramStruct().camera.path) +
                "/res/mlabel/" + _dirList[_fileNum].left(_dirList[_fileNum].size() - 4) + ".txt";
        QString ylabel = QString::fromStdString(paramManage.model()->paramStruct().camera.path) +
                "/res/ylabel/" + _dirList[_fileNum].left(_dirList[_fileNum].size() - 4) + ".txt";
        //checkResult(mlabel, ylabel, &sendimage, widget); //

        //model->setData(detectRes);
        ServiceManager::getInstance().setDetectRes(detectRes, max_box);
        ServiceManager::getInstance().requestSyncResultModel();
        ServiceManager::getInstance().requestUartMessage(ASK_RESULT);
    } else {
        std::vector<std::pair<int, double>> detectRes;
        std::vector<double> max_box;

        if (ServiceManager::getInstance().getAlgParam().yolo) {
        _yoloAlg->handleImage(image, imageDisplay, detectRes, max_box, _dirList[_fileNum].left(_dirList[_fileNum].size() - 4).toStdString(), paramManage.model()->paramStruct().capture.savePath +
                              "/res/ylabel/");
        }
//        //convert the image from bayer to rgb
        cvtColor(imageDisplay, imageDisplay, cv::COLOR_BGR2RGB);

        //convert the image from cv::Mat in 16bits to QImage in 8bits for display
        quint16* img16 = (quint16*)imageDisplay.data;

        for(int i = 0; i < 2432*896*3; i++) {
            sendimage.bits()[i] = img16[i] >> 8;
        }

        //model->setData(detectRes);
        ServiceManager::getInstance().setDetectRes(detectRes, max_box);
        ServiceManager::getInstance().requestSyncResultModel();
        ServiceManager::getInstance().requestUartMessage(ASK_RESULT);

    }
    emit sendImage(sendimage);
}

/**
 * @brief Scan the address and set the read direction
 */
void FileCamera::setDir() {
    clearDir();
    _fileNum = -1;
    _dir = QString::fromStdString(ParamManage::getInstance().model()->paramStruct().camera.path) + "/raw/";
    _dirList = _dir.entryList(QDir::Files);
}

/**
 * @brief Clear the read direction
 */
void FileCamera::clearDir() {
    _dirList.clear();
}
