#ifndef MKCP_SPQR_HPP
#define MKCP_SPQR_HPP

#include "globals.hpp"
#include "maxKCutPreprocessorUtils.hpp"
#include "networkit/Globals.hpp"
#include "ogdf/basic/Graph_d.h"
#include <deque>
#include <vector>

namespace mkcp {

struct SPQRCurrentStatus {
  ogdf::Graph ogdfGraph;

  std::vector<int> spqrNodeStatus;
  std::vector<unsigned> spqrNodeDegree;
  std::deque<ogdf::node> currentLeafs;

  //! Maps Networkit Node IDs to OGDF Node IDs
  std::vector<ogdf::node> nkToOGDFNode;
  //! Maps OGDF Node IDs to Networkit Node IDs
  std::vector<NetworKit::node> ogdfToNKNode;
};

class SPQRReconstructor : public DataReductionReconstructor {
public:
  /**
   * Constructs a reconstructor for the SPQR reduction rule
   * @param connectorSame optimum solution if the two connector vertices are in
   * the same partition
   * @param connectorDifferent optimum solution of the two connector vertices
   * are in different partitions
   * @param originalIDs the original IDs of the vertices (so the solutions can
   * be stored compact)
   * @param sep1down, sep2down the IDs of the seperator vertices in the
   * compacted solution
   */
  SPQRReconstructor(Partition connectorSame, Partition connectorDifferent,
                    std::vector<NetworKit::node> originalIDs,
                    NetworKit::node sep1down, NetworKit::node sep2down);

  ~SPQRReconstructor() override = default;

  /**
   * Constructs a reconstructor for the two layer separation rule
   * @param twoLayerConnector the subgraph that is the two layer connector with
   * vertex ids moved down to 0,...,numNodes
   * @param sideA the nodes in the twoLayerConnector that are on sida A
   * @param originalVertexIDs the original IDs of the vertices in the separator
   */
  SPQRReconstructor();

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

  std::vector<NetworKit::node> originalIDs;
  NetworKit::node connectorU, connectorV;
  Partition connectorSame, connectorDifferent;
};

struct LeafParent {
  ogdf::node leaf;
  ogdf::node parent;
  ogdf::edge edge;
};

std::optional<LeafParent> findContractibleEdge(SPQRCurrentStatus &status);

} // namespace mkcp

#endif // MKCP_SPQR_HPP
