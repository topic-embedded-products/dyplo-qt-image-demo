#include "dyploimageprocessor.h"
#include <dyplo/hardware.hpp>

struct DyploImageContext
{
    dyplo::HardwareContext hwContext;
    dyplo::HardwareControl hwControl;

    DyploImageContext():
        hwControl(hwContext)
    {}
};

struct DyploImagePipeline
{
    DyploImageProcessor *owner;
    dyplo::HardwareDMAFifo from_logic;
    dyplo::HardwareFifo to_logic;
    unsigned int blockSize;
    int width;
    int height;
    int bpl;
    QImage::Format format;

    DyploImagePipeline(DyploImageProcessor *_owner, DyploImageContext *diContext, int processing_node_id):
        owner(_owner),
        from_logic(diContext->hwContext.openAvailableDMA(O_RDONLY)),
        to_logic(diContext->hwContext.openAvailableDMA(O_RDWR)),
        blockSize(0)
    {
        if (processing_node_id < 0)
        {
            // Set up routing (i.o.w. do nothing...)
            from_logic.addRouteFrom(to_logic.getNodeAndFifoIndex());
        }
        else
        {
            from_logic.addRouteFrom(processing_node_id);
            to_logic.addRouteTo(processing_node_id);
        }
    }

    void setBlockSize(unsigned int blocksize)
    {
        from_logic.reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, blocksize, 2, true);
        for (unsigned int i = 0; i < from_logic.count(); ++i)
        {
            dyplo::HardwareDMAFifo::Block *block = from_logic.dequeue();
            block->bytes_used = blocksize; // How many bytes we want to receive
            from_logic.enqueue(block);
        }
        blockSize = blocksize;
    }

    void sendImage(const QImage &input)
    {
        int size = input.byteCount();
        if (size != (int)blockSize)
            setBlockSize(size);
        to_logic.write(input.bits(), size);
        width = input.width();
        height = input.height();
        bpl = input.bytesPerLine();
        format = input.format();
    }

    void receiveImage()
    {
        dyplo::HardwareDMAFifo::Block *block = from_logic.dequeue();

        owner->imageReceived(QImage((const uchar*)block->data, width, height, bpl, format));

        // Return the DMA buffer to the hardware for re-use
        block->bytes_used = blockSize;
        from_logic.enqueue(block);
    }
};

DyploImageProcessor::DyploImageProcessor():
    diContext(NULL),
    dip(NULL)
{
}

DyploImageProcessor::~DyploImageProcessor()
{
    delete dip;
    delete diContext;
}

void DyploImageProcessor::createPipeline(const char *partial)
{
    if (!diContext)
        diContext = new DyploImageContext();
    if (dip)
        delete dip;

    dip = new DyploImagePipeline(this, diContext, -1);
}

void DyploImageProcessor::processImageSync(const QImage &input)
{
    if (!dip)
        createPipeline(NULL);
    dip->sendImage(input);
    dip->receiveImage();
}

void DyploImageProcessor::imageReceived(const QImage &image)
{
    emit renderedImage(image);
}
