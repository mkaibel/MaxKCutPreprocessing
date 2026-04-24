//
// Created by michael-kaibel on 10/03/2026.
//

#include <chrono>
#include <fstream>
#include <string>

#include "cxxopts.hpp"
#include "networkit/graph/Graph.hpp"
#include "networkit/io/EdgeListReader.hpp"
#include "util/io.hpp"

#include "preprocessing/maxKCutPreprocessor.hpp"
#include "solver/localSearchHeuristic.hpp"

std::string runHeuristicExperiment(const std::string& instance,
                                   const mkcp::PreprocessorSetting useDataReducer,
                                   unsigned k,
                                   unsigned timeLimit, unsigned seed)
{
    NetworKit::Graph graph = NetworKit::EdgeListReader().read(instance);

    graph.indexEdges();
    graph.setMaintainCompactEdges();

    NetworKit::edgeweight solutionVal = 0.0;

    auto t0 = std::chrono::steady_clock::now();

    if (useDataReducer == mkcp::PreprocessorSetting::NO)
    {
        mkcp::KCutLocalSearch localSearch(graph, k, timeLimit, seed);

        localSearch.run();
        solutionVal = localSearch.getSolutionValue();
    }
    else
    {
        mkcp::DataReducer dataReducer(graph, k, seed);

        std::cout << "Running reprocessor" << std::endl;
        dataReducer.run(useDataReducer == mkcp::PreprocessorSetting::NAIVE);
        std::cout << "Preprocessor done" << std::endl;

        NetworKit::edgeweight totalKernelEdgeWeights = 0.0;
        std::vector<NetworKit::edgeweight> edgeWeightByKernel(dataReducer.getGraphKernel().size(), 0.0);

        std::vector<mkcp::Partition> kernelSolutions;

        for (unsigned i = 0; i < dataReducer.getGraphKernel().size(); i++)
        {
            for (const NetworKit::WeightedEdge& e : dataReducer.getGraphKernel()[i].edgeWeightRange())
            {
                if (e.weight > 0.0)
                {
                    totalKernelEdgeWeights += e.weight;
                    edgeWeightByKernel[i] += e.weight;
                }
            }
        }

        for (unsigned i = 0; i < dataReducer.getGraphKernel().size(); i++)
        {
            auto t1 = std::chrono::steady_clock::now();
            unsigned runtime = static_cast<unsigned>(std::max(static_cast<double>(timeLimit - std::chrono::duration_cast<
                std::chrono::milliseconds>(
                t1 - t0).count()), 100.0) * std::max((edgeWeightByKernel[i] / totalKernelEdgeWeights), 0.00001));
            totalKernelEdgeWeights -= edgeWeightByKernel[i];

            mkcp::KCutLocalSearch localSearch(dataReducer.getGraphKernel()[i], k, runtime, seed);
            localSearch.run();

            kernelSolutions.emplace_back(localSearch.getSolution());
            solutionVal += localSearch.getSolutionValue();
        }

        // We run the reconstruction so it is included in the runtime
        dataReducer.constructFullSolution(kernelSolutions);
        solutionVal += dataReducer.getOffset();
    }

    auto t1 = std::chrono::steady_clock::now();

    unsigned runtime =
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    std::string fileName = getFilename(instance);

    return "heuristic," + fileName + "," + std::to_string(k) + "," + std::to_string(seed) + "," +
        std::to_string(useDataReducer) + "," +
        std::to_string(runtime) + "," +
        std::to_string(solutionVal) + "\n";
}

int main(int argc, char** argv)
{
    cxxopts::Options options("Benchmark max k cut preprocessing");
    options.add_options()("d,datareducer",
                          "choose if and the data reducer should be used (0 not at all, 1 full preprocessing, 2 naive preprocessing)",
                          cxxopts::value<unsigned>())("i,instance", "Choose instance file to be run",
                                cxxopts::value<std::string>())(
        "o,output", "Specify output file", cxxopts::value<std::string>())(
        "k,k-value", "", cxxopts::value<unsigned>())(
        "s,seed", "", cxxopts::value<unsigned>()->default_value("1234"))(
        "t,time-limit", "", cxxopts::value<unsigned>());

    options.parse_positional({"instance", "output"});
    auto arguments = options.parse(argc, argv);

    const unsigned useDataReducer = arguments["datareducer"].as<unsigned>();
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

    std::string ret = runHeuristicExperiment(inst_fname, dataReducerSetting, k, timeLimit * 1000, seed);

    std::fstream output(output_fname, std::fstream::out);
    output << ret;
    output.close();

    return 0;
}
