[![INFORMS Journal on Computing Logo](https://INFORMSJoC.github.io/logos/INFORMS_Journal_on_Computing_Header.jpg)](https://pubsonline.informs.org/journal/ijoc)

# Cluster Branching for Vehicle Routing Problems

This archive is distributed in association with the [INFORMS Journal on
Computing](https://pubsonline.informs.org/journal/ijoc) under the [MIT License](LICENSE).

The software and data in this repository are a snapshot of the software and data
that were used in the research reported on in the paper 
[Cluster Branching for Vehicle Routing Problems](https://doi.org/10.1287/ijoc.2024.1036) by Joao Marcos Pereira Silva, Eduardo Uchoa, and Anand Subramanian. 


## Cite

To cite the contents of this repository, please cite both the paper and this repo, using their respective DOIs.

https://doi.org/10.1287/ijoc.2024.1036

https://doi.org/10.1287/ijoc.2024.1036.cd

Below is the BibTex for citing this snapshot of the repository.

```
@misc{CBforVRPs,
  author =        {Joao Marcos Pereira Silva and Eduardo Uchoa and Anand Subramanian},
  publisher =     {INFORMS Journal on Computing},
  title =         {{Cluster Branching for Vehicle Routing Problems}},
  year =          {2025},
  doi =           {10.1287/ijoc.2024.1036.cd},
  url =           {https://github.com/INFORMSJoC/2024.1036},
  note =          {Available for download at https://github.com/INFORMSJoC/2024.1036},
}  
```

## Description

This distribution includes the source code and the data sets needed to reproduce the results of the paper
"Cluster Branching for Vehicle Routing Problems" by Joao Marcos Pereira Silva, Eduardo Uchoa, and Anand Subramanian
published in the Operations Research journal.


## Requirements

In order to run the code, one needs 

- Branch-Cut-and-Price solver **BaPCod** (version 0.82.5 and above) which can be downloaded for free (for academic use only) on its web-site https://bapcod.math.u-bordeaux.fr/
- MILP solver **IBM ILOG Cplex** (version 20.1 and above) which can be downloaded also for free (academic edition only) on its web-site https://www.ibm.com/products/ilog-cplex-optimization-studio.

BaPCod also requires free softwares **CMake**, **Boost**, and **LEMON**. Installation instructions for BaPCod, Boost, and LEMON libraries can be found in the README file distributed with BaPCod. 

The *CVRPSep* library may also be required. For instructions on integrating this library with BaPCod, please refer to the documentation distributed with BaPCod.

## Compiling code

1. Install *Cplex*, *CMake*, *Boost*, *LEMON*, and *BaPCod*. We suppose that *BaPCod* is installed to the folder *BapcodFramework*. 
2. Create the folder *BapcodFramework/Applications/cvrp* and put all files from this distribution inside this folder. For example this file should have path *BapcodFramework/Applications/cvrp/README.md*
3. Open file *BapcodFramework/Applications/CMakeLists.txt* and add *cvrp* directory inside *add_subdirectories(...)*
4. Go to folder *BapcodFramework* and type the following on Linux or Mac OS
        
        cmake -B "build"  

    Then, compilation can be launched with 

        cmake --build build --config Release --target cvrp -j 8

For further details on compiling BaPCod applications, please refer to the documentation.


## Results

The detailed results are provided in the `results` directory.

## Data and Replication

- Since the paper evaluates the proposed methodology on two VRP problems, the `src` directory contains a separate folder for each of them.
- Instructions to replicate each experiment are provided in the corresponding `README.md` file within these folders.
- The experiments reported in the paper were conducted using three benchmark sets:

1. **The XML benchmark** (Queiroga et al., 2021) 
   Reference: Queiroga E, Sadykov R, Uchoa E, Vidal T (2021). *10,000 optimal CVRP solutions for testing machine learning based heuristics.* AAAI-22 Workshop on Machine Learning for Operations Research (ML4OR).

2. **The X benchmark** (Uchoa et al., 2017) 
   Reference: Uchoa E, Pecin D, Pessoa A, Poggi M, Vidal T, Subramanian A (2017). *New benchmark instances for the capacitated vehicle routing problem.* European Journal of Operational Research, 257(3):845–858.

3. **The Homberger and Gehring benchmark** (Gehring and Homberger, 1999)
   Reference: Gehring H, Homberger J (1999). *A parallel hybrid evolutionary metaheuristic for the vehicle routing problem with time windows.* Proceedings of EUROGEN99, volume 2, 57–64.

Please note that these datasets are **not included** in this repository. Anyone interested in using them should consult the original references.
All datasets can also be accessed online at [VRP Web Page, PUC-Rio](https://vrp.atd-lab.inf.puc-rio.br/index.php/en/).


## Support

The code is not supported.
