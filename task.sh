#!/bin/bash
#SBATCH --job-name=game_of_life
#SBATCH --output=project.out
#SBATCH --error=project.err
#SBATCH --partition=short
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --time=0:30:00
#SBATCH --cpus-per-task=1
#SBATCH --exclusive

make
