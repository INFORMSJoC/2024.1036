# README for CVRP and VRPTW Executable Usage

This README explains the parameters and options for running the CVRP (Capacitated Vehicle Routing Problem) and VRPTW (Vehicle Routing Problem with Time Windows) executables. It also provides examples of their usage.

## Directory Structure

### Root Directory
- **`bapcodframework/`**: Contains the BaPCod C++ library and other files needed by the CMake build system. For more details, refer to the README file inside the `bapcodframework/` directory and the user guide located in `bapcodframework/documentation/`.
  - **`bapcodframework/Applications/`**: Contains the folders with the source code for the CVRP (`cvrp/`) and VRPTW (`vrptw/`) BaPCod applications. The main files are:
    - **`<app folder>/src/Model.cpp`**: Contains the BCP model.
    - **`<app folder>/src/Clustering.cpp`**: Contains the implementation where the clusters are defined.
    - **`<app folder>/src/Cutsets.cpp`**: Calls the CutSet of the CVRPSep library.
    - **`<app folder>/src/RCSPsolver.cpp`**: Defines the RCSP graphs.
    - **`<app folder>/src/Branching.cpp`**: Contains the user-defined functor for separating branching constraints. The unique string which characterizes the branching constraints in the BaPCod output are:
      - **EDGE[$i, j$]** - edge branching on edge $(i,j)$.
      - **Cutset[$k$]** - CutSet branching on cutset $k$.
      - **DegCluster[$k$]** - Cluster branching on cluster $C_k$ degree (in the paper are the variables $\omega_{C_k}$).
      - **EDGE[$k, l$]** - Cluster branching on the sum of the edge variables between a pair of clusters $(C_k, C_l)$ (in the paper are the variables $\psi_{C_k, C_l}$).
      - **Ryan&Foster** - Ryan & Foster branching.
      - **VNB** - Branching on the number of routes.

      Please see Section 5 of the user guide in `bapcodframework/documentation/` for an explanation about the BaPCod outputs.
    - **`<app folder>/config/`**: This folder contains the configuration files used in the experiments. There are the following types of configuration files:
      - **bc_*.cfg**: Branch-and-Cut-and-Price algorithm configuration files. See Sections 3 and 6.1 of the user guide in `bapcodframework/documentation/` for an explanation about the BaPCod parameterization.  
      - **app_*.cfg**: Application-specific configuration files.
    - **(CVRP Specific) `cvrp/clusters/`**: Contains the clustering files for the XML benchmark. The clustering was obtained using Kmeans, Kmedoids, and DBSCAN, and the files are identified by `<instance name>_<clustering algorithm name>.txt`. The file format is intuitive.
  - **`bapcodframework/build/Applications/<app folder>/bin/`**: Contains the compiled executables for the CVRP and VRPTW applications.

## Parameters and Options

### Common Parameters
- `-i`: Input file path for the problem instance (e.g., .vrp or .txt file).
- `--cutOffValue`: The cutoff value for the objective function (e.g., total distance or cost).
- `-b`: Path to the branch-and-cut-and-price algorithm configuration file.
- `-a`: Path to the application configuration file.
- `-t`: Path to the output file where the branching-and-bound tree will be saved in DOT format. To convert the DOT file to a PDF, use the command: `dot -Tpdf file.dot -o file.pdf`.

### Optional Parameters (Common)
- `--roundDistances`: Boolean option to round distances (default is `true`).
- `--enableEdgeBranching`: Boolean option to enable edge branching (default is `true`).
- `--enableCutsetsBranching`: Boolean option to enable cutsets branching (default is `false`).
- `--enableRyanFoster`: Boolean option to enable Ryan-Foster branching (default is `false`).
- `--enableClusterBranching`: Boolean option to enable cluster branching (default is `false`).
- `--clusterBranchingMode`: Mode for cluster branching (default is `1`). If set to `1`, MST-based clustering is called inside the code. If set to `2`, a file with the clusters must be specified using the `--clustersFilePath` option.
- `--enableSingletons`: Boolean option to allow singleton clusters (default is `false`).
- `--enableBigClusters`: Boolean option to allow the use of big generated clusters (default is `false`), otherwise, big clusters are split following some rule.
- `--stDevMultiplier`: Specifies the value of the $\vartheta$ parameter in the MST-based clustering (e.g., 0.5, 1.0, or 1.5).
- `--clustersFilePath`: Path to the file containing cluster information.

Options `--enableSingletons`, `--enableBigClusters`, and `--stDevMultiplier` only take effect when `--clusterBranchingMode` is set to 1, that is when using the MST-based clustering.

