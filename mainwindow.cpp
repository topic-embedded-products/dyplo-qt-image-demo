#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&dip, SIGNAL(renderedImage(QImage)), this, SLOT(updateOutput(QImage)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateOutput(const QImage &image)
{
    QPixmap pixmap = QPixmap::fromImage(image);
    /*
     * The data in the image object is only valid in this scope, and will be
     * lost once this method returns. Qt will attempt to keep only a pointer
     * to the data, unless we call "detach" to make a copy explicitly.
     * See: https://doc.qt.io/qt-5/implicit-sharing.html
     */
    pixmap.detach();
    ui->lblOutputImage->setPixmap(pixmap);
}

void MainWindow::on_pbGo_clicked()
{
    const QPixmap *input = ui->lblInputImage->pixmap();
    QImage inputImage = input->toImage();
    ui->lblOutputImage->setText("");

    try {
        dip.processImageSync(inputImage);
    }
    catch (const std::exception &ex)
    {
        QMessageBox msgBox;
        msgBox.setText(ex.what());
        msgBox.exec();
    }
}

void MainWindow::on_pbQuit_clicked()
{
    close();
}

void MainWindow::on_pbGoAsync_clicked()
{
    const QPixmap *input = ui->lblInputImage->pixmap();
    QImage inputImage = input->toImage();
    ui->lblOutputImage->setText("");

    try {
        dip.processImageASync(inputImage);
    }
    catch (const std::exception &ex)
    {
        QMessageBox msgBox;
        msgBox.setText(ex.what());
        msgBox.exec();
    }
}
