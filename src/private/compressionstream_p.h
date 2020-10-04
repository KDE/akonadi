/*
    SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COMPRESSIONSTREAM_H_
#define AKONADI_COMPRESSIONSTREAM_H_

#include "akonadiprivate_export.h"

#include <QIODevice>

#include <memory>
#include <system_error>

namespace Akonadi
{

class Compressor;
class AKONADIPRIVATE_EXPORT CompressionStream : public QIODevice
{
    Q_OBJECT
public:
    explicit CompressionStream(QIODevice *stream, QObject *parent = nullptr);
    ~CompressionStream() override;

    bool open(QIODevice::OpenMode mode) override;
    void close() override;
    bool atEnd() const override;

    std::error_code error() const;

    static bool isCompressed(QIODevice *data);

protected:
    qint64 readData(char *data, qint64 maxSize) override;
    qint64 writeData(const char *data, qint64 maxSize) override;

private:
    QIODevice *mStream = nullptr;
    QByteArray mBuffer;
    std::error_code mResult;
    std::unique_ptr<Compressor> mCompressor;
};

} // namespace Akonadi

#endif

