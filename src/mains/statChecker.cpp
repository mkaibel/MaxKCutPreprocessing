#include <chrono>
#include <fstream>
#include <string>

#include "cxxopts.hpp"
#include "networkit/graph/Graph.hpp"
#include "networkit/io/EdgeListReader.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "preprocessing/maxKCutPreprocessorUtils.hpp"
#include "solver/constraintGenerator.hpp"
#include "util/io.hpp"

std::string collectStats(const std::string& instance, unsigned k,
                         unsigned seed, mkcp::DataReducerConfig config)
{
    NetworKit::Graph graph = NetworKit::EdgeListReader().read(instance);

    auto t0 = std::chrono::steady_clock::now();

    mkcp::DataReducer preprocessor(graph, k, seed, config);
    preprocessor.run();

    auto t1 = std::chrono::steady_clock::now();

    unsigned runtimePreprocessor =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    const mkcp::DataReducerStats& stats = preprocessor.getStats();

    NetworKit::count numVerticesInKernel = 0, numEdgesInKernel = 0,
                     numKernelGraphs = 0, maxKernelvertices = 0,
                     maxKernelEdges = 0;

    unsigned runtomeKernelConstraint = 0;
    unsigned kernelExtraConstraints = 0;
    for (NetworKit::Graph g : preprocessor.getGraphKernel())
    {
        numVerticesInKernel += g.numberOfNodes();
        numEdgesInKernel += g.numberOfEdges();
        numKernelGraphs++;
        maxKernelvertices = std::max(maxKernelvertices, g.numberOfNodes());
        maxKernelEdges = std::max(maxKernelEdges, g.numberOfEdges());

        t0 = std::chrono::steady_clock::now();

        g.indexEdges();
        g.setMaintainCompactEdges();
        mkcp::DominatingEdges extraConstraints(g, k);
        extraConstraints.run();

        t1 = std::chrono::steady_clock::now();

        runtomeKernelConstraint +=
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

        kernelExtraConstraints += extraConstraints.numFoundConstraints();
    }

    t0 = std::chrono::steady_clock::now();

    graph.indexEdges();
    graph.setMaintainCompactEdges();

    mkcp::DominatingEdges extraConstraints(graph, k);

    extraConstraints.run();

    t1 = std::chrono::steady_clock::now();

    unsigned runtimeExtraConstraints =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::string fileName = getFilename(instance);

    return "stats," + fileName + "," + std::to_string(k) + "," + std::to_string(seed) + "," +
        std::to_string(config.contractNegativeEdges) + "," +
        std::to_string(config.contractNegativeTriangles) + "," +
        std::to_string(config.contractSimilarVertices) + "," +
        std::to_string(config.removeCliques) + "," +
        std::to_string(config.splitTwoLayer) + "," +
        std::to_string(config.TwoLayerSmallSide) + "," +
        std::to_string(config.SPQRReduction) + "," +
        std::to_string(graph.numberOfNodes()) + "," +
        std::to_string(graph.numberOfEdges()) + "," +
        std::to_string(runtimePreprocessor) + "," +
        std::to_string(numKernelGraphs) + "," +
        std::to_string(numVerticesInKernel) + "," +
        std::to_string(numEdgesInKernel) + "," +
        std::to_string(preprocessor.getOffset()) + "," +
        std::to_string(maxKernelvertices) + "," +
        std::to_string(maxKernelEdges) + "," +
        std::to_string(stats.lowDegreeStats.removedVertices) + "," +
        std::to_string(stats.lowDegreeStats.removedEdges) + "," +
        std::to_string(stats.fullClique.removedVertices) + "," +
        std::to_string(stats.fullClique.removedEdges) + "," +
        std::to_string(stats.clique.removedVertices) + "," +
        std::to_string(stats.clique.removedEdges) + "," +
        std::to_string(stats.dominatingEdges.removedVertices) + "," +
        std::to_string(stats.dominatingEdges.removedEdges) + "," +
        std::to_string(stats.dominatingTriangles.removedVertices) + "," +
        std::to_string(stats.dominatingTriangles.removedEdges) + "," +
        std::to_string(stats.connectedSimilarVertices.removedVertices) + "," +
        std::to_string(stats.connectedSimilarVertices.removedEdges) + "," +
        std::to_string(stats.disconnectedSimilarVertices.removedVertices) +
        "," + std::to_string(stats.disconnectedSimilarVertices.removedEdges) +
        "," + std::to_string(stats.spqr.removedVertices) + "," +
        std::to_string(stats.spqr.removedEdges) + "," +
        std::to_string(stats.connectedComponentSPlits) + "," +
        std::to_string(stats.biconnectedComponentSplits) + "," +
        std::to_string(stats.twoLayerSplits) + "," +
        std::to_string(stats.twoLayerConnector.removedEdges) + "," +
        std::to_string(stats.twoLayerSmallSide.removedVertices) + "," +
        std::to_string(stats.twoLayerSmallSide.removedEdges) + "," +
        std::to_string(runtimeExtraConstraints) + "," +
        std::to_string(extraConstraints.numFoundConstraints()) + "," +
        std::to_string(runtomeKernelConstraint) + "," +
        std::to_string(kernelExtraConstraints) + "\n";
}

