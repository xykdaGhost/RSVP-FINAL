#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "JsonWork/ParamManage.h"
#include "Delegates/ValueDelegate.h"
#include "TableModel/ResultModel.h"
#include "Camera/FileCamera.h"
#include "Camera/GenCamera.h"
#include "TableModel/StatusModel.h"
#include <QDateTime>
#include <QStorageInfo>
#include <QProcess>
#include <QThread>
#include <QDebug>

/**
 * @brief Constructor of MainWindow,
 * @param parent : the parent widget
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      manager(&ServiceManager::getInstance()),
      myUart(&Uart::getInstance()),
      camera(&GenCamera::getInstance()),
      ui(new Ui::MainWindow),
      _analysis(false)
{
    ui->setupUi(this);

    ParamManage param = ParamManage::getInstance();

    //user codes from here
    QThread* uartThread = new QThread();
    myUart->moveToThread(uartThread);
    connect(uartThread, SIGNAL(started()), myUart, SLOT(doWork()));
    connect(uartThread, SIGNAL(finished()), uartThread, SLOT(deleteLater()));
    uartThread->start();

    StatusModel* statusModel = new StatusModel(this);
    ui->statusView->setModel(statusModel);
    ui->statusView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->statusView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->statusView->horizontalHeader()->setHidden(true);
    ui->statusView->setAlternatingRowColors(true);

    connect(&GenCamera::getInstance(), &GenCamera::sendStatus, statusModel, &StatusModel::setCamera);

    QThread* cameraThread = new QThread();
    camera->moveToThread(cameraThread);
    connect(cameraThread, SIGNAL(started()), camera, SLOT(doWork()));
    connect(cameraThread, SIGNAL(finished()), cameraThread, SLOT(deleteLater()));
    cameraThread->start();


    //setup parameter model, view and delegate
    ValueDelegate* delegate = new ValueDelegate(this);

    ui->parameterView->setModel(param.model());
    ui->parameterView->expandAll();
    ui->parameterView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->parameterView->setItemDelegate(delegate);
    ui->parameterView->setEditTriggers(QAbstractItemView::AllEditTriggers);

    //setup result model and view
    ResultModel* result = new ResultModel(this);

    ui->resultView->setFont(QFont("", 16));
    ui->resultView->setModel(result);
    ui->resultView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->resultView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->resultView->setAlternatingRowColors(true);


    //setup mode button
    ui->workModeButton->setEnabled(false);

    ui->analysisWidget->setFont(QFont("", 14));
    ui->analysisWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->analysisWidget->horizontalHeader()->setFont(QFont("", 18));
    ui->analysisWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->analysisWidget->verticalHeader()->setFont(QFont("", 18));
    ui->analysisWidget->setAlternatingRowColors(true);


    //connect button signals


    connect(ui->applyButton, &QPushButton::clicked, this, [=] {
        //on apply action, update the Json root and parameter struct and enable the new parameter
        ParamManage::getInstance().updateJsonRoot();
        ParamManage::getInstance().writeJsonToFile("/home/mv/release/settings.json");
        updateParameter();

        manager->setSaveRaw(param.model()->paramStruct().capture.saveRaw);
        manager->setYolo(ParamManage::getInstance().model()->paramStruct().alg.yolo);

        manager->requestUartMessage(ASK_SAVE);
        manager->setAutoExpo(ParamManage::getInstance().model()->paramStruct().alg.autoExpo);
        manager->syncParamManager();
        manager->requestUartMessage(ASK_PARAM);
    });

    //connect the sendImage signal and display the image in the widow
    connect(&FileCamera::getInstance(), &FileCamera::sendImage, [=] (QImage image) {
        if(ui->pictureLabel->width() / ui->pictureLabel->height() > image.width() / image.height())
            ui->pictureLabel->setPixmap(QPixmap::fromImage(image.scaled(image.width() * ui->pictureLabel->height() / image.height(), ui->pictureLabel->height())));
        else
            ui->pictureLabel->setPixmap(QPixmap::fromImage(image.scaled(ui->pictureLabel->width(), image.height() * ui->pictureLabel->width() / image.width())));
    });
    connect(&GenCamera::getInstance(), &GenCamera::sendImage, [=] (QImage image) {
        if(ui->pictureLabel->width() / ui->pictureLabel->height() > image.width() / image.height())
            ui->pictureLabel->setPixmap(QPixmap::fromImage(image.scaled(image.width() * ui->pictureLabel->height() / image.height(), ui->pictureLabel->height())));
        else
            ui->pictureLabel->setPixmap(QPixmap::fromImage(image.scaled(ui->pictureLabel->width(), image.height() * ui->pictureLabel->width() / image.width())));
    });

    connect(ui->tabWidget, &QTabWidget::currentChanged, [=] (int tab) {
        _analysis = (tab == 1);
    });

    //acquireImage from different camera by work mode
    connect(ui->nextButton, &QPushButton::clicked, this, [=] {
        if(ServiceManager::getInstance().getGlobalControlParam().workMode == WORKMODE_SHOW)
            FileCamera::getInstance().acquireImage(true, _analysis? ui->analysisWidget : nullptr, result);
        else {
            //GenCamera::getInstance().acquireImage();
        }
    });
    connect(ui->previousButton, &QPushButton::clicked, this, [=] {
        if(ServiceManager::getInstance().getGlobalControlParam().workMode == WORKMODE_SHOW)
            FileCamera::getInstance().acquireImage(false, _analysis? ui->analysisWidget : nullptr, result);
    });
    //set timer event, acquire image on every timer period
    connect(ui->playButton, &QPushButton::clicked, this, [=] {
        if(ui->playButton->text() == "启动") {
            ui->playButton->setText("停止");
            ui->nextButton->setEnabled(false);
            ui->previousButton->setEnabled(false);
            if (manager->getGlobalControlParam().workMode == WORKMODE_SHOW) {
                _timerId = startTimer(1300);
            }
            ServiceManager::getInstance().setIsStart(true);
            ServiceManager::getInstance().requestUartMessage(ASK_CONTROL);
        } else {
            ui->playButton->setText("启动");
            ui->nextButton->setEnabled(true);
            ui->previousButton->setEnabled(true);
            ServiceManager::getInstance().setIsStart(false);
            ServiceManager::getInstance().requestUartMessage(ASK_CONTROL);
            killTimer(_timerId);
        }
    });

    //change work mode
    connect(ui->workModeButton, &QPushButton::clicked, this, [=] {
        manager->setWorkMode(WORKMODE_WORK);
        ui->workModeButton->setEnabled(false);
        ui->showModeButton->setEnabled(true);
        ui->debugModeButton->setEnabled(true);
        manager->requestUartMessage(ASK_MODE);
    });
    connect(ui->showModeButton, &QPushButton::clicked, this, [=] {
        manager->setWorkMode(WORKMODE_SHOW);
        ui->workModeButton->setEnabled(true);
        ui->showModeButton->setEnabled(false);
        ui->debugModeButton->setEnabled(true);
        manager->requestUartMessage(ASK_MODE);
    });
    connect(ui->debugModeButton, &QPushButton::clicked, this, [=] {
        manager->setWorkMode(WORKMODE_DEBUG);
        ui->workModeButton->setEnabled(true);
        ui->showModeButton->setEnabled(true);
        ui->debugModeButton->setEnabled(false);
        manager->requestUartMessage(ASK_MODE);
    });

    connect(ui->resetButton, &QPushButton::clicked, this, [=] {
        ParamManage::getInstance().model()->getRootItem()->child(2)->child(4)->setData(4000, 1);  //save interval
        ParamManage::getInstance().model()->getRootItem()->child(2)->child(0)->setData(4000, 1);  //shoot interval
        ParamManage::getInstance().model()->getRootItem()->child(3)->child(1)->setData(true, 1);  //yolo
        ParamManage::getInstance().model()->getRootItem()->child(2)->child(3)->setData(true, 1);  //save
        ServiceManager::getInstance().syncParamManager();
    });

    connect(ui->closeButton, &QPushButton::clicked, this, [=] {this->close();});
    ResultModel* model = static_cast<ResultModel*>(ui->resultView->model());
    ServiceManager::getInstance().setResultModelPointer(model);
    connect(&ServiceManager::getInstance(), SIGNAL(requestSyncResultModel()), this, SLOT(syncResultModelHandler()));
    connect(&ServiceManager::getInstance(), SIGNAL(requestChangeWorkModeButton(int)), this, SLOT(changeWorkModeButtonHandler(int)));
    connect(&ServiceManager::getInstance(), SIGNAL(requestChangeStartButton(int)), this, SLOT(changeStartButtonHandler(int)));
}

/**
 * @brief Destructor of MainWindow
 */
