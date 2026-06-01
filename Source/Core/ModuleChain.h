#pragma once

#include <vector>
#include "ModuleState.h"

class ModuleChain
{
public:
    ModuleChain()
    {
        resetToDefault();
    }

    void resetToDefault()
    {
        modules.clear();

        modules.push_back ({ ModuleType::NoiseGT1, true });
        modules.push_back ({ ModuleType::EQ4K,     true });
        modules.push_back ({ ModuleType::Comp76,   true });
        modules.push_back ({ ModuleType::Comp2A,   true });
        modules.push_back ({ ModuleType::Tube,     true });
        modules.push_back ({ ModuleType::Esser,    true });
    }

    std::vector<ModuleState>& getModules()
    {
        return modules;
    }

    const std::vector<ModuleState>& getModules() const
    {
        return modules;
    }

private:
    std::vector<ModuleState> modules;
};
