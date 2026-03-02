#pragma once

#include "nodes/AGPHHNode.h"
#include "nodes/terminals/CTerminals.h"
#include "nodes/functions/CFunctions.h"
#include "utils/random/CRandom.h"
#include <vector>
#include <string>
#include <random>
#include <iostream>

class GPHHUtils {
public:
    static AGPHHNode* GenerateRandomTree(int maxDepth, const std::vector<std::string>& terminals, const std::vector<std::string>& functions, bool full = false) {
        if (maxDepth <= 0 || functions.empty()) {
            int idx = CRandom::GetInt(0, terminals.size());
            return CreateTerminal(terminals[idx]);
        }

        bool isTerminal = false;
        if (!full) {
            isTerminal = (CRandom::GetInt(0, 2) == 0);
        }

        if (isTerminal) {
            int idx = CRandom::GetInt(0, terminals.size());
            return CreateTerminal(terminals[idx]);
        } else {
            int funcIdx = CRandom::GetInt(0, functions.size());
            std::string func = functions[funcIdx];
            AGPHHNode* left = GenerateRandomTree(maxDepth - 1, terminals, functions, full);
            AGPHHNode* right = GenerateRandomTree(maxDepth - 1, terminals, functions, full);
            return CreateFunction(func, left, right);
        }
    }

    static AGPHHNode* CreateTerminal(const std::string& name) {
        if (name == "Dist") return new CDist();
        if (name == "Demand") return new CDemand();
        if (name == "RC") return new CRemCapacity();
        if (name == "DRC") return new CDemandRemCapRatio();
        if (name == "NCC") return new CNextClosest();
        if (name == "Const") {
             float val = CRandom::GetFloat(-1.0f, 1.0f);
             return new CConstant(val);
        }
        return new CConstant(0);
    }

    static AGPHHNode* CreateFunction(const std::string& name, AGPHHNode* left, AGPHHNode* right) {
        if (name == "+") return new CAdd(left, right);
        if (name == "-") return new CSub(left, right);
        if (name == "*") return new CMul(left, right);
        if (name == "/") return new CDiv(left, right);
        if (name == "min") return new CMin(left, right);
        if (name == "max") return new CMax(left, right);
        return new CAdd(left, right);
    }

 
    struct NodeInfo {
        AGPHHNode* node;
        AGPHHNode* parent;
        bool isLeftChild; 
        int depth;
    };
    
    static void CollectNodesInfo(AGPHHNode* node, AGPHHNode* parent, bool isLeft, std::vector<NodeInfo>& info, int depth = 0) {
        if (!node) return;
        info.push_back({node, parent, isLeft, depth});
        
        CFunction* func = dynamic_cast<CFunction*>(node);
        if (func) {
            CollectNodesInfo(func->GetLeft(), node, true, info, depth + 1);
            CollectNodesInfo(func->GetRight(), node, false, info, depth + 1);
        }
    }
    
    static void ReplaceNode(AGPHHNode*& root, const NodeInfo& target, AGPHHNode* newNode) {
        if (target.parent == nullptr) {
            // It's root
            delete root;
            root = newNode;
        } else {
            CFunction* parentFunc = dynamic_cast<CFunction*>(target.parent);
            if (target.isLeftChild) {
                delete parentFunc->GetLeft();
                parentFunc->SetLeft(newNode);
            } else {
                delete parentFunc->GetRight();
                parentFunc->SetRight(newNode);
            }
        }
    }
    
    // Point mutation: replace a node with another of the same type (terminal or function)
    static void PointMutateNode(AGPHHNode*& root, const NodeInfo& target, 
        const std::vector<std::string>& terminals, 
        const std::vector<std::string>& functions) {
        CFunction* targetFunc = dynamic_cast<CFunction*>(target.node);
        
        if (targetFunc) {
            int funcIdx = CRandom::GetInt(0, functions.size());
            std::string newFuncName = functions[funcIdx];
            AGPHHNode* left = targetFunc->GetLeft();
            AGPHHNode* right = targetFunc->GetRight();
       
            AGPHHNode* leftClone = left->Clone();
            AGPHHNode* rightClone = right->Clone();
            
            AGPHHNode* newFunc = CreateFunction(newFuncName, leftClone, rightClone);
    
            if (target.parent == nullptr) {
                delete root;
                root = newFunc;
            } else {
                CFunction* parentFunc = dynamic_cast<CFunction*>(target.parent);
                if (target.isLeftChild) {
                    delete parentFunc->GetLeft();
                    parentFunc->SetLeft(newFunc);
                } else {
                    delete parentFunc->GetRight();
                    parentFunc->SetRight(newFunc);
                }
            }
        } else {
            int termIdx = CRandom::GetInt(0, terminals.size());
            std::string newTerminalName = terminals[termIdx];
            AGPHHNode* newTerminal = CreateTerminal(newTerminalName);
   
            if (target.parent == nullptr) {
                delete root;
                root = newTerminal;
            } else {
                CFunction* parentFunc = dynamic_cast<CFunction*>(target.parent);
                if (target.isLeftChild) {
                    delete parentFunc->GetLeft();
                    parentFunc->SetLeft(newTerminal);
                } else {
                    delete parentFunc->GetRight();
                    parentFunc->SetRight(newTerminal);
                }
            }
        }
    }
    
    // Prune tree to max depth by replacing deep branches with terminals
    static void PruneTreeToDepth(AGPHHNode*& node, int currentDepth, int maxDepth, 
        const std::vector<std::string>& terminals) {
        if (!node) return;
        
        if (currentDepth >= maxDepth) {
            int idx = CRandom::GetInt(0, terminals.size());
            AGPHHNode* terminal = CreateTerminal(terminals[idx]);
            delete node;
            node = terminal;
            return;
        }
        
        CFunction* func = dynamic_cast<CFunction*>(node);
        if (func) {
            AGPHHNode* left = func->GetLeft();
            AGPHHNode* right = func->GetRight();
            PruneTreeToDepth(left, currentDepth + 1, maxDepth, terminals);
            PruneTreeToDepth(right, currentDepth + 1, maxDepth, terminals);
            func->SetLeft(left);
            func->SetRight(right);
        }
    }
    
    // Deep clone is handled by Clone().
    
    static int GetDepth(AGPHHNode* node) {
        if (!node) return 0;
        CFunction* func = dynamic_cast<CFunction*>(node);
if (func) {
    return 1 + std::max(GetDepth(func->GetLeft()), GetDepth(func->GetRight()));
        }
        return 1;
    }
};
