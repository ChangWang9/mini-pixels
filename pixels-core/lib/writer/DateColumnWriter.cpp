/*
 * Copyright 2024 PixelsDB.
 *
 * This file is part of Pixels.
 *
 * Pixels is free software: you can redistribute it and/or modify
 * it under the terms of the Affero GNU General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Pixels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Affero GNU General Public License for more details.
 *
 * You should have received a copy of the Affero GNU General Public
 * License along with Pixels.  If not, see
 * <https://www.gnu.org/licenses/>.
 */
#include "writer/DateColumnWriter.h"
#include "utils/BitUtils.h"
DateColumnWriter::DateColumnWriter(std::shared_ptr<TypeDescription> type, std::shared_ptr<PixelsWriterOption> writerOption)
    : ColumnWriter(type, writerOption), curPixelVector(pixelStride)
{
    runlengthEncoding = encodingLevel.ge(EncodingLevel::Level::EL2);
    if (runlengthEncoding)
    {
        encoder = std::make_unique<RunLenIntEncoder>(1, 1);
    }
}

int DateColumnWriter::write(std::shared_ptr<ColumnVector> vector, int size)
{
    auto columnVector = std::static_pointer_cast<DateColumnVector>(vector);
    if (!columnVector)
    {
        throw std::invalid_argument("Invalid vector type");
    }
    int *dates = columnVector->dates;
    int curPartLength;         // size of the partition which belongs to current pixel
    int curPartOffset = 0;     // starting offset of the partition which belongs to current pixel
    int nextPartLength = size; // size of the partition which belongs to next pixel
    while ((curPixelIsNullIndex + nextPartLength) >= pixelStride)
    {
        curPartLength = pixelStride - curPixelIsNullIndex;
        writeCurPartTime(columnVector, dates, curPartLength, curPartOffset);
        newPixel();
        curPartOffset += curPartLength;
        nextPartLength = size - curPartOffset;
    }
    return outputStream->getWritePos();
}
void DateColumnWriter::close()
{
    if (runlengthEncoding && encoder)
    {
        encoder->clear();
    }
    ColumnWriter::close();
}

void DateColumnWriter::writeCurPartTime(std::shared_ptr<ColumnVector> columnVector, 
                                      int* values, int curPartLength, int curPartOffset) {
    writeValues(values, columnVector->isNull, curPartLength, curPartOffset);
    
    // 更新 isNull 数组
    std::copy(columnVector->isNull + curPartOffset, 
              columnVector->isNull + curPartOffset + curPartLength, 
              isNull.begin() + curPixelIsNullIndex);
    curPixelIsNullIndex += curPartLength;
}

void DateColumnWriter::writeValues(const int* values, const bool* isNullArr, 
                                 int length, int offset) {
    for (int i = 0; i < length; i++) {
        curPixelEleIndex++;
        if (isNullArr[i + offset]) {
            hasNull = true;
            if (nullsPadding) {
                curPixelVector[curPixelVectorIndex++] = 0;
            }
            continue;
        }
        curPixelVector[curPixelVectorIndex++] = values[i + offset];
    }
}
void DateColumnWriter::newPixel() {
    if (runlengthEncoding) {
        encodeWithRunLength(curPixelVector.data(), curPixelVectorIndex);
    } else {
        encodeWithoutRunLength(curPixelVector.data(), curPixelVectorIndex);
    }
    ColumnWriter::newPixel();
}

void DateColumnWriter::encodeWithRunLength(const int* data, size_t length) {
    std::vector<byte> buffer(length * sizeof(int));
    int resLen;
    encoder->encode(data, buffer.data(), length, resLen);
    outputStream->putBytes(buffer.data(), resLen);
}

void DateColumnWriter::encodeWithoutRunLength(const int* data, size_t length) {
    auto buffer = std::make_shared<ByteBuffer>(length * sizeof(int));
    EncodingUtils encodingUtils;
    
    auto writeFunc = (byteOrder == ByteOrder::PIXELS_LITTLE_ENDIAN) 
        ? &EncodingUtils::writeIntLE 
        : &EncodingUtils::writeIntBE;
    
    for (size_t i = 0; i < length; i++) {
        (encodingUtils.*writeFunc)(buffer, data[i]);
    }
    
    outputStream->putBytes(buffer->getBuffer(), buffer->size());
}


pixels::proto::ColumnEncoding DateColumnWriter::getColumnChunkEncoding()
{
    pixels::proto::ColumnEncoding columnEncoding;
    if (runlengthEncoding)
    {
        columnEncoding.set_kind(pixels::proto::ColumnEncoding::Kind::ColumnEncoding_Kind_RUNLENGTH);
    }
    else
    {
        columnEncoding.set_kind(pixels::proto::ColumnEncoding::Kind::ColumnEncoding_Kind_NONE);
    }
    return columnEncoding;
}

bool DateColumnWriter::decideNullsPadding(std::shared_ptr<PixelsWriterOption> writerOption)
{
    if (writerOption->getEncodingLevel().ge(EncodingLevel::Level::EL2))
    {
        return false;
    }
    return writerOption->isNullsPadding();
}