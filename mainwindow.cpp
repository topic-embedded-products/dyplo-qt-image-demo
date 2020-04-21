#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dyploimageprocessor.h"
#include <QMessageBox>

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

void MainWindow::updateOutput(const QImage &image)
{
    ui->lblOutputImage->setPixmap(QPixmap::fromImage(image));
}

void MainWindow::on_pbGo_clicked()
{
    const QPixmap *input = ui->lblInputImage->pixmap();
    QImage inputImage = input->toImage();

    try {
        DyploImageProcessor p;
        connect(&p, SIGNAL(renderedImage(QImage)), this, SLOT(updateOutput(QImage)));
        p.processImageSync(inputImage);
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
