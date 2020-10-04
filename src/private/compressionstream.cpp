/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "compressionstream_p.h"
#include "akonadiprivate_debug.h"

#include <QByteArray>

#include <lzma.h>

#include <array>

using namespace Akonadi;

namespace {

class LZMAErrorCategory : public std::error_category
{
public:
    const char *name() const noexcept override { return "lzma"; }
    std::string message(int ev) const noexcept override
    {
        switch (static_cast<lzma_ret>(ev)) {
        case LZMA_OK: return "Operation completed succesfully";
        case LZMA_STREAM_END: return "End of stream was reached";
        case LZMA_NO_CHECK: return "Input stream has no integrity check";
        case LZMA_UNSUPPORTED_CHECK: return "Cannot calculate the integrity check";
        case LZMA_GET_CHECK: return "Integrity check type is now available";
        case LZMA_MEM_ERROR: return "Cannot allocate memory";
        case LZMA_MEMLIMIT_ERROR: return "Memory usage limit was reached";
        case LZMA_FORMAT_ERROR: return "File format not recognized";
        case LZMA_OPTIONS_ERROR: return "Invalid or unsupported options";
        case LZMA_DATA_ERROR: return "Data is corrupt";
        case LZMA_BUF_ERROR: return "No progress is possible";
        case LZMA_PROG_ERROR: return "Programming error";
        }

        Q_UNREACHABLE();
    }
};

const LZMAErrorCategory &lzmaErrorCategory()
{
    static const LZMAErrorCategory lzmaErrorCategory {};
    return lzmaErrorCategory;
}

} // namespace

namespace std
{
template<> struct is_error_code_enum<lzma_ret> : std::true_type {};

std::error_condition make_error_condition(lzma_ret ret)
{
    return std::error_condition(static_cast<int>(ret), lzmaErrorCategory());
}

QDebug operator<<(QDebug dbg, const std::string &str)
{
    dbg << QString::fromStdString(str);
    return dbg;
}

} // namespace std

std::error_code make_error_code(lzma_ret e)
{
    return {static_cast<int>(e), lzmaErrorCategory()};
}

class Akonadi::Compressor {
public:
    std::error_code initialize(QIODevice::OpenMode openMode)
    {
        if (openMode == QIODevice::ReadOnly) {
            return lzma_auto_decoder(&mStream, 100 * 1024 * 1024 /* 100 MiB */, 0);
        } else {
            return lzma_easy_encoder(&mStream, LZMA_PRESET_DEFAULT, LZMA_CHECK_CRC32);
        }
    }

    void setInputBuffer(const char *data, qint64 size)
    {
        mStream.next_in = reinterpret_cast<const uint8_t *>(data);
        mStream.avail_in = size;
    }

    void setOutputBuffer(char *data, qint64 maxSize)
    {
        mStream.next_out = reinterpret_cast<uint8_t *>(data);
        mStream.avail_out = maxSize;
    }

    int inputBufferAvailable() const
    {
        return mStream.avail_in;
    }

    int outputBufferAvailable() const
    {
        return mStream.avail_out;
    }

    std::error_code finalize()
    {
        lzma_end(&mStream);
        return LZMA_OK;
    }

    std::error_code inflate()
    {
        return lzma_code(&mStream, LZMA_RUN);
    }

    std::error_code deflate(bool finish)
    {
        return lzma_code(&mStream, finish ? LZMA_FINISH : LZMA_RUN);
    }

protected:
    lzma_stream mStream = LZMA_STREAM_INIT;
};


CompressionStream::CompressionStream(QIODevice *stream, QObject *parent)
    : QIODevice(parent)
    , mStream(stream)
    , mResult(LZMA_OK)
{}

CompressionStream::~CompressionStream()
{
    CompressionStream::close();
}

bool CompressionStream::open(OpenMode mode)
{
    if ((mode & QIODevice::ReadOnly) && (mode & QIODevice::WriteOnly)) {
        qCWarning(AKONADIPRIVATE_LOG) << "Invalid open mode for CompressionStream.";
        return false;
    }

    mCompressor = std::make_unique<Compressor>();
    if (const auto err = mCompressor->initialize(mode & QIODevice::ReadOnly ? QIODevice::ReadOnly : QIODevice::WriteOnly); err != LZMA_OK) {
        qCWarning(AKONADIPRIVATE_LOG) << "Failed to initialize LZMA stream coder:" << err.message();
        return false;
    }

    if (mode & QIODevice::WriteOnly) {
        mBuffer.resize(BUFSIZ);
        mCompressor->setOutputBuffer(mBuffer.data(), mBuffer.size());
    }

    return QIODevice::open(mode);
}

