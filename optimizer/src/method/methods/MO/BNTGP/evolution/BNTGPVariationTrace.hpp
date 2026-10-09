#pragma once

#include "../gp/GPTree.hpp"

#include <cstdint>
#include <optional>

namespace bntgp
{
    enum class BNTGPStructuralMutationKind : std::uint8_t
    {
        None = 0U,
        Structural = 1U,
        MacroSubtree = 2U
    };

    struct BNTGPChildVariationTrace final
    {
        bool parameterMutationSelected{ false };

        BNTGPStructuralMutationKind structuralMutation{
            BNTGPStructuralMutationKind::None
        };
        std::optional<gp::GPTree> postCrossoverTree{};
        std::optional<gp::GPTree> postParameterMutationTree{};
    };

    struct BNTGPVariationTrace final
    {
        bool crossoverSelected{ false };

        BNTGPChildVariationTrace firstChild{};
        BNTGPChildVariationTrace secondChild{};
    };
}
