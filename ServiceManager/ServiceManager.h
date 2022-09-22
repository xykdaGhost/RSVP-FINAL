#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QDateTime>
#include "../TableModel/ResultModel.h"


typedef struct {
    int gain;
    std::string filePath;
} cameraParam;

typedef struct {
    int maxExpTime;
    int mannualExpTime;
    int mannualGain;
    int speed;
} aceParam;

typedef struct {
    int shootInterval;
    std::string savePath;
    bool saveRaw;
    int saveRawInterval;
} captureParam;

typedef struct {
    bool hdr;
    bool yolo;
    bool autoExpo;
} algParam;

typedef struct {
    int amount;
    int density;
    int maxSize;
    ResultModel * model;
    int result[12];
    std::vector<std::pair<int, double>> detectRes;
    std::vector<double> maxBox;
    int level;
} resultParam;

typedef struct {
    QDateTime startTime;
    bool isConnectCamera;
    int storage;
    int upTime;
} systemInfoParam;

typedef struct {
    int pictureCount;
    bool folderFlag;
    int shootExpoTime;
    int shootnGain;
    QDateTime startShootTime;
} shootInfoParam;

typedef struct {
    int workMode;
    bool isStart;
    int scheduleCount;

} globalControlParam;

#define WORKMODE_WORK   1
#define WORKMODE_SHOW   2
#define WORKMODE_DEBUG  3


class ServiceManager : public QObject
{
    Q_OBJECT
public:
    static ServiceManager& getInstance() {
        static ServiceManager manager;
        return manager;
    }
    ~ServiceManager();
    void requestUartMessage(int id);
    cameraParam getCameraParam();
    aceParam getAceParam();
    captureParam getCaptureParam();
    algParam getAlgParam();
    resultParam getResultParam();
    systemInfoParam getSystemInfoParam();
    shootInfoParam getShootInfoParam();
    globalControlParam getGlobalControlParam();
    void setSaveRaw(bool v);
    void setYolo(bool v);
    void setAutoExpo(bool v);
    void setIsStart(bool v);
    void setWorkMode(int v);
    void setShootExpoTime(int v);
    void setShootGain(int v);
    void setFolderFlag(bool v);
    void setStartShootTime(QDateTime v);
    void increasePictureCount();
    void increaseScheduleCount();
    void setStorage(int v);
    void setStartTime(QDateTime v);
    void setDensity(int v);
    void setAmount(int v);
    void setMaxSize(int v);
    void setResultModelPointer(ResultModel * p);
    void syncParamManager();
    void setUpTime(int v);
    void setDetectRes(std::vector<std::pair<int, double>> v1, std::vector<double> v2);
    void setResultArray(int* v);
    void setCameraConnect(bool v);
    void setLevel(int v);

signals:
    void requestUartMessageSignal(int id);
    void requestSyncResultModel();
    void requestAcquireImage();
    void requestChangeWorkModeButton(int workMode);
    void requestChangeStartButton(int isStart);

private:
    explicit ServiceManager();
    cameraParam myCameraParam;
    aceParam myAceParam;
    captureParam myCaptureParam;
    algParam myAlgParam;
    resultParam myResultParam;
    systemInfoParam mySystemInfoParam;
    shootInfoParam myShootInfoParam;
    globalControlParam myGlobalControlParam;

};

#endif // UARTREQUEST_H
