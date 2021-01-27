#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <random>
#include <QtCharts>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <algorithm>
#include <QDebug>
#include <math.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <thread>
#include <future>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setBins();
    void NBinChanged();
    void cumulative();
    void checkChange();
    void eraseCumu();
    void NBinSet();
    void spawnRequestThread();
    bool sendHTTPRequest();
    bool readData(QNetworkReply *reply);
    QVector<qreal>* createData(int num, QString theType);
    void pointChange();
    void pointSet();
    void dataRecieved(QNetworkReply *reply);
public slots:
    void buttonClicked(QString text);

private:
    Ui::MainWindow *ui;
    QChart *chart;
    QBarCategoryAxis *axisX;
    QValueAxis *y;
    QValueAxis *rightY;
    QVector<qreal>* dist;
    QLineSeries* cumu;
    int bins;
    int points;
    int recv;
    bool check;
    bool web;
    const int MAXLABEL = 0;
    const int RECVPOINT = 2;
    const int POINTSLIDER = 1;
    QSlider * pointSlide;
    QLabel * maxPoints;
    QLabel * recvPoints;
    QString token = "";
    QString stationID = "COOP:010008";
    QNetworkAccessManager* manager;
    QNetworkRequest request;
    QString url = "https://www.ncdc.noaa.gov/cdo-web/api/v2/data?datasetid=PRECIP_15&stationid=COOP:010008&startdate=2012-01-01&enddate=2012-12-31&limit=%1";
};

#endif // MAINWINDOW_H
