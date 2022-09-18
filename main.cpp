#include "mainwindow.h"

#include <QApplication>
#include <QPushButton>
#include <iostream>
#include <QThread>


int main(int argc, char *argv[])
{
    QThread::msleep(2300);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
