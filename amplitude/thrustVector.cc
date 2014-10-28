///////////////////////////////////////////////////////////////////////////
//
//    Copyright 2014
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
//      A wrapper class to allow using thrust vectors in code not compiled with nvcc.
//
//
// Author List:
//      Armin Gensler          TUM            (original author)
//
//
//-------------------------------------------------------------------------

#include <vector>

#include "typedefs.h"
#include "thrustVector.h"

using namespace rpwa;

template<typename T>
ThrustVector<T>::ThrustVector()
: vec(new thrust::host_vector<T>)
{

}

template<typename T>
ThrustVector<T>::ThrustVector(const ThrustVector<T>& v)
: vec(new thrust::host_vector<T>(*(v.vec)))
{

}

template<typename T>
ThrustVector<T>::ThrustVector(std::size_t s, T v)
: vec(new thrust::host_vector<T>(s, v))
{

}

template<typename T>
ThrustVector<T>::ThrustVector(const std::vector<T>& v)
: vec(new thrust::host_vector<T>(v.size()))
{
	std::copy(v.begin(), v.end(), vec->begin());
}

template<typename T>
ThrustVector<T>::~ThrustVector()
{
	delete vec;
}

template<typename T>
std::vector<T> ThrustVector<T>::toStdVector() const
{
	std::vector<T> result(vec->size());
	std::copy(vec->begin(), vec->end(), result.begin());
	return result;
}

template<typename T>
thrust::host_vector<T>& ThrustVector<T>::toRawThrustVector()
{
	return *vec;
}

template<typename T>
T& ThrustVector<T>::operator[](int i)
{
	return (*vec)[i];
}

template<typename T>
const T& ThrustVector<T>::operator[](int i) const
{
	return (*vec)[i];
}

template<typename T>
T& ThrustVector<T>::front()
{
	return vec->front();
}

template<typename T>
const T& ThrustVector<T>::front() const
{
	return vec->front();
}

template<typename T>
T& ThrustVector<T>::back()
{
	return vec->back();
}

template<typename T>
const T& ThrustVector<T>::back() const
{
	return vec->back();
}

template<typename T>
void ThrustVector<T>::push_back(const T& x)
{
	vec->push_back(x);
}

template<typename T>
void ThrustVector<T>::clear()
{
	vec->clear();
}

template<typename T>
void ThrustVector<T>::resize(std::size_t s, T v)
{
	vec->resize(s, v);
}

template<typename T>
std::size_t ThrustVector<T>::size() const
{
	return vec->size();
}

template<typename T>
typename ThrustVector<T>::iterator ThrustVector<T>::begin()
{
	return vec->begin();
}

template<typename T>
typename ThrustVector<T>::iterator ThrustVector<T>::end()
{
	return vec->end();
}

template<typename T>
typename ThrustVector<T>::const_iterator ThrustVector<T>::begin() const
{
	return vec->begin();
}

template<typename T>
typename ThrustVector<T>::const_iterator ThrustVector<T>::end() const
{
	return vec->end();
}

// instantiate template because it has to be compiled with nvcc but is used by gcc
template class ThrustVector<Scalar>;
template class ThrustVector<Complex>;
template class ThrustVector<Vector3>;
template class ThrustVector<LorentzVector>;
template class ThrustVector<Rotation>;
template class ThrustVector<LorentzRotation>;