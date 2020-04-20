#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pbGo_clicked()
{
    const QPixmap *input = ui->lblInputImage->pixmap();
    QImage inputImage = input->toImage();

    uchar *bits = inputImage.bits();
    int size = inputImage.byteCount();

    for (int i = 0; i < size; ++i)
    {
        bits[i] = bits[i] ^ 0x80;
    }

    ui->lblOutputImage->setPixmap(QPixmap::fromImage(inputImage));
}
