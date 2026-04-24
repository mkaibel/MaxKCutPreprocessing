//
// Created by michael-kaibel on 09/03/2026.
//

#include <chrono>
#include <fstream>
#include <string>

#include "cxxopts.hpp"
#include "networkit/graph/Graph.hpp"
#include "networkit/io/EdgeListReader.hpp"
#include "preprocessing/maxKCutPreprocessor.hpp"
#include "solver/maxKCutSolver.hpp"
#include "util/io.hpp"

std::string runAblationExperiment(const std::string& instance,
                                  mkcp::DataReducerConfig config, unsigned k,
                                  unsigned timeLimit, unsigned seed)
{
    NetworKit::Graph graph = NetworKit::EdgeListReader().read(instance);

    graph.indexEdges();
    graph.setMaintainCompactEdges();

    auto t0 = std::chrono::steady_clock::now();

    mkcp::MaxKCutSolver solver(graph, k, mkcp::PreprocessorSetting::FULL, false,
                               timeLimit, seed, {true, true, true, true}, config);

    solver.run();

    auto t1 = std::chrono::steady_clock::now();

    unsigned runtime =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::string fileName = getFilename(instance);

    if (solver.foundOptimum())
    {
        return "ablation," + fileName + "," + std::to_string(k) + "," + std::to_string(seed) + "," +
            std::to_string(config.contractNegativeEdges) + "," +
            std::to_string(config.contractNegativeTriangles) + "," +
            std::to_string(config.contractSimilarVertices) + "," +
            std::to_string(config.removeCliques) + "," +
            std::to_string(config.splitTwoLayer) + "," +
            std::to_string(config.TwoLayerSmallSide) + "," +
            std::to_string(config.SPQRReduction) + "," +
            std::to_string(runtime) + ",1," +
            std::to_string(solver.getObjectiveValue()) + "," +
            std::to_string(solver.getLowerBound()) + "," +
            std::to_string(solver.getUpperBound()) + "\n";
    }

    return "ablation," + fileName + "," + std::to_string(k) + "," + std::to_string(seed) + "," +
        std::to_string(config.contractNegativeEdges) + "," +
        std::to_string(config.contractNegativeTriangles) + "," +
        std::to_string(config.contractSimilarVertices) + "," +
        std::to_string(config.removeCliques) + "," +
        std::to_string(config.splitTwoLayer) + "," +
        std::to_string(config.TwoLayerSmallSide) + "," +
        std::to_string(config.SPQRReduction) + "," +
        std::to_string(runtime) + ",0,-1.0," +
        std::to_string(solver.getLowerBound()) + "," +
        std::to_string(solver.getUpperBound()) + "\n";
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
        "spqr,reduce-spqr", "Should SPQR-trees be employed", cxxopts::value<bool>())(
        "tlcs,split-tlc-solve-side", "Should two-layer-connectors be split where one side can be solved easily", cxxopts::value<bool>())(
        "k,k-value", "", cxxopts::value<unsigned>())(
        "s,seed", "", cxxopts::value<unsigned>()->default_value("1234"))(
        "t,time-limit", "", cxxopts::value<unsigned>()->default_value("300"));

    options.parse_positional({"instance", "output"});
    auto arguments = options.parse(argc, argv);

    bool contractNegativeEdges = arguments["contract-negative-edges"].as<bool>();
    bool contractNegativeTriangles = arguments["contract-negative-triangles"].as<bool>();
    bool contractSimilarVertices = arguments["contract-similar-vertices"].as<bool>();
    bool removeCliques = arguments["remove-cliques"].as<bool>();
    bool splitTLCs = arguments["split-tlc"].as<bool>();
    bool solveOneSidedTLCs = arguments["split-tlc-solve-side"].as<bool>();
    bool reduceSPQRs = arguments["reduce-spqr"].as<bool>();

    const std::string inst_fname = arguments["instance"].as<std::string>();
    const std::string output_fname = arguments["output"].as<std::string>();
    const unsigned k = arguments["k-value"].as<unsigned>();
    const unsigned seed = arguments["seed"].as<unsigned>();
    const unsigned timeLimit = arguments["time-limit"].as<unsigned>();

    std::string ret = runAblationExperiment(inst_fname, mkcp::DataReducerConfig{
                                                removeCliques, contractNegativeEdges, contractNegativeTriangles,
                                                contractSimilarVertices, splitTLCs, reduceSPQRs, solveOneSidedTLCs
                                            }, k, timeLimit, seed);

    std::fstream output(output_fname, std::fstream::out);
    output << ret;
    output.close();

    return 0;
}
