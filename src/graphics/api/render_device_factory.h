#pragma once

#include <memory>

#include "render_device.h"

class IRenderDeviceFactory {
public:
    virtual ~IRenderDeviceFactory() = default;

    std::unique_ptr<IRenderDevice> operator()() {
        return std::unique_ptr<IRenderDevice>(create());
    }

protected:
    virtual IRenderDevice *create() = 0;
};
