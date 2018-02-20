#!/bin/bash

source $HOME/.profile

check_process() {
  echo "$ts: checking $1"
  [ "$1" = "" ]  && return 0
  [ `pgrep -n $1` ] && return 1 || return 0
}


ts=`date +%T`
echo "$ts: begin checking..."
check_process "lm_plda"
[ $? -eq 0 ] && echo "$ts: not running, restarting..." && mpiexec -f /home/dev/plda/lm_hosts -n 17 /home/dev/plda/lm_plda >> /var/log/lm_plda.log 2>&1

