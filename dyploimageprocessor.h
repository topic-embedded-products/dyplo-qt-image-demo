#ifndef DYPLOIMAGEPROCESSOR_H
#define DYPLOIMAGEPROCESSOR_H

#include <QObject>
#include <QImage>
#include <dyplo/hardware.hpp>

class DyploImageProcessor: public QObject
{
    Q_OBJECT
public:
    DyploImageProcessor();
    ~DyploImageProcessor();

    // Pass pixels through dyplo and wait for result. Blocks the UI.
    // will emit the renderedimage call synchronously
    void processImageSync(const QImage &input);

signals:
    void renderedImage(const QImage &image);

protected:
    dyplo::HardwareContext hwContext;
    dyplo::HardwareControl hwControl;
};

#endif // DYPLOIMAGEPROCESSOR_H
