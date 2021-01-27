#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Instansiates the network manager, and connects it to the data handler
    manager = new QNetworkAccessManager();
    connect(manager, &QNetworkAccessManager::finished, this, &MainWindow::dataRecieved);

    //sets strings to be used on the slot call from the signal
    QString uni = "uniform";
    QString norm = "normal";

    //Sets the default value for the bin slider and points slider associated variables
    bins = 20;
    web = false;
    points = 100;
    recv = 0;

    //Connects each button clicked signal to the slot
    connect(ui->uniform, &QRadioButton::clicked, [this, uni] {buttonClicked(uni); });
    connect(ui->normal, &QRadioButton::clicked, [this, norm] {buttonClicked(norm); });
    connect(ui->file, &QRadioButton::clicked, this, &MainWindow::spawnRequestThread);
    //instansiates the private variable chart
    chart = new QChart();
    chart->setTitle("Sample Data Distribution");

    //Instansiate both axes
    axisX = new QBarCategoryAxis();
    y = new QValueAxis();
    axisX->setLabelsAngle(-90);

    rightY = new QValueAxis();

    //Add axes to the chart
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(y, Qt::AlignLeft);

    //Sets legend so data appears in the bottom
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    //Adds chart to chart view, and adds to the UI vertical layout
    ui->chartView->setChart(chart);

    //Sets the parameters for the bin slider
    ui->binSelect->setTickInterval(1);
    ui->binSelect->setRange(4,100);
    ui->binSelect->setValue(20);

    //Sets the parameters for the point slider
    pointSlide = (QSlider *)ui->topLeft->itemAt(POINTSLIDER)->widget();
    pointSlide->setTickInterval(1);
    pointSlide->setRange(1,1000);
    pointSlide->setValue(100);


    //Sets the slider functions for when the sliders change and are released
    connect(ui->binSelect, &QSlider::valueChanged, this, &MainWindow::NBinChanged);
    connect(ui->binSelect, &QSlider::sliderReleased, this, &MainWindow::NBinSet);
    connect(pointSlide, &QSlider::valueChanged, this, &MainWindow::pointChange);
    connect(pointSlide, &QSlider::sliderReleased, this, &MainWindow::pointSet);

    //Sets the default values for the QLabels associated with sliders
    ui->NBins->setText("NBins = 20");

    maxPoints = (QLabel *)ui->topLeft->itemAt(MAXLABEL)->widget();
    recvPoints = (QLabel *)ui->topLeft->itemAt(RECVPOINT)->widget();
    maxPoints->setText("MAXP=100");
    recvPoints->setText("PtsRecv=---");

    //Sets the connection for the cumulative button, and the associated boolean
    connect(ui->cumulative, &QCheckBox::stateChanged, this, &MainWindow::checkChange);
    check = false;
    //Clicks the normal layout radio button for initial chart
    ui->normal->click();
}


MainWindow::~MainWindow()
{
    delete ui;
}

//void MainWindow::checkChange() MRD 10-21-20
//This function handles the checkbox value being changed.
void MainWindow::checkChange()
{
    check = !check;
    if(check)
        cumulative();
    else
        eraseCumu();
}

//void MainWindow::NBinChanged() MRD 10-21-20
//This function handles the bin slider being moved, and updating the text to reflect that
void MainWindow::NBinChanged()
{
    bins = ui->binSelect->value();
    ui->NBins->setText(QStringLiteral("NBins = %1").arg(bins));
}

//void MainWindow::NBinSet()MRD 10-21-20
//This function handles changing the graph when the slider is released.
void MainWindow::NBinSet()
{
    setBins();
    if(check)
        cumulative();
}

//void MainWindow::pointChange() MRD 11-12-20
//This function handles the max points slider being moved, and updating the text to reflect that
void MainWindow::pointChange()
{
    points = pointSlide->value();
    ((QLabel *)ui->topLeft->itemAt(MAXLABEL)->widget())->setText(QStringLiteral("MAXP=%1").arg(points));
}

//void MainWindow::pointSet()MRD 11-12-20
//This function handles changing the max points when the slider is released.
void MainWindow::pointSet()
{
    if(web)
        spawnRequestThread();
}

