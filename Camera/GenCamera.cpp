#include "GenCamera.h"
#include "../JsonWork/ParamManage.h"
#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/core/core.hpp>
#include <QDebug>
#include <QDateTime>
#include <chrono>
#include "WriteImageThread.h"

#include <QDateTime>
#include <QDir>
#include "../ServiceManager/ServiceManager.h"
using namespace Pylon;
using namespace Basler_UniversalCameraParams;
using namespace GenApi;



// in fact,its a hardware trigger define,if you wants to use the real softwaretrigger
//  _camera->TriggerSource.SetValue(TriggerSource_Line1); needs to be TriggerSource_Software
// and _camera->ExecuteSoftwareTrigger(); needs to be added to simulate the trigger action when you want to get the image.

//#define FixedWhiteBalance

inline cv::Point getTargetPoint(cv::Point pt_origin, cv::Mat warpMatrix) {
    cv::Mat_<double> mat_pt(3, 1);
    mat_pt(0, 0) = pt_origin.x;
    mat_pt(1, 0) = pt_origin.y;
    mat_pt(2, 0) = 1;
    cv::Mat mat_pt_view = warpMatrix * mat_pt;
    double a1 = mat_pt_view.at<double>(0, 0);
    double a2 = mat_pt_view.at<double>(1, 0);
    double a3 = mat_pt_view.at<double>(2, 0);
    return cv::Point(a1 * 1.0 / a3, a2 * 1.0 / a3);
}

inline int minmax(int pixel,int minv,int maxv) {
        pixel = pixel > maxv ? maxv : pixel;
        pixel = pixel < minv ? minv : pixel;
        return pixel;
    }
/**
 * @brief Constructor of GenCamera
 */
GenCamera::GenCamera()
{
    autoExpo = new AutoExposure();
    _yoloAlg = new AlgYolov5s();
}

void GenCamera::camera_init()
{
    PylonInitialize();
    openCamera();
}

void GenCamera::openCamera() {
    try {
        _camera = new CBaslerUniversalInstantCamera(CTlFactory::GetInstance().CreateFirstDevice());
        _camera->RegisterConfiguration(new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);
        _camera->Open();

        // Get camera device information.
        qDebug() << "Camera Device Information" << endl
             << "=========================" << endl;
        qDebug() << "Vendor           : "
             << _camera->DeviceVendorName.GetValue() << endl;
        qDebug() << "Model            : "
             << _camera->DeviceModelName.GetValue() << endl;
        qDebug() << "Firmware version : "
             << _camera->DeviceFirmwareVersion.GetValue() << endl;
        qDebug() << "Format : "
             << _camera->PixelFormat.ToString() << endl;
        // Camera settings.
        qDebug() << "Camera Device Settings" << endl
             << "======================" << endl;
        _camera->MaxNumBuffer = 5;

        _camera->RegisterConfiguration(new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);
        _camera->TriggerSelector.SetValue(TriggerSelector_FrameStart);
        _camera->TriggerMode.SetValue(TriggerMode_On); //TriggerMode_Off TriggerMode_OnŽ¥·¢Ä£Êœ TriggerModeEnums
        _camera->TriggerSource.SetValue(TriggerSource_Line1); // TriggerSource_Line1 TriggerSourceEnums TriggerSource_Software
        _camera->BalanceWhiteAuto.SetValue(BalanceWhiteAuto_Continuous);// BalanceWhiteAuto_Continuous Once
        _camera->TriggerActivation.SetValue(TriggerActivation_RisingEdge);
        _camera->StartGrabbing(GrabStrategy_LatestImageOnly);
        _camera->LightSourceSelector.SetValue(LightSourceSelector_Daylight); //LightSourceSelector_Daylight  LightSourceSelector_Daylight6500K

        vector<double> Mdata{ 0.46464507552316109,0.31322056430355943,-384.00841183616387,0.0000000000000000,-0.91177886268307751,1117.8408856494530,-2.5774733953409634e-19,0.0017305003552682847,1.0000000000000000 };
        M.create(3, 3, CV_64FC1);
        for (int i = 0; i < 3; ++i) {
            double* data = M.ptr<double>(i);
            for (int j = 0; j < 3; ++j) {
                data[j] = Mdata[i * 3 + j];
            }
        }
        cv::invert(M, M_1);

        emit sendStatus("已连接");

    }  catch (const GenericException& e) {
        // Error handling.
        std::cerr << "An exception occurred. at open camera" << std::endl
            << e.GetDescription() << std::endl;
        emit sendStatus("未连接");
    }
}

