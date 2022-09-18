#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "UartNet/Uart.h"
#include "ServiceManager/ServiceManager.h"
#include "Camera/GenCamera.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT


public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    //the work mode enum
    virtual void timerEvent(QTimerEvent *event) override;
    void executeReset();

public slots:
    void syncResultModelHandler();
    void changeWorkModeButtonHandler(int workMode);

private:
    Ui::MainWindow *ui;
    void updateParameter();
    int _timerId;
    bool _analysis;
    Uart * myUart;
    GenCamera * camera;
    ServiceManager * manager;
};
#endif // MAINWINDOW_H