void CompressionStream::close()
{
    if (!isOpen()) {
        return;
    }

    if (openMode() & QIODevice::WriteOnly && mResult == LZMA_OK) {
        write(nullptr, 0);
    }

    mResult = mCompressor->finalize();

    setOpenMode(QIODevice::NotOpen);
}

std::error_code CompressionStream::error() const
{
    return mResult == LZMA_STREAM_END ? LZMA_OK : mResult;
}

bool CompressionStream::atEnd() const
{
    return mResult == LZMA_STREAM_END && QIODevice::atEnd() && mStream->atEnd();
}

qint64 CompressionStream::readData(char *data, qint64 dataSize)
{
    qint64 dataRead = 0;

    if (mResult == LZMA_STREAM_END) {
        return 0;
    } else if (mResult != LZMA_OK) {
        return -1;
    }

    mCompressor->setOutputBuffer(data, dataSize);

    while (dataSize > 0) {
        if (mCompressor->inputBufferAvailable() == 0) {
            mBuffer.resize(BUFSIZ);
            const auto compressedDataRead = mStream->read(mBuffer.data(), mBuffer.size());

            if (compressedDataRead > 0) {
                mCompressor->setInputBuffer(mBuffer.data(), compressedDataRead);
            } else {
                break;
            }
        }

        mResult = mCompressor->inflate();

        if (mResult != LZMA_OK && mResult != LZMA_STREAM_END) {
            qCWarning(AKONADIPRIVATE_LOG) << "Error while decompressing LZMA stream:" << mResult.message();
            break;
        }

        const auto decompressedDataRead = dataSize - mCompressor->outputBufferAvailable();
        dataRead += decompressedDataRead;
        dataSize -= decompressedDataRead;

        if (mResult == LZMA_STREAM_END) {
            if (mStream->atEnd()) {
                break;
            }
        }

        mCompressor->setOutputBuffer(data + dataRead, dataSize);
    }

    return dataRead;
}


qint64 CompressionStream::writeData(const char *data, qint64 dataSize)
{
    if (mResult != LZMA_OK) {
        return 0;
    }

    bool finish = (data == nullptr);
    if (!finish) {
        mCompressor->setInputBuffer(data, dataSize);
    }

    qint64 dataWritten = 0;

    while (dataSize > 0 || finish) {
        mResult = mCompressor->deflate(finish);

        if (mResult != LZMA_OK && mResult != LZMA_STREAM_END) {
            qCWarning(AKONADIPRIVATE_LOG) << "Error while compressing LZMA stream:" << mResult.message();
            break;
        }

        if (mCompressor->inputBufferAvailable() == 0 || (mResult == LZMA_STREAM_END)) {
            const auto wrote = dataSize - mCompressor->inputBufferAvailable();

            dataWritten += wrote;
            dataSize -= wrote;

            if (dataSize > 0) {
                mCompressor->setInputBuffer(data + dataWritten, dataSize);
            }
        }

        if (mCompressor->outputBufferAvailable() == 0 || (mResult == LZMA_STREAM_END) || finish) {
            const auto toWrite = mBuffer.size() - mCompressor->outputBufferAvailable();
            if (toWrite > 0) {
                const auto writtenSize = mStream->write(mBuffer.constData(), toWrite);
                if (writtenSize != toWrite) {
                    qCWarning(AKONADIPRIVATE_LOG) << "Failed to write compressed data to output device:" << mStream->errorString();
                    setErrorString(QStringLiteral("Failed to write compressed data to output device."));
                    return 0;
                }
            }

            if (mResult == LZMA_STREAM_END) {
                Q_ASSERT(finish);
                break;
            }
            mBuffer.resize(BUFSIZ);
            mCompressor->setOutputBuffer(mBuffer.data(), mBuffer.size());
        }
    }

    return dataWritten;
}

bool CompressionStream::isCompressed(QIODevice *data)
{
    constexpr std::array<uchar, 6> magic = {0xfd, 0x37, 0x7a, 0x58, 0x5a, 0x00};

    if (!data->isOpen() && !data->isReadable()) {
        return false;
    }

    char buf[6] = {};
    if (data->peek(buf, sizeof(buf)) != sizeof(buf)) {
        return false;
    }

    return memcmp(magic.data(), buf, sizeof(buf)) == 0;
}

