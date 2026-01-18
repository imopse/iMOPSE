#pragma once

#include "../AGPHHNode.h"
#include <string>
#include <sstream>

// Generic Feature node
class CFeature : public AGPHHNode {
public:
    CFeature(int index, const std::string& name) : m_Index(index), m_Name(name) {}

    float Evaluate(const SGPHHContext& context) override {
        if (m_Index < 0 || m_Index >= context.features.size()) return 0.0f;
        return context.features[m_Index];
    }
    AGPHHNode* Clone() override { return new CFeature(m_Index, m_Name); }
    std::string ToString() override { return m_Name; }
    EGPHHNodeType GetNodeType() const override { return EGPHHNodeType::Terminal; }
    bool IsTerminal() const override { return true; }

private:
    int m_Index;
    std::string m_Name;
};

// CVRP Specific Terminals (using CFeature internally)
class CDist : public CFeature {
public:
    CDist() : CFeature(0, "Dist") {}
};

class CDemand : public CFeature {
public:
    CDemand() : CFeature(1, "Demand") {}
};

class CRemCapacity : public CFeature {
public:
    CRemCapacity() : CFeature(2, "RC") {}
};

class CDemandRemCapRatio : public AGPHHNode {
public:
    float Evaluate(const SGPHHContext& context) override {
        if (context.features.size() < 3) return 0.0f;
        float demand = context.features[1];
        float rc = context.features[2];
        if (rc == 0) return 1e9;
        return demand / rc;
    }
    AGPHHNode* Clone() override { return new CDemandRemCapRatio(); }
    std::string ToString() override { return "DRC"; }
    EGPHHNodeType GetNodeType() const override { return EGPHHNodeType::Terminal; }
    bool IsTerminal() const override { return true; }
};

class CNextClosest : public CFeature {
public:
    CNextClosest() : CFeature(3, "NCC") {}
};

// Constant value
class CConstant : public AGPHHNode {
public:
    CConstant(float val) : m_Value(val) {}
    
    float Evaluate(const SGPHHContext& context) override {
        return m_Value;
    }
    AGPHHNode* Clone() override { return new CConstant(m_Value); }
    std::string ToString() override { 
        std::stringstream ss;
        ss << m_Value;
        return ss.str();
    }
    EGPHHNodeType GetNodeType() const override { return EGPHHNodeType::Terminal; }
    bool IsTerminal() const override { return true; }

private:
    float m_Value;
};
