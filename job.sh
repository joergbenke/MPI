#!/bin/bash

#SBATCH --partition=std
#SBATCH --job-name=test
#SBATCH --time=00:05:00
#SBATCH --export=NONE     # Never forget that! Strange happenings ensue otherwise.

#SBATCH --output=result_integral.txt
#SBATCH --error=error_integral.txt 

#SBATCH --mail-user=joerg.benke@uni-hamburg.de
#SBATCH --mail-type=FAIL

#SBATCH --nodes=2
#SBATCH --ntasks-per-node=16

set -e # Good Idea to stop operation on first error.
source /sw/batch/init.sh

#
# Load environment modules for your application here.

module list
module unload env/system-gcc
module load env/gcc-5.3.0_openmpi-1.8.8
module list

#
# Actual work starting here. You might need to call
# srun or mpirun depending on your type of application
# for proper parallel work.
# Example for a simple command (that might itself handle
# parallelisation).

echo "----- START: SLURM settings -----"
echo
echo "SLURM JOBID = "$SLURM_JOBID
echo "SLURM working directory = "$SLURM_SUBMIT_DIR
echo "SLURM TMPDIR = "$TMPDIR
echo
echo "Number of nodes of the job" = $SLURM_NNODES
echo "SLURM JOB_NODELIST" = $SLURM_JOB_NODELIST
echo "Number of cores on node"=$SLURM_CPUS_ON_NODE
echo "Number of cores per task"=$SLURM_CPUS_PER_TASK

time mpirun -n 32 --map-by ppr:16:node -map-by core -bind-to core --report-bindings ./midpointrule_mpi_error_criterion

