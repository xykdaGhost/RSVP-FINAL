#include "Uart.h"
#include <QDebug>
#include <QThread>

Uart::Uart()
{
}

Uart::~Uart()
{
    port->close();
    port->deleteLater();
}

void Uart::init()
{
    port->setPortName("/dev/ttyTHS0");
    port->setBaudRate(QSerialPort::Baud115200);
    port->setDataBits(QSerialPort::Data8);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);
    port->setParity(QSerialPort::OddParity);

    if (port->open(QIODevice::ReadWrite)) {
        qDebug() << "Port have been opened";
    } else {
        qDebug() << "open it failed";
    }
    connect(port, SIGNAL(readyRead()), this, SLOT(handle_data()), Qt::DirectConnection);
}

void Uart::handle_data()
{
    QByteArray data = port->readAll();
    qDebug() << "receive : " << data.toHex();
    on_receive(data);
}

void Uart::doWork()
{
    port = new QSerialPort();
    init();
    connect(&ServiceManager::getInstance(), SIGNAL(requestUartMessageSignal(int)), this, SLOT(sendUartMessageHandler(int)), Qt::QueuedConnection);
    ask_date();
}

char BCDtoUINT (char p) {
    return ((p>>4)*10 + (p&0x0f));
}

void Uart::on_receive(QByteArray tmpdata) {
    ServiceManager::getInstance().syncParamManager();
    if (tmpdata[0] == 0xea) {
        if (tmpdata[1] == 0x02) {
            if (tmpdata[2] == 0x00) {
                if (tmpdata[3] == 0x90) {
                    ack_shoot();
                    ServiceManager::getInstance().requestAcquireImage();
                }
                else if (tmpdata[3] == 0x11) {
                    ack_controlStatus(2-ServiceManager::getInstance().getGlobalControlParam().isStart);
                }
                else if (tmpdata[3] == 0x23) {
                    ack_modeStauts(ServiceManager::getInstance().getGlobalControlParam().workMode);
                }
                else if (tmpdata[3] == 0x72) {
                    ack_saveStatus(ServiceManager::getInstance().getAlgParam().yolo << 1 | ServiceManager::getInstance().getCaptureParam().saveRaw);
                }
                else if (tmpdata[3] == 0x53) {
                    ack_paramStatus(ServiceManager::getInstance().getAceParam().maxExpTime,
                                    ServiceManager::getInstance().getAceParam().mannualExpTime,
                                    ServiceManager::getInstance().getAceParam().mannualGain,
                                    ServiceManager::getInstance().getCaptureParam().shootInterval,
                                    ServiceManager::getInstance().getCaptureParam().saveRawInterval,
                                    ServiceManager::getInstance().getAlgParam().autoExpo+1);
                }
                else if (tmpdata[3] == 0x80) {
                    ack_speed();

                }
                else if (tmpdata[3] == 0xb1) {
                    QStorageInfo storage= QStorageInfo::mountedVolumes().at(3);
                    storage.refresh();
                    if (storage.isReadOnly()) {
                        qDebug() << "isReadOnly:" << storage.isReadOnly();
                    }

                    ServiceManager::getInstance().setStorage(storage.bytesAvailable()/1000/1000/1000);
                    QDateTime now = QDateTime::currentDateTime();
                    int seconds = ServiceManager::getInstance().getSystemInfoParam().startTime.secsTo(now);
                    ServiceManager::getInstance().setUpTime(seconds);
                    int hours = seconds / 3600;
                    seconds -= hours * 3600;
                    int mins = seconds /60;

                    int left = ServiceManager::getInstance().getSystemInfoParam().storage * 1024 /  18;
                    ack_storage(ServiceManager::getInstance().getSystemInfoParam().storage,
                                ServiceManager::getInstance().getShootInfoParam().pictureCount,
                                hours, mins, left);
                }
                else if (tmpdata[3] == 0xb5) {
                    ack_recovery();
                }
            }
            else if (tmpdata[2] == 0x63) {
                if (tmpdata[3] == 0x01) {
                    ack_search();
                }
                else if (tmpdata[3] == 0x20) {
                    ack_heart();
                }
            }
            else if (tmpdata[2] == 0x07) {
                    ack_connect(1);
            }


        }
        else if (tmpdata[1] == 0x0a) {

            if (tmpdata[3] == 0x41) {

                QString date = "sudo date -s \"" +
                QString::number(BCDtoUINT(tmpdata.data()[4])*100 + BCDtoUINT(tmpdata.data()[5])) + "-" +
                QString::number(BCDtoUINT(tmpdata.data()[6])) + "-" +
                QString::number(BCDtoUINT(tmpdata.data()[7])) + " " +
                QString::number(BCDtoUINT(tmpdata.data()[9])) + ":" +
                QString::number(BCDtoUINT(tmpdata.data()[10])) + ":" +
                QString::number(BCDtoUINT(tmpdata.data()[11])) + "\"";
                qDebug() << date;

                QProcess::startDetached(date);
                QProcess::startDetached("hwclock -w");
                QProcess::startDetached("sync");

                if (ServiceManager::getInstance().getGlobalControlParam().scheduleCount == 0) {
                    ServiceManager::getInstance().increaseScheduleCount();
                    ServiceManager::getInstance().setStartTime(QDateTime::currentDateTime());
                }
                ack_date();
            }



        }
        else if (tmpdata[1] == 0x03) {
            if (tmpdata[3] == 0x20) {
                ServiceManager::getInstance().setWorkMode(tmpdata[4]);
                ServiceManager::getInstance().requestChangeWorkModeButton(tmpdata[4]);
                ack_mode();
            }
            else if (tmpdata[3] == 0x10) {
                qDebug() << "isStart: " << tmpdata[4];
                if (tmpdata[4] == 1) {
                    ServiceManager::getInstance().setIsStart(true);
                } else {
                    ServiceManager::getInstance().setIsStart(false);
                }
                ack_control();
                ServiceManager::getInstance().requestChangeStartButton(tmpdata[4]);
            }
            else if (tmpdata[3] == 0x11) {
                ack_controlStatus(2-ServiceManager::getInstance().getGlobalControlParam().isStart);
            }
            else if (tmpdata[3] == 0x80) {

                ack_speed();
            }
            else if (tmpdata[3] == 0x70) {

                ack_save();
            }

        }
        else if (tmpdata[1] == 0x0c) {

            if (tmpdata[3] == 0x50) {

                ParamManage::getInstance().model()->getRootItem()->child(0)->child(0)->setData(tmpdata[4]*256 + tmpdata[5], 1);
                ParamManage::getInstance().model()->getRootItem()->child(0)->child(1)->setData(tmpdata[6]*256 + tmpdata[7], 1);
                ParamManage::getInstance().model()->getRootItem()->child(0)->child(2)->setData((int)tmpdata[8], 1);
                ParamManage::getInstance().model()->getRootItem()->child(2)->child(0)->setData(tmpdata[9]*256 + tmpdata[10], 1);
                ParamManage::getInstance().model()->getRootItem()->child(2)->child(4)->setData(tmpdata[11]*256 + tmpdata[12], 1);
                if(tmpdata[13]==0x01) { ParamManage::getInstance().model()->getRootItem()->child(3)->child(2)->setData(true, 1);}
                else if (tmpdata[13] == 0x02) { ParamManage::getInstance().model()->getRootItem()->child(3)->child(2)->setData(false, 1);}
                ServiceManager::getInstance().syncParamManager();
                ParamManage::getInstance().updateJsonRoot();
                ParamManage::getInstance().writeJsonToFile("/home/mv/release/settings.json");

            }
        }

    }
}


