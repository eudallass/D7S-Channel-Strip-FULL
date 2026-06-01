#pragma once

#include "ModuleChain.h"

class RackManager
{
public:
    RackManager()
    {
        chain.resetToDefault();
    }

    ModuleChain& getChain()
    {
        return chain;
    }

    const ModuleChain& getChain() const
    {
        return chain;
    }

private:
    ModuleChain chain;
};
