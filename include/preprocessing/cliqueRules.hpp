#ifndef CLIQUERULES_HPP
#define CLIQUERULES_HPP

#include "networkit/Globals.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include <cassert>
#include <vector>
namespace mkcp {

class CliqueReductionReconstructor : public DataReductionReconstructor {
public:
  ~CliqueReductionReconstructor() override = default;

  /**
   * Contructs a reconstruction rule for removed cliques
   * @param internalNodes the nodes in the clique that were deleted entirely
   * @param externalNodes the nodes in the cliaue that remained and are already
   * assigned to partitions
   */
  CliqueReductionReconstructor(std::vector<NetworKit::node> internalNodes,
                               std::vector<NetworKit::node> externalNodes,
                               unsigned int k)
      : k(k), internalNodes(internalNodes), externalNodes(externalNodes) {
    assert(externalNodes.size() <=
           (externalNodes.size() + internalNodes.size() + k - 1) / k);
  }

  std::shared_ptr<Partition> getPartition() const override;

  void reconstructData() override;

  const std::vector<std::shared_ptr<DataReductionReconstructor>> &
  getChildren() const override;

  void addChildReduction(
      std::shared_ptr<DataReductionReconstructor> &child) override;

private:
  bool calledReconstruction = false;

  std::shared_ptr<Partition> partition;
  std::vector<std::shared_ptr<DataReductionReconstructor>> childRule;

  unsigned int k;

  std::vector<NetworKit::node> internalNodes;
  std::vector<NetworKit::node> externalNodes;
};

} // namespace mkcp

#endif // CLIQUERULES_HPP
