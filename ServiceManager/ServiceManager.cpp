#include "ServiceManager.h"
#include "../JsonWork/ParamManage.h"
#include <QDebug>

ServiceManager::ServiceManager()
{

    myShootInfoParam.folderFlag = true;
    myShootInfoParam.pictureCount = 0;
    myShootInfoParam.shootExpoTime = 0;
    myShootInfoParam.shootnGain = 0;
    myResultParam.model = nullptr;

    myGlobalControlParam.workMode = WORKMODE_WORK;

    mySystemInfoParam.isConnectCamera = false;
}

ServiceManager::~ServiceManager()
{

}

void ServiceManager::requestUartMessage(int id)
{
    requestUartMessageSignal(id);
}

cameraParam ServiceManager::getCameraParam(){
    return myCameraParam;
}

aceParam ServiceManager::getAceParam(){
    return myAceParam;
}

captureParam ServiceManager::getCaptureParam(){
    return myCaptureParam;
}

algParam ServiceManager::getAlgParam(){
    return myAlgParam;
}

resultParam ServiceManager::getResultParam(){
    return myResultParam;
}

systemInfoParam ServiceManager::getSystemInfoParam(){
    return mySystemInfoParam;
}

shootInfoParam ServiceManager::getShootInfoParam(){
    return myShootInfoParam;
}

globalControlParam ServiceManager::getGlobalControlParam(){
    return myGlobalControlParam;
}

void ServiceManager::setSaveRaw(bool v) {
    myCaptureParam.saveRaw = v;
}

void ServiceManager::setYolo(bool v) {
    myAlgParam.yolo = v;
}

void ServiceManager::setAutoExpo(bool v) {
    myAlgParam.autoExpo = v;
}

void ServiceManager::setIsStart(bool v) {
    myGlobalControlParam.isStart = v;
}

void ServiceManager::setWorkMode(int v) {
    myGlobalControlParam.workMode = v;
}

void ServiceManager::setFolderFlag(bool v) {
    myShootInfoParam.folderFlag = v;
}

void ServiceManager::setShootExpoTime(int v) {
    myShootInfoParam.shootExpoTime = v;
}

void ServiceManager::setShootGain(int v) {
    myShootInfoParam.shootnGain = v;
}

void ServiceManager::setStartShootTime(QDateTime v) {
    myShootInfoParam.startShootTime = v;
}

void ServiceManager::increasePictureCount() {
    myShootInfoParam.pictureCount++;
}

void ServiceManager::setStorage(int v) {
    mySystemInfoParam.storage = v;
}

void ServiceManager::increaseScheduleCount() {
    myGlobalControlParam.scheduleCount++;
}

void ServiceManager::setStartTime(QDateTime v) {
    mySystemInfoParam.startTime = v;
}

void ServiceManager::setAmount(int v) {
    myResultParam.amount = v;
}

void ServiceManager::setDensity(int v) {
    myResultParam.density = v;
}

void ServiceManager::setMaxSize(int v) {
    myResultParam.maxSize = v;
}

void ServiceManager::setResultModelPointer(ResultModel *p) {
    myResultParam.model = p;
}

void ServiceManager::syncParamManager() {
    myAceParam.maxExpTime           =       ParamManage::getInstance().model()->paramStruct().aec.maxExpTime;
    myAceParam.mannualExpTime       =       ParamManage::getInstance().model()->paramStruct().aec.mannualExpTime;
    myAceParam.mannualGain          =       ParamManage::getInstance().model()->paramStruct().aec.mannualGain;
    myAceParam.speed                =       ParamManage::getInstance().model()->paramStruct().aec.speed;

    myAlgParam.autoExpo             =       ParamManage::getInstance().model()->paramStruct().alg.autoExpo;
    myAlgParam.yolo                 =       ParamManage::getInstance().model()->paramStruct().alg.yolo;

    myCameraParam.gain              =       ParamManage::getInstance().model()->paramStruct().camera.gain;
    myCameraParam.filePath          =       ParamManage::getInstance().model()->paramStruct().camera.path;

    myCaptureParam.shootInterval    =       ParamManage::getInstance().model()->paramStruct().capture.interval;
    myCaptureParam.savePath         =       ParamManage::getInstance().model()->paramStruct().capture.savePath;
    myCaptureParam.saveRawInterval  =       ParamManage::getInstance().model()->paramStruct().capture.saveRawInterval;
    myCaptureParam.saveRaw          =       ParamManage::getInstance().model()->paramStruct().capture.saveRaw;
}

void ServiceManager::setUpTime(int v) {
    mySystemInfoParam.upTime = v;
}

void ServiceManager::setDetectRes(std::vector<std::pair<int, double> > v1, std::vector<double> v2) {
    myResultParam.detectRes = v1;
    myResultParam.maxBox = v2;
}

void ServiceManager::setResultArray(int* v) {
    for (int i = 0; i < 12; i++ ) {
        myResultParam.result[i] = v[i];
    }
}

void ServiceManager::setCameraConnect(bool v) {
    mySystemInfoParam.isConnectCamera = v;
}

void ServiceManager::setLevel(int v) {
    myResultParam.level = v;
}
