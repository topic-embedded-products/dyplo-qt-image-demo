#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <dyploimageprocessor.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void updateOutput(const QImage &image);

private slots:
    void on_pbGo_clicked();
    void on_pbQuit_clicked();
    void on_pbGoAsync_clicked();
    void on_pbRelease_clicked();
    void on_pbOpen_clicked();

protected:
    DyploImageProcessor dip;
    QString lastFileName;

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
