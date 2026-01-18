#pragma once

#include "../AGPHHNode.h"
#include <vector>
#include <algorithm>

class CFunction : public AGPHHNode {
public:
    CFunction(AGPHHNode* left, AGPHHNode* right) : m_Left(left), m_Right(right) {}
    
    ~CFunction() override {
        delete m_Left;
        delete m_Right;
    }

    AGPHHNode* GetLeft() { return m_Left; }
    AGPHHNode* GetRight() { return m_Right; }
    void SetLeft(AGPHHNode* node) { m_Left = node; }
    void SetRight(AGPHHNode* node) { m_Right = node; }
    
    bool IsTerminal() const override { return false; }

protected:
    AGPHHNode* m_Left;
    AGPHHNode* m_Right;
};

class CAdd : public CFunction {
public:
    using CFunction::CFunction;
    float Evaluate(const SGPHHContext& context) override {
        return m_Left->Evaluate(context) + m_Right->Evaluate(context);
    }
    AGPHHNode* Clone() override { return new CAdd(m_Left->Clone(), m_Right->Clone()); }
    std::string ToString() override { return "(" + m_Left->ToString() + " + " + m_Right->ToString() + ")"; }
    EGPHHNodeType GetNodeType() const override { return EGPHHNodeType::Add; }
};

class CSub : public CFunction {
public:
    using CFunction::CFunction;
    float Evaluate(const SGPHHContext& context) override {
        return m_Left->Evaluate(context) - m_Right->Evaluate(context);
    }
    AGPHHNode* Clone() override { return new CSub(m_Left->Clone(), m_Right->Clone()); }
    std::string ToString() override { return "(" + m_Left->ToString() + " - " + m_Right->ToString() + ")"; }
    EGPHHNodeType GetNodeType() const override { return EGPHHNodeType::Sub; }
};

class CMul : public CFunction {
public:
    using CFunction::CFunction;
    float Evaluate(const SGPHHContext& context) override {
        return m_Left->Evaluate(context) * m_Right->Evaluate(context);
    }
    AGPHHNode* Clone() override { return new CMul(m_Left->Clone(), m_Right->Clone()); }
    std::string ToString() override { return "(" + m_Left->ToString() + " * " + m_Right->ToString() + ")"; }
    EGPHHNodeType GetNodeType() const override { return EGPHHNodeType::Mul; }
};

class CDiv : public CFunction {
public:
    using CFunction::CFunction;
    float Evaluate(const SGPHHContext& context) override {
        float denom = m_Right->Evaluate(context);
        if (std::abs(denom) < 1e-6) return 1.0f; // Protected division
        return m_Left->Evaluate(context) / denom;
    }
    AGPHHNode* Clone() override { return new CDiv(m_Left->Clone(), m_Right->Clone()); }
    std::string ToString() override { return "(" + m_Left->ToString() + " / " + m_Right->ToString() + ")"; }
    EGPHHNodeType GetNodeType() const override { return EGPHHNodeType::Div; }
};

class CMin : public CFunction {
public:
    using CFunction::CFunction;
    float Evaluate(const SGPHHContext& context) override {
        return std::min(m_Left->Evaluate(context), m_Right->Evaluate(context));
    }
    AGPHHNode* Clone() override { return new CMin(m_Left->Clone(), m_Right->Clone()); }
    std::string ToString() override { return "min(" + m_Left->ToString() + ", " + m_Right->ToString() + ")"; }
    EGPHHNodeType GetNodeType() const override { return EGPHHNodeType::Min; }
};

class CMax : public CFunction {
public:
    using CFunction::CFunction;
    float Evaluate(const SGPHHContext& context) override {
        return std::max(m_Left->Evaluate(context), m_Right->Evaluate(context));
    }
    AGPHHNode* Clone() override { return new CMax(m_Left->Clone(), m_Right->Clone()); }
    std::string ToString() override { return "max(" + m_Left->ToString() + ", " + m_Right->ToString() + ")"; }
    EGPHHNodeType GetNodeType() const override { return EGPHHNodeType::Max; }
};
