#ifndef TWOLAYERSPLIT_HPP
#define TWOLAYERSPLIT_HPP

#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include "util/unionFind.hpp"
#include <memory>
#include <random>
#include <vector>

namespace mkcp {

class TwoLayerReconstructor : public DataReductionReconstructor {
public:
  ~TwoLayerReconstructor() override = default;

  /**
   * Constructs a reconstructor for the two layer separation rule
   * @param twoLayerConnector the subgraph that is the two layer connector with
   * vertex ids moved down to 0,...,numNodes
   * @param sideA the nodes in the twoLayerConnector that are on sida A
   * @param originalVertexIDs the original IDs of the vertices in the separator
   */
  TwoLayerReconstructor(unsigned int k, const NetworKit::Graph& twoLayerConnector,
                        const std::vector<NetworKit::node> &sideA,
                        const std::vector<NetworKit::node>& originalConnectorVertexIDs,
                        NetworKit::node maxNodeID,
                        const std::vector<NetworKit::node> &originalVertexIDsA,
                        const std::vector<NetworKit::node> &originalVertexIDsB,
                        const std::shared_ptr<DataReductionReconstructor>& childA,
                        const std::shared_ptr<DataReductionReconstructor>& childB);

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

  NetworKit::Graph twoLayerConnector;
  std::vector<NetworKit::node> sideA;
  std::vector<NetworKit::node> originalConnectorVertexIDs;

  NetworKit::node maxNodeID;
  std::vector<NetworKit::node> originalVertexIDsA, originalVertexIDsB;
};

class ContractorGraph {
public:
  ContractorGraph(NetworKit::count n, unsigned int seed = 1234);

  ContractorGraph(NetworKit::count n, std::vector<NetworKit::Edge> edges,
                  unsigned int seed = 1234);

  void reseed(unsigned int newSeed);

  NetworKit::count numNodes() const;

  NetworKit::count numOriginalNodes() const;

  NetworKit::node setRepresentative(NetworKit::node v);

  NetworKit::count setSize(NetworKit::node v);

  void addEdge(NetworKit::node u, NetworKit::node v);

  /**
   * Returns all edges that have not been contracted yet
   */
  const std::vector<NetworKit::Edge> &currentEdges();

  /**
   * Performs k random contractions (or less if the graph would have only 1
   * node less or reaches 0 edges)
   * @param k the number of contractions
   * @returns true iff all k contractions were possible, false if it had to
   * stop eraly due to reaching 2 nodes or no edges being left
   */
  bool contractKEdges(unsigned int k);

  /**
   * Tells the graph to explicitely contract v and w
   */
  void contractNodes(NetworKit::node v, NetworKit::node w);

  /**
   * @returns which vertices have been contracted already
   */
  std::vector<std::vector<NetworKit::node>> contractedSets();

private:
  std::mt19937 randomizer;
  UnionFind contractedVertices;
  std::vector<NetworKit::Edge> edges;
  NetworKit::count n;
};

} // namespace mkcp
#endif // TWOLAYERSPLIT_HPP
