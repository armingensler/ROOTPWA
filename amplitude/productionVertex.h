///////////////////////////////////////////////////////////////////////////
//
//    Copyright 2010
//
//    This file is part of rootpwa
//
//    rootpwa is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    rootpwa is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with rootpwa. If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------
// File and Version Information:
// $Rev::                             $: revision of last commit
// $Author::                          $: author of last commit
// $Date::                            $: date of last commit
//
// Description:
//      production vertex virtual base class
//
//
// Author List:
//      Boris Grube          TUM            (original author)
//
//
//-------------------------------------------------------------------------


#ifndef PRODUCTIONVERTEX_H
#define PRODUCTIONVERTEX_H


#include <complex>

#include <boost/shared_ptr.hpp>

#include "TVector3.h"

#include "interactionVertex.h"


class TClonesArray;


namespace rpwa {


	class productionVertex;
	typedef boost::shared_ptr<productionVertex> productionVertexPtr;


	class productionVertex : public interactionVertex {

	public:
  
		productionVertex();
		virtual ~productionVertex();

		// production specific accessors
		virtual const TLorentzVector& referenceLzVec() const = 0;           ///< returns Lorentz-vector that defines z-axis for angular distributions
		virtual const particlePtr&    XParticle     () const = 0;           ///< returns X particle
		virtual std::complex<double>  productionAmp () const { return 1; }  ///< returns production amplitude
    
		virtual bool readData(const TClonesArray& names,
		                      const TClonesArray& momenta) = 0;  ///< general interface to read data from TClonesArrays

		virtual bool revertMomenta() = 0;  ///< general interface to reset momenta to the values of last event read

		virtual std::string label() const { return "production vertex"; }  ///< returns label used in graph visualization and reporting

		static bool debug() { return _debug; }                             ///< returns debug flag
		static void setDebug(const bool debug = true) { _debug = debug; }  ///< sets debug flag


	private:

		static bool _debug;  ///< if set to true, debug messages are printed

	};


}  // namespace rpwa


#endif  // PRODUCTIONVERTEX_H
