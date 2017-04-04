#!/bin/bash
# Sample script for running on a cluster using a SLURM job submission system

PROG_DIR=./pathtracer
PROG=$PROG_DIR/pathtracer.out
CHARMRUN=$PROG_DIR/charmrun
PROG_ARGS="+balancer GreedyCommLB --tile 32 32 --pos 0 0 4 --target 0 0 0 --up 0 1 0 --img 40 40 --spp 2048"
SET_ENV_SCRIPT=./mcp_set_env.sh
WORK_DIR=~/Repos/Charm-experiments/

cd $WORK_DIR
# Build the node list to pass to charmrun
nodelist=$(mktemp /tmp/${SLURM_JOB_NAME}-${SLURM_JOB_ID}.XXXXXXX)
worker_hosts=`scontrol show hostname $SLURM_NODELIST | tr '\n' ' '`
echo "group main ++cpus ${SLURM_CPUS_ON_NODE} ++shell ssh" > $nodelist
for host in ${worker_hosts[@]}; do
	echo "    host $host" >> $nodelist
done

cat $nodelist

total_procs=$(($SLURM_CPUS_ON_NODE * $SLURM_JOB_NUM_NODES))

module load gcc/6.2.0

set -x

# Will: unsure on ++mpiexec for TACC systems, but on mcp this seems to be required
# to get the program to run properly
$CHARMRUN ++mpiexec ++runscript $SET_ENV_SCRIPT $PROG +p $total_procs ++ppn $SLURM_CPUS_ON_NODE \
	++nodelist $nodelist +isomalloc_sync $PROG_ARGS

rm $nodelist

