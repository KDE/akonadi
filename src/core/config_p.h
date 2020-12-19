/*
    SPDX-FileCopyrightText: 2020  Daniel Vrátil <dvratil@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_CONFIG_P_H
#define AKONADI_CONFIG_P_H

#include <memory>

namespace Akonadi
{

class Config
{
public:
    explicit Config();
    ~Config() = default;

    static const Config &get();

    struct PayloadCompression {
        /**
         * Whether or not the payload compression feature should be enabled.
         * Default is false (currently).
         *
         * This only disables only compressing the payload. If the feature is disabled,
         * Akonadi can still decompress payloads that have been compressed previously.
         */
        bool enabled = false;
    };

    /**
     * Configures behavior of the payload compression feature.
     */
    PayloadCompression payloadCompression = {};
};

} // namespace Akonadi

#endif