MainWindow::~MainWindow()
{
    delete ui;
}

/**
 * @brief The timerEvent of MainWindow
 * @param event : the active timer event
 */
void MainWindow::timerEvent(QTimerEvent *event)
{
    ResultModel* result = static_cast<ResultModel*>(ui->resultView->model());
    if(event->timerId() == _timerId)
    {
        FileCamera::getInstance().acquireImage(true, _analysis? ui->analysisWidget : nullptr, result);
    }
}

/**
 * @brief Enable the latest parameter, e.g. camera parameters, alg parameters etc.
 */
void MainWindow::updateParameter() {
    struct ParameterModel::Parameter& param = ParamManage::getInstance().model()->paramStruct();
    GenCamera& cam = GenCamera::getInstance();
    FileCamera::getInstance().setDir();
}


void MainWindow::syncResultModelHandler() {
    static_cast<ResultModel*>(ui->resultView->model())->setData(ServiceManager::getInstance().getResultParam().detectRes, ServiceManager::getInstance().getResultParam().maxBox);
}

void MainWindow::changeWorkModeButtonHandler(int workMode) {
    if (workMode == WORKMODE_WORK) {
        //work mode
        ui->workModeButton->setEnabled(false);
        ui->showModeButton->setEnabled(true);
        ui->debugModeButton->setEnabled(true);
    } else if (workMode == WORKMODE_SHOW) {
        //show mode
        ui->showModeButton->setEnabled(false);
        ui->workModeButton->setEnabled(true);
        ui->debugModeButton->setEnabled(true);
    } else if (workMode == WORKMODE_DEBUG) {
        //debug mode
        ui->debugModeButton->setEnabled(false);
        ui->workModeButton->setEnabled(true);
        ui->showModeButton->setEnabled(true);
    }
}

void MainWindow::changeStartButtonHandler(int isStart) {
    if (isStart == true) {
        ui->playButton->setText("停止");
        ui->nextButton->setEnabled(false);
        ui->previousButton->setEnabled(false);
        if (manager->getGlobalControlParam().workMode == WORKMODE_SHOW) {
            _timerId = startTimer(1300);
        }
    }
    else {
        ui->playButton->setText("启动");
        ui->nextButton->setEnabled(true);
        ui->previousButton->setEnabled(true);
        ServiceManager::getInstance().setIsStart(false);
        ServiceManager::getInstance().requestUartMessage(ASK_CONTROL);
        killTimer(_timerId);
    }
}
