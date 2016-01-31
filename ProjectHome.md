plda is a parallel C++ implementation of Latent Dirichlet Allocation (LDA) (1,2).  We are expecting to present a highly optimized parallel implemention of the Gibbs sampling algorithm for the training/inference of LDA (3).  The carefully designed architecture is expected to support extensions of this algorithm.

We will release an enhanced parallel implementation of LDA, named as PLDA+ (2), which can improve scalability of LDA by signiﬁcantly reducing the unparallelizable communication bottleneck and achieve good load balancing.

If you wish to publish any work based on plda, please cite our paper as:

Zhiyuan Liu, Yuzhou Zhang, Edward Y. Chang, Maosong Sun, PLDA+: Parallel Latent Dirichlet Allocation with Data Placement and Pipeline Processing. ACM Transactions on Intelligent Systems and Technology, special issue on Large Scale Machine Learning. 2011. Software available at http://code.google.com/p/plda.

If you have any questions, please visit http://groups.google.com/group/plda

The bibtex format is
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

References:

(1) PLDA+: Parallel Latent Dirichlet Allocation with Data Placement and Pipeline Processing. Zhiyuan Liu, Yuzhou Zhang, Edward Y. Chang, Maosong Sun. ACM Transactions on Intelligent Systems and Technology, special issue on Large Scale Machine Learning. 2011.
> http://plda.googlecode.com/files/plda_plus_acmtist2011.pdf
(2) PLDA: Parallel Latent Dirichlet Allocation for Large-scale Applications. Yi Wang, Hongjie Bai, Matt Stanton, Wen-Yen Chen, and Edward Y. Chang. AAIM 2009.
> > http://plda.googlecode.com/files/aaim.pdf
(3) Latent Dirichlet Allocation, Blei et al., JMLR (3), 2003.
> > http://www.cs.princeton.edu/~blei/papers/BleiNgJordan2003.pdf
(4) Finding scientific topics, Griffiths and Steyvers, PNAS (101), 2004.
> > http://www.pnas.org/content/101/suppl.1/5228.full.pdf
(5) Fast collapsed gibbs sampling for latent dirichlet allocation, Porteous et al., KDD 2008.
> > http://portal.acm.org/citation.cfm?id=1401960
(6) Distributed Inference for Latent Dirichlet Allocation, Newman et al., NIPS 2007.
> > http://books.nips.cc/papers/files/nips20/NIPS2007_0672.pdf

Papers using plda code:

(7) Collaborative Filtering for Orkut Communities: Discovery of User Latent Behavior. Wen-Yen Chen et al., WWW 2009.

> http://www.cs.ucsb.edu/~wychen/publications/fp365-chen.pdf