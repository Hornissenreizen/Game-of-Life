#!/bin/bash
#SBATCH --job-name=game_of_life
#SBATCH --output=project.out
#SBATCH --error=project.err
#SBATCH --partition=short
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --time=0:30:00
#SBATCH --cpus-per-task=1
#SBATCH --exclusive

# compile with version 4.1.1
module load mpi/openmpi/4.1.1
mpicxx main.cpp -o game_of_life

# change the MPI version, because Draco is not set up correctly.
module load mpi/openmpi/4.1.0
mpirun -np 4 game_of_life