void Uart::ack_search() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0xe3;
    data[3] = 0x01;
    data[4] = (data[2]+data[3])%256;
    data[5] = 0xeb;
    qDebug() << "write thread is :" << QThread::currentThread();
    qDebug() << "can write ? :" << port->isWritable();
    port->write(data, 6);

    //qDebug() << "ack serach";
}

void Uart::ack_heart() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0xe3;
    data[3] = 0x20;
    data[4] = (data[2]+data[3])%256;
    data[5] = 0xeb;
    port->write(data, 6);

//    qDebug() << "heart checked";
}

void Uart::ack_control() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0x80;
    data[3] = 0x10;
    data[4] = (data[2]+data[3])%256;
    data[5] = 0xeb;
    port->write(data, 6);

//    qDebug() << "ack control";
}

void Uart::ack_controlStatus(uchar m) {
    QByteArray data;
    data.resize(7);
    data[0] = 0xea;
    data[1] = 0x03;
    data[2] = 0x80;
    data[3] = 0x11;
    data[4] = m;
    data[5] = (data[2]+data[3]+data[4])%256;
    data[6] = 0xeb;
    port->write(data, 7);

//    qDebug() << "ack control status";
}

void Uart::ask_control(uchar m) {
    QByteArray data;
    data.resize(7);
    data[0] = 0xea;
    data[1] = 0x03;
    data[2] = 0x00;
    data[3] = 0x11;
    data[4] = m;
    data[5] = (data[2]+data[3]+data[4])%256;
    data[6] = 0xeb;
    port->write(data, 7);

   // qDebug() << "ask control" << " m: " << m;
}

