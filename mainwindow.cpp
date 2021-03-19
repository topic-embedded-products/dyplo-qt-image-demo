#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>

static const char *filterName = "rgb_contrast";

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

/* Synchronous processing, blocks the GUI and if there's no output from logic, it will hang the application */
void MainWindow::on_pbGo_clicked()
{
    const QPixmap *input = ui->lblInputImage->pixmap();
    QImage inputImage = input->toImage();
    ui->lblOutputImage->setText("");

    try {
        dip.processImageSync(inputImage, filterName);
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

/* Asynchronous processing, does not block the UI thread. When the FPGA has processed the data in background, updateOutput will be called on the UI thread */
void MainWindow::on_pbGoAsync_clicked()
{
    const QPixmap *input = ui->lblInputImage->pixmap();
    QImage inputImage = input->toImage();
    ui->lblOutputImage->setText("");

    try {
        /* Allocate the pipeline only when needed. It'll be ready for more immediately */
        if (!dip.hasPipeline())
            dip.createPipeline(filterName);
        dip.processImageASync(inputImage);
    }
    catch (const std::exception &ex)
    {
        QMessageBox msgBox;
        msgBox.setText(ex.what());
        msgBox.exec();
    }
}

/* Releases the Dyplo resources. The async method will re-use the Dyplo pipeline until you press this button */
void MainWindow::on_pbRelease_clicked()
{
    dip.releasePipeline();
    ui->lblOutputImage->setPixmap(QPixmap());
}

void MainWindow::on_pbOpen_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Image"), lastFileName, tr("Images (*.png *.jpg *.bmp *.xbm)"));
    QPixmap newPixmap(fileName, Q_NULLPTR, Qt::ColorOnly);
    ui->lblInputImage->setPixmap(newPixmap);
    ui->lblOutputImage->setPixmap(QPixmap());
    lastFileName = fileName;
}
