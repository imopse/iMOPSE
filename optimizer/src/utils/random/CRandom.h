#pragma once

#include <random>
#include <iterator>

class CRandom
{
public:
    static void SetSeed(unsigned int seed);
    static int GetBool();
    static int GetInt(int min, int max);
    static float GetFloat(float min, float max);
    static void Shuffle(int start, int end, std::vector<int> &vector);

private:
    static std::mt19937 rng;
};