/**
 * @brief Destructor of GenCamera
 */
GenCamera::~GenCamera() {
    try {
        if(_camera != nullptr)
            _camera->Close();
        PylonTerminate();
    }  catch (const GenericException& e) {
        // Error handling.
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
    }
}


void GenCamera::doWork()
{
    camera_init();
//    while (true) {

//        acquireImage(ServiceManager::getInstance().getResultParam().model);
//    }
    connect(&ServiceManager::getInstance(), SIGNAL(requestAcquireImage()), this, SLOT(acquireImage()), Qt::QueuedConnection);
}

/**
 * @brief Acquire an image from camera
 */
void GenCamera::acquireImage() {
    std::cout << endl;
    std::cout << "new grab" << endl;




    if(!_camera) {
        return;
        ServiceManager::getInstance().setCameraConnect(false);
    }

    if (_camera->IsCameraDeviceRemoved()) {
        return;
        ServiceManager::getInstance().setCameraConnect(false);
    }


    CGrabResultPtr ptr;
    try {
        ServiceManager::getInstance().setCameraConnect(true);
        auto t1 = std::chrono::steady_clock::now();
        if (_camera->IsGrabbing())
        {
            if (_camera->CanWaitForFrameTriggerReady())
            {
                if (_camera->WaitForFrameTriggerReady(500, TimeoutHandling_ThrowException))
                {
                    _camera->RetrieveResult(-1, ptr, TimeoutHandling_ThrowException);
                    if (ptr->GrabSucceeded()) {
                        qDebug() << "grab success " << QDateTime::currentDateTime();
                        if (ServiceManager::getInstance().getGlobalControlParam().scheduleCount == 1) {
                            ServiceManager::getInstance().increaseScheduleCount();
                            ServiceManager::getInstance().setStartShootTime(QDateTime::currentDateTime());
                        }
                    }
                    else {

                        cout << "Error: " << ptr->GetErrorCode() << " " << _ptrGrabBuffer->GetErrorDescription() << endl;

                    }

                }
            }
        }
        //check the image validation
        if(!ptr.IsValid()) { return; }
        //create a Mat image from buffer
        cv::Mat image(ptr->GetHeight(), ptr->GetWidth(), CV_16UC1, ptr->GetBuffer());
        if(image.empty()) { return; }
        //convert the image from bayer to rgb
        cvtColor(image, image, cv::COLOR_BayerRG2RGB);
        image.convertTo(image, CV_16UC3);

//2021.10.20 lmd
        //cv::Mat expoImage = image(cv::Rect(0, 0, 2432, 329));
        /*
         20211128 area cut
         */

        float x = (0.058+ lastt12)*arg(ParamManage::getInstance().model()->paramStruct().aec.speed)*0.1; //+arg(paramManage.model()->paramStruct().capture.interval) //0.058camera delay
        cv::Point r2 = getTargetPoint(cv::Point(0, x+520), M_1);//×óÉÏ
        cv::Point ur = getTargetPoint(cv::Point(362, x+520), M_1);
        cv::Point dr = getTargetPoint(cv::Point(362, x), M_1);
        cv::Point r1 = getTargetPoint(cv::Point(0,x), M_1); //×óÏÂ

        int lp = (r2.x - r1.x) / 2.0 + r1.x;
        lp = minmax(lp,1,1215);
        int ly = minmax(r2.y, 1, 1224);
        int dly = minmax(r1.y, ly, 1224);
        cv::Rect area1(lp,ly, 2432-2*lp, minmax(r1.y-r2.y,2,dly-ly));
        std::cout<<"x:"<<lp<<",y:"<<ly<<",width:"<<2432-2*lp<<",height:"<<minmax(r1.y-r2.y,2,dly-ly)<<std::endl;

        cv::Mat expoImage = image(area1);

        //new folder
        if (ServiceManager::getInstance().getShootInfoParam().folderFlag) {
            QDir * folder = new QDir;

            QString dir = QString::fromStdString(ParamManage::getInstance().model()->paramStruct().capture.savePath) + "/" + ServiceManager::getInstance().getShootInfoParam().startShootTime.toString("yyyyMMdd_hh_mm");
            folder->mkdir(dir);
            folder->mkdir(dir + "/res/");
            folder->mkdir(dir + "/res/ylabel/");
            folder->mkdir(dir + "/raw/");
            ServiceManager::getInstance().setFolderFlag(false);
        }

        //photo name
        std::string photoName = ServiceManager::getInstance().getShootInfoParam().startShootTime.toString("yyyyMMdd_hh_mm").toStdString() + "_"
                + QString("%1").arg(ServiceManager::getInstance().getShootInfoParam().pictureCount, 5, 'g', -1, '0').toStdString()
                + "_" + "e" + std::to_string(ServiceManager::getInstance().getShootInfoParam().shootExpoTime)
                + "_" + "g" + std::to_string(ServiceManager::getInstance().getShootInfoParam().shootnGain)
                + "_" + "s" + std::to_string(ParamManage::getInstance().model()->paramStruct().aec.speed)
                + "_" + "t" + std::to_string(ParamManage::getInstance().model()->paramStruct().capture.interval)
                + "_" + "x" + std::to_string(lp)
                + "_" + "y" + std::to_string(ly)
                + "_" + "w" + std::to_string(2432-2*lp)
                + "_" + "h" + std::to_string(minmax(r1.y-r2.y,2,dly-ly));

        //<date>_<hour>_<minute>_<XXXXX(order)>_<eXXXX(expo time)>_<gXXX(gain)>_<sXX(speed)>_<tXXXX(interval)>
        //20220226_17_01_00394_e 3600_g   86_s  0_t1000_x 302_y 329_w1828_h 895.png

        cv::Mat target = image(cv::Rect(0, 329, 2432, 896));


        if(ParamManage::getInstance().model()->paramStruct().capture.saveRaw) {
//            target.convertTo(target, CV_8UC1);
            std::string writeName = ParamManage::getInstance().model()->paramStruct().capture.savePath + "/" + ServiceManager::getInstance().getShootInfoParam().startShootTime.toString("yyyyMMdd_hh_mm").toStdString() + "/raw/";
            writeName = writeName + photoName + ".png";

            cv::Mat target_raw = target.clone();

            target_raw.convertTo(target_raw, CV_16UC3);
            target_raw = target_raw * 16;

            ServiceManager::getInstance().increasePictureCount();

            qDebug() << "writeName : " << QString().fromStdString(writeName);
            std::vector<int> flag;
            flag.push_back(cv::IMWRITE_PNG_COMPRESSION);
            flag.push_back(0);
            cv::imwrite(writeName, target_raw, flag);

        }


        std::vector<std::pair<int, double>> detectRes;
        std::vector<double> max_box;

        if (ParamManage::getInstance().model()->paramStruct().alg.yolo) {

            _yoloAlg->handleImage(target, target, detectRes, max_box);//, photoName, ParamManage::getInstance().model()->paramStruct().capture.savePath + "/" + ServiceManager::getInstance().getShootInfoParam().shootTime.toString("yyyyMMdd_hh_mm").toStdString() + "/res/ylabel/");
            ServiceManager::getInstance().setDetectRes(detectRes, max_box);
            ServiceManager::getInstance().requestUartMessage(ASK_RESULT);
        }

        ServiceManager::getInstance().requestSyncResultModel();

        //convert the image from cv::Mat in 16bits to QImage in 8bits for display
        QImage sendimage(QSize(2432, 896), QImage::Format::Format_RGB888);
        cv::cvtColor(target, target, COLOR_BGR2RGB);

        quint16* img16 = (quint16*)target.data;

        for(int i = 0; i < 2432*896*3; i++) {
            sendimage.bits()[i] = img16[i] >> 4;
        }

        emit sendImage(sendimage);


        ParamManage::getInstance().updateJsonRoot();
        expoImage = expoImage / 16;
        expoImage.convertTo(expoImage, CV_8UC3);
        //
        cv::Mat gray;

        cv::cvtColor(expoImage, gray, COLOR_RGB2GRAY);

        vector<float> aecRes = autoExpo->getMyNextExpTime(gray, ParamManage::getInstance().model()->paramStruct().aec.speed);
        //
        ServiceManager::getInstance().setShootExpoTime(aecRes[0]*1000);
        ServiceManager::getInstance().setShootGain((int)aecRes[1]);
        qDebug() <<"exptime:aaaa:"<<aecRes[0]*1000<<endl;
        setExposure(aecRes[0]*1000);
        setGain((int)aecRes[1]);
        //setExposure(2000);
        //setGain(0);
        //
        setFixWhiteBalance(aecRes[2]>0.01?false:true);
        qDebug() <<"balance:aaaa:"<<aecRes[2];
        //

        auto t2 = std::chrono::steady_clock::now();
        auto t = t2 - t1;
        lastt12 = (float)t.count();
        std::cout<<"time all:"<<std::chrono::duration<double>(t).count()<<"s"<<std::endl;
        std::cout<<endl;
        std::cout<<endl;

    }  catch (const GenericException& e) {
        // Error handling.
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
        if (ptr->GetErrorCode() == 3774873620) {
            ServiceManager::getInstance().requestUartMessage(ASK_GRABAGAIN);
        }
    }

}

