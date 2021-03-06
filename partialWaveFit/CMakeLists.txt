#///////////////////////////////////////////////////////////////////////////
#//
#//    Copyright 2010
#//
#//    This file is part of rootpwa
#//
#//    rootpwa is free software: you can redistribute it and/or modify
#//    it under the terms of the GNU General Public License as published by
#//    the Free Software Foundation, either version 3 of the License, or
#//    (at your option) any later version.
#//
#//    rootpwa is distributed in the hope that it will be useful,
#//    but WITHOUT ANY WARRANTY; without even the implied warranty of
#//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#//    GNU General Public License for more details.
#//
#//    You should have received a copy of the GNU General Public License
#//    along with rootpwa.  If not, see <http://www.gnu.org/licenses/>.
#//
#///////////////////////////////////////////////////////////////////////////
#//-------------------------------------------------------------------------
#//
#// Description:
#//      build file for RootPwa library
#//
#//
#// Author List:
#//      Sebastian Neubert    TUM            (original author)
#//
#//
#//-------------------------------------------------------------------------


message_setup_this_dir()


# set include directories
set(INCLUDE_DIRECTORIES
	SYSTEM ${Boost_INCLUDE_DIRS}
	${Libconfig_INCLUDE_DIR}
	${ROOT_INCLUDE_DIR}
	${CUDACODE_INCLUDE_DIR}
	${UTILITIES_INCLUDE_DIR}
	${AMPLITUDE_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	)
include_directories(${INCLUDE_DIRECTORIES})


# source files that are compiled into library
set(SOURCES
	fitResult.cc
	complexMatrix.cc
	pwaLikelihood.cc
	pwaPlotter.cc
	parameterSpace.cc
	)


# library name
set(THIS_LIB "RootPwaPartialWaveFit")


# ROOT dictionary
set(ROOTPWA_DICTIONARY ${CMAKE_CURRENT_BINARY_DIR}/rootPwaDict.cc)
root_generate_dictionary(
	${ROOTPWA_DICTIONARY}
	fitResult.h pwaPlotter.h complexMatrix.h
	MODULE ${THIS_LIB}
	LINKDEF rootPwaLinkDef.h
	)
set(SOURCES ${SOURCES} ${ROOTPWA_DICTIONARY})


# library
make_shared_library(
	"${THIS_LIB}"
	"${SOURCES}"
	"${ROOT_LIBS}"
	"${CUDACODE_LIBS}"
	"${AMPLITUDE_LIBS}"
	)


# executables
make_executable(pwafit  pwafit.cc   ${THIS_LIB} "${UTILITIES_LIBS}")
make_executable(pwaplot pwaplots.cc ${THIS_LIB})


# build tests
add_subdirectory(test)
