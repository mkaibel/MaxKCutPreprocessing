#ifndef MAXKCUTPREPROCESSINGUTILS
#define MAXKCUTPREPROCESSINGUTILS

#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"

#include "globals.hpp"
#include "util/indexedSet.hpp"
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mkcp
{
    /**
     * Struct to store how many vertices and edges have been removed by a rule
     */
    struct ruleStats
    {
        int removedVertices = 0;
        int removedEdges = 0;

        ruleStats& operator+=(const ruleStats& rhs)
        {
            this->removedEdges += rhs.removedEdges;
            this->removedVertices += rhs.removedVertices;

            return *this;
        }
    };

    /**
     * An abstract class as a template for how data reduction can be reconstructed
     */
    class DataReductionReconstructor
    {
    public:
        virtual ~DataReductionReconstructor() = default;
        /**
         * @return The partition after applying the data reduction rule
         */
        [[nodiscard]] virtual std::shared_ptr<Partition> getPartition() const
        {
            throw std::runtime_error("Someone forgot to implement getPartition");
        }

        /**
         * Reverse Data Reduction rule
         */
        virtual void reconstructData()
        {
            throw std::runtime_error("Someone forgot to implement reconstructData");
        }

        /**
         * Get child nodes spawned from this rule
         * @return Vector of shared pointers to the child rules spawned here
         */
        [[nodiscard]] virtual const std::vector<std::shared_ptr<DataReductionReconstructor>>&
        getChildren() const
        {
            throw std::runtime_error("Someone forgot to implement getChildren");
        }

        /**
         * Add child rule spawned from here
         */
        virtual void
        addChildReduction(std::shared_ptr<DataReductionReconstructor>&)
        {
            throw std::runtime_error("Someone forgot to implement addChildReduction");
        }
    };

    /**
     * Helper class to represent a node in the reconstruction tree that does
     * nothing. Used to make tree building easier.
     */
    class DataReductionDummy : public DataReductionReconstructor
    {
    public:
        ~DataReductionDummy() override = default;

        std::shared_ptr<Partition> getPartition() const override
        {
            assert(reconstructed);
            return partition;
        }

        void reconstructData() override
        {
            reconstructed = true;
            partition = childRule[0]->getPartition();
        }

        const std::vector<std::shared_ptr<DataReductionReconstructor>>&
        getChildren() const override
        {
            return childRule;
        }

        void addChildReduction(
            std::shared_ptr<DataReductionReconstructor>& child) override
        {
            if (!childRule.empty())
            {
                throw std::runtime_error(
                    "Attempted to add a second child rule to a non-split rule");
            }
            childRule.emplace_back(std::shared_ptr<DataReductionReconstructor>(child));
        }

    private:
        bool reconstructed = false;

        std::shared_ptr<Partition> partition;
        std::vector<std::shared_ptr<DataReductionReconstructor>> childRule;
    };

    /**
     * Helper class to compress graphs from which nodes have been deleted down so
     * that the node IDs are 0,...,n-1 again
     */
    class GraphCompactorReconstructor : public DataReductionReconstructor
    {
    public:
        ~GraphCompactorReconstructor() override = default;

        /**
         * Constructs a reconstructor for graph compression
         * @param originalID a vector of the form that originalID[v] equals the ID
         * that v had before compression
         */
        explicit GraphCompactorReconstructor(
            const std::vector<NetworKit::node>& originalID,
            NetworKit::node maxNodeIDPrev)
            : maxNodeID(maxNodeIDPrev), originalID(originalID)
        {
        }

        std::shared_ptr<Partition> getPartition() const override
        {
            assert(reconstructed);
            return partition;
        }

        void reconstructData() override
        {
            assert(childRule.size() == 1);
            assert(childRule[0]->getPartition()->getN() >= originalID.size());

            reconstructed = true;
            partition = std::make_shared<Partition>(maxNodeID + 1);
            std::cout << "Max Node ID for uncompressed partition = " << maxNodeID + 1
                << std::endl;

            for (NetworKit::node v = 0; v < originalID.size(); v++)
            {
                partition->assignElement(originalID[v],
                                         childRule[0]->getPartition()->assignmentOf(v));
            }
        }

        const std::vector<std::shared_ptr<DataReductionReconstructor>>&
        getChildren() const override
        {
            return childRule;
        }

        void addChildReduction(
            std::shared_ptr<DataReductionReconstructor>& child) override
        {
            if (!childRule.empty())
            {
                throw std::runtime_error(
                    "Attempted to add a second child rule to a non-split rule");
            }
            childRule.emplace_back(std::shared_ptr<DataReductionReconstructor>(child));
        }

    private:
        bool reconstructed = false;

        NetworKit::node maxNodeID = 0;
        //! Stores the original id of a node before compression
        std::vector<NetworKit::node> originalID;
        std::shared_ptr<Partition> partition;
        std::vector<std::shared_ptr<DataReductionReconstructor>> childRule;
    };

    /**
     * Helper class to represent a node in the reconstruction tree that will have
     * its partition set to the solution of some outside solver
     */
    class DataReductionKernel : public DataReductionReconstructor
    {
    public:
        std::shared_ptr<Partition> getPartition() const override { return partition; }

        void reconstructData() override
        {
        }

        const std::vector<std::shared_ptr<DataReductionReconstructor>>&
        getChildren() const override
        {
            return childRule;
        }

        void
        addChildReduction(std::shared_ptr<DataReductionReconstructor>&) override
        {
            throw std::runtime_error(
                "A kernel node can't be assigned a child reduction");
        }

        void setPartition(const Partition& kernelSolution)
        {
            partition = std::make_shared<Partition>(kernelSolution);
        }

    private:
        std::shared_ptr<Partition> partition;
        std::vector<std::shared_ptr<DataReductionReconstructor>> childRule;
    };

    class SolvedGraphReconstructor : public DataReductionReconstructor
    {
    public:
        SolvedGraphReconstructor(Partition solution, NetworKit::node upperNodeIDBound) : _solution(std::move(solution)), _upperNodeIDBound(upperNodeIDBound)
        {

        }

        std::shared_ptr<Partition> getPartition() const override
        {
            if (!_calledReconstruction)
            {
                throw std::runtime_error("Call reconstruction first");
            }

            return _partition;
        }

        void reconstructData() override
        {
            _calledReconstruction = true;

            _partition = std::make_shared<Partition>(_upperNodeIDBound);

            for (NetworKit::node v = 0; v < _solution.getN(); v++)
            {
                _partition->assignElement(v, _solution.assignmentOf(v));
            }
        }

        const std::vector<std::shared_ptr<DataReductionReconstructor>>&
        getChildren() const override
        {
            return _childRule;
        }

        void
        addChildReduction(std::shared_ptr<DataReductionReconstructor>&) override
        {
            throw std::runtime_error(
                "An empty graph node can't be assigned a child reduction");
        }

    private:
        bool _calledReconstruction = false;

        std::shared_ptr<Partition> _partition;
        std::vector<std::shared_ptr<DataReductionReconstructor>> _childRule;

        NetworKit::node _upperNodeIDBound;

        Partition _solution;
    };

    /**
     * Helper class to represent a node in the reconstruction tree that corresponds
     * to an empty graph so we don't have to hand it to a solver explicitely
     */
    class EmptyGraphReconstructor : public DataReductionReconstructor
    {
    public:
        EmptyGraphReconstructor(NetworKit::node upperNodeIDBound)
            : upperNodeIDBound(upperNodeIDBound)
        {
        }

        std::shared_ptr<Partition> getPartition() const override { return partition; }

        void reconstructData() override
        {
            partition = std::make_shared<Partition>(upperNodeIDBound);
        }

        const std::vector<std::shared_ptr<DataReductionReconstructor>>&
        getChildren() const override
        {
            return childRule;
        }

        void
        addChildReduction(std::shared_ptr<DataReductionReconstructor>&) override
        {
            throw std::runtime_error(
                "An empty graph node can't be assigned a child reduction");
        }

    private:
        std::shared_ptr<Partition> partition;
        NetworKit::node upperNodeIDBound;
        std::vector<std::shared_ptr<DataReductionReconstructor>> childRule;
    };

    enum RuleFlag
    {
        LowDegreeVertex = 0,
        NegativeDominatingEdge = 1,
        ConnectedSimilarVertices = 2,
        FullClique =
        3, // For removing cliques where a part of the clique is the boundary
        Clique = 4, // For removing cliques where their neighbourhood may lie ourside
        // the clique itself
        Triangle = 5,
        AllVertexRules = 6, // Special enum to represent all vertex rules

        Connectivity = 11,
        Biconnectivity = 12,
        TwoLayerSeperators = 13,
        NearClique = 14, // For removing near cliques
        SimilarVertices = 15,
        SPQRTree = 16,
        TwoLayerSolveSmallSide = 17,
        AllGraphRules = 18,
        AllGraphSeparators = 19,
    };

    /**
     * Class to store queued graphs and manage some of their properties
     */
    class QueuedGraph
    {
    public:
        /**
         * @param pgraph the underlying graph stored
         * @param k the value k for max k cut, used to update underlying data
         * structures
         * @param pParentReconstructor a weak pointer to the current parent
         * reconstructor rule
         */
        QueuedGraph(const NetworKit::Graph& pgraph, unsigned int k,
                    std::weak_ptr<DataReductionReconstructor> pParentReconstructor =
                        std::weak_ptr<DataReductionReconstructor>());

        /**
         + Grants read access to the underlying graph
         */
        const NetworKit::Graph& readGraph() const;

        /**
         * Returns if for the queued graph a rule is marked as "makes sense to apply
         * right now"
         * @param rule the rule to check
         * @return true iff the rule is marked as "makes sense to apply"
         */
        bool flagActive(RuleFlag rule) const;

        /**
         * Returns if for a vertex in the queued graph a rule is marked as "makes
         * sense to apply right now"
         * @param rule the rule to check
         * @param v the vertex to check
         * @return true iff the rule is marked as "makes sense to apply" for v
         */
        bool flagActiveForVertex(RuleFlag rule, NetworKit::node v) const;

        /**
         * Mark a rule applied on the whole graph as "makes sense" or "doesn't make
         * sense"
         * @param rule the rule to mark
         * @param usabe if the rule is to be marked as usable or not
         */
        void markRule(RuleFlag rule, bool usable);

        /**
         * Mark a vertex as applicable for a rule or not
         * @param rule the rule to mark
         * @param v the node
         * @param usable if the rule is applicable or not
         */
        void markVertexRule(RuleFlag rule, NetworKit::node v, bool usable);

        /**
         * Deletes vertex and readjusts underlying data structures used by data
         * reduction rules
         * @param v the vertex to be deleted
         * @return number of removed vertices and edges
         */
        [[nodiscard]] ruleStats deleteVertex(NetworKit::node v);

        /**
         * Deleted the edge {v, w}
         * @return number of removed vertices and edges
         */
        [[nodiscard]] ruleStats deleteEdge(NetworKit::node v, NetworKit::node w);

        /**
         * Inserts an edge
         * @param u, v the endpoints of the edge
         * @param weight the weight of the edge
         * @return number of removed vertices and edges
         */
        ruleStats insertEdge(NetworKit::node u, NetworKit::node v,
                             NetworKit::edgeweight weight);

        /**
         * Changes the weight of an edge in the graph to a new value
         * If the edge {u, v} did not exist previously it is inserted
         * If newWeight = 0 then the edge is deleted
         * @param u, v the endpoints of the edge
         * @param newWeight the new weight of the edge {u, v}
         * @return number of removed vertices and edges
         */
        [[nodiscard]] ruleStats changeEdgeWeight(NetworKit::node u, NetworKit::node v,
                                                 NetworKit::edgeweight newWeight);

        /**
         * Contracts two nodes and adds weights of duplicate edges
         * To be used when you can prove that two vertices are in the same partition
         * @param u, v the vertices to contract, v is deleted after
         * @return number of removed vertices and edges
         */
        [[nodiscard]] ruleStats contractNodes(NetworKit::node u, NetworKit::node v);

        /**
         * Attach a new reconstruction rule to the graph
         * The new rule has to be fully initialized already, this just updates parent
         * and child pointers
         * @param newRule the new rule to be attached
         * @return number of removed vertices and edges
         */
        void attachNewRule(std::shared_ptr<DataReductionReconstructor> newRule);

        /**
         * Returns how many node IDs between 0 and the maximum node ID are currently
         * used
         * @return #nodes / #node IDs for which space is occupied
         */
        float nodeIDDensity() const
        {
            return static_cast<float>(graph.numberOfNodes() + 1) /
                static_cast<float>(graph.upperNodeIdBound() + 1);
        }

        /**
         * Relabels nodes so that the IDs are 0,...,n-1
         */
        void compactNodeIDs();

        /**
         * @returns a container with the current vertices that have only positive
         * incident edges and a degree < k
         */
        const std::vector<unsigned int>& positiveDegreeLeqKVertices() const;

        /**
         * @returns a container with the current open candidates for negative
         * dominating edges
         */
        const std::vector<unsigned int>& negativeDominatingCandidates() const;

        /**
         * @returns a container with the current open candidates for negative
         * dominating triangles
         */
        const std::vector<unsigned int>& negativeDominatingTriangleCandidates() const;

        /**
         * @returns a container with the current open candidates for the full cliaue
         * removal rule
         */
        const std::vector<unsigned int>& fullCliqueCandidates() const;

        /**
         * @returns a container with the current open candidates for the full cliaue
         * removal rule
         */
        const std::vector<unsigned int>& cliqueCandidates() const;

        /**
         * @returns a container with the current open candidates for negative
         * dominating edges
         */
        const std::vector<unsigned int>& connectedSimilarVerticesCandidates() const;

        /**
         * @param v the evrtex to check
         * @return true iff all edges incident to v have positive weight
         */
        bool locallyPositive(NetworKit::node v) const
        {
            return numIncidentNegativeEdges[v] == 0;
        }

        /**
         * @param v the vertex to check
         * @return true iff all edges incident to v have the same weight
         */
        bool locallyUnitWeight(NetworKit::node v) const
        {
            return numEdgesNotWithLocalEdgeWeight[v] == 0;
        }

    private:
        NetworKit::Graph graph;
        std::weak_ptr<DataReductionReconstructor> parentReconstructor;
        unsigned int k;

        NetworKit::node maxNodeID;

        //! vector to store which graph rules are to be used
        std::vector<bool> applicableGraphRules;

        //! Stores vertices with a degree < k and only positive incident edges
        IndexedSet lowDegreePositiveVertices;
        IndexedSet negativeDominatingEdges;
        IndexedSet connectedSimilarVertices;
        IndexedSet fullCliqueCheck;
        IndexedSet cliqueCheck;
        IndexedSet negativeTriangle;

        //! Helper for contracting nodes, all entries are 0 when not currently in use
        //! (clean up after)
        std::vector<NetworKit::edgeweight> tempWeitht;

        //! Stores the number of edges of negative weight incident to a vertex
        std::vector<NetworKit::count> numIncidentNegativeEdges;

        //! Function to call before deleting the last edge with the current edge
        //! weight
        void setNewLocalEdgeWeight(NetworKit::node v);
        //! Stores the weight of one edge incident to a vertex (arbotrary for vertices
        //! without any incident edges)
        std::vector<NetworKit::edgeweight> localEdgeWeight;
        //! Stores how many edges comply with the local weight
        std::vector<NetworKit::count> numEdgesNotWithLocalEdgeWeight;

        void assertFlagConsistency();
    };
} // namespace mkcp

#endif // MAXKCUTPREPROCESSINGUTILS
