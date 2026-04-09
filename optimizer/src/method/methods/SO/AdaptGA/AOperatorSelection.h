#include <memory>
#include <vector>

template <typename T> class OperatorSelection {
public:
  virtual ~OperatorSelection() = default;

  virtual std::shared_ptr<T> provide() = 0;
  virtual void updateWeights(const std::vector<float> &solutions) = 0;
};