//void MainWindow::NBinSet()MRD 10-21-20
//This function handles erasing the cumulkative series and right y axis
void MainWindow::eraseCumu()
{
    chart->removeSeries(cumu);
    chart->removeAxis(rightY);
}

//void MainWindow::spawnRequestThread() MRD 11-15-20
//This function spawns a new thread to send a request to recieve data.
//On a failure to send, a debug box is thrown and the window reset.
void MainWindow::spawnRequestThread()
{
    //Spawns a thread to make the request
    auto future = std::async(&MainWindow::sendHTTPRequest, this);
    //Waits for the thread to finish.
    bool success = future.get();
    //On failure, makes a dialogue box and resets the window.
    if(!success)
    {
        QDialog *errorDialog = new QDialog();
        QLabel *label = new QLabel("Failed to send data to server");
        QHBoxLayout *box = new QHBoxLayout();
        box->addWidget(label);
        errorDialog->setLayout(box);
        errorDialog->show();
        errorDialog->raise();
        errorDialog->activateWindow();
        if(check)
            ui->cumulative->click();
        check = false;
        ui->binSelect->setValue(20);
        ui->normal->click();
        pointSlide->setValue(100);
        return;
    }
    //sends out the request
    manager->get(request);
}

//void MainWindow::dataRecieved(QNetworkReply *reply) MRD 11-15-20
//This function spawns a new thread to handle the recieved data, then calls functions to
//update the graphs. On a failure to read the data, a debug box is thrown and the window reset.
void MainWindow::dataRecieved(QNetworkReply *reply)
{
    //Spawn a thread to read the data recieved
    auto future = std::async(&MainWindow::readData, this, reply);
    //Waits for the function to finish
    bool success = future.get();
    //On a failure, makes a new dialogue box and resets the window
    if(!success)
    {
        QDialog *errorDialog = new QDialog();
        QLabel *label = new QLabel("Failed to send data to server");
        QHBoxLayout *box = new QHBoxLayout();
        box->addWidget(label);
        errorDialog->setLayout(box);
        errorDialog->show();
        errorDialog->raise();
        errorDialog->activateWindow();
        if(check)
            ui->cumulative->click();
        check = false;
        ui->binSelect->setValue(20);
        ui->normal->click();
        pointSlide->setValue(100);
        return;
    }
    //Sets the text to the correct number of points recieved
    recvPoints->setText(QString("PtsRecv=%1").arg(recv));
    //sets web to true so the color changes and point slider updates the graph
    web = true;
    //sets up the graphs
    setBins();
    if(check)
        cumulative();
}

//bool MainWindow::readData(QNetworkReply *reply) MRD 11-15-20
//This function reads the data recieved from the HTTP request, and
//stores it
bool MainWindow::readData(QNetworkReply *reply)
{
    if(reply->error() != QNetworkReply::NoError)
        return false;
    //Code taken from https://stackoverflow.com/questions/28499619/qnetworkaccessmanager-parsing-json
    //Takes the message recieved from the server, and puts the data in the array "results" into a JSON array
    QString message = (QString)reply->readAll();
    QJsonDocument jsonReply = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject jsonObject = jsonReply.object();
    QJsonArray data = jsonObject["results"].toArray();
    //Resets points recieved and the QVector of data
    recv = 0;
    dist = new QVector<qreal>();
    //Iterates through the recieved data, and adds it to the QVector if it is valid
    foreach(const QJsonValue &value, data)
    {
        QJsonObject obj = value.toObject();
        double toAdd = obj["value"].toDouble();
        if(toAdd >= 0 && toAdd < 200)
        {
            recv++;
            dist->push_front(toAdd);
        }
    }
    return true;
}

//bool MainWindow::sendHTTPRequest() MRD 11-15-20
//This function creates an HTTP request based on the slider data
bool MainWindow::sendHTTPRequest()
{
    //Tries to set the request, and returns false if an error is thrown
    try{
        request.setUrl(QUrl(QString(url).arg(points)));
        request.setRawHeader(QByteArray("Token"), token.toLocal8Bit());
        return true;
    } catch (QException e) {
        return false;
    }
}

