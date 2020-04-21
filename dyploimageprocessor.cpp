#include "dyploimageprocessor.h"
#include <QDebug>

DyploImageProcessor::DyploImageProcessor():
    hwControl(hwContext)
{
}

DyploImageProcessor::~DyploImageProcessor()
{
}

void DyploImageProcessor::processImageSync(const QImage &input)
{
    emit renderedImage(input);
}

