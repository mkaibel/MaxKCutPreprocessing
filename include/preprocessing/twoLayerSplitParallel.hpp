//
// Created by michael-kaibel on 04/03/2026.
//

#ifndef MKCP_TWOLAYERSPLITPARALLEL_HPP
#define MKCP_TWOLAYERSPLITPARALLEL_HPP

#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace mkcp
{
    struct TwoLayerConnector
    {
        std::vector<NetworKit::node> sideABoundary, sideBBoundary;
        std::vector<NetworKit::node> sideBNodes;
        std::vector<NetworKit::node> originalID;
        NetworKit::Graph twoLayerConnectorGraph;
    };

    class TwoLayerParallelReconstructor : public DataReductionReconstructor
    {
    public:
        ~TwoLayerParallelReconstructor() override = default;

        TwoLayerParallelReconstructor(unsigned k, NetworKit::node maxNodeID,
                                      const std::vector<std::vector<NetworKit::node>>& originalVertexIDs,
                                      const std::vector<std::shared_ptr<DataReductionReconstructor>>& childRule,
                                      const std::vector<std::vector<NetworKit::Edge>>& twoLayerConnectors,
                                      const std::vector<std::vector<NetworKit::node>>& sideAInConnector
        ) : _childRule(childRule), _k(k), _maxNodeID(maxNodeID), _twoLayerConnectors(twoLayerConnectors),
            _sideAInConnector(sideAInConnector),
            _originalVertexIDs(originalVertexIDs)
        {
            assert(originalVertexIDs.size() == _childRule.size());
            assert(twoLayerConnectors.size() == sideAInConnector.size());
        }

        [[nodiscard]] std::shared_ptr<Partition> getPartition() const override;

        void reconstructData() override;

        [[nodiscard]] const std::vector<std::shared_ptr<DataReductionReconstructor>>&
        getChildren() const override;

        void addChildReduction(
            std::shared_ptr<DataReductionReconstructor>& child) override;

    private:
        //! Computes the colour permutation for reconstructing twoLayerConnector i
        [[nodiscard]] std::vector<partitionID> computeColourPermutation(const NetworKit::Graph& connector,
                                                                        const std::vector<NetworKit::node>& originalIDs,
                                                                        const std::vector<NetworKit::node>& sideA)
        const;

        bool _calledReconstruction = false;

        std::shared_ptr<Partition> _partition;
        std::vector<std::shared_ptr<DataReductionReconstructor>> _childRule;

        unsigned int _k;
        NetworKit::node _maxNodeID;

        //! Stores the twoLayerConnectors to be reconstructed in order first split to last split (therefore the last one should be reconstructed first)
        std::vector<std::vector<NetworKit::Edge>> _twoLayerConnectors;
        //! Stores which vertices are on one side in a twoLayerConnector (as the graphs may not have a unique bipartition)
        std::vector<std::vector<NetworKit::node>> _sideAInConnector;

        //! Stores for each child graph the original IDs of the vertices
        std::vector<std::vector<NetworKit::node>> _originalVertexIDs;
    };

    class TwoLayyerConnectorSideSolvedReconstructor : public DataReductionReconstructor
    {
    public:
        ~TwoLayyerConnectorSideSolvedReconstructor() override = default;

        TwoLayyerConnectorSideSolvedReconstructor(unsigned k, Partition  subgraphSolution,
                                                  const std::vector<NetworKit::node>& subgraphOriginalIDs,
                                                  NetworKit::Graph  twoLayerConnector,
                                                  const std::vector<NetworKit::node>& connectorOriginalIDs,
                                                  const std::vector<NetworKit::node>& largeSideBoundary) :
            _k(k), _subgraphSolution(std::move(subgraphSolution)), _subgraphOriginalIDs(subgraphOriginalIDs),
            _twoLayerConnector(std::move(twoLayerConnector)), _connectorOriginalIDs(connectorOriginalIDs),
            _largeSideBoundary(largeSideBoundary)
        {
        }

        [[nodiscard]] std::shared_ptr<Partition> getPartition() const override;

        void reconstructData() override;

        [[nodiscard]] const std::vector<std::shared_ptr<DataReductionReconstructor>>&
        getChildren() const override;

        void addChildReduction(
            std::shared_ptr<DataReductionReconstructor>& child) override;

    private:
        [[nodiscard]] std::vector<partitionID> computeColourPermutation()
        const;

        bool _calledReconstruction = false;

        std::shared_ptr<Partition> _partition;
        std::vector<std::shared_ptr<DataReductionReconstructor>> _childRule;

        unsigned int _k;

        Partition _subgraphSolution;
        std::vector<NetworKit::node> _subgraphOriginalIDs;

        NetworKit::Graph _twoLayerConnector;
        std::vector<NetworKit::node> _connectorOriginalIDs;
        std::vector<NetworKit::node> _largeSideBoundary;
    };

    bool checkTwoLayerSplitCriteria(const NetworKit::Graph& connector, const std::vector<NetworKit::node>& oneSide,
                                    unsigned k);

    std::pair<NetworKit::Graph, std::vector<NetworKit::node>> constructContractedTwoLayerConnector(
        const TwoLayerConnector& tlc, const std::vector<NetworKit::node>& subgraphBoundary,
        const std::vector<NetworKit::node>& subgraphOriginalIDs, const Partition& solution,
        std::vector<NetworKit::count>& tempCountValues, unsigned numColoursUsedInBoundary);
}

#endif //MKCP_TWOLAYERSPLITPARALLEL_HPP
