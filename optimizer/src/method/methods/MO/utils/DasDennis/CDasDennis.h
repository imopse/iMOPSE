#pragma once
#include <vector>
#include <stddef.h>
#include <cstddef>

class DasDennis
{
public:
	DasDennis(std::size_t axisPartitions, std::size_t dimNumb);
	std::size_t GetPointsNumber() const;
	const std::vector<std::vector<float>>& GetPoints() const { return m_Points; }
	void GeneratePoints();
private:

	std::size_t m_AxisPartitions;
	std::size_t m_DimensionNumber;

	std::size_t m_M;
	std::vector<std::vector<float>> m_Points;

	void GenerateLayerRecursive(const std::vector<float>& layer, std::size_t d, std::size_t l);
	float SumVector(const std::vector<float> vec) const;
	std::vector<float> Linspace(float start, float end, std::size_t partitions) const;
	float BinomialCoefficient(std::size_t n, std::size_t k) const;
	std::size_t Factorial(std::size_t n) const;
};
