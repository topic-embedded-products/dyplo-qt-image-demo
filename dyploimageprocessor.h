#ifndef DYPLOIMAGEPROCESSOR_H
#define DYPLOIMAGEPROCESSOR_H

#include <QObject>
#include <QImage>

/* Inner workings, no need to publish this */
struct DyploImagePipeline;
struct DyploImageContext;

namespace dyplo {
    class HardwareConfig;
}

class DyploImageProcessor: public QObject
{
    Q_OBJECT
public:
    DyploImageProcessor();
    ~DyploImageProcessor();

    void createPipeline(const char* partial);
    // Pass pixels through dyplo and wait for result. Blocks the UI.
    // will emit the renderedimage call synchronously
    void processImageSync(const QImage &input);

    void imageReceived(const QImage &image);

signals:
    void renderedImage(const QImage &image);

protected:
    DyploImageContext *diContext;
    DyploImagePipeline *dip;
    dyplo::HardwareConfig *pr_node;
};

#endif // DYPLOIMAGEPROCESSOR_H
