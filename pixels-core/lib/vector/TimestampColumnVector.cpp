//
// Created by liyu on 12/23/23.
//

#include "vector/TimestampColumnVector.h"
#include <iostream>
#include <sstream>
#include <ctime>

TimestampColumnVector::TimestampColumnVector(int precision, bool encoding)
    : TimestampColumnVector(VectorizedRowBatch::DEFAULT_SIZE, precision, encoding) {
}

TimestampColumnVector::TimestampColumnVector(uint64_t len, int precision, bool encoding)
    : ColumnVector(len, encoding) {
    this->precision = precision;
    posix_memalign(reinterpret_cast<void **>(&this->times), 64, len * sizeof(long));
}

TimestampColumnVector::~TimestampColumnVector() {
    if (!closed) {
        TimestampColumnVector::close();
    }
}


void TimestampColumnVector::add(std::string &val) {
    std::istringstream ss(val);
    char t;
    long ts;
    struct tm time = {0};
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0, millis = 0, micros = 0;
    ss >> year >> t >> month >> t >> day >> t >> hour >> t >> minute >> t >> second >> t >> millis >> micros;

    time.tm_year = year - 1900;
    time.tm_mon = month - 1;
    time.tm_mday = day;
    time.tm_hour = hour;
    time.tm_min = minute;
    time.tm_sec = second;
    time.tm_isdst = -1;
    time.tm_zone = "CST";

    ts = mktime(&time);
    struct tm epoch = {0};
    epoch.tm_year = 70;
    epoch.tm_mon = 0;
    epoch.tm_mday = 1;
    long epoch_ts = mktime(&epoch);
    ts -= epoch_ts;

    if (ts == -1) {
        throw InvalidArgumentException("Error converting to timestamp!");
    }
    add(ts * 1000'000 + millis * 1000 + micros);
}

void TimestampColumnVector::add(int64_t value) {
    if (writeIndex >= length) {
        ensureSize(writeIndex * 2, true);
    }
    set(writeIndex++, value);
}

void TimestampColumnVector::ensureSize(uint64_t size, bool preserveData) {
    ColumnVector::ensureSize(size, preserveData);
    if (length < size) {
        long *oldVector = times;
        posix_memalign(reinterpret_cast<void **>(&times), 64, size * sizeof(long));
        if (preserveData) {
            std::copy(oldVector, oldVector + length, times);
        }
        delete[] oldVector;
        memoryUsage += static_cast<long>(sizeof(long)) * (size - length);
        resize(size);
    }
}

void TimestampColumnVector::close() {
    if (!closed) {
        ColumnVector::close();
        if (encoding && this->times != nullptr) {
            free(this->times);
        }
        this->times = nullptr;
    }
}

void TimestampColumnVector::print(int rowCount) {
    throw InvalidArgumentException("not support print longcolumnvector.");
}

void *TimestampColumnVector::current() {
    return (this->times == nullptr) ? nullptr : (this->times + readIndex);
}

void TimestampColumnVector::set(int elementNum, long ts) {
    if (elementNum >= writeIndex) {
        writeIndex = elementNum + 1;
    }
    times[elementNum] = ts;
    // TODO: isNull
}