void Uart::ack_mode() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0x80;
    data[3] = 0x20;
    data[4] = 0xa0;
    data[5] = 0xeb;
    port->write(data, 6);

//    qDebug() << "ack mode";
}

void Uart::ack_modeStauts(uchar m) {
    QByteArray data;
    data.resize(7);
    data[0] = 0xea;
    data[1] = 0x03;
    data[2] = 0x80;
    data[3] = 0x23;
    data[4] = m;
    data[5] = (data[2]+data[3]+data[4])%256;
    data[6] = 0xeb;
    port->write(data, 7);

//    qDebug() << "ack mode status";
}

void Uart::ask_mode(uchar m) {
    QByteArray data;
    data.resize(7);
    data[0] = 0xea;
    data[1] = 0x03;
    data[2] = 0x00;
    data[3] = 0x26;
    data[4] = m;
    data[5] = (data[2]+data[3]+data[4])%256;
    data[6] = 0xeb;
    port->write(data, 7);

//    qDebug() << "ask mode";
}

void Uart::ack_date() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0x80;
    data[3] = 0x40;
    data[4] = 0xc0;
    data[5] = 0xeb;
    port->write(data, 6);

//    qDebug() << "ack date";
}

void Uart::ask_date() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0x00;
    data[3] = 0x41;
    data[4] = 0x41;
    data[5] = 0xeb;
    port->write(data, 6);

//    qDebug() << "ask date";
}

void Uart::ack_param() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0x80;
    data[3] = 0x50;
    data[4] = 0xd0;
    data[5] = 0xeb;
    port->write(data, 6);

//    qDebug() << "ack param";
}

void Uart::ack_paramStatus(int expoTime, int maxExpoTime, int gain, int shootInterval, int saveInterval, int mode) {
    QByteArray data;
    data.resize(16);
    data[0] = 0xea;
    data[1] = 12;
    data[2] = 0x80;
    data[3] = 0x53;
    data[4] = (uchar)(expoTime / 256);
    data[5] = (uchar)(expoTime % 256);
    data[6] = (uchar)(maxExpoTime / 256);
    data[7] = (uchar)(maxExpoTime % 256);
    data[8] = (uchar)gain;
    data[9] = (uchar)(shootInterval / 256);
    data[10] = (uchar)(shootInterval % 256);
    data[11] = (uchar)(saveInterval / 256);
    data[12] = (uchar)(saveInterval % 256);
    data[13] = (uchar)mode;
    data[14] = (data[2]+data[3]+data[4]+data[5]+data[6]+data[7]+data[8]+data[9]+data[10]+data[11]+data[12]+data[13])%256;
    data[15] = 0xeb;
    port->write(data, 16);

//    qDebug() << "ack param status";
}

