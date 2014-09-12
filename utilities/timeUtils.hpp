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
//
// Description:
//      functions geting time
//
//
// Author List:
//      Armin Gensler          TUM            (original author)
//
//
//-------------------------------------------------------------------------


#ifndef TIMEUTILS_HPP
#define TIMEUTILS_HPP

#include "stdint.h"
#include "time.h"


namespace rpwa {

	inline
	uint64_t
	getTimeMilliseconds()
	{
		const boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
	}


}  // namespace rpwa


#endif  // PARALLELUTILS_HPP