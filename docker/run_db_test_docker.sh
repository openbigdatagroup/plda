#!/bin/bash
echo "Usage: ./run_docker.sh MPI_Node_Number (example: ./run_docker 3)"

if [[ `docker images -q plda:latest 2> /dev/null` == "" ]]; then
  echo "Docker image plda does not exist, build it first..."
  docker build -t plda .
fi

NODE_NUM=${1:-1}
if [ $NODE_NUM -le 0 ]; then  NODE_NUM=1; fi
echo "Setting up $NODE_NUM node"

echo "Run dockers and collect ips..."
# at least setting up 1 node called master
docker run -v $(readlink -f ..):/root/plda -d -h master --name plda-master plda
docker inspect --format '{{ .NetworkSettings.IPAddress }}' plda-master > hosts
for((i=2; i<=$NODE_NUM; i++)); do
  docker run -v $(readlink -f ..):/root/plda -d --link=plda-master:master --name plda-node-$i plda
  docker inspect --format '{{ .NetworkSettings.IPAddress }}' plda-node-$i >> hosts
done

echo "Building..."
docker exec plda-master bash -c "cd /root/plda && make clean && make"

echo "Training..."
docker exec plda-master bash -c "time mpiexec -f ./docker/hosts -n $NODE_NUM ./mpi_lda --num_topics 2 --alpha 0.1 --beta 0.01 --training_data_file testdata/test_data.txt --model_file testdata/lda_model.txt --total_iterations 150"
echo "Finished training."

echo "Stop and remove containers..."
docker stop plda-master
docker rm plda-master
for((i=2; i<=$NODE_NUM; i++)); do
  docker stop plda-node-$i
  docker rm plda-node-$i
done
echo "Finished."
