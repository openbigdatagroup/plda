#!/bin/bash

source $HOME/.profile

check_process() {
  echo "$ts: checking $1"
  [ "$1" = "" ]  && return 0
  [ `pgrep -n $1` ] && return 1 || return 0
}


ts=`date +%T`

# echo "$ts: Django setting: $DJANGO_SETTINGS_MODULE"
# echo -n "$ts: mls02 ip is " && ssh -G mls02 | awk '/^hostname / { print $2 }'
echo "$ts: check if lm_plda is running"
check_process "lm_plda"
[ $? -eq 0 ] && echo "$ts: not running, restarting..." && mpiexec -f /home/dev/plda/lm_hosts -n 17 /home/dev/plda/lm_plda
