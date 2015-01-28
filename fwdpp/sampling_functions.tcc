//  -*- C++ -*- 
#ifndef __FWDPP_SAMPLING_FUNCTIONS_TCC__
#define __FWDPP_SAMPLING_FUNCTIONS_TCC__

#include <fwdpp/fwd_functional.hpp>
#include <fwdpp/internal/ms_sampling.hpp>
#include <limits>
#include <iostream>

namespace KTfwd
{
  template< typename gamete_type,
	    typename vector_type_allocator,
	    template<typename,typename> class vector_type>
  std::vector<unsigned> sample(gsl_rng * r,
			       const vector_type<gamete_type,vector_type_allocator > & gametes,
			       const unsigned & n, const unsigned & N)
  {
    std::vector<double> freqs(gametes.size(),0);
    std::vector<unsigned> counts(gametes.size(),0);
    for(unsigned i=0;i<gametes.size();++i)
      {
	double f = double(gametes[i].n)/double(N);
	freqs[i]=f;
      }
    gsl_ran_multinomial(r,gametes.size(),n,&freqs[0],&counts[0]);
    return counts;
  }

  template< typename gamete_type,
	    typename vector_type_allocator,
	    template<typename,typename> class vector_type>
  std::vector<unsigned> sample_sfs(gsl_rng * r, 
				   const vector_type<gamete_type,vector_type_allocator > & gametes,
				   const unsigned & n, const unsigned & N)
  {
    std::vector<unsigned> counts = sample(r,gametes,n,N);
    std::map<double,unsigned> samplemuts;
    std::map<double,unsigned>::iterator itr;
    for(unsigned i=0;i<gametes.size();++i)
      {
	if(counts[i]>0)
	  {
	    for(unsigned j=0;j<gametes[i].mutations.size();++j)
	      {
		itr = samplemuts.find(gametes[i].mutations[j]->pos);
		if( itr == samplemuts.end() )
		  {
		    samplemuts[gametes[i].mutations[j]->pos] = counts[i];
		  }
		else
		  {
		    itr->second += counts[i];
		  }
	      }
	    for(unsigned j=0;j<gametes[i].smutations.size();++j)
	      {
		itr = samplemuts.find(gametes[i].smutations[j]->pos);
		if( itr == samplemuts.end() )
		  {
		    samplemuts[gametes[i].smutations[j]->pos] = counts[i];
		  }
		else
		  {
		    itr->second += counts[i];
		  }
	      }
	  }
      }
    std::vector<unsigned> samplesfs(n,0);
    for(itr=samplemuts.begin();itr!=samplemuts.end();++itr)
      {
	samplesfs[itr->second-1]++;
      }
    return samplesfs;
  }

