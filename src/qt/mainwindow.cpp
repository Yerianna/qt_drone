#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <opencv/cv.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <QtWidgets>

using namespace cv;
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    Mat inMat;
        inMat = imread("Lenna.jpg");   // Read the file

    cout << inMat.type();
    QImage image( inMat.data, inMat.cols, inMat.rows, inMat.step, QImage::Format_RGB32);

    ui->lblScreenshot->setPixmap(QPixmap::fromImage(image));

    ui->lblScreenshot->adjustSize();



}

MainWindow::~MainWindow()
{
    delete ui;
}


