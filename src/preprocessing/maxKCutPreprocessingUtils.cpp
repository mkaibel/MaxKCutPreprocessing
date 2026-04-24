#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include <cassert>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mkcp {

QueuedGraph::QueuedGraph(
    const NetworKit::Graph &pgraph, unsigned int k,
    std::weak_ptr<DataReductionReconstructor> pParentReconstructor)
    : k(k), lowDegreePositiveVertices(pgraph.upperNodeIdBound()),
      negativeDominatingEdges(pgraph.upperNodeIdBound()),
      connectedSimilarVertices(pgraph.upperNodeIdBound()),
      fullCliqueCheck(pgraph.upperNodeIdBound()),
      cliqueCheck(pgraph.upperNodeIdBound()),
      negativeTriangle(pgraph.upperNodeIdBound()) {
  graph = pgraph;
  parentReconstructor = std::move(pParentReconstructor);
  maxNodeID = graph.upperNodeIdBound();

  applicableGraphRules.resize(RuleFlag::AllGraphRules - RuleFlag::Connectivity,
                              true);

  tempWeitht.resize(graph.upperNodeIdBound(), 0.0);

  numIncidentNegativeEdges.resize(graph.upperNodeIdBound(), 0);

  localEdgeWeight.resize(graph.upperNodeIdBound(), 0.0);
  numEdgesNotWithLocalEdgeWeight.resize(graph.upperNodeIdBound(), 0);

  for (NetworKit::WeightedEdge e : graph.edgeWeightRange()) {
    if (e.weight < 0) {
      numIncidentNegativeEdges[e.u]++;
      numIncidentNegativeEdges[e.v]++;
    }

    if (localEdgeWeight[e.u] == 0.0) {
      localEdgeWeight[e.u] = e.weight;
    } else if (e.weight != localEdgeWeight[e.u]) {
      numEdgesNotWithLocalEdgeWeight[e.u]++;
    }

    if (localEdgeWeight[e.v] == 0.0) {
      localEdgeWeight[e.v] = e.weight;
    } else if (e.weight != localEdgeWeight[e.v]) {
      numEdgesNotWithLocalEdgeWeight[e.v]++;
    }
  }

  // Initialise lists of candidates for different preprocessing rules
  for (NetworKit::node v : graph.nodeRange()) {
    if (locallyPositive(v) && (graph.degree(v) < k)) {
      lowDegreePositiveVertices.insertElement(v);
    }
    if (!locallyPositive(v)) {
      negativeDominatingEdges.insertElement(v);
      connectedSimilarVertices.insertElement(v);
      negativeTriangle.insertElement(v);
    }
    if (locallyPositive(v) && locallyUnitWeight(v)) {
      fullCliqueCheck.insertElement(v);
      cliqueCheck.insertElement(v);
    }
  }

  graph.indexEdges();
  graph.setMaintainCompactEdges();
}

const NetworKit::Graph &QueuedGraph::readGraph() const { return graph; }

void QueuedGraph::setNewLocalEdgeWeight(NetworKit::node v) {
  // This function is supposed to only be called when the last edge of the
  // current edge weight is about to be deleted
  if (graph.degree(v) == 1) {
    localEdgeWeight[v] = 0.0;
  } else {
    // Find a new weight for which we check if all edges locally have this
    // weight
    if (graph.getIthNeighborWeight(v, 0) != localEdgeWeight[v]) {
      localEdgeWeight[v] = graph.getIthNeighborWeight(v, 0);
    } else {
      localEdgeWeight[v] = graph.getIthNeighborWeight(v, 1);
    }

    numEdgesNotWithLocalEdgeWeight[v] = 0;

    for (std::pair<NetworKit::node, NetworKit::edgeweight> weight :
         graph.weightNeighborRange(v)) {
      if (weight.second != localEdgeWeight[v]) {
        numEdgesNotWithLocalEdgeWeight[v]++;
      }
    }

    // Account for the fact that one edge not with the new weight gets deleted
    numEdgesNotWithLocalEdgeWeight[v]--;

    // TODO decide if we want a bool array for quicker access than "are
    // there 0 non-complaicant edges at w?"
  }
}

