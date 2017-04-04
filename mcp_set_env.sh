#!/bin/bash
# Sample run script to setup environment variables for the spawned
# Charm processes on a module based system

module load gcc/6.2.0

# Run the rest of the arguments, which will be the program followed by its args
$*

