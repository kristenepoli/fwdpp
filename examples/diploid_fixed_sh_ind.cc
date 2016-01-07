/*
  \include diploid_fixed_sh_ind.cc
  
  Same as diploid_fixed_sh.cc, but individual-based
 */

#include <fwdpp/diploid.hh>
#include <Sequence/SimData.hpp>
#include <numeric>
#include <functional>
#include <cassert>
#include <iomanip>
#include <fwdpp/sugar/infsites.hpp>
#define SINGLEPOP_SIM
//the type of mutation
using mtype = KTfwd::mutation;
#include <common_ind.hpp>

using namespace KTfwd;

int main(int argc, char ** argv)
{
  if (argc != 11)
    {
      std::cerr << "Too few arguments.\n"
		<< "Usage: diploid_fixed_sh_ind N theta_neutral theta_deleterious rho s h ngens samplesize nreps seed\n";
      exit(10);
    }
  int argument=1;
  const unsigned N = atoi(argv[argument++]);
  const double theta_neutral = atof(argv[argument++]);
  const double theta_del = atof(argv[argument++]);
  const double rho = atof(argv[argument++]);
  const double s = atof(argv[argument++]);
  const double h = atof(argv[argument++]);
  const unsigned ngens = atoi(argv[argument++]);
  const unsigned samplesize1 = atoi(argv[argument++]);
  int nreps = atoi(argv[argument++]);
  const unsigned seed = atoi(argv[argument++]);

  const double mu_neutral = theta_neutral/double(4*N);
  const double mu_del = theta_del/double(4*N);
  const double littler = rho/double(4*N);

  std::copy(argv,argv+argc,std::ostream_iterator<char*>(std::cerr," "));
  std::cerr << '\n';

  GSLrng r(seed);

  //recombination map is uniform[0,1)
  std::function<double(void)> recmap = std::bind(gsl_rng_uniform,r.get());

  while(nreps--)
    {
      singlepop_t pop(N);
      pop.mutations.reserve(size_t(std::ceil(std::log(2*N)*(theta_neutral+theta_del)+0.667*(theta_neutral+theta_del))));
      unsigned generation;

      double wbar=1;
      for( generation = 0; generation < ngens; ++generation )
	{
#ifndef NDEBUG
	  for( singlepop_t::glist_t::iterator itr = pop.gametes.begin(); 
	       itr != pop.gametes.end() ; ++itr )
	    {
	      assert( itr->n > 0 );
	    }
#endif
	  assert(KTfwd::check_sum(pop.gametes,2*N));
	  wbar = KTfwd::sample_diploid(r.get(),
				       pop.gametes,
				       pop.diploids,
				       pop.mutations,
				       pop.mcounts,
				       N,
				       mu_neutral+mu_del,
				       std::bind(KTfwd::infsites(),std::placeholders::_1,std::placeholders::_2,r.get(),std::ref(pop.mut_lookup),
						 mu_neutral,mu_del,[&r](){return gsl_rng_uniform(r.get());},[&s](){return s;},[&h](){return h;}),
				       std::bind(KTfwd::genetics101(),std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4,
						 std::ref(pop.neutral),std::ref(pop.selected),
						 std::ref(pop.gametes),std::ref(pop.mutations),
						 littler,
						 r.get(),
						 recmap),
				       std::bind(KTfwd::emplace_back<singlepop_t::gamete_t,singlepop_t::gvec_t>,std::placeholders::_1,std::placeholders::_2),
				       std::bind(KTfwd::multiplicative_diploid(),std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,2.),
				       std::bind(KTfwd::mutation_remover(),std::placeholders::_1,2*N));
	  KTfwd::update_mutations(pop.mutations,pop.fixations,pop.fixation_times,pop.mut_lookup,pop.mcounts,generation,2*N);
	  assert(KTfwd::check_sum(pop.gametes,2*N));
	}
      Sequence::SimData neutral_muts,selected_muts;

      //Take a sample of size samplesize1.  Two data blocks are returned, one for neutral mutations, and one for selected
      std::pair< std::vector< std::pair<double,std::string> >,
		 std::vector< std::pair<double,std::string> > > sample = ms_sample_separate(r.get(),pop.mutations,pop.gametes,pop.diploids,samplesize1);

      neutral_muts.assign( sample.first.begin(), sample.first.end() );
      selected_muts.assign( sample.second.begin(), sample.second.end() );

      std::cout << neutral_muts << '\n' << selected_muts << '\n';
    }
}