void MainWindow::buttonClicked(QString text)
{
    //sets the http data fields back to default
    web = false;
    recvPoints->setText("PtsRecv=---");
    //creates the distribution data
    dist = createData(10000, text);

    //callse the function to create the graph
    setBins();
    if(check)
        cumulative();
}
void MainWindow::setBins()
{
    //min max code segments found here: https://amin-ahmadi.com/2016/01/19/how-to-find-min-and-max-in-a-vector-qt-stl/
    double min = *std::min_element(dist->constBegin(),dist->constEnd());
    double max = *std::max_element(dist->constBegin(),dist->constEnd());

    // The next lines of code define the arrays to be used, and find the labels and cutoff
    // thresholds for each group.
    double * cutoff = new double[bins+2];
    int *counts = new int[bins];
    for(int i = 0; i < bins; i++)
    {
        counts[i] = 0;
        cutoff[i+1] = (i+1)*(max-min)/(double)bins + min;
    }
    cutoff[0] = min;
    cutoff[bins+1] = max;
    //The data is iterated through and categorized into the correct bin
    QVector<qreal>::iterator it = dist->begin();
    while(it++ != dist->end())
        for(int i = 0; i < bins; i++)
            if(*it <= cutoff[i+1])
            {
                counts[i]++;
                break;
            }

    //Adds the x-axis values and each bar value to the correct array
    QBarSet *data = new QBarSet("Distribution");
    QStringList xlabels;
    for(int i = 0; i < bins; i++)
    {
        *data << counts[i];
        xlabels << QString("%1 to %2").arg(cutoff[i]).arg(cutoff[i+1]);
    }

    //adds the series to the chart after removing all other series
    chart->removeAllSeries();
    QBarSeries *series = new QBarSeries();
    if(web)
      data->setColor(Qt::darkGreen);
    series->append(data);
    chart->addSeries(series);


    //sets the y-axis values
    y->setRange(0, *std::max_element(counts, counts+bins));

    //sets the x-axis
    axisX->clear();
    axisX->append(xlabels);

    //deletes the pointers that were used
    delete[] cutoff;
    delete[] counts;
}

void MainWindow::cumulative()
{
    //min max code segments found here: https://amin-ahmadi.com/2016/01/19/how-to-find-min-and-max-in-a-vector-qt-stl/
    double min = *std::min_element(dist->constBegin(),dist->constEnd());
    double max = *std::max_element(dist->constBegin(),dist->constEnd());

    // The next lines of code define the arrays to be used, and find the labels and cutoff
    // thresholds for each group.
    double * cutoff = new double[bins];
    int *counts = new int[bins];
    for(int i = 0; i < bins; i++)
    {
        counts[i] = 0;
        cutoff[i] = (i+1)*(max-min)/(double)bins + min;
    }
    //The data is iterated through and categorized into the correct bin
    QVector<qreal>::iterator it = dist->begin();
    while(it++ != dist->end())
        for(int i = 0; i < bins; i++)
            if(*it <= cutoff[i])
            {
                counts[i]++;
                break;
            }
    //Finds the cumulative value for each bin, by effectively adding all previous values
    for(int i = 1; i < bins; i++)
    {
        counts[i] += counts[i-1];

    }
    //Sets the line series name and appends the data
    cumu = new QLineSeries();
    cumu->setName("Cumulative");
    for(int i = 0; i < bins; i++)
    {
        cumu->append(i, counts[i]);
    }
    //adds the series to the chart
    chart->addSeries(cumu);

    //sets the right y-axis values
    rightY->setRange(0, counts[bins-1]);

    //Adds the axis if it isn't already added.
    if(chart->axes().length() <= 2)
        chart->addAxis(rightY, Qt::AlignRight);

    //deletes the pointers that were used.
    delete[] cutoff;
    delete[] counts;
}

QVector<qreal>* MainWindow::createData(int num, QString theType)
{
    QVector<qreal> *dat = new QVector<qreal>;
    std::default_random_engine generator;
    // the distribution type is determined by string input
    if (theType.contains("unif"))
    {
        std::uniform_real_distribution<double> distribution(0, 99.9999);
        // params are arbitrarily chosen
        for (int incr = 0; incr < num; incr++)
            dat->push_front(distribution(generator));
    }
    else // default behavior is the normal distribution
    {
        std::normal_distribution<double> distribution(10.0, 2.0);
        // params are arbitrarily chosen
        for (int incr = 0; incr < num; incr++)
            dat->push_front(distribution(generator));
    }
    return dat;
}