bool QueuedGraph::flagActive(RuleFlag rule) const {
  // Check if it's a whole graph rule
  if (RuleFlag::Connectivity <= rule && rule < RuleFlag::AllGraphRules) {
    return applicableGraphRules[rule - RuleFlag::Connectivity];
  }

  assert(rule != RuleFlag::AllVertexRules);

  switch (rule) {
  case RuleFlag::LowDegreeVertex:
    return !lowDegreePositiveVertices.empty();
  case RuleFlag::NegativeDominatingEdge:
    return !negativeDominatingEdges.empty();
  case RuleFlag::ConnectedSimilarVertices:
    return !connectedSimilarVertices.empty();
  case RuleFlag::FullClique:
    return !fullCliqueCheck.empty();
  case RuleFlag::Clique:
    return !cliqueCheck.empty();
  case mkcp::RuleFlag::Triangle:
    return !negativeTriangle.empty();
  default:
    throw std::runtime_error("Something went wrong in rules checking, a rule "
                             "was probably forgotten in returning stuff");
  }
}

bool QueuedGraph::flagActiveForVertex(RuleFlag rule, NetworKit::node v) const {
  assert(RuleFlag::LowDegreeVertex <= rule && rule < RuleFlag::AllVertexRules);

  switch (rule) {
  case RuleFlag::LowDegreeVertex:
    return lowDegreePositiveVertices.containsElement(v);
  case RuleFlag::NegativeDominatingEdge:
    return negativeDominatingEdges.containsElement(v);
  case RuleFlag::ConnectedSimilarVertices:
    return connectedSimilarVertices.containsElement(v);
  case RuleFlag::FullClique:
    return fullCliqueCheck.containsElement(v);
  case RuleFlag::Clique:
    return cliqueCheck.containsElement(v);
  case mkcp::RuleFlag::Triangle:
    return negativeTriangle.containsElement(v);
  default:
    throw std::runtime_error("Something went wrong in rules checking, a rule "
                             "was probably forgotten in returning stuff");
  }
}

void QueuedGraph::markRule(RuleFlag rule, bool usable) {
  assert(rule >= Connectivity);
  assert(rule <= RuleFlag::AllGraphSeparators);

  if (rule == RuleFlag::AllGraphRules) {
    for (unsigned i = Connectivity; i < AllGraphRules; i++) {
      applicableGraphRules[i - RuleFlag::Connectivity] = usable;
    }
    return;
  }

  if (rule == RuleFlag::AllGraphSeparators) {
    applicableGraphRules[RuleFlag::Connectivity - RuleFlag::Connectivity] =
        usable;
    applicableGraphRules[RuleFlag::Biconnectivity - RuleFlag::Connectivity] =
        usable;
    applicableGraphRules[RuleFlag::TwoLayerSeperators -
                         RuleFlag::Connectivity] = usable;

    return;
  }

  applicableGraphRules[rule - RuleFlag::Connectivity] = usable;
}

