#include "dyploimageprocessor.h"

DyploImageProcessor::DyploImageProcessor():
    hwControl(hwContext)
{
}

DyploImageProcessor::~DyploImageProcessor()
{
}

void DyploImageProcessor::processImageSync(const QImage &input)
{
    // Incoming DMA open in read-only mode
    dyplo::HardwareDMAFifo from_logic(hwContext.openAvailableDMA(O_RDONLY));
    // Outgoing DMA must be opened with read+write permission because you cannot have "write only" memory
    dyplo::HardwareFifo to_logic(hwContext.openAvailableDMA(O_RDWR));

    int blocksize = input.byteCount();

    // Set up routing
    from_logic.addRouteFrom(to_logic.getNodeAndFifoIndex());

    // Set up receiving end first - allocate enough room for our image
    from_logic.reconfigure(dyplo::HardwareDMAFifo::MODE_COHERENT, blocksize, 2, true);
    dyplo::HardwareDMAFifo::Block *block;
    for (unsigned int i = 0; i < from_logic.count(); ++i)
    {
        block = from_logic.dequeue();
        block->bytes_used = blocksize; // How many bytes we want to receive
        from_logic.enqueue(block);
    }

    // Send the data through
    to_logic.write(input.bits(), blocksize);

    // Blocking wait for result to arrive
    block = from_logic.dequeue();

    // Emit event. Note that the QImage object will be invalid when this method returns

    emit renderedImage(QImage((const uchar*)block->data, input.width(), input.height(), input.bytesPerLine(), input.format()));

    // End of scope closes all resources and disposes image
}
