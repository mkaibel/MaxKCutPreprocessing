#ifndef DOMINATINGEDGES_HPP
#define DOMINATINGEDGES_HPP

#include "networkit/Globals.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mkcp {

class NegativeDominatingEdgesReconstructor : public DataReductionReconstructor {
public:
  ~NegativeDominatingEdgesReconstructor() override = default;

  NegativeDominatingEdgesReconstructor();

  std::shared_ptr<Partition> getPartition() const override;

  void reconstructData() override;

  const std::vector<std::shared_ptr<DataReductionReconstructor>> &
  getChildren() const override;

  void addChildReduction(
      std::shared_ptr<DataReductionReconstructor> &child) override;

  void contractedVertices(NetworKit::node remainingVertex,
                          NetworKit::node removedVertex);

private:
  bool calledReconstruction = false;

  //! Stores node contractions in order with (remaining, removed)
  std::vector<std::pair<NetworKit::node, NetworKit::node>> contractedNodes;

  std::shared_ptr<Partition> partition;
  std::vector<std::shared_ptr<DataReductionReconstructor>> childRule;
};

} // namespace mkcp

#endif // DOMINATINGEDGES_HPP