/**
 * @brief Set the exposure parameter of camera
 * @param exposure : the exposure parameter value
 */
void GenCamera::setExposure(int exposure) {
    if(_camera == nullptr)
        return;
    try {
        _camera->ExposureTimeRaw.SetValue(exposure);
        qDebug() << "Exposure updated:" << _camera->ExposureTimeRaw.GetValue() << endl;
    }  catch (const GenericException& e) {
        // Error handling.
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
    }
}

/**
 * @brief Set the gain parameter of camera
 * @param gain : the gain parameter value
 */
void GenCamera::setGain(int gain) {
    if(_camera == nullptr)
        return;
    try {
        _camera->GainRaw.SetValue(gain);
        qDebug() << "Gain updated:" << _camera->GainRaw.GetValue() << endl;
    }  catch (const GenericException& e) {
        // Error handling.
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
    }

}
//2022.1.11 lmd
void GenCamera::setFixWhiteBalance(bool flag) {
    if (_camera == nullptr)
        return;
    if (flag) { //fixed

        _camera->LightSourceSelector.SetValue(LightSourceSelector_Daylight6500K);

//        _camera->BalanceWhiteAuto.SetValue(BalanceWhiteAuto_Continuous);
        _camera->BalanceWhiteAuto.SetValue(BalanceWhiteAuto_Off);
        _camera->BalanceRatioSelector.SetValue(BalanceRatioSelector_Blue);
        _camera->BalanceRatioAbs.SetValue(2.28125);//2.28125
    }
    //auto
    else {
        _camera->BalanceWhiteAuto.SetValue(BalanceWhiteAuto_Continuous);
    }
    cout << "WhiteBalance updated:" << ((flag == true) ? "Fixed":"Auto") << endl;
}


/**
 * @brief Set the ROT parameters of camera
 * @param x : the x position
 * @param y : the y position
 * @param width : the width of image
 * @param height : the height of image
 */
void GenCamera::setROI(int x, int y, int width, int height) {
    if(_camera == nullptr)
        return;
    try {
        _camera->OffsetX.SetValue(x);
        _camera->OffsetY.SetValue(y);
        _camera->Width.SetValue(width);
        _camera->Height.SetValue(height);

        qDebug() << "ROI updated:" << _camera->OffsetX.GetValue() << " "
                                << _camera->OffsetY.GetValue() << " "
                                << _camera->Width.GetValue() << " "
                                << _camera->Height.GetValue() << endl;
    }  catch (const GenericException& e) {
        // Error handling.
        std::cerr << "An exception occurred." << std::endl
            << e.GetDescription() << std::endl;
    }

}
