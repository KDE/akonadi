#pragma once

#include "control.h"

class SystemDControl : AbstractControl
{
public:
    SystemDControl();
    ~SystemDControl() override;

    Status status() const override;
    bool start() override;
    bool stop() override;
};