int main(int argc, char** argv)
{
    cxxopts::Options options("Benchmark max k cut preprocessing");
    options.add_options()("i,instance", "Choose instance file to be run",
                          cxxopts::value<std::string>())(
        "o,output", "Specify output file", cxxopts::value<std::string>())(
        "cne,contract-negative-edges", "Should negative edges be contracted", cxxopts::value<bool>())(
        "cnt,contract-negative-triangles", "Should negative triangles be contracted", cxxopts::value<bool>())(
        "csv,contract-similar-vertices", "Should similar vertices contracted", cxxopts::value<bool>())(
        "clq,remove-cliques", "Should cliques with a small boundary be removed", cxxopts::value<bool>())(
        "tlc,split-tlc", "Should two-layer-connectors be split", cxxopts::value<bool>())(
        "tlcs,split-tlc-solve-side", "Should two-layer-connectors be split where one side can be solved easily",
        cxxopts::value<bool>())(
        "spqr,reduce-spqr", "Should the SPQR-reduction be employed", cxxopts::value<bool>())(
        "k,k-value", "", cxxopts::value<unsigned>())(
        "s,seed", "", cxxopts::value<unsigned>()->default_value("1234"));

    options.parse_positional({"instance", "output"});
    auto arguments = options.parse(argc, argv);

    bool contractNegativeEdges = arguments["contract-negative-edges"].as<bool>();
    bool contractNegativeTriangles = arguments["contract-negative-triangles"].as<bool>();
    bool contractSimilarVertices = arguments["contract-similar-vertices"].as<bool>();
    bool removeCliques = arguments["remove-cliques"].as<bool>();
    bool splitTLCs = arguments["split-tlc"].as<bool>();
    bool solveOneSidedTLCs = arguments["split-tlc-solve-side"].as<bool>();
    bool reduceSPQR = arguments["reduce-spqr"].as<bool>();

    const std::string inst_fname = arguments["instance"].as<std::string>();
    const std::string output_fname = arguments["output"].as<std::string>();
    const unsigned k = arguments["k-value"].as<unsigned>();
    const unsigned seed = arguments["seed"].as<unsigned>();

    std::string ret = collectStats(inst_fname, k, seed, mkcp::DataReducerConfig{
                                       removeCliques, contractNegativeEdges, contractNegativeTriangles,
                                       contractSimilarVertices, splitTLCs, reduceSPQR, solveOneSidedTLCs
                                   });

    std::fstream output(output_fname, std::fstream::out);
    output << ret;
    output.close();

    return 0;
}
