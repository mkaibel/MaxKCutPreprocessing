# An import just runs the script file once
# from plotting_scripts import extra_constraints
# from plotting_scripts import kernelSize
# from plotting_scripts import solution_bounds_by_instance
# from plotting_scripts import number_solved_instances
# from plotting_scripts import runtime_per_k
# from plotting_scripts import runtime_per_instance
# from plotting_scripts import ablation_solved_instances
# from plotting_scripts import ablation_average_runtime

from plotting_scripts import tables_benchmark
from plotting_scripts import tables_heuristic
from plotting_scripts import tables_stats
from plotting_scripts import tables_ablation
from plotting_scripts import tables_instances

from plotting_scripts import plot_solved_instances

tables_benchmark.run()

print("\n==========\n Stats table \n==========\n\n")

tables_stats.run()

print("\n==========\n Heuristic table \n==========\n\n")

tables_heuristic.run()

print("\n==========\n Ablation Table table \n==========\n\n")

tables_ablation.run()

print("\n==========\n Plots \n==========\n\n")

plot_solved_instances.run()

print("\n==========\n Instance Stats \n==========\n\n")

tables_instances.run()
