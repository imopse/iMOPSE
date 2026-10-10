#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace bntgp::gp
{
    enum class FeatureId : std::uint8_t
    {
        DURATION,
        REQ_LEVEL,
        AVAIL_SKILL,
        CRITLEN,
        SLACK,
        DESC_COUNT,
        TASK_RELEASE_PRESSURE,
        TASK_CRITICAL_PRESSURE,
        AVAIL_GAP,
        CHEAPEST_COST_NOW,
        COST_PER_SKILL_NOW,
        TASK_RES_COUNT,
        AVG_RES_COST,
        UNSCHED_TASKS,
        MIN_FEASIBLE_COST_NOW,
        COST_REGRET_NOW,

        RES_WAGE,
        RES_SKILL_LEVEL,
        RES_IDLE_TIME,
        RES_CAN_START_NOW,
        RES_UTILIZATION,
        RES_WAGE_PER_LEVEL,
        RES_ASSIGN_COST,
        RES_ASSIGN_PREMIUM_ALL,
        RES_RESERVE_PRESSURE,
        RES_FAMILY_MISMATCH,
        RES_FUTURE_BRANCH_FIT,
        RES_BOTTLENECK_PRESERVATION,
        RES_SPECIALIST_MISUSE,
        RES_RELATIVE_WAGE,

        COUNT
    };

    inline constexpr std::size_t FeatureCount =
        static_cast<std::size_t>(FeatureId::COUNT);

    enum class NodeKind : std::uint8_t
    {
        CONST,
        FEATURE,
        UNARY,
        BINARY
    };

    enum class UnaryOp : std::uint8_t
    {
        NEG,
        ABS
    };

    enum class BinaryOp : std::uint8_t
    {
        ADD,
        SUB,
        MUL,
        DIV,
        MIN,
        MAX
    };

    using NodeIndex = std::int32_t;

    inline constexpr NodeIndex InvalidNodeIndex = -1;

    struct GPNode final
    {
        double constant{ 0.0 };

        NodeIndex left{ InvalidNodeIndex };
        NodeIndex right{ InvalidNodeIndex };

        FeatureId feature{ FeatureId::DURATION };
        NodeKind kind{ NodeKind::CONST };
        UnaryOp unaryOperation{ UnaryOp::NEG };
        BinaryOp binaryOperation{ BinaryOp::ADD };
    };

    static_assert(
        std::is_trivially_copyable_v<GPNode>,
        "GPNode must remain trivially copyable.");

    static_assert(
        sizeof(GPNode) <= 32U,
        "GPNode unexpectedly occupies too much memory.");
}