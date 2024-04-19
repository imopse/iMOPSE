#include <algorithm>
#include "CRandom.h"

std::mt19937 CRandom::rng{std::random_device{}()};

void CRandom::SetSeed(unsigned int seed)
{
    rng.seed(seed);
}

int CRandom::GetBool()
{
    return (bool) GetInt(0, 2);
}

int CRandom::GetInt(int min, int max)
{
    std::uniform_int_distribution<int> dist(min, max - 1);
    return dist(rng);
}

float CRandom::GetFloat(float min, float max)
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

void CRandom::Shuffle(int start, int end, std::vector<int> &vector)
{
    std::shuffle(vector.begin() + start, vector.begin() + end, rng);
}

int CRandom::GetWeightedInt(const std::vector<float>& weights)
{
    std::vector<float> copiedWeights;
    std::copy(weights.begin(), weights.end(), std::back_inserter(copiedWeights));
    std::sort(copiedWeights.begin(), copiedWeights.end(), std::greater<float>());
    float sum_of_weight = 0;
    for (int i = 0; i < copiedWeights.size(); i++) {
        sum_of_weight += copiedWeights[i];
    }
    float rnd = CRandom::GetFloat(0, sum_of_weight);
    for (int i = 0; i < copiedWeights.size(); i++) {
        if (rnd < copiedWeights[i])
            return i;
        rnd -= copiedWeights[i];
    }
}
