#include <chrono>
#include <fstream>
#include <string>

#include "cxxopts.hpp"
#include "networkit/graph/Graph.hpp"
#include "networkit/io/EdgeListReader.hpp"
#include "solver/maxKCutSolver.hpp"
#include "util/io.hpp"

std::string runExperiment(const std::string& instance,
                          const mkcp::PreprocessorSetting useDataReducer,
                          const bool useConstraintGenerator, unsigned k,
                          unsigned timeLimit, unsigned seed)
{
    NetworKit::Graph graph = NetworKit::EdgeListReader().read(instance);

    graph.indexEdges();
    graph.setMaintainCompactEdges();

    auto t0 = std::chrono::steady_clock::now();

    mkcp::MaxKCutSolver solver(graph, k, useDataReducer, useConstraintGenerator,
                               timeLimit, seed);

    solver.run();

    auto t1 = std::chrono::steady_clock::now();

    unsigned runtime =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::string fileName = getFilename(instance);

    if (solver.foundOptimum())
    {
        return "basic," + fileName + "," + std::to_string(k) + "," + std::to_string(seed) + "," +
            std::to_string(useDataReducer) + "," +
            std::to_string(useConstraintGenerator) + "," +
            std::to_string(runtime) + ",1," +
            std::to_string(solver.getObjectiveValue()) + "," +
            std::to_string(solver.getLowerBound()) + "," +
            std::to_string(solver.getUpperBound()) + "\n";
    }

    return "basic," + fileName + "," + std::to_string(k) + "," + std::to_string(seed) + "," +
        std::to_string(useDataReducer) + "," +
        std::to_string(useConstraintGenerator) + "," +
        std::to_string(runtime) + ",0,-1.0," +
        std::to_string(solver.getLowerBound()) + "," +
        std::to_string(solver.getUpperBound()) + "\n";
}

int main(int argc, char** argv)
{
    cxxopts::Options options("Benchmark max k cut preprocessing");
    options.add_options()("d,datareducer",
                          "choose if and the data reducer should be used (0 not at all, 1 full preprocessing, 2 naive preprocessing)",
                          cxxopts::value<unsigned>())(
        "c,constraint-generator",
        "choose if extra constraints should be used or not",
        cxxopts::value<bool>())("i,instance", "Choose instance file to be run",
                                cxxopts::value<std::string>())(
        "o,output", "Specify output file", cxxopts::value<std::string>())(
        "k,k-value", "", cxxopts::value<unsigned>())(
        "s,seed", "", cxxopts::value<unsigned>()->default_value("1234"))(
        "t,time-limit", "", cxxopts::value<unsigned>()->default_value("300"));

    options.parse_positional({"instance", "output"});
    auto arguments = options.parse(argc, argv);

    const unsigned useDataReducer = arguments["datareducer"].as<unsigned>();
    const bool useConstraintGenerator =
        arguments["constraint-generator"].as<bool>();
    const std::string inst_fname = arguments["instance"].as<std::string>();
    const std::string output_fname = arguments["output"].as<std::string>();
    const unsigned k = arguments["k-value"].as<unsigned>();
    const unsigned seed = arguments["seed"].as<unsigned>();
    const unsigned timeLimit = arguments["time-limit"].as<unsigned>();

    mkcp::PreprocessorSetting dataReducerSetting = mkcp::NO;
    if (useDataReducer == 1)
    {
        dataReducerSetting = mkcp::FULL;
    }
    else if (useDataReducer == 2)
    {
        dataReducerSetting = mkcp::NAIVE;
    }

    std::string ret = runExperiment(inst_fname, dataReducerSetting,
                                    useConstraintGenerator, k, timeLimit, seed);

    std::fstream output(output_fname, std::fstream::out);
    output << ret;
    output.close();

    return 0;
}
