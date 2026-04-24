#ifndef LOWDEGREERULES_HPP
#define LOWDEGREERULES_HPP

#include "globals.hpp"
#include "networkit/Globals.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include <deque>
#include <memory>
#include <utility>
#include <vector>
namespace mkcp {

class LowDegreePositiveReconstructor : public DataReductionReconstructor {
public:
  ~LowDegreePositiveReconstructor() override = default;

  explicit LowDegreePositiveReconstructor(unsigned int k);

  std::shared_ptr<Partition> getPartition() const override;

  void reconstructData() override;

  const std::vector<std::shared_ptr<DataReductionReconstructor>> &
  getChildren() const override;

  void addChildReduction(
      std::shared_ptr<DataReductionReconstructor> &child) override;

  void addLowDegreeVertex(NetworKit::node v,
                          const std::vector<NetworKit::node> &neighbours);

private:
  unsigned int k;
  bool calledReconstruction = false;

  std::shared_ptr<Partition> partition;
  std::vector<std::shared_ptr<DataReductionReconstructor>> childRule;

  //! Stores vertices and the neighbourhood they are supposed to be
  //! reconstructed with in order first to be reconstructed to last
  std::deque<std::pair<NetworKit::node, std::vector<NetworKit::node>>>
      reconstructionOrder;
};
} // namespace mkcp

#endif // LOWDEGREERULES_HPP
