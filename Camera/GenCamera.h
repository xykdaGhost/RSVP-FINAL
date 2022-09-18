#ifndef RSVP_GENCAMERA_H
#define RSVP_GENCAMERA_H
#include "Camera.h"

#include <pylon/PylonIncludes.h>
#include <pylon/BaslerUniversalInstantCamera.h>
#include <QImage>
#include "../AutoExpo/AutoExpo.h"
#include "../../yolo/RsvpAlgorithm/AlgYolov5/AlgYolov5.h"
#include "../UartNet/Uart.h"


/**
 * @brief An implement of Camera interface, get pictures from Basler camera
 */
class GenCamera : public QObject {
    Q_OBJECT
public:
    static GenCamera& getInstance() {
        static GenCamera camera;
        return camera;
    }
    ~GenCamera();



public slots:
    void doWork();
    void acquireImage();

signals:
    void sendImage(QImage image);
    void sendStatus(QString status);

private:
    explicit GenCamera();
    Pylon::CBaslerUniversalInstantCamera* _camera;
    Pylon::CGrabResultPtr _ptrGrabBuffer;
    Pylon::CGrabResultPtr _ptrGrabResult;
    void openCamera();
    void camera_init();
//    void acquireImage();
    void setExposure(int exposure);
    void setGain(int gain);
    void setROI(int x, int y, int width, int height);
    void setFixWhiteBalance(bool flag = true);
    AutoExposure* autoExpo;
    AlgYolov5s* _yoloAlg;
    float lastt12=0;
    cv::Mat M_1,M;
};

#endif //RSVP_GENCAMERA_H
