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

class QSocketNotifier;

class DyploImageProcessor: public QObject
{
    Q_OBJECT
public:
    DyploImageProcessor();
    ~DyploImageProcessor();

    void createPipeline(const char* partial);
    void releasePipeline();
    bool hasPipeline() { return dip != nullptr; }
    // Pass pixels through dyplo and wait for result. Blocks the UI.
    // will emit the renderedimage call synchronously
    void processImageSync(const QImage &input, const char *partial);
    // Sends pixels to process and signals when done
    void processImageASync(const QImage &input);

    void imageReceived(const QImage &image);

signals:
    /*
     * Called when a frame is available. The data pointed to in the image object is still
     * located in the DMA buffer. After returning from this method, the buffer will be returned
     * to the driver, so any data processing must take place in this method. If the data
     * is needed outside this method, you must make a copy.
     * On some platforms the DMA buffer may reside in non-cacheable memory. This will make
     * random access and small access to the memory slow, so it might speed up further processing
     * to do a memcpy first. Whether this is the case depends on your platform and configuration.
     * On an i686 based system, using PCIe this is in general not the case. On ARM systems, the
     * memory will be non-cacheable unless some special hardware coherency port is being used.
     */
    void renderedImage(const QImage &image);

private slots:
    void frameAvailableDyplo(int socket);

protected:
    DyploImageContext *diContext;
    DyploImagePipeline *dip;
    dyplo::HardwareConfig *pr_node;
    QSocketNotifier *fromLogicNotifier;
};

#endif // DYPLOIMAGEPROCESSOR_H
