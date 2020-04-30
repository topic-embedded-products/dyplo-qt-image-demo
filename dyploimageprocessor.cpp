#include "dyploimageprocessor.h"
#include <dyplo/hardware.hpp>
#include <QSocketNotifier>

struct DyploImageContext
{
    dyplo::HardwareContext hwContext;
    dyplo::HardwareControl hwControl;

    DyploImageContext():
        hwControl(hwContext)
    {}

    dyplo::HardwareConfig *createConfig(const char *name)
    {
        /* Find a matching PR node */
        unsigned int candidates = hwContext.getAvailablePartitions(name);
        if (candidates == 0)
            throw dyplo::IOException(name, ENODEV);
        int id = 0;
        while (candidates)
        {
            if ((candidates & 0x01) != 0)
            {
                int handle = hwContext.openConfig(id, O_RDWR);
                if (handle == -1)
                {
                    if (errno != EBUSY) /* Non existent? Bail out, last node */
                        throw dyplo::IOException(name);
                }
                else
                {
                    hwControl.disableNode(id);
                    std::string filename = hwContext.findPartition(name, id);
                    {
                        dyplo::HardwareProgrammer programmer(hwContext, hwControl);
                        programmer.fromFile(filename.c_str());
                    }
                    return new dyplo::HardwareConfig(handle);
                }
            }
            ++id;
            candidates >>= 1;
        }
        throw dyplo::IOException(name, ENODEV);
    }
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
        int size = input.sizeInBytes();
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

    int waitHandle() const
    {
        return from_logic.handle;
    }
};

DyploImageProcessor::DyploImageProcessor():
    diContext(NULL),
    dip(NULL),
    pr_node(NULL),
    fromLogicNotifier(NULL)
{
}

DyploImageProcessor::~DyploImageProcessor()
{
    releasePipeline();
    delete diContext;
}

void DyploImageProcessor::createPipeline(const char *partial)
{
    releasePipeline();

    if (!diContext)
        diContext = new DyploImageContext();

    int node_id = -1;
    if (partial)
    {
        pr_node = diContext->createConfig(partial);
        node_id = pr_node->getNodeIndex();
    }
    dip = new DyploImagePipeline(this, diContext, node_id);
    /*
     * createConfig disables the node while programming. Now that all the
     * routing has been done, the node must be enabled again so it will
     * process data.
     */
    if (pr_node)
        pr_node->enableNode();
}

void DyploImageProcessor::releasePipeline()
{
    if (fromLogicNotifier)
    {
        delete fromLogicNotifier;
        fromLogicNotifier = NULL;
    }

    if (dip)
    {
        delete dip;
        dip = NULL;
    }

    if (pr_node)
    {
        delete pr_node;
        pr_node = NULL;
    }
}

void DyploImageProcessor::processImageSync(const QImage &input, const char *partial)
{
    if (fromLogicNotifier)
    {
        delete fromLogicNotifier;
        fromLogicNotifier = NULL;
    }

    createPipeline(partial);
    dip->sendImage(input);
    dip->receiveImage();
    releasePipeline();
}

void DyploImageProcessor::processImageASync(const QImage &input)
{
    if (fromLogicNotifier)
    {
        delete fromLogicNotifier;
        fromLogicNotifier = NULL;
    }

    if (!dip)
        createPipeline("rgb_contrast");

    /* dyplo offers poll/select for its descriptors, so you can use a QSocketNotifier to wait for events */
    fromLogicNotifier = new QSocketNotifier(dip->waitHandle(), QSocketNotifier::Read, this);
    connect(fromLogicNotifier, SIGNAL(activated(int)), this, SLOT(frameAvailableDyplo(int)));
    fromLogicNotifier->setEnabled(true);

    dip->sendImage(input);
}

void DyploImageProcessor::imageReceived(const QImage &image)
{
    emit renderedImage(image);
}

void DyploImageProcessor::frameAvailableDyplo(int)
{
    if (dip)
        dip->receiveImage();
}