void Uart::ask_param(int expoTime, int maxExpoTime, int gain, int shootInterval, int saveInterval, int mode) {
    QByteArray data;
    data.resize(16);
    data[0] = 0xea;
    data[1] = 0x0c;
    data[2] = 0x00;
    data[3] = 0x58;
    data[4] = (uchar)(expoTime / 256);
    data[5] = (uchar)(expoTime % 256);
    data[6] = (uchar)(maxExpoTime / 256);
    data[7] = (uchar)(maxExpoTime % 256);
    data[8] = (uchar)gain;
    data[9] = (uchar)(shootInterval / 256);
    data[10] = (uchar)(shootInterval % 256);
    data[11] = (uchar)(saveInterval / 256);
    data[12] = (uchar)(saveInterval % 256);
    data[13] = (uchar)mode;
    data[14] = (data[2]+data[3]+data[4]+data[5]+data[6]+data[7]+data[8]+data[9]+data[10]+data[11]+data[12]+data[13])%256;
    data[15] = 0xeb;
    port->write(data, 16);

//    qDebug() << "ask param : " << data.toHex();
//    qDebug() << "shootInterval: " << shootInterval;
//    qDebug() << "saveInterval: " << saveInterval;
//    qDebug() << "autoExpo:" << mode << ParamManage::getInstance().model()->paramStruct().alg.autoExpo;
}

void Uart::ask_result(int *result) {
    QByteArray data;
    data.resize(18);
    data[0] = 0xea;
    data[1] = 0x0e;
    data[2] = 0x00;
    data[3] = 0x62;
    for (int i = 0; i < 12; i++)
    {
        data[i+4] = (uchar)result[i];
    }
    data[16] = (data[2]+data[3]+data[4]+data[5]+data[6]+data[7]+data[8]+data[9]+data[10]+data[11]+data[12]+data[13]+data[14]+data[15])%256;
    data[17] = 0xeb;
    port->write(data, 18);

    qDebug() << "ask result : " << data.toHex() ;
}

void Uart::ack_save() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0x80;
    data[3] = 0x70;
    data[4] = 0xf0;
    data[5] = 0xeb;
    port->write(data, 6);

//    qDebug() << "ack save";
}

void Uart::ack_saveStatus(uchar m) {
    QByteArray data;
    data.resize(7);
    data[0] = 0xea;
    data[1] = 0x03;
    data[2] = 0x80;
    data[3] = 0x72;
    data[4] = m;
    data[5] = (data[2] + data[3] + data[4])%256;
    data[6] = 0xeb;
    port->write(data, 7);

//    qDebug() << "ack save status -> " << "value : " << m;
}

void Uart::ask_save(uchar m) {
    QByteArray data;
    data.resize(7);
    data[0] = 0xea;
    data[1] = 0x03;
    data[2] = 0x00;
    data[3] = 0x75;
    data[4] = m;
    data[5] = (data[2] + data[3] + data[4])%256;
    data[6] = 0xeb;
    port->write(data, 7);

//    qDebug() << "ask save status: " << data.toHex();
}

void Uart::ack_connect(uchar m) {
    QByteArray data;
    data.resize(7);
    data[0] = 0xea;
    data[1] = 0x03;
    data[2] = 0x87;
    data[3] = 0x02;
    data[4] = m;
    data[5] = (data[2] + data[3] + data[4])%256;
    data[6] = 0xeb;
    port->write(data, 7);

//    qDebug() << "ack save status";
}

void Uart::ack_speed() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0x80;
    data[3] = 0x80;
    data[4] = 0x00;
    data[5] = 0xeb;
    port->write(data, 6);

//   qDebug() << "ack speed";
}

void Uart::ack_shoot() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0x80;
    data[3] = 0x90;
    data[4] = (data[2] + data[3])%256;
    data[5] = 0xeb;
    port->write(data, 6);

    //qDebug() << "ack_shoot success";
}

void Uart::ack_storage(int size, int num, int hour, int minute, int avliableNum) {
    QByteArray data;
    data.resize(14);
    data[0] = 0xea;
    data[1] = 0x0a;
    data[2] = 0x80;
    data[3] = 0xb1;
    data[4] = (uchar)(size/256);
    data[5] = (uchar)(size%256);
    data[6] = (uchar)(num/256);
    data[7] = (uchar)(num%256);
    data[8] = (uchar)hour;
    data[9] = (uchar)minute;
    data[10] = (uchar)(avliableNum/256);
    data[11] = (uchar)(avliableNum%256);
    data[12] = (data[2]+data[3]+data[4]+data[5]+data[6]+data[7]+data[8]+data[9]+data[10]+data[11])%256;
    data[13] = 0xeb;
    port->write(data, 14);

//    qDebug() << "ack storage";
}

