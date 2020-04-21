#include "dyploimageprocessor.h"
#include <dyplo/hardware.hpp>

struct DyploImagePipeline
{
    DyploImageProcessor *owner;
    dyplo::HardwareContext hwContext;
    dyplo::HardwareControl hwControl;
    dyplo::HardwareDMAFifo from_logic;
    dyplo::HardwareFifo to_logic;
    unsigned int blockSize;
    int width;
    int height;
    int bpl;
    QImage::Format format;

    DyploImagePipeline(DyploImageProcessor *_owner):
        owner(_owner),
        hwControl(hwContext),
        from_logic(hwContext.openAvailableDMA(O_RDONLY)),
        to_logic(hwContext.openAvailableDMA(O_RDWR)),
        blockSize(0)
    {
        // Set up routing (i.o.w. do nothing...)
        from_logic.addRouteFrom(to_logic.getNodeAndFifoIndex());
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
      dip(NULL)
{
}

DyploImageProcessor::~DyploImageProcessor()
{
    delete dip;
}

void DyploImageProcessor::processImageSync(const QImage &input)
{
    if (!dip)
        dip = new DyploImagePipeline(this);
    dip->sendImage(input);
    dip->receiveImage();
}

void DyploImageProcessor::imageReceived(const QImage &image)
{
    emit renderedImage(image);
}
