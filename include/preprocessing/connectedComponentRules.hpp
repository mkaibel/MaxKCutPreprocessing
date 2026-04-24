#ifndef CONNECTEDCOMPONENTRULES_HPP
#define CONNECTEDCOMPONENTRULES_HPP

#include "globals.hpp"
#include "networkit/Globals.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include <memory>
#include <vector>

namespace mkcp {
class ConnectedComponentReconstructor : public DataReductionReconstructor {
public:
  /**
   * Creates a reconstructor rule for splitting a graph into connected
   * components
   * @param originalIDs the original vertex IDs of the connected component
   * @param childRule a pointer to the child rule associated with the rule
   */
  ConnectedComponentReconstructor(
      NetworKit::count maximumNodeID,
      const std::vector<std::vector<NetworKit::node>>& originalIDs,
      const std::vector<std::shared_ptr<DataReductionReconstructor>> &childRule);

  std::shared_ptr<Partition> getPartition() const override;

  void reconstructData() override;

  const std::vector<std::shared_ptr<DataReductionReconstructor>> &
  getChildren() const override;

  void addChildReduction(
      std::shared_ptr<DataReductionReconstructor> &child) override;

private:
  bool calledReconstruction = false;

  std::vector<std::shared_ptr<DataReductionReconstructor>> childRule;

  NetworKit::count maximumNodeID;

  std::shared_ptr<Partition> partition;
  std::vector<std::vector<NetworKit::node>> originalIDs;
};

class BiconnectedComponentReconstructor : public DataReductionReconstructor {
public:
  /**
   * Creates a reconstructor rule for splitting a graph into connected
   * components
   * @param originalIDs the original vertex IDs of the connected component
   * @param childRule a pointer to the child rule associated with the rule
   */
  BiconnectedComponentReconstructor(
      NetworKit::count maximumNodeID,
      const std::vector<std::vector<NetworKit::node>>& originalIDs,
      const std::vector<std::shared_ptr<DataReductionReconstructor>> &childRule);

  std::shared_ptr<Partition> getPartition() const override;

  void reconstructData() override;

  const std::vector<std::shared_ptr<DataReductionReconstructor>> &
  getChildren() const override;

  void addChildReduction(
      std::shared_ptr<DataReductionReconstructor> &child) override;

private:
  bool calledReconstruction = false;

  std::vector<std::shared_ptr<DataReductionReconstructor>> childRule;

  NetworKit::count maximumNodeID;

  std::shared_ptr<Partition> partition;
  std::vector<std::vector<NetworKit::node>> originalIDs;
};
} // namespace mkcp

#endif // CONNECTEDCOMPONENTRULES_HPP