void QueuedGraph::markVertexRule(RuleFlag rule, NetworKit::node v,
                                 bool usable) {
  // Checks if the rule is a vertex rule
  assert(rule >= 0);

  if (rule == RuleFlag::AllVertexRules) {
    if (usable) {
      if (!negativeDominatingEdges.containsElement(v))
        negativeDominatingEdges.insertElement(v);
      if (!negativeTriangle.containsElement(v))
        negativeTriangle.insertElement(v);
      if (!connectedSimilarVertices.containsElement(v))
        connectedSimilarVertices.insertElement(v);
      if (!fullCliqueCheck.containsElement(v))
        fullCliqueCheck.insertElement(v);
      if (!cliqueCheck.containsElement(v))
        cliqueCheck.insertElement(v);
    } else {
      if (negativeDominatingEdges.containsElement(v))
        negativeDominatingEdges.removeElement(v);
      if (negativeTriangle.containsElement(v))
        negativeTriangle.removeElement(v);
      if (connectedSimilarVertices.containsElement(v))
        connectedSimilarVertices.removeElement(v);
      if (fullCliqueCheck.containsElement(v))
        fullCliqueCheck.removeElement(v);
      if (cliqueCheck.containsElement(v))
        cliqueCheck.removeElement(v);
    }
    return;
  }

  assert(rule < RuleFlag::AllVertexRules);

  if (usable) {
    switch (rule) {
    case RuleFlag::LowDegreeVertex:
      if (!lowDegreePositiveVertices.containsElement(v))
        lowDegreePositiveVertices.insertElement(v);
      return;
    case RuleFlag::NegativeDominatingEdge:
      if (!negativeDominatingEdges.containsElement(v))
        negativeDominatingEdges.insertElement(v);
      return;
    case RuleFlag::FullClique:
      if (!fullCliqueCheck.containsElement(v))
        fullCliqueCheck.insertElement(v);
      return;
    case RuleFlag::Clique:
      if (!cliqueCheck.containsElement(v))
        cliqueCheck.insertElement(v);
      return;
    case mkcp::RuleFlag::Triangle:
      if (!negativeTriangle.containsElement(v))
        negativeTriangle.insertElement(v);
      return;
    case mkcp::RuleFlag::ConnectedSimilarVertices:
      if (!connectedSimilarVertices.containsElement(v))
        connectedSimilarVertices.insertElement(v);
      return;
    default:
      throw std::runtime_error(
          "Something went wrong when flagging a vertex for a rule");
    }
  } else {
    switch (rule) {
    case RuleFlag::LowDegreeVertex:
      if (lowDegreePositiveVertices.containsElement(v))
        lowDegreePositiveVertices.removeElement(v);
      return;
    case RuleFlag::NegativeDominatingEdge:
      if (negativeDominatingEdges.containsElement(v))
        negativeDominatingEdges.removeElement(v);
      return;
    case RuleFlag::FullClique:
      if (fullCliqueCheck.containsElement(v))
        fullCliqueCheck.removeElement(v);
      return;
    case RuleFlag::Clique:
      if (cliqueCheck.containsElement(v))
        cliqueCheck.removeElement(v);
      return;
    case mkcp::RuleFlag::Triangle:
      if (negativeTriangle.containsElement(v))
        negativeTriangle.removeElement(v);
      return;
    case mkcp::RuleFlag::ConnectedSimilarVertices:
      if (connectedSimilarVertices.containsElement(v))
        connectedSimilarVertices.removeElement(v);
      return;
    default:
      throw std::runtime_error(
          "Something went wrong when flagging a vertex for a rule");
    }
  }
}

ruleStats QueuedGraph::deleteVertex(NetworKit::node v) {
  ruleStats ret = {1, static_cast<int>(graph.degree(v))};

  for (std::pair<NetworKit::node, NetworKit::edgeweight> w :
       graph.weightNeighborRange(v)) {
    // Update all tracked information (locally positive, locally unit weight
    // etc)

    if (w.second < 0) {
      numIncidentNegativeEdges[w.first]--;
    }

    if (w.second == localEdgeWeight[w.first]) {
      // Check case that w has no more incident edges with weight
      // localEdgeWeight[w]
      if (numEdgesNotWithLocalEdgeWeight[w.first] ==
          (graph.degree(w.first) - 1)) {
        setNewLocalEdgeWeight(w.first);
      }
    } else {
      numEdgesNotWithLocalEdgeWeight[w.first]--;
    }

    markVertexRule(RuleFlag::AllVertexRules, w.first, true);

    if (locallyPositive(w.first) && (graph.degree(w.first) <= k)) {
      lowDegreePositiveVertices.insertElement(w.first);
    }
  }

  graph.removeNode(v);

  if (lowDegreePositiveVertices.containsElement(v))
    lowDegreePositiveVertices.removeElement(v);
  markVertexRule(RuleFlag::AllVertexRules, v, false);

  assertFlagConsistency();

  return ret;
}

ruleStats QueuedGraph::deleteEdge(NetworKit::node v, NetworKit::node w) {
  NetworKit::edgeweight weight = graph.weight(v, w);

  if (weight == 0.0) {
    return {0, 0};
  }

  if (weight < 0.0) {
    numIncidentNegativeEdges[v]--;
    numIncidentNegativeEdges[w]--;
  }

  if (weight != localEdgeWeight[v]) {
    numEdgesNotWithLocalEdgeWeight[v]--;
  } else if ((weight == localEdgeWeight[v]) &&
             (numEdgesNotWithLocalEdgeWeight[v] == graph.degree(v) - 1)) {
    setNewLocalEdgeWeight(v);
  }
  if (weight != localEdgeWeight[w]) {
    numEdgesNotWithLocalEdgeWeight[w]--;
  } else if ((weight == localEdgeWeight[w]) &&
             (numEdgesNotWithLocalEdgeWeight[w] == graph.degree(w) - 1)) {
    setNewLocalEdgeWeight(w);
  }

  graph.removeEdge(v, w);

  // TODO decide if we always want to flag everything on deletions
  markVertexRule(RuleFlag::AllVertexRules, v, true);
  markVertexRule(RuleFlag::AllVertexRules, w, true);

  if (locallyPositive(v) && graph.degree(v) < k) {
    lowDegreePositiveVertices.insertElement(v);
  }
  if (locallyPositive(w) && graph.degree(w) < k) {
    lowDegreePositiveVertices.insertElement(w);
  }

  assertFlagConsistency();

  return {0, 1};
}

