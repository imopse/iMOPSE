#pragma once
#include <vector>

class DasDennis
{
public:
	DasDennis(size_t axisPartitions, size_t dimNumb);
	size_t GetPointsNumber() const;
	const std::vector<std::vector<float>>& GetPoints() const { return m_Points; }
	void GeneratePoints();
private:

	size_t m_AxisPartitions;
	size_t m_DimensionNumber;

	size_t m_M;
	std::vector<std::vector<float>> m_Points;

	void GenerateLayerRecursive(const std::vector<float>& layer, size_t d, size_t l);
	float SumVector(const std::vector<float> vec) const;
	std::vector<float> Linspace(float start, float end, size_t partitions) const;
	float BinomialCoefficient(size_t n, size_t k) const;
	size_t Factorial(size_t n) const;
};