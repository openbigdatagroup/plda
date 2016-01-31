# Introduction #


Parallel lda must be run in linux environment with mpich2-1.0.8.


# Installation #

  * Parallel Infrastructure is based on MPI. Firstly mpich2 must be installed before running parallel lda. You can download it from http://www.mcs.anl.gov/research/projects/mpich2/.
    * Install mpich2
      * Download mpich2-1.0.8.tar.gz
      * Extract it to a path
      * `./configure`
      * `make`
      * `make install`
      * After installing, you will find some binaries and scripts in $PATH. Test by running `mpd` to see if it exists
    * Create password file `~/.mpd.conf` with access mode 600 (rw-------) in home directory. The file should contain a single line `MPD_SECRETWORD=PASSWORD`. Because you may have many  machines, you must do this on each machine.
      * `touch ~/.mpd.conf`
      * `chmod 600 ~/.mpd.conf`
      * `cat "MPD_SECRETWORD=anypassword" > ~/.mpd.conf`
    * Pick one machine as the master and startup mpd(mpi daemon)
      * `mpd --daemon --listenport=55555`
    * Other machines act as slave and must connect to the master
      * `mpd --daemon -h serverHostName -p 55555`
    * Check whether the environment is setup successfully: on master, run `mpdtrace`, you will see all the slaves. If no machines show up, you must check your mpi setup and refer to mpich2 user manual.
  * Download plda package and extract and run `make mpi_lda` in its directory. You will see a new binary generated `mpi_lda`. We use mpich2 builtin compiler mpicxx to compile, it is a wrap of g++.

# Train parallelly #
  * Prepare data the same as the single processor version.
  * `mpiexec -n 5 ./mpi_lda --num_topics 2 --alpha 0.1 --beta 0.01 --training_data_file testdata/test_data.txt --model_file /tmp/lda_model.txt --total_iterations 150`
  * The input and output are the same with single processor version.

# Parallel Training Performance #
  * Memory:
    * All the documents are distributely stored. If your corpus is huge, you could add more machines.
    * The model is replicatedly stored on all machines. The memory cost is NUM\_VOCABULARY `*` NUM\_TOPICS `*` sizeof(int64) bytes, you have to make sure all machines' memory must be greater than this. If not, you have to reduce your vocabulary size or reduce num\_topics.
  * Speedup:
    * For each iteration, all computers do GibbsSampling parallelly. At the end of iterations, they reduces updated model. Generally, if the corpus is large and the model is small, the speedup is almost linear. If the model is large and the corpus is small, the speedup may be worse than linear.