void Uart::ack_recovery() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0x80;
    data[3] = 0xb5;
    data[4] = (data[2]+data[3])%256;
    data[5] = 0xeb;
    port->write(data, 6);

//    qDebug() << "ack recovery";
}

void Uart::ask_grabAgain() {
    QByteArray data;
    data.resize(6);
    data[0] = 0xea;
    data[1] = 0x02;
    data[2] = 0x00;
    data[3] = 0xb9;
    data[4] = (data[2]+data[3])%256;
    data[5] = 0xeb;
    port->write(data, 6);
    ServiceManager::getInstance().requestAcquireImage();

    //qDebug() << "request again";
}

void Uart::sendUartMessageHandler(int id) {
    ServiceManager::getInstance().syncParamManager();
    switch (id) {
    case 1:
        ack_search();
        break;
    case 2:
        ack_heart();
        break;
    case 3:
        ack_control();
        break;
    case 4:
        ack_controlStatus(2-ServiceManager::getInstance().getGlobalControlParam().isStart);
        break;
    case 5:
        ask_control(2-ServiceManager::getInstance().getGlobalControlParam().isStart);
        break;
    case 6:
        ack_mode();
        break;
    case 7:
        ack_modeStauts(ServiceManager::getInstance().getGlobalControlParam().workMode);
        break;
    case 8:
        ask_mode(ServiceManager::getInstance().getGlobalControlParam().workMode);
        break;
    case 9:
        ack_date();
        break;
    case 10:
        ask_date();
        break;
    case 11:
        ack_param();
        break;
    case 12:
        ack_paramStatus(ServiceManager::getInstance().getAceParam().mannualExpTime, ServiceManager::getInstance().getAceParam().maxExpTime,
                        ServiceManager::getInstance().getCameraParam().gain, ServiceManager::getInstance().getCaptureParam().shootInterval,
                        ServiceManager::getInstance().getCaptureParam().saveRawInterval, ServiceManager::getInstance().getAlgParam().autoExpo);
        break;
    case 13:
        ask_param(ServiceManager::getInstance().getAceParam().mannualExpTime, ServiceManager::getInstance().getAceParam().maxExpTime,
                  ServiceManager::getInstance().getCameraParam().gain, ServiceManager::getInstance().getCaptureParam().shootInterval,
                  ServiceManager::getInstance().getCaptureParam().saveRawInterval, ServiceManager::getInstance().getAlgParam().autoExpo);
        break;
    case 14:{

        ask_result(ServiceManager::getInstance().getResultParam().result);
        break;
    }
    case 15:
        ack_save();
        break;
    case 16:
        ack_saveStatus(ServiceManager::getInstance().getAlgParam().yolo << 1 | ServiceManager::getInstance().getCaptureParam().saveRaw);
        break;
    case 17:
        ask_save(ServiceManager::getInstance().getAlgParam().yolo << 1 | ServiceManager::getInstance().getCaptureParam().saveRaw);
        break;
    case 18:
        ack_connect(ServiceManager::getInstance().getSystemInfoParam().isConnectCamera);
        break;
    case 19:
        ack_speed();
        break;
    case 20:
        ack_shoot();
        break;
    case 21:
    {
        int seconds = ServiceManager::getInstance().getSystemInfoParam().upTime;
        int hours = seconds / 3600;
        seconds -= hours * 3600;
        int mins = seconds /60;
        ack_storage(ServiceManager::getInstance().getSystemInfoParam().storage,
                    ServiceManager::getInstance().getShootInfoParam().pictureCount,
                    hours, mins,
                    ServiceManager::getInstance().getSystemInfoParam().storage * 1024 /  18);
        break;
    }
    case 22:
        ack_recovery();
        break;
    case 23:
        ask_grabAgain();
        break;
    default:
        break;
    }
}
