#pragma once

#include "AOperatorSelection.h"
#include "utils/random/CRandom.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>

struct Vec2 {
  double_t x, y;
};

struct QualityDiversity {
  double_t quality;
  double_t diversity;
};

template <typename T> class CompassProvider : public OperatorSelection<T> {
public:
  CompassProvider(const std::vector<std::shared_ptr<T>> &ops, double_t theta,
                  uint32_t window_size, double_t min_p)
      : window_size(window_size), ops(ops), min_p(min_p) {
    this->theta_vec = {.x = std::cos(theta), .y = std::sin(theta)};

    for (size_t i = 0; i < ops.size(); i++) {
      weights.push_back({.quality = 1, .diversity = 1});
      this->history.emplace_back();
    }
  }

  void updateWeights(const std::vector<float> &solutions) override {
    QualityDiversity diversity = populationQd(solutions);
    // First generation
    if (this->previous_qd == nullptr) {
      this->previous_qd = std::make_unique<QualityDiversity>(diversity);
      this->last_used_operator_idx = -1;
      return;
    }

    if (this->last_used_operator_idx == -1) {
      throw std::runtime_error("unknown operator used!");
    }

    // Calculate and save delta
    const QualityDiversity delta_qd = {
        .quality = diversity.quality - this->previous_qd->quality,
        .diversity = diversity.diversity - this->previous_qd->diversity};
    this->previous_qd = std::make_unique<QualityDiversity>(diversity);

    // Update history and weights
    this->history[this->last_used_operator_idx].push_back(delta_qd);
    if (this->history[this->last_used_operator_idx].size() >
        this->window_size) {
      this->history[this->last_used_operator_idx].erase(
          this->history[this->last_used_operator_idx].begin());
    }

    std::vector<double_t> qualities;
    std::transform(this->history[this->last_used_operator_idx].begin(),
                   this->history[this->last_used_operator_idx].end(),
                   std::back_inserter(qualities),
                   [](const QualityDiversity &qd) { return qd.quality; });

    std::vector<double_t> diversities;
    std::transform(this->history[this->last_used_operator_idx].begin(),
                   this->history[this->last_used_operator_idx].end(),
                   std::back_inserter(diversities),
                   [](const QualityDiversity &qd) { return qd.diversity; });

    this->weights[this->last_used_operator_idx] = {
        .quality = std::accumulate(qualities.begin(), qualities.end(), 0.0) /
                   static_cast<double_t>(qualities.size()),
        .diversity =
            std::accumulate(diversities.begin(), diversities.end(), 0.0) /
            static_cast<double_t>(diversities.size())};

    this->last_used_operator_idx = -1;
  }

  std::shared_ptr<T> provide() override {
    std::vector<double_t> awards;
    calculateAwards(awards);

    std::vector<double_t> probabilities(awards.size());

    calculateProbas(awards, probabilities);

    double_t cum_sum = 0;
    int32_t operator_idx = -1;

    const double_t prob = CRandom::GetFloat(0, 1);
    for (size_t i = 0; i < probabilities.size(); i++) {
      cum_sum += probabilities[i];
      if (cum_sum >= prob) {
        operator_idx = i;
        break;
      }
    }

    if (operator_idx == -1) {
      throw std::runtime_error("No operator chosen");
    }

    this->last_used_operator_idx = operator_idx;
    return this->ops[operator_idx];
  }

private:
  std::vector<QualityDiversity> weights;

  std::shared_ptr<QualityDiversity> previous_qd;
  std::vector<std::vector<QualityDiversity>> history;

  uint32_t window_size;

  std::vector<std::shared_ptr<T>> ops;
  Vec2 theta_vec{};
  double_t min_p;
  int32_t last_used_operator_idx = -1;

  static QualityDiversity populationQd(const std::vector<float> &solutions) {
    const double sum = std::accumulate(solutions.begin(), solutions.end(), 0.0);
    const double avg = sum / static_cast<double_t>(solutions.size());

    std::vector<double_t> diffs(solutions.size());
    std::transform(solutions.begin(), solutions.end(), diffs.begin(),
                   [avg](const float solution) { return solution - avg; });
    const double_t sq_sum =
        std::inner_product(diffs.begin(), diffs.end(), diffs.begin(), 0.0);
    const double_t std_dev =
        std::sqrt(sq_sum / static_cast<double_t>(solutions.size()));

    return {.quality = avg, .diversity = std_dev};
  }

  void calculateProbas(const std::vector<double_t> &awards,
                               std::vector<double_t> &probabilities) const {
    const double_t award_sum =
        std::accumulate(awards.begin(), awards.end(), 0.0);
    const double_t min_award = *std::min_element(awards.begin(), awards.end());

    if (min_award < 0) {
      throw std::runtime_error("Negative award encountered!");
    }

    if (award_sum == 0) {
      for (size_t i = 0; i < awards.size(); i++) {
        probabilities[i] = 1.0 / static_cast<double_t>(awards.size());
      }
      return;
    }

    double_t ksi = 0.0;

    if (min_award / award_sum < this->min_p) {
      ksi = (this->min_p * award_sum - min_award) /
            (1.0 - this->min_p * static_cast<double_t>(awards.size()));
    }

    for (size_t i = 0; i < awards.size(); i++) {
      probabilities[i] =
          (awards[i] + ksi) /
          (award_sum + ksi * static_cast<double_t>(awards.size()));
    }
  }

  void calculateAwards(std::vector<double_t> &awards) const {
    awards.reserve(this->weights.size());
    std::vector<double_t> qualities;
    std::transform(this->weights.begin(), this->weights.end(),
                   std::back_inserter(qualities),
                   [](const QualityDiversity &qd) { return qd.quality; });

    std::vector<double_t> diversities;
    std::transform(this->weights.begin(), this->weights.end(),
                   std::back_inserter(diversities),
                   [](const QualityDiversity &qd) { return qd.diversity; });

    // Normalize
    const double_t max_quality =
        *std::max_element(qualities.begin(), qualities.end());
    const double_t max_diversity =
        *std::max_element(diversities.begin(), diversities.end());

    for (uint32_t i = 0; i < qualities.size(); i++) {
      qualities[i] /= max_quality;
      diversities[i] /= max_diversity;
    }

    std::vector<Vec2> o_vecs(qualities.size());
    for (uint32_t i = 0; i < qualities.size(); i++) {
      o_vecs[i] = {.x = qualities[i], .y = diversities[i]};
    }

    for (auto [x, y] : o_vecs) {
      awards.push_back((x * this->theta_vec.x + y * this->theta_vec.y) /
                       std::sqrt(this->theta_vec.x * this->theta_vec.x +
                                 this->theta_vec.y * this->theta_vec.y));
    }
    const double_t min_reward = *std::min_element(awards.begin(), awards.end());
    for (auto &award : awards) {
      award -= min_reward;
    }
  }
};
