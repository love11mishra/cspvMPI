Thesis work...

In this work, we have used crest as concolic execution tool and PAT (Process Analysis Toolkit) as the CSP model checker. Please note that a part of this project needs a modified version of crest.

# Installation
	1. `It requires a modified version of crest, to get that please email at pkalita@cse.iitk.ac.in.`
	2. `Please install PAT, from the link given in Important material section.`
	3. `After successful installation of crest and PAT, please set the variables 'crest_dir' and 'pat_dir' in the file src/Verifier.py.`
	4. `To see the usage run 'python3 Verifier.py --help'.`

# Important material
## 1. PAT
	1. [PAT: Process Analysis Toolkit]<https://pat.comp.nus.edu.sg/>
	2. [PAT & CSP Resources](https://pat.comp.nus.edu.sg/?page_id=2611)
	3. [PAT User Manual](https://pat.comp.nus.edu.sg/wp-source/resources/OnlineHelp/htm/index.htm)
## 2. CSP
	1. [Communicating sequential processes](https://doi.org/10.1145/359576.359585)
	2. [The Theory and Practice of Concurrency](https://dl.acm.org/doi/10.5555/550448)
## 3. Other Important links
	1. [MPI-SV](https://mpi-sv.github.io/)
	2. [MPI 3.1 Draft](https://www.mpi-forum.org/docs/mpi-3.1/mpi31-report.pdf)

**Important docker commands**
* To create a named container and run a shell in it
	1. `docker run -it --name <container-name> <image> /bin/bash`
	2. `docker start -ia <container-name>`
* Running docker using files on host machine
		1. Compile MPI-SV:  $pwd will be mounted to /test directory inside the container
			`docker run -v $(pwd):/test mpisv/mpi-sv mpisvcc /test/<filename>.c -o /test/<filename>`
		2. Run MPI-SV: `docker run -v $(pwd):/test mpisv/mpi-sv mpisv <#processes> -wild-opt -use-directeddfs-search /test/<filename>`




