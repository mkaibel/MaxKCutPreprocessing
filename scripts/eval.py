import glob

# Find all files
files = sorted(glob.glob("output/basic_*.txt"))

with open("experimental_results/experiments_basic.csv", "w") as outfile:
    print("type,instance,k,seed,used_data_reducer,used_extra_constraints,runtime,found_optimum,objective_value,lower_bound,upper_bound", file=outfile, end='\n')
    for fname in files:
        with open(fname) as infile:
            outfile.write(infile.readline().lstrip())

files = sorted(glob.glob("output/ablation_*.txt"))

with open("experimental_results/experiments_ablation.csv", "w") as outfile:
    print("type,instance,k,seed,cne,cnt,csv,clq,tlc,tlcs,spqr,runtime,found_optimum,objective_value,lower_bound,upper_bound", file=outfile, end='\n')
    for fname in files:
        with open(fname) as infile:
            outfile.write(infile.readline().lstrip())

files = sorted(glob.glob("output/stats_*.txt"))

with open("experimental_results/experiments_stats.csv", "w") as outfile:
    print("type,instance,k,seed,cne,cnt,csv,clq,tlc,tlcs,spqr,n,m,runtimePreprocessor,numKernels,nKernels,mKernels,offset,nMaxKernel,mMaxKernel,nLD,mLD,nFCLQ,mFCLQ,nCLQ,mCLQ,nDE,mDE,nDT,mDT,nCSV,mCSV,nDSV,mDSV,nSPQR,mSPQR,CCS,BCS,TLS,mTL,nTLCS,mTLCS,runtimeConstraints,numConstraints,runtimeKernelConstraints,numKernelConstraints", file=outfile, end='\n')
    for fname in files:
        with open(fname) as infile:
            outfile.write(infile.readline().lstrip())

files = sorted(glob.glob("output/heuristic_*.txt"))

with open("experimental_results/experiments_heuristic.csv", "w") as outfile:
    print("type,instance,k,seed,used_data_reducer,runtime,objective_value", file=outfile, end='\n')
    for fname in files:
        with open(fname) as infile:
            outfile.write(infile.readline().lstrip())