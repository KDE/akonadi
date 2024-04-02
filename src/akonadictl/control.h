#pragma once

enum class Status { Unknown, Stopped, Starting, Running, Stopping, Error };

class AbstractControl
{
public:
    virtual ~AbstractControl() = default;

    virtual Status status() const = 0;
    virtual bool start() = 0;
    virtual bool stop() = 0;
};