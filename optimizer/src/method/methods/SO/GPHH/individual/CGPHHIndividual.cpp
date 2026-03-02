#include "CGPHHIndividual.h"
#include <cmath>


CGPHHIndividual::CGPHHIndividual()
	: SSOIndividual(*(new SGenotype()), *(new std::vector<float>()), *(new std::vector<float>())),
	m_Root(nullptr),
	m_IsFlattened(false) {
}

CGPHHIndividual::CGPHHIndividual(const CGPHHIndividual& other)
	: SSOIndividual(other),
	m_Root(nullptr),
	m_IsFlattened(false) {
	if (other.m_Root) {
		m_Root = other.m_Root->Clone();
	}
}

CGPHHIndividual::~CGPHHIndividual() {
	Clear();
}

void CGPHHIndividual::Clear() {
	if (m_Root) {
		delete m_Root;
		m_Root = nullptr;
	}
	m_PostOrderNodes.clear();
	m_IsFlattened = false;
}

void CGPHHIndividual::PostOrderTraversal(AGPHHNode* node, std::vector<AGPHHNode*>& nodes) {
	if (!node) return;

	CFunction* func = dynamic_cast<CFunction*>(node);
	if (func) {
		PostOrderTraversal(func->GetLeft(), nodes);
		PostOrderTraversal(func->GetRight(), nodes);
	}

	// current node after children (postorder)
	nodes.push_back(node);
}

void CGPHHIndividual::FlattenTree() {
	m_PostOrderNodes.clear();
	if (m_Root) {
		PostOrderTraversal(m_Root, m_PostOrderNodes);
	}
	m_IsFlattened = true;
}

float CGPHHIndividual::EvaluateIterative(const SGPHHContext& context) {

	if (!m_IsFlattened) {
		FlattenTree();
	}

	if (m_PostOrderNodes.empty()) {
		return 0.0f;
	}

	for (AGPHHNode* node : m_PostOrderNodes) {
		if (node->IsTerminal()) {
			// Evaluate terminal and cache result
			node->m_CachedResult = node->Evaluate(context);
		}
		else {
			CFunction* func = static_cast<CFunction*>(node);
			float left = func->GetLeft()->m_CachedResult;
			float right = func->GetRight()->m_CachedResult;

			float result = 0.0f;
			switch (node->GetNodeType()) {
			case EGPHHNodeType::Add:
				result = left + right;
				break;
			case EGPHHNodeType::Sub:
				result = left - right;
				break;
			case EGPHHNodeType::Mul:
				result = left * right;
				break;
			case EGPHHNodeType::Div:
				result = (std::abs(right) < 1e-6) ? 1.0f : (left / right);
				break;
			case EGPHHNodeType::Min:
				result = std::min(left, right);
				break;
			case EGPHHNodeType::Max:
				result = std::max(left, right);
				break;
			default:
				result = 0.0f;
				break;
			}

			node->m_CachedResult = result;
		}
	}

	// Final result is in the root (last node in postorder)
	return m_PostOrderNodes.back()->m_CachedResult;
}

