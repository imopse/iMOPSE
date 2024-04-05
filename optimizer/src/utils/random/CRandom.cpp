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
    
