//
// Created by yuly on 05.04.23.
//

#include "vector/DecimalColumnVector.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <string>
/**
 * The decimal column vector with precision and scale.
 * The values of this column vector are the unscaled integer value
 * of the decimal. For example, the unscaled value of 3.14, which is
 * of the type decimal(3,2), is 314. While the precision and scale
 * of this decimal are 3 and 2, respectively.
 *
 * <p><b>Note: it only supports short decimals with max precision
 * and scale 18.</b></p>
 *
 * Created at: 05/03/2022
 * Author: hank
 */

DecimalColumnVector::DecimalColumnVector(int precision, int scale, bool encoding)
    : DecimalColumnVector(VectorizedRowBatch::DEFAULT_SIZE, precision, scale, encoding) {
}

DecimalColumnVector::DecimalColumnVector(uint64_t len, int precision, int scale, bool encoding) : ColumnVector(len, encoding)
{
    // decimal column vector has no encoding so we don't allocate memory to this->vector
    posix_memalign(reinterpret_cast<void **>(&this->vector), 32,
                   len * sizeof(int64_t));
    this->precision = precision;
    this->scale = scale;
    memoryUsage += (uint64_t)sizeof(uint64_t) * len + sizeof(int) * 2;
    // todo  check the precision and scale
}

void DecimalColumnVector::add(std::string &val) {
    if (writeIndex >= length) {
        ensureSize(writeIndex * 2, true);
    }
    std::istringstream ss(val);
    long a = 0, b = 0, value = 0;
    char dot;
    ss >> a >> dot >> b;
    long t = b, cnt = 0;
    while (t > 0) {
        t /= 10; 
        cnt++;
        a *= 10;
    }
    value = (a > 0) ? (a + b) : (a - b);
    while (cnt > scale + 1) {
        value /= 10;
        cnt--;
    }
    if (cnt > scale) {
        long modVal = value % 10;
        value /= 10;
        if ((value > 0 && modVal >= 5) || (value < 0 && modVal <= -5)) {
            (value > 0) ? value++ : value--;
        }
    }
    while (cnt < scale) {
        value *= 10;
        cnt++;
    }
    vector[writeIndex] = value;
    isNull[writeIndex] = false;
    writeIndex++;
}


void DecimalColumnVector::add(int64_t value)
{
    throw std::invalid_argument("Invalid argument type");
    }

void DecimalColumnVector::add(int value)
{
    throw std::invalid_argument("Invalid argument type");
} 

void DecimalColumnVector::ensureSize(uint64_t size, bool preserveData) {
    ColumnVector::ensureSize(size, preserveData);
    if (length < size) {
        long *oldVector = vector;
        posix_memalign(reinterpret_cast<void **>(&vector), 32, size * sizeof(int64_t));
        if (preserveData) {
            std::copy(oldVector, oldVector + length, vector);
        }
        delete[] oldVector;
        memoryUsage += (uint64_t)sizeof(uint64_t) * (size - length);
        resize(size);
    }
}


void DecimalColumnVector::close()
{
    if (!closed)
    {
        ColumnVector::close();
        vector = nullptr;
    }
}

void DecimalColumnVector::print(int rowCount)
{
    //    throw InvalidArgumentException("not support print Decimalcolumnvector.");
    for (int i = 0; i < rowCount; i++)
    {
        std::cout << vector[i] << std::endl;
    }
}

void *DecimalColumnVector::current() {
    return (vector == nullptr) ? nullptr : (vector + readIndex);
}

int DecimalColumnVector::getPrecision() {
    return precision;
}

int DecimalColumnVector::getScale() {
    return scale;
}

DecimalColumnVector::~DecimalColumnVector() {
    if (!closed) {
        DecimalColumnVector::close();
    }
}

