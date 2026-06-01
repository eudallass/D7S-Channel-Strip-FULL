#pragma once

#include <cstddef>
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

    void setModuleEnabled (std::size_t index, bool enabled)
    {
        if (index < chain.getModules().size())
            chain.getModules()[index].enabled = enabled;
    }

    bool isModuleEnabled (std::size_t index) const
    {
        if (index < chain.getModules().size())
            return chain.getModules()[index].enabled;

        return false;
    }

private:
    ModuleChain chain;
};
