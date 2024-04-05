#include "../ACrossover.h"

class CCVRP_OX : public ACrossover
{
public:
    explicit CCVRP_OX(float crossoverProbability) : m_CrossoverProbability(crossoverProbability)
    {};
    ~CCVRP_OX() override = default;

    void Crossover(
            const SProblemEncoding& problemEncoding,
            AIndividual &firstParent,
            AIndividual &secondParent,
            AIndividual &firstChild,
            AIndividual &secondChild) override;
private:
    float m_CrossoverProbability;

    void FixChild(AIndividual &firstChild);
};
