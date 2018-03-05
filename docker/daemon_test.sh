#!/bin/bash
echo "Usage: ./run_docker.sh MPI_Node_Number (example: ./run_docker 3)"

if [[ `docker images -q db_plda:latest 2> /dev/null` == "" ]]; then
  echo "Docker image db_plda does not exist, build it first..."
  docker build -f db_test_dockerfile -t  db_plda .
fi

NODE_NUM=${1:-1}
if [ $NODE_NUM -le 0 ]; then  NODE_NUM=1; fi
echo "Setting up $NODE_NUM node"

echo "Run dockers and collect ips..."
# at least setting up 1 node called master
docker run -v $(greadlink -f ..):/root/plda -d -h master --name plda-master -e DJANGO_SETTINGS_MODULE='docker' db_plda
docker inspect --format '{{ .NetworkSettings.IPAddress }}' plda-master > hosts


echo "Building..."
docker exec plda-master bash -c "cd /root/plda && make clean && make daemon_test"

echo "executing"
docker exec plda-master bash -c "./daemon_test"

docker exec plda-master bash -c "sleep 20"


docker cp plda-master:/var/log/daemon_test.log .

echo "Stop and remove containers..."
docker stop plda-master
docker rm plda-master

echo "Finished."
