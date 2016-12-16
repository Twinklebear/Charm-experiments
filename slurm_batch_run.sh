#!/bin/bash
# Sample script for running on a cluster using a SLURM job submission system

PROG=./aobench/aobench.out
CHARMRUN=./aobench/charmrun
PROG_ARGS="+balancer GreedyCommLB --tile 32 32 --ao-samples 12 --spp 64"
SET_ENV_SCRIPT=./lmod_set_env.sh
WORK_DIR=$WORK/Charm-experiments/

cd $WORK_DIR
# Build the node list to pass to charmrun
nodelist=$(mktemp /tmp/${SLURM_JOB_NAME}-${SLURM_JOB_ID}.XXXXXXX)
worker_hosts=`scontrol show hostname $SLURM_NODELIST | tr '\n' ' '`
echo "group main ++cpus ${SLURM_CPUS_ON_NODE} ++shell ssh" > $nodelist
for host in ${worker_hosts[@]}; do
	echo "    host $host" >> $nodelist
done

total_procs=$(($SLURM_CPUS_ON_NODE * $SLURM_JOB_NUM_NODES))

set -x

$CHARMRUN ++runscript $SET_ENV_SCRIPT $PROG +p $total_procs ++ppn $SLURM_CPUS_ON_NODE \
	++nodelist $nodelist +isomalloc_sync $PROG_ARGS

rm $nodelist

