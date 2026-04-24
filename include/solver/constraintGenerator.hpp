#ifndef CONSTRAINTGENERATOR_HPP
#define CONSTRAINTGENERATOR_HPP

#include "globals.hpp"
#include "networkit/Globals.hpp"
#include "networkit/graph/Graph.hpp"
#include <stdexcept>
#include <vector>
namespace mkcp {

class DominatingEdges {
public:
  DominatingEdges(const NetworKit::Graph &graph, unsigned int k);

  void run();

  bool hasRun() const;

  /**
   * Returns indicator if edges are guaranteed to be in the cut
   * solution Choice is made so that there exists an optimum solution containing
   * every edge that is listed here
   * @return a vector indexed by edge ids such that solution[e] = true => e is
   * contained in the optimum solution
   */
  const std::vector<EdgeConstraint> &guaranteedInCut() const;

  NetworKit::count numFoundConstraints() const {
    if (!hasBeenRun) {
      throw std::runtime_error("Run algorithm first");
    }

    NetworKit::count ret = 0;

    for (bool b : edgeIsDominating) {
      if (b)
        ret++;
    }

    return ret;
  }

private:
  bool hasBeenRun = false;

  const NetworKit::Graph &graph;
  unsigned int k;

  std::vector<EdgeConstraint> edgeIsDominating;
};

} // namespace mkcp

#endif // CONSTRAINTGENERATOR_HPP
