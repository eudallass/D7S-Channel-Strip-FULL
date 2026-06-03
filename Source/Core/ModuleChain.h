#pragma once

#include <array>
#include <vector>
#include "ModuleState.h"

class ModuleChain
{
public:
    static constexpr int maxModules = 7;

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
        modules.push_back ({ ModuleType::Delay,    true });
    }

    void setOrder (const std::array<ModuleType, maxModules>& newOrder)
    {
        for (int i = 0; i < maxModules; ++i)
            modules[(size_t) i].type = newOrder[(size_t) i];
    }

    std::array<ModuleType, maxModules> getOrder() const
    {
        std::array<ModuleType, maxModules> order {};
        for (int i = 0; i < maxModules; ++i)
            order[(size_t) i] = modules[(size_t) i].type;
        return order;
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