ruleStats QueuedGraph::insertEdge(NetworKit::node u, NetworKit::node v,
                                  NetworKit::edgeweight weight) {
  assert(!graph.hasEdge(u, v));

  if (weight == 0.0) {
    return {0, 0};
  }

  if (weight < 0.0) {
    numIncidentNegativeEdges[u]++;
    numIncidentNegativeEdges[v]++;
  }

  if (graph.degree(u) == 0) {
    localEdgeWeight[u] = weight;
  } else if (weight != localEdgeWeight[u]) {
    numEdgesNotWithLocalEdgeWeight[u]++;
  }
  if (graph.degree(v) == 0) {
    localEdgeWeight[v] = weight;
  } else if (weight != localEdgeWeight[v]) {
    numEdgesNotWithLocalEdgeWeight[v]++;
  }

  graph.addEdge(u, v, weight);

  markVertexRule(RuleFlag::AllVertexRules, u, true);
  markVertexRule(RuleFlag::AllVertexRules, v, true);

  if (!locallyPositive(u) || graph.degree(u) >= k) {
    if (lowDegreePositiveVertices.containsElement(u)) {
      lowDegreePositiveVertices.removeElement(u);
    }
  }
  if (!locallyPositive(v) || graph.degree(v) >= k) {
    if (lowDegreePositiveVertices.containsElement(v)) {
      lowDegreePositiveVertices.removeElement(v);
    }
  }

  return {0, -1};
}

ruleStats QueuedGraph::changeEdgeWeight(NetworKit::node u, NetworKit::node v,
                                        NetworKit::edgeweight newWeight) {
  ruleStats ret = deleteEdge(u, v);
  ret += insertEdge(u, v, newWeight);

  return ret;
}

ruleStats QueuedGraph::contractNodes(NetworKit::node u, NetworKit::node v) {
  assert(u != v);
  assert(graph.hasNode(u));
  assert(graph.hasNode(v));

  ruleStats ret = {0, 0};

  for (NetworKit::node x : graph.neighborRange(u)) {
    assert(tempWeitht[x] == 0.0);
  }
  for (NetworKit::node x : graph.neighborRange(v)) {
    assert(tempWeitht[x] == 0.0);
  }

  for (auto [neighbour, weight] : graph.weightNeighborRange(u)) {
    tempWeitht[neighbour] = weight;
  }

  for (auto [neighbour, weight] : graph.weightNeighborRange(v)) {
    if (neighbour == u)
      continue;
    if (tempWeitht[neighbour] == 0.0) {
      ret += insertEdge(u, neighbour, weight);
    } else {
      ret += changeEdgeWeight(u, neighbour, tempWeitht[neighbour] + weight);
      tempWeitht[neighbour] = 0.0; // Covers case that edges disappear during contraction due to getting weight 0
    }

    markVertexRule(RuleFlag::AllVertexRules, neighbour, true);
  }

  for (auto [neighbour, weight] : graph.weightNeighborRange(u)) {
    tempWeitht[neighbour] = 0.0;
  }
  tempWeitht[v] = 0.0;

  ret += deleteVertex(v);

  assertFlagConsistency();
  for (NetworKit::node v = 0; v < graph.upperNodeIdBound(); v++)
  {
    assert(tempWeitht[v] == 0.0);
  }

  return ret;
}

const std::vector<unsigned int> &
QueuedGraph::positiveDegreeLeqKVertices() const {
  return lowDegreePositiveVertices.currentElements();
}

const std::vector<unsigned int> &
QueuedGraph::negativeDominatingCandidates() const {
  return negativeDominatingEdges.currentElements();
}

const std::vector<unsigned int> &
QueuedGraph::negativeDominatingTriangleCandidates() const {
  return negativeTriangle.currentElements();
}

