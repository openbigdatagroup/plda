#!/bin/sh
docker build -t plda .

docker run -v $(readlink -f .):/root/plda -d -h master --name master plda

docker run -v $(readlink -f .):/root/plda -d --link=master:master --name node1 plda

docker run -v $(readlink -f .):/root/plda -d --link=master:master --name node2 plda

docker inspect --format '{{ .NetworkSettings.IPAddress }}' node1 node2 > hosts

docker exec master bash -c "cd /root/plda && make && mpiexec -f hosts -n 2 ./mpi_lda --num_topics 2 --alpha 0.1 --beta 0.01 --training_data_file testdata/test_data.txt --model_file /tmp/lda_model.txt --total_iterations 150"
