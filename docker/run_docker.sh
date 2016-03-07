#!/bin/sh
echo "Building plda docker images..."
docker build -t plda .

echo "Run dockers..."
docker run -v $(readlink -f ..):/root/plda -d -h master --name master plda

docker run -v $(readlink -f ..):/root/plda -d --link=master:master --name node1 plda

docker run -v $(readlink -f ..):/root/plda -d --link=master:master --name node2 plda

echo "Get the ip of node1 and node2..."
docker inspect --format '{{ .NetworkSettings.IPAddress }}' node1 node2 > hosts

echo "Training..."
docker exec master bash -c "cd /root/plda && make && mpiexec -f ./docker/hosts -n 2 ./mpi_lda --num_topics 2 --alpha 0.1 --beta 0.01 --training_data_file testdata/test_data.txt --model_file testdata/lda_model.txt --total_iterations 150"
echo "Finished."

echo "Stop and remove containers..."
docker stop master node1 node2

docker rm master node1 node2
