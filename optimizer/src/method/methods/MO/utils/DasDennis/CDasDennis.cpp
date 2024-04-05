#include "CDasDennis.h"

DasDennis::DasDennis(size_t axisPartitions, size_t dimNumb)
: m_AxisPartitions(axisPartitions)
, m_DimensionNumber(dimNumb)
, m_M(dimNumb - 1)
{
}

size_t DasDennis::GetPointsNumber() const
{
	return BinomialCoefficient(m_AxisPartitions + m_DimensionNumber - 1, m_DimensionNumber - 1);
}

void DasDennis::GeneratePoints()
{
	m_Points.clear();

	std::vector<float> layer;
	for (size_t i = 0; i < m_AxisPartitions + 1; ++i)
	{
		layer.push_back(float(i) / float(m_AxisPartitions));
	}
	for (size_t i = 0; i < layer.size(); ++i)
	{
		std::vector<float> l1;
		l1.push_back(layer[i]);
		GenerateLayerRecursive(l1, 0, layer.size() - i);
	}
	for (size_t i = 0; i < m_Points.size(); ++i)
	{
		float s = SumVector(m_Points[i]);
		m_Points[i].push_back(1.f - s);
	}
}

void DasDennis::GenerateLayerRecursive(const std::vector<float>& layer, size_t d, size_t l)
{
	if (d == (m_M - 1))
	{
		m_Points.push_back(layer);
	}
	else
	{
		for (size_t i = 0; i < l; ++i)
		{
			std::vector<float> layerCopy = layer;
			layerCopy.push_back(float(i) / float(m_AxisPartitions));
			GenerateLayerRecursive(layerCopy, d + 1, l - i);
		}
	}
}

float DasDennis::SumVector(const std::vector<float> vec) const
{
	float s = 0;
	for (const float& f : vec)
	{
		s += f;
	}
	return s;
}

std::vector<float> DasDennis::Linspace(float start, float end, size_t partitions) const
{
	float stepSize = (end - start) / (partitions - 1);
	std::vector<float> points(partitions, 0.f);
	for (size_t i = 0; i < partitions; ++i)
	{
		points[i] = start + stepSize * i;
	}
	return points;
}

float DasDennis::BinomialCoefficient(size_t n, size_t k) const
{
	return Factorial(n) / float(Factorial(k) * Factorial(n - k));
}

size_t DasDennis::Factorial(size_t n) const
{
	return n <= 1 ? 1 : n * Factorial(n - 1);
}
