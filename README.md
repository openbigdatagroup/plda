# Introduction #

Plda is a parallel C++ implementation of Latent Dirichlet Allocation (LDA) (1,2). We are expecting to present a highly optimized parallel implemention of the Gibbs sampling algorithm for the training/inference of LDA (3). The carefully designed architecture is expected to support extensions of this algorithm.

We will release an enhanced parallel implementation of LDA, named as PLDA+ (1), which can improve scalability of LDA by signiﬁcantly reducing the unparallelizable communication bottleneck and achieve good load balancing.

# Requirement #
  * Parallel lda must be run in linux environment with g++ compiler and mpich2-1.0.8 installed.

# Quick Start #

## Installation #
  * Install mpich2
      * Download mpich2-1.0.8.tar.gz  
      `wget http://www.mpich.org/static/downloads/1.0.8/mpich2-1.0.8.tar.gz`
      * Extract it to a path  
      `./configure`  
      `make && make install`
      * After installing, you will find some binaries and scripts in $PATH. Test by running `mpd` to see if it exists
  * Configure mpich2
    * Create password file `~/.mpd.conf` with access mode 600 (rw-------) in home directory. The file should contain a single line `MPD_SECRETWORD=PASSWORD`. Because you may have many  machines, you must do this on each machine.  
      `touch ~/.mpd.conf`  
      `chmod 600 ~/.mpd.conf`  
      `cat "MPD_SECRETWORD=anypassword" > ~/.mpd.conf`
    * Pick one machine as the master and startup mpd(mpi daemon)  
      `mpd --daemon --listenport=55555`
    * Other machines act as slave and must connect to the master  
      `mpd --daemon -h serverHostName -p 55555`
    * Check whether the environment is setup successfully: on master, run `mpdtrace`, you will see all the slaves. If no machines show up, you must check your mpi setup and refer to mpich2 user manual.
  * Install plda
    * Download and build plda  
     `git clone https://github.com/obdg/plda.git`  
     `cd plda`  
     `make all`
    * You will see a binary file `lda`, `mpi_lda` and `infer` generated in the folder
    * We use mpich2 builtin compiler mpicxx to compile, it is a wrap of g++.

## Data Format #
  * Data is stored using a sparse representation, with one document per line. Each line is the words of this document together with the word count. The format of the data file is:

    ```
    <word1> <word1_count> <word2> <word2_count> <word3> <word3_count> ...
    .
    .
    .
    ```
  * Each word is an arbitrary string, but it is not expected to contain space/newline or other special characters.
  * Example: Suppose there are two documents. The first one is `"a is a character"`; The second one is `"b is a character after a"`. Then the datafile would look like:

    ```
    a 2 is 1 character 1
    a 2 is 1 b 1 character 1 after 1
    ```

## Usage and Examples #
  * Train
      * `./lda --num_topics 2 --alpha 0.1 --beta 0.01 --training_data_file testdata/test_data.txt --model_file /tmp/lda_model.txt --burn_in_iterations 100 --total_iterations 150`
      * After training completes, you will see a file `/tmp/lda_model.txt` generated. This file stores the training result. Each line is the topic distribution of a word. The first element is the word string, then its occurrence count within each topic. You could use view\_model.py to convert the model to a readable text.


  * Train parallelly
      * Prepare data the same as the single processor version.
      * `mpiexec -n 5 ./mpi_lda --num_topics 2 --alpha 0.1 --beta 0.01 --training_data_file testdata/test_data.txt --model_file /tmp/lda_model.txt --total_iterations 150`
      * The input and output are the same with single processor version.


  * Training flags
      * `alpha`: Suggested to be 50/number\_of\_topics
      * `beta`: Suggested to be 0.01
      * `num_topics`: The total number of topics.
      * `total_iterations`: The total number of GibbsSampling iterations.
      * `burn_in_iterations`: After --burn\_in\_iterations iteration, the model will be almost converged. Then we will average models of the last (total\_iterations-burn\_in\_iterations) iterations as the final model. This only takes effect for single processor version. For example: you set total\_iterations to 200, you found that after 170 iterations, the model is almost converged. Then you could set burn\_in\_iterations to 170 so that the final model will be the average of the last 30 iterations.
      * `model_file`: The output file of the trained model.
      * `training_data_file`: The training data.
      * Inferring flags:
      * `alpha` and `beta` should be the same with training.
      * `total_iterations`: The total number of GibbsSampling iterations for an unseen document to determine its word topics. This number needs not be as much as training, usually tens of iterations is enough.
      * `burn_in_iterations`: For an unseen document, we will average the document\_topic\_distribution of the last (total\_iterations-burn\_in\_iterations) iterations as the final document\_topic\_distribution.


  * Infer unseen documents:
      * `./infer --alpha 0.1 --beta 0.01 --inference_data_file testdata/test_data.txt --inference_result_file /tmp/inference_result.txt --model_file /tmp/lda_model.txt --total_iterations 15 --burn_in_iterations 10`

# Citation #

If you wish to publish any work based on plda, please cite our paper as:

```
Zhiyuan Liu, Yuzhou Zhang, Edward Y. Chang, Maosong Sun, PLDA+: Parallel Latent Dirichlet Allocation with Data Placement and Pipeline Processing. ACM Transactions on Intelligent Systems and Technology, special issue on Large Scale Machine Learning. 2011. Software available at http://code.google.com/p/plda.
```

The bibtex format is:

```
@article{
  plda,
  author = {Zhiyuan Liu and Yuzhou Zhang and Edward Y. Chang and Maosong Sun},
  title = {PLDA+: Parallel Latent Dirichlet Allocation with Data Placement and Pipeline Processing},
  year = {2011},
  journal = {ACM Transactions on Intelligent Systems and Technology, special issue on Large Scale Machine Learning},
  note = {Software available at \url{http://code.google.com/p/plda}}
}
```

If you have any questions, please visit http://groups.google.com/group/plda

# References #

(1) PLDA+: Parallel Latent Dirichlet Allocation with Data Placement and Pipeline Processing. Zhiyuan Liu, Yuzhou Zhang, Edward Y. Chang, Maosong Sun. ACM Transactions on Intelligent Systems and Technology, special issue on Large Scale Machine Learning. 2011.
> http://plda.googlecode.com/files/plda_plus_acmtist2011.pdf

(2) PLDA: Parallel Latent Dirichlet Allocation for Large-scale Applications. Yi Wang, Hongjie Bai, Matt Stanton, Wen-Yen Chen, and Edward Y. Chang. AAIM 2009.
> http://plda.googlecode.com/files/aaim.pdf

(3) Latent Dirichlet Allocation, Blei et al., JMLR (3), 2003.
> http://www.cs.princeton.edu/~blei/papers/BleiNgJordan2003.pdf

(4) Finding scientific topics, Griffiths and Steyvers, PNAS (101), 2004.
> http://www.pnas.org/content/101/suppl.1/5228.full.pdf

(5) Fast collapsed gibbs sampling for latent dirichlet allocation, Porteous et al., KDD 2008.
> http://portal.acm.org/citation.cfm?id=1401960

(6) Distributed Inference for Latent Dirichlet Allocation, Newman et al., NIPS 2007.
> http://books.nips.cc/papers/files/nips20/NIPS2007_0672.pdf

Papers using plda code:

(7) Collaborative Filtering for Orkut Communities: Discovery of User Latent Behavior. Wen-Yen Chen et al., WWW 2009.
> http://www.cs.ucsb.edu/~wychen/publications/fp365-chen.pdf