**Note:** In our experiments using the MST-based clustering, `--enableSingletons` and `--enableBigClusters` are always set to `true`.

### Optional Parameters (CVRP Specific)
- `--dcvrp`: Boolean option to indicate that the problem is a Distance Constrained VRP (default is `false`).
- `--enableCapacityResource`: Boolean option to enable capacity resource constraints on the RCSP graph (default is `true`).

## Example Usages

### Running CVRP on CMT13
This example is for the experiment reported in Section 3 of the paper. The options `--dcvrp true`, `--roundDistances false`, and `--enableCapacityResource false` are used. The configuration files `bc_CMT.cfg` and `app.cfg` are located in `cvrp/config/`.
```
bin/cvrp -i data/CMT/CMT13.vrp --cutOffValue 1541.15 -b config/bc_CMT.cfg -a config/app.cfg -t outBaPTree-CMT13.dot --dcvrp true --roundDistances false --enableCapacityResource false --enableCutsetsBranching true --enableEdgeBranching false --enableClusterBranching true --enableSingletons true --enableBigClusters true --stDevMultiplier 1.0
```

### Running CVRP on X benchmark
The configuration file `app.cfg` is applied in all X benchmark tests. However, the `bc_*.cfg` depends on the average route size. If `n/k < 15` use the `bc_nodememory.cfg`, otherwise use `bc_arcmemory.cfg`.

Examples:
```
bin/cvrp -i /home/jsilva/datasets/cvrp/X/X-n120-k6.vrp --cutOffValue 13333 -b config/bc_arcmemory.cfg -a config/app.cfg -t outBaPTree-X-n120-k6.dot --enableRyanFoster true --enableEdgeBranching false
```

```
bin/cvrp -i data/X/X-n129-k18.vrp --cutOffValue 28941 -b config/bc_nodememory.cfg -a config/app.cfg -t outBaPTree-X-n129-k18.dot --enableCutsetsBranching true --enableEdgeBranching false --enableClusterBranching true --enableSingletons true --enableBigClusters true --stDevMultiplier 1.5
```

### Running CVRP on XML benchmark
The configuration file `app.cfg` is applied in all XML benchmark tests. However, the `bc_<k>.cfg` with $k \in \{1,..,6\}$ indicating the class of the instance average route size. For the XML benchmark, you can find the average route size class of each instance in the **supplementary_material.xlsx** (sheet XML, column E) in the root directory.

Examples:
```
bin/cvrp -i data/XML/XML100_1254_04.vrp --cutOffValue 9868 -b config/4bc.cfg -a config/app.cfg -t outBaPTree-XML100_1254_04.dot --enableCutsetsBranching true --enableEdgeBranching false
```

**Note:** For the Kmeans, Kmedoids, and DBSCAN clustering algorithms, the clusters used by the CB are provided in `cvrp/clusters/`. This was made to ensure that the same clustering are used in all experiments since those algorithms can involve randomness in their initialization or selection process, which can lead to different results in different runs.

```
bin/cvrp -i data/XML/XML100_1322_10.vrp --cutOffValue 24688 -b config/2bc.cfg -a config/app.cfg -t outBaPTree-XML100_1322_10.dot --enableClusterBranching true --clusterBranchingMode 2 --clustersFilePath clusters/XML100_1322_10_dbscan.txt --enableCutsetsBranching true --enableEdgeBranching false
```

### Running VRPTW on Homberger benchmark: Class 1 instances
This example is for a Class 1 instance. All instances of this class in the experiments reported in Section 5.3 of the paper use `bc_set1.cfg` and `app_set1.cfg`. These files are located in the `vrptw/config/`.
```
bin/vrptw -i data/Homberger/C1_2_2.txt --cutOffValue 2694.4 -b config/bc_set1.cfg -a config/app_set1.cfg -t outBaPTree-C1_2_2.dot
```

### Running VRPTW on Homberger benchmark: Class 2 instances
This example is for a Class 2 instance. All instances of this class in the experiments reported in Section 5.3 of the paper use `bc_set2.cfg` and `app_set2.cfg`. These files are located in the `vrptw/config/`.
```
bin/vrptw -i data/Homberger/RC2_2_10.txt --cutOffValue 1989.3 -b config/bc_set2.cfg -a config/app_set1.cfg -t outBaPTree-RC2_2_10.dot --enableCutsetsBranching true --enableEdgeBranching false --enableClusterBranching true --enableSingletons true --enableBigClusters true --stDevMultiplier 0.5
```

**Warning:** Options `--enableSingletons` and `--enableBigClusters` are always true in our experiments using the MST-based clustering.
