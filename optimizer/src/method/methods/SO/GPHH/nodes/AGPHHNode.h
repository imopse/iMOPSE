#pragma once

#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

struct SGPHHContext {
	std::vector<float> features;
};

enum class EGPHHNodeType {
	Terminal,
	Add,
	Sub,
	Mul,
	Div,
	Min,
	Max
};

class AGPHHNode {
public:
	virtual ~AGPHHNode() = default;

	virtual float Evaluate(const SGPHHContext& context) = 0;
	virtual AGPHHNode* Clone() = 0;

	// For iterative evaluation
	virtual EGPHHNodeType GetNodeType() const = 0;
	virtual bool IsTerminal() const = 0;

	// For debugging/logging
	virtual std::string ToString() = 0;

	// Cached evaluation result for stack-free iterative evaluation
	mutable float m_CachedResult = 0.0f;
};
