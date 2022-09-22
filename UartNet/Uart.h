#ifndef UART_H
#define UART_H

#include <QObject>
#include <QSerialPort>
#include <QDateTime>
#include <QStorageInfo>
#include <QProcess>
#include "../ServiceManager/ServiceManager.h"
#include "../JsonWork/ParamManage.h"

#define ACK_SEARCH          1
#define ACK_HEART           2
#define ACK_CONTROL         3
#define ACK_CONTROLSTAUTS   4
#define ASK_CONTROL         5
#define ACK_MODE            6
#define ACK_MODESTATUS      7
#define ASK_MODE            8
#define ACK_DATE            9
#define ASK_DATE            10
#define ACK_PARAM           11
#define ACK_PARAMSTATUS     12
#define ASK_PARAM           13
#define ASK_RESULT          14
#define ACK_SAVE            15
#define ACK_SAVESTATUS      16
#define ASK_SAVE            17
#define ACK_CONNECT         18
#define ACK_SPEED           19
#define ACK_SHOOT           20
#define ACK_STORAGE         21
#define ACK_RECOVERY        22
#define ASK_GRABAGAIN       23


class Uart : public QObject
{
    Q_OBJECT
public:
    static Uart& getInstance() {
        static Uart uart;
        return uart;
    }
    ~Uart();
    void init();


public slots:
    void handle_data();
    void doWork();

    void sendUartMessageHandler(int id);


private:
    explicit Uart();
    QSerialPort * port;
    volatile bool readFlag;
    bool writeFlag;

    void on_receive(QByteArray tmpdata);
    void ack_search();
    void ack_heart();
    void ack_control();
    void ack_controlStatus(uchar m);
    void ask_control(uchar m);
    void ack_mode();
    void ack_modeStauts(uchar m);
    void ask_mode(uchar m);
    void ack_date();
    void ask_date();
    void ack_param();
    void ack_paramStatus(int expoTime, int maxExpoTime, int gain, int shootInterval, int saveInterval, int mode);
    void ask_param(int expoTime, int maxExpoTime, int gain, int shootInterval, int saveInterval, int mode);
    void ask_result(int *result, int level);
    void ack_save();
    void ack_saveStatus(uchar m);
    void ask_save(uchar m);
    void ack_connect(uchar m);
    void ack_speed();
    void ack_shoot();
    void ack_storage(int size, int num, int hour, int minute, int avliableNum);
    void ack_recovery();
    void ask_grabAgain();

    void closePort();
    void openPort();


};

#endif // UART_H