const std::vector<unsigned int> &QueuedGraph::fullCliqueCandidates() const {
  return fullCliqueCheck.currentElements();
}

const std::vector<unsigned int> &QueuedGraph::cliqueCandidates() const {
  return cliqueCheck.currentElements();
}

const std::vector<unsigned int> &
QueuedGraph::connectedSimilarVerticesCandidates() const {
  return connectedSimilarVertices.currentElements();
}

void QueuedGraph::attachNewRule(
    std::shared_ptr<DataReductionReconstructor> newRule) {
  parentReconstructor.lock()->addChildReduction(newRule);

  parentReconstructor = newRule;
}

void QueuedGraph::compactNodeIDs() {
  // TODO update local positivity and unit weight
  if (nodeIDDensity() == 1.0) {
    return;
  } else if (nodeIDDensity() > 0.5) {
    //    std::cout << "Graph compacted despite having node ID density of "
    //              << nodeIDDensity() << " > 0.5" << std::endl;
  }

  unsigned int n = graph.numberOfNodes();

  std::vector<NetworKit::node> downAlias(graph.upperNodeIdBound(),
                                         NetworKit::none);
  std::vector<NetworKit::node> originalID(n, NetworKit::none);

  NetworKit::Graph newGraph(n, true);
  IndexedSet newPosDegLessK(n);
  IndexedSet newNegativeDominatingEdges(n);
  IndexedSet newConnectedSimilarVertices(n);
  IndexedSet newFullCliqueCheck(n);
  IndexedSet newCliqueCheck(n);
  IndexedSet newNegativeTriangle(n);

  NetworKit::node nextID = 0;

  for (NetworKit::node v : graph.nodeRange()) {
    downAlias[v] = nextID;
    originalID[nextID] = v;

    if (lowDegreePositiveVertices.containsElement(v)) {
      newPosDegLessK.insertElement(nextID);
    }
    if (negativeDominatingEdges.containsElement(v)) {
      newNegativeDominatingEdges.insertElement(nextID);
    }
    if (connectedSimilarVertices.containsElement(v)) {
      newConnectedSimilarVertices.insertElement(nextID);
    }
    if (fullCliqueCheck.containsElement(v)) {
      newFullCliqueCheck.insertElement(nextID);
    }
    if (cliqueCheck.containsElement(v)) {
      newCliqueCheck.insertElement(nextID);
    }
    if (negativeTriangle.containsElement(v)) {
      newNegativeTriangle.insertElement(nextID);
    }

    numIncidentNegativeEdges[nextID] = numIncidentNegativeEdges[v];
    localEdgeWeight[nextID] = localEdgeWeight[v];
    numEdgesNotWithLocalEdgeWeight[nextID] = numEdgesNotWithLocalEdgeWeight[v];

    nextID++;
  }

  assert(nextID == n);

  for (NetworKit::WeightedEdge e : graph.edgeWeightRange()) {
    newGraph.addEdge(downAlias[e.u], downAlias[e.v], e.weight);
  }

  std::shared_ptr<DataReductionReconstructor> reconstructor =
      std::make_shared<GraphCompactorReconstructor>(originalID,
                                                    graph.upperNodeIdBound());
  attachNewRule(reconstructor);

  graph = newGraph;
  graph.indexEdges();
  graph.setMaintainCompactEdges();
  lowDegreePositiveVertices = newPosDegLessK;
  negativeDominatingEdges = newNegativeDominatingEdges;
  connectedSimilarVertices = newConnectedSimilarVertices;
  fullCliqueCheck = newFullCliqueCheck;
  cliqueCheck = newCliqueCheck;
  negativeTriangle = newNegativeTriangle;

  numIncidentNegativeEdges.resize(n);
  localEdgeWeight.resize(n);
  numEdgesNotWithLocalEdgeWeight.resize(n);
}

void QueuedGraph::assertFlagConsistency() {
  for (NetworKit::node v = 0; v < graph.upperNodeIdBound(); v++) {
    if (!graph.hasNode(v)) {
      assert(!lowDegreePositiveVertices.containsElement(v));
      assert(!negativeDominatingEdges.containsElement(v));
      assert(!connectedSimilarVertices.containsElement(v));
      assert(!fullCliqueCheck.containsElement(v));
      assert(!cliqueCheck.containsElement(v));
      assert(!negativeTriangle.containsElement(v));
    }
  }
}

} // namespace mkcp
