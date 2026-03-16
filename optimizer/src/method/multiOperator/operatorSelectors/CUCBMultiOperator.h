#pragma once

#include "method/multiOperator/AMultiOperator.h"
#include <iostream>

template <typename O>
class CUCBMultiOperator: public AMultiOperator<O>
{
public:
    virtual CAtomicOperator<O>* SelectOperator()
    {
        if (this->m_AtomicOperators.empty())
        {
            return nullptr;
        }
        else
        {
            float m_TotalGlobalCalls = 0.f;
            //float m_TotalCredits = 1.f; // 1 so to make sure we do not divide by 0
            float m_TotalCredits = 0.000001f; // so to make sure we do not divide by 0

            for (CAtomicOperator<O>& atomicOperator : this->m_AtomicOperators)
            {
                const auto& operatorData = atomicOperator.GetData();
                //m_TotalGlobalCalls += (float)operatorData.m_AccCalls;
                m_TotalGlobalCalls += (float)operatorData.m_Calls;
                m_TotalCredits += (float)operatorData.m_Credits;
            }

            //float bestVal = 0.f;
            float bestVal = -FLT_MAX;
            //float lambda = 1.f / sqrtf(2.f);
            //float lambda = sqrtf(2.f);
            //float lambda = 10.f;
            //float lambda = 2.f;
            float lambda = 1.f;
            //float lambda = 0.1f;
            //float lambda = 3.f;
            //float lambda = -2.f;
            CAtomicOperator<O>* bestOperator = nullptr;

            //std::cout << "xxxxxxxxxxxxxxxx" << std::endl;
            for (CAtomicOperator<O>& atomicOperator : this->m_AtomicOperators)
            {
                const auto& operatorData = atomicOperator.GetData();
                float operatorScore = FLT_MAX;
                //if (operatorData.m_AccCalls > 0.f)
                if (operatorData.m_Calls > 0.f)
                {
                    //loat creditScore = ((float)operatorData.m_Credits / m_TotalCredits); // test
                    //float creditScore = ((float)operatorData.m_Credits / (float)operatorData.m_AccCalls);
                    float creditScore = ((float)operatorData.m_Credits / (float)operatorData.m_Calls); // this main
                    //float callsScore = sqrtf(logf(m_TotalGlobalCalls) / ((float)operatorData.m_AccCalls));
                    //float callsScore = sqrtf(logf(m_TotalGlobalCalls + 1.f) / ((float)operatorData.m_AccCalls));
                    float callsScore = sqrtf(logf(m_TotalGlobalCalls + 1.f) / ((float)operatorData.m_Calls)); // this main
                    //float callsScore = sqrtf(2 * logf(m_TotalGlobalCalls) / ((float)operatorData.m_Calls)); // test
                    //float callsScore = ((float)operatorData.m_AccCalls / m_TotalGlobalCalls);
                    operatorScore = creditScore + lambda * callsScore;

                    // debug
                    //std::cout << "creditScore: " << creditScore << " + callsScore: " << callsScore << " = " << operatorScore << std::endl;
                }
                if (operatorScore > bestVal)
                {
                    bestVal = operatorScore;
                    bestOperator = &atomicOperator;
                }
            }
            return bestOperator;
        }
    }
};


