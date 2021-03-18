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

    /*
     * Looks for a partial matching the requested name. Then
     * allocates an programs a suitable Dyplo node.
     * It returns a newly created dyplo::HardwareConfig object
     * that the caller must free at some time. Until the
     * object is freed, no other process can use this node.
     */
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

/*
 * This object integrates Dyplo into the Qt framework, and
 * allows an image to be processed in the FPGA.
 */
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
        unsigned int size = static_cast<unsigned int>(input.byteCount());
        if (size != blockSize)
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

        owner->imageReceived(QImage(reinterpret_cast<const uchar*>(block->data), width, height, bpl, format));

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
    diContext(nullptr),
    dip(nullptr),
    pr_node(nullptr),
    fromLogicNotifier(nullptr)
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
        fromLogicNotifier = nullptr;
    }

    if (dip)
    {
        delete dip;
        dip = nullptr;
    }

    if (pr_node)
    {
        delete pr_node;
        pr_node = nullptr;
    }
}

void DyploImageProcessor::processImageSync(const QImage &input, const char *partial)
{
    if (fromLogicNotifier)
    {
        delete fromLogicNotifier;
        fromLogicNotifier = nullptr;
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
        fromLogicNotifier = nullptr;
    }

    if (!dip)
        throw std::runtime_error("No pipeline. Call createPipeline() before processImageASync()");

    /* dyplo offers poll/select for its descriptors, so you can use a QSocketNotifier to wait for events */
    fromLogicNotifier = new QSocketNotifier(dip->waitHandle(), QSocketNotifier::Read, this);
    connect(fromLogicNotifier, SIGNAL(activated(int)), this, SLOT(frameAvailableDyplo(int)));
    fromLogicNotifier->setEnabled(true);

    /*
     * Sending the image will not block in this case, since there is enough buffer space
     * available for both source and target image. If we were processing a stream of data,
     * we should use a QSocketNotifier in Write mode so we can wait for outgoing buffer
     * space to become available without blocking the UI thread.
     */
    dip->sendImage(input);
}

/*
 * Called when a frame is available. The data pointed to in the image object is still
 * located in the DMA buffer. After returning from this method, the buffer will be returned
 * to the driver, so any data processing must take place in this method.
 */
void DyploImageProcessor::imageReceived(const QImage &image)
{
    emit renderedImage(image);
}

/*
 * This gets called by the QSocketNotifier on the UI thread when there's a data
 * available on the incoming DMA channel from Dyplo.
 */
void DyploImageProcessor::frameAvailableDyplo(int)
{
    if (dip)
        dip->receiveImage();
}