  template< typename gamete_type,
	    typename vector_type_allocator,
	    template<typename,typename> class vector_type>
  std::vector< std::pair<double, std::string> > 
  ms_sample(gsl_rng * r,
	    const vector_type<gamete_type,vector_type_allocator > & gametes,
	    const unsigned & n, const unsigned & N,
	    bool remove_fixed)
  {
    std::vector<unsigned> counts = sample(r,gametes,n,N);	
    std::vector< std::pair<double, std::string> > rv;
    std::vector< std::pair<double, std::string> >::iterator itr;

    std::function<bool(const std::pair<double,std::string> &, const double &)> sitefinder = [](const std::pair<double,std::string> & site,
											       const double & d ) 
      {
	return std::fabs(site.first-d) <= std::numeric_limits<double>::epsilon();
      };

    unsigned individual = 0;
  for(unsigned i=0;i<counts.size();++i)
    {
      for(unsigned j=0;j<counts[i];++j,++individual)
	{
	  fwdpp_internal::update_sample_block(rv,gametes[i].mutations,individual,n,sitefinder);
	  fwdpp_internal::update_sample_block(rv,gametes[i].smutations,individual,n,sitefinder);
	}
    }
  assert(individual==n);
		
  if(remove_fixed&&!rv.empty())
    {
      //remove fixations, to make it really ms-like
      itr = rv.end()-1;
      while( itr >= rv.begin() )
	{
	  if(unsigned(std::count(itr->second.begin(),itr->second.end(),'1'))==n)
	    {
	      rv.erase(itr);
	      itr=rv.end();
	    }
	  --itr;
	}
    }
  if(!rv.empty())
    {
      std::sort(rv.begin(),
		rv.end(),
		[](std::pair<double,std::string> lhs,
		   std::pair<double,std::string> rhs) { return lhs.first < rhs.first; });
    }
  return rv;
}

template< typename gamete_type,
	  typename vector_type_allocator,
	  template<typename,typename> class vector_type>
std::pair< std::vector< std::pair<double, std::string> > ,
	   std::vector< std::pair<double, std::string> > >
ms_sample_separate(gsl_rng * r,
		   const vector_type<gamete_type,vector_type_allocator > & gametes,
		   const unsigned & n, const unsigned & N,
		   bool remove_fixed)
{
  std::vector<unsigned> counts = sample(r,gametes,n,N);	
  std::vector< std::pair<double, std::string> > rvsel,rvneut;
  std::vector< std::pair<double, std::string> >::iterator itr;

  std::function<bool(const std::pair<double,std::string> &, const double &)> sitefinder = [](const std::pair<double,std::string> & site,
											     const double & d ) 
    {
      return std::fabs(site.first-d) <= std::numeric_limits<double>::epsilon();
    };

  unsigned individual = 0;
  for(unsigned i=0;i<counts.size();++i)
    {
      for(unsigned j=0;j<counts[i];++j,++individual)
	{
	  fwdpp_internal::update_sample_block(rvneut,gametes[i].mutations,individual,n,sitefinder);
	  fwdpp_internal::update_sample_block(rvneut,gametes[i].smutations,individual,n,sitefinder);
	  /*
	  for(unsigned mut = 0 ; mut < gametes[i].mutations.size() ; ++mut)
	    {
	      double mutpos = gametes[i].mutations[mut]->pos;
	      itr = std::find_if(rvneut.begin(),rvneut.end(),
				 std::bind(sitefinder,std::placeholders::_1,mutpos));
	      if( itr == rvneut.end() )
		{
		  rvneut.push_back( std::make_pair(mutpos,std::string(n,'0')) );
		  rvneut[rvneut.size()-1].second[individual] = '1';
		}
	      else
		{
		  itr->second[individual] = '1';
		}
	    }
	  for(unsigned mut = 0 ; mut < gametes[i].smutations.size() ; ++mut)
	    {
	      double mutpos = gametes[i].smutations[mut]->pos;
	      itr = std::find_if(rvsel.begin(),rvsel.end(),
				     std::bind(sitefinder,std::placeholders::_1,mutpos));
	      if( itr == rvsel.end() )
		{
		  rvsel.push_back( std::make_pair(mutpos,std::string(n,'0')) );
		  rvsel[rvsel.size()-1].second[individual] = '1';
		}
	      else
		{
		  itr->second[individual] = '1';
		}
	    }
	  */
	}
    }
  assert(individual==n);
		
  if(remove_fixed)
    {
      if(!rvneut.empty())
	{
	  //remove fixations, to make it really ms-like
	  itr = rvneut.end()-1;
	  while( itr >= rvneut.begin() )
	    {
	      if(unsigned(std::count(itr->second.begin(),itr->second.end(),'1'))==n)
		{
		  rvneut.erase(itr);
		  itr=rvneut.end();
		}
	      --itr;
	    }
	}
      if(!rvsel.empty())
	{
	  //remove fixations, to make it really ms-like
	  itr = rvsel.end()-1;
	  while( itr >= rvsel.begin() )
	    {
	      if(unsigned(std::count(itr->second.begin(),itr->second.end(),'1'))==n)
		{
		  rvsel.erase(itr);
		  itr=rvsel.end();
		}
	      --itr;
	    }
	}

    }
  if(!rvneut.empty())
    {
      std::sort(rvneut.begin(),rvneut.end(),
		[](std::pair<double,std::string> lhs,
		   std::pair<double,std::string> rhs) { return lhs.first < rhs.first; });
    }
  if(!rvsel.empty())
    {
      std::sort(rvsel.begin(),rvsel.end(),
		[](std::pair<double,std::string> lhs,
		   std::pair<double,std::string> rhs) { return lhs.first < rhs.first; });
    }
  return std::make_pair(rvneut,rvsel);
}

//SAMPLERS FOR INDIVIDUAL-BASED SIMULATIONS

template<typename iterator_type,
	 typename allocator,
	 template<typename,typename> class vector_type >
std::vector< std::pair<double,std::string> >
ms_sample( gsl_rng * r,
	   const vector_type< std::pair<iterator_type,iterator_type>, allocator > * diploids,
	   const unsigned & n,
	   const bool & remove_fixed)
{
  std::vector< std::pair<double,std::string> > rv;
  std::vector< std::pair<double, std::string> >::iterator itr;
  
  std::function<bool(const std::pair<double,std::string> &, const double &)> sitefinder = [](const std::pair<double,std::string> & site,
											     const double & d ) 
    {
      return std::fabs(site.first-d) <= std::numeric_limits<double>::epsilon();
    };

  const auto dptr = diploids->cbegin();
  for( unsigned i = 0 ; i < n/2 ; ++i )
    {
      typename decltype(dptr)::difference_type ind = decltype(ind)(gsl_ran_flat(r,0.,double(diploids->size())));
      assert(ind >= 0);
      assert( unsigned(ind) < diploids->size() );
      fwdpp_internal::update_sample_block( rv,(dptr+ind)->first->mutations,i,n,sitefinder);
      fwdpp_internal::update_sample_block( rv,(dptr+ind)->second->mutations,i,n,sitefinder,1);
      fwdpp_internal::update_sample_block( rv,(dptr+ind)->first->smutations,i,n,sitefinder);
      fwdpp_internal::update_sample_block( rv,(dptr+ind)->second->smutations,i,n,sitefinder,1);
    }
  if(remove_fixed&&!rv.empty())
    {
      //remove fixations, to make it really ms-like
      itr = rv.end()-1;
      while( itr >= rv.begin() )
	{
	  if(unsigned(std::count(itr->second.begin(),itr->second.end(),'1'))==n)
	    {
	      rv.erase(itr);
	      itr=rv.end();
	    }
	  --itr;
	}
    }
  if(!rv.empty())
    {
      std::sort(rv.begin(),rv.end(),
		[](std::pair<double,std::string> lhs,
		   std::pair<double,std::string> rhs) { return lhs.first < rhs.first; });
    }
  return rv;
}

template<typename iterator_type,
	 typename allocator,
	 template<typename,typename> class vector_type >
std::pair<std::vector< std::pair<double,std::string> >,
	  std::vector< std::pair<double,std::string> > >
ms_sample_separate( gsl_rng * r,
		    const vector_type< std::pair<iterator_type,iterator_type>, allocator > * diploids,
		    const unsigned & n,
		    const bool & remove_fixed)
{
  std::pair<std::vector< std::pair<double,std::string> >,
	    std::vector< std::pair<double,std::string> > > rv;
  std::vector< std::pair<double, std::string> >::iterator itr;
  
  std::function<bool(const std::pair<double,std::string> &, const double &)> sitefinder = [](const std::pair<double,std::string> & site,
											     const double & d ) 
    {
      return std::fabs(site.first-d) <= std::numeric_limits<double>::epsilon();
    };

  const auto dptr = diploids->cbegin();
  for( unsigned i = 0 ; i < n/2 ; ++i )
    {
      typename decltype(dptr)::difference_type ind = decltype(ind)(gsl_ran_flat(r,0.,double(diploids->size())));
      assert(ind>=0);
      assert( unsigned(ind) < diploids->size() );
      fwdpp_internal::update_sample_block( rv.first,(dptr+ind)->first->mutations,i,n,sitefinder);
      fwdpp_internal::update_sample_block( rv.first,(dptr+ind)->second->mutations,i,n,sitefinder,1);
      fwdpp_internal::update_sample_block( rv.second,(dptr+ind)->first->smutations,i,n,sitefinder);
      fwdpp_internal::update_sample_block( rv.second,(dptr+ind)->second->smutations,i,n,sitefinder,1);
    }
  if(remove_fixed&&!rv.first.empty())
    {
      //remove fixations, to make it really ms-like
      itr = rv.first.end()-1;
      while( itr >= rv.first.begin() )
	{
	  if(unsigned(std::count(itr->second.begin(),itr->second.end(),'1'))==n)
	    {
	      rv.first.erase(itr);
	      itr=rv.first.end();
	    }
	  --itr;
	}
    }
  if(!rv.first.empty())
    {
      std::sort(rv.first.begin(),rv.first.end(),
		[](std::pair<double,std::string> lhs,
		   std::pair<double,std::string> rhs) { return lhs.first < rhs.first; });
    }
  if(remove_fixed&&!rv.second.empty())
    {
      //remove fixations, to make it really ms-like
      itr = rv.second.end()-1;
      while( itr >= rv.second.begin() )
	{
	  if(unsigned(std::count(itr->second.begin(),itr->second.end(),'1'))==n)
	    {
	      rv.second.erase(itr);
	      itr=rv.second.end();
	    }
	  --itr;
	}
    }
  if(!rv.second.empty())
    {
      std::sort(rv.second.begin(),rv.second.end(),
		[](std::pair<double,std::string> lhs,
		   std::pair<double,std::string> rhs) { return lhs.first < rhs.first; });
    }
  return rv;
}

//Individual-based sims, multilocus algorithm
template<typename iterator_type,
	 typename allocator,
	 typename outer_allocator,
	 template<typename,typename> class vector_type,
	 template<typename,typename> class outer_vector_type>
std::vector< std::vector< std::pair<double,std::string> > >
ms_sample( gsl_rng * r,
	   const outer_vector_type< vector_type< std::pair<iterator_type,iterator_type>, allocator >, outer_allocator > * diploids,
	   const unsigned & n,
	   const bool & remove_fixed)
{
  typedef std::vector< std::vector< std::pair<double,std::string> > > rvtype;
  typedef std::vector< std::pair<double,std::string> >::iterator rv_inner_itr;
  typedef vector_type< std::pair<iterator_type,iterator_type>, allocator > genotype;
  typedef outer_vector_type< genotype, outer_allocator > dip_ctr;
  typedef typename iterator_type::value_type::mcont_iterator mut_itr;

  rvtype rv( diploids->size() );

  std::vector< typename dip_ctr::size_type > individuals;
  for( unsigned i = 0 ; i < n/2  ; ++i )
    {
      individuals.push_back( typename dip_ctr::size_type( gsl_ran_flat(r,0,diploids->size()) ) );
    }

  std::function<bool(const std::pair<double,std::string> &, const double &)> sitefinder = [](const std::pair<double,std::string> & site,
											     const double & d ) 
    {
      return std::fabs(site.first-d) <= std::numeric_limits<double>::epsilon();
    };

  //Go over each indidivual's mutations and update the return value
  typename dip_ctr::const_iterator dbegin = diploids->begin();
  for( unsigned ind = 0 ; ind < individuals.size() ; ++ind )
    {
      unsigned rv_count=0;
      for( typename genotype::const_iterator locus = (dbegin+ind)->begin() ; 
	   locus < (dbegin+ind)->end() ; ++locus, ++rv_count )
	{
	  //finally, we can go over mutations
	  fwdpp_internal::update_sample_block(rv[rv_count],locus->first->mutations,ind,n,sitefinder);
	  fwdpp_internal::update_sample_block(rv[rv_count],locus->first->smutations,ind,n,sitefinder);
	  fwdpp_internal::update_sample_block(rv[rv_count],locus->second->mutations,ind,n,sitefinder,1);
	  fwdpp_internal::update_sample_block(rv[rv_count],locus->second->smutations,ind,n,sitefinder,1);
	}
    }
  
  if( remove_fixed )
    {
      for( unsigned i = 0 ; i < rv.size() ; ++i )
	{
	  if( ! rv[i].empty() )
	    {
	      rv_inner_itr mitr = rv[i].end() - 1;
	      while ( mitr >= rv[i].begin() )
		{
		  unsigned c = std::count(mitr->second.begin(),
					  mitr->second.end(), '1');
		  if(c==n)
		    {
		      rv[i].erase(mitr);
		      mitr = rv[i].end();
		    }
		  --mitr;
		}
	    }
	}
    }
  //sort on position
  for( unsigned i = 0 ; i < rv.size() ; ++i )
    {
      std::sort(rv[i].begin(),rv[i].end(),
		[](std::pair<double,std::string> lhs,
		   std::pair<double,std::string> rhs) { return lhs.first < rhs.first; });
    }
  return rv;
}
}

#endif
