///////////////////////////////////////////////////////////////////////////
//
//    Copyright 2009 Sebastian Neubert
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
//    along with rootpwa.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////


#include <iomanip>
#include <fstream>
#include <limits>

#include "TVector3.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TRandom3.h"
#include "TFile.h"

#include "reportingUtils.hpp"
#include "physUtils.hpp"
#include "diffractivePhaseSpace.h"

using namespace std;
using namespace rpwa;


// /////////////////////////////////////////////////////////////////////////////////
// // global constants


// // target position
// const double _targetZPos = -300.0; // [cm]

// // beam parameters:
// const double _beamMomSigma = 1.2;  // [GeV/c]
// const double _beamMom      = 189;  // [GeV/c]
// // 2004 beam:
// // const double _beamDxDz      = 0.00026; // tilt from Quirin was in mrad
// // const double _beamDxDzSigma = 0.00010;
// // const double _beamDyDz      = 0.00001; // tilt from Quirin was in mrad
// // const double _beamDyDzSigma = 0.00018;
// // ideal beam:
// const double _beamDxDz      = 0.0;
// const double _beamDxDzSigma = 0.0;
// const double _beamDyDz      = 0.0;
// const double _beamDyDzSigma = 0.0;

// // cut on t-distribution
// const double _tMin = 0.001;  // [(GeV/c)^2]


diffractivePhaseSpace::diffractivePhaseSpace()
	: _tMin(0.),
	  _tprimeMin(0.),
	  _tprimeMax(numeric_limits<double>::max()),
	  _xMassMin(0),
	  _xMassMax(0),
	  _protonMass(0.938272013),
	  _pionMass(0.13957018),
	  _pionMass2(_pionMass * _pionMass)
{
	_primaryVertexGen = NULL;
	_phaseSpace.setWeightType    (nBodyPhaseSpaceGen::S_U_CHUNG);
	_phaseSpace.setKinematicsType(nBodyPhaseSpaceGen::BLOCK);
	_tprime = 0;
	_invSlopePar = NULL;
	_invM        = NULL;
}

diffractivePhaseSpace::~diffractivePhaseSpace() {
	delete _primaryVertexGen;
}


// /////////////////////////////////////////////////////////////////////////////////
// helper functions

// constructs beam Lorentz vector
TLorentzVector
diffractivePhaseSpace::makeBeam()
{
	// throw magnituide of beam momentum
	const double pBeam = gRandom->Gaus(_beamMom, _beamMomSigma);
	// throw beam inclination
	const double dxdz = gRandom->Gaus(_beamDxDz, _beamDxDzSigma);
	const double dydz = gRandom->Gaus(_beamDyDz, _beamDyDzSigma);
	// construct tilted beam momentum Lorentz vector
	const double pz    = pBeam / sqrt(1 + dxdz * dxdz + dydz * dydz);
	const double px    = dxdz * pz;
	const double py    = dydz * pz;
	const double EBeam = sqrt(pBeam * pBeam + _pionMass2);
	return TLorentzVector(px, py, pz, EBeam);
}


// writes event to ascii file read by gamp
bool
diffractivePhaseSpace::writePwa2000Ascii(ostream&  out,
                                          const int beamGeantId,
                                          const int beamCharge)
{
	if(!out) {
		printErr << "output stream is not writable." << endl;
		return false;
	}
	unsigned int nmbDaughters = _decayProducts.size();
	out << nmbDaughters + 1 << endl;
	// beam particle: geant ID, charge, p_x, p_y, p_z, E
	out << beamGeantId << " " << beamCharge
	    << setprecision(numeric_limits<double>::digits10 + 1)
	    << " " << _beamLab.Px() << " " << _beamLab.Py() << " " << _beamLab.Pz()
	    << " " << _beamLab.E() << endl;
	for(unsigned int i = 0; i < nmbDaughters; ++i) {
		const TLorentzVector& hadron = _phaseSpace.daughter(i);
		// hadron: geant ID, charge, p_x, p_y, p_z, E
		out << _decayProducts[i]._gId << " " << _decayProducts[i]._charge
		    << setprecision(numeric_limits<double>::digits10 + 1)
		    << " " << hadron.Px() << " " << hadron.Py() << " " << hadron.Pz()
		    << " " << hadron.E() << endl;
	}
	return true;
}

bool
diffractivePhaseSpace::writeComGeantAscii(ostream& out, bool  formated) {

	if(!out) {
		cerr << "Output stream is not writable." << endl;
		return false;
	}

	if(formated) {
		// total number of particles including recoil proton and beam particle
		unsigned int nmbDaughters = _decayProducts.size();
		out << nmbDaughters+1+1 << endl;
		// vertex position in cm
		// note that Comgeant's coordinate system is different
		out << _vertex.Z() << " " << _vertex.X() << " " << _vertex.Y() << endl;
		// beam particle: geant ID , -p_z, -p_x, -p_y must go the opposite direction upstream and should be defined as mulike with PID 44 in Comgeant
		out << setprecision(numeric_limits<double>::digits10 + 1)
		    << "44 " << -_beamLab.Pz() << " " << -_beamLab.Px() << " " << -_beamLab.Py() << endl;// << " " << beam.E() << endl;
		// the recoil proton
		out << setprecision(numeric_limits<double>::digits10 + 1)
		    << "14 " << _recoilprotonLab.Pz() << " " << _recoilprotonLab.Px() << " " << _recoilprotonLab.Py() << endl;// << " " << beam.E() << endl;
		for (unsigned int i = 0; i < nmbDaughters; ++i) {
			const TLorentzVector& hadron = _phaseSpace.daughter(i);
			// hadron: geant ID, p_z, p_x, p_y
			out << setprecision(numeric_limits<double>::digits10 + 1)
			    << _decayProducts[i]._gId << " "
			    << hadron.Pz() << " "
			    << hadron.Px() << " "
			    << hadron.Py() << endl;// << " " << hadron->E() << endl;
		}
	} else {
		int intval;
		float floatval;
		// total number of particles including recoil proton and beam particle
		unsigned int nmbDaughters = _decayProducts.size();
		//out << nmbDaughters+1+1 << endl;
		intval = (int)nmbDaughters+1+1; out.write((char*)&intval,4);
		// vertex position in cm
		// note that Comgeant's coordinate system is different
		floatval = (float)_vertex.Z(); out.write((char*)&floatval,4);
		floatval = (float)_vertex.X(); out.write((char*)&floatval,4);
		floatval = (float)_vertex.Y(); out.write((char*)&floatval,4);
		//out << _vertex.Z() << " " << _vertex.X() << " " << _vertex.Y() << endl;
		// beam particle: geant ID , -p_z, -p_x, -p_y must go the opposite direction upstream and should be defined as mulike with PID 44 in Comgeant
		intval = 44; out.write((char*)&intval,4);
		floatval = (float)-_beamLab.Pz(); out.write((char*)&floatval,4);
		floatval = (float)-_beamLab.Px(); out.write((char*)&floatval,4);
		floatval = (float)-_beamLab.Py(); out.write((char*)&floatval,4);
		//out << setprecision(numeric_limits<double>::digits10 + 1)
		//  << "44 " << -_beamLab.Pz() << " " << -_beamLab.Px() << " " << -_beamLab.Py() << endl;// << " " << beam.E() << endl;
		// the recoil proton
		intval = 14; out.write((char*)&intval,4);
		floatval = (float)_recoilprotonLab.Pz(); out.write((char*)&floatval,4);
		floatval = (float)_recoilprotonLab.Px(); out.write((char*)&floatval,4);
		floatval = (float)_recoilprotonLab.Py(); out.write((char*)&floatval,4);
		//out << setprecision(numeric_limits<double>::digits10 + 1)
		//  << "14 " << _recoilprotonLab.Pz() << " " << _recoilprotonLab.Px() << " " << _recoilprotonLab.Py() << endl;// << " " << beam.E() << endl;
		for (unsigned int i = 0; i < nmbDaughters; ++i) {
			const TLorentzVector& hadron = _phaseSpace.daughter(i);
			// hadron: geant ID, p_z, p_x, p_y
			intval = (int)_decayProducts[i]._gId; out.write((char*)&intval,4);
			floatval = (float)hadron.Pz(); out.write((char*)&floatval,4);
			floatval = (float)hadron.Px(); out.write((char*)&floatval,4);
			floatval = (float)hadron.Py(); out.write((char*)&floatval,4);
			//out << setprecision(numeric_limits<double>::digits10 + 1)
			//<< _decayProducts[i]._gId << " "
			//<< hadron.Pz() << " "
			//<< hadron.Px() << " "
			//<< hadron.Py() << endl;// << " " << hadron->E() << endl;
		}
	}
	return true;
}


void
diffractivePhaseSpace::SetSeed(int seed)
{
	gRandom->SetSeed(seed);
	_phaseSpace.setSeed(seed);
}


void
diffractivePhaseSpace::SetDecayProducts(const vector<particleInfo>& info)
{
	_decayProducts.clear();
	_decayProducts = info;
	BuildDaughterList();
}


void
diffractivePhaseSpace::AddDecayProduct(const particleInfo& info)
{
	_decayProducts.push_back(info);
	BuildDaughterList();
}


void
diffractivePhaseSpace::BuildDaughterList()
{
	const unsigned int nmbDaughters = _decayProducts.size();
	vector<double> daughterMasses(nmbDaughters, 0);
	for(unsigned int i = 0; i < nmbDaughters; ++i) {
		daughterMasses[i] = _decayProducts[i]._mass;
	}
	if(nmbDaughters > 1) {
		_phaseSpace.setDecay(daughterMasses);
		if(_xMassMax == 0) {
			printErr << "mass range [" << _xMassMin << ", " << _xMassMin << "] GeV/c^2 "
			         << "does mot make sense. exiting." << endl;
			throw;
		} else {
			printInfo << "calculating max weight (" << nmbDaughters << " FS particles) "
			          << "for m = " << _xMassMax << " GeV/c^2" << endl;
			_phaseSpace.setMaxWeight(1.01 * _phaseSpace.estimateMaxWeight(_xMassMax, 1000000));
			cout << "    max weight = " << _phaseSpace.maxWeight() << endl;
		}
	}
}


/////////////////////////////////////////////////////////////////////////////////
// main routine
// void
// diffractivePhaseSpace::genPhaseSpaceData(const double   xMassMin          = 2.100,  // lower bound of mass bin [GeV/c^2]
// 		  const double   xMassMax          = 2.140,  // upper bound of mass bin [GeV/c^2]
// 		  const TString& outFileName       = "2100.2140.genbod.evt",
// 		  const TString& thetaHistFileName = "./hTheta.root",  // histogram with experimental distribution of scattering angle
// 		  const int      nmbEvent          = 2000,
// 		  const bool     plot              = false)
// {
//   Double_t daughterMasses[3] = {_pionMass, _pionMass, _pionMass};

//   gRandom->SetSeed(12345);


//   // setup histograms
//   TH1D* ht;
//   TH1D* hm;
//   TH1D* hTheta;
//   TH3D* hVertex3D;
//   TH2D* hVertex;
//   TH1D* hVz;
//   TH1D* hE;
//   if (plot) {
//     ht        = new TH1D("ht", "t", 10000, -0.1, 1);
//     hm        = new TH1D("hm", "3pi mass", 1000, 0.5, 2.5);
//     hTheta    = new TH1D("hThetaGen", "cos theta", 100, 0.99985, 1);
//     hVertex3D = new TH3D("hVertex3D", "Vertex xyz", 100, -2, 2, 100, -2, 2, 200, _targetZPos - 5, _targetZPos + 40);
//     hVertex   = new TH2D("hVertex", "Vertex xy", 100, -2, 2, 100, -2, 2);
//     hVz       = new TH1D("hVz","Vertex z", 1000, _targetZPos - 40, _targetZPos + 40);
//     hE        = new TH1D("hE", "E", 100, 180, 200);
//   }

//   // open output file
//   ofstream outFile(outFileName);
//   cout << "Writing " << nmbEvent << " events to file '" << outFileName << "'." << endl;

//   // get theta histogram
//   TH1* thetaDist = NULL;
//   {
//     TFile* thetaHistFile = TFile::Open(thetaHistFileName, "READ");
//     if (!thetaHistFile || thetaHistFile->IsZombie()) {
//       cerr << "Cannot open histogram file '" << thetaHistFileName << "'. exiting." << endl;
//       return;
//     }
//     thetaHistFile->GetObject("h1", thetaDist);
//     if (!thetaDist) {
//       cerr << "Cannot find theta histogram in file '" << thetaHistFileName << "'. exiting." << endl;
//       return;
//     }
//   }

//   int countEvent = 0;
//   int attempts   = 0;
//   int tenpercent = (int)(nmbEvent * 0.1);
//   while (countEvent < nmbEvent) { // loop over events
//     ++attempts;


//   }
// }

double
diffractivePhaseSpace::Get_inv_SlopePar(double invariant_M) {
	double result = 1.;
	if(!_invSlopePar) {
		return result;
	}
	if(_ninvSlopePar == 1) {
		return _invSlopePar[0];
	}
	if(invariant_M < 0.) {
		return _invSlopePar[0];
	}
	// assuming to have sorted entries
	// case of linear extrapolation
	int i_1 = -1;
	int i_2 = -1;
	if(invariant_M < _invM[0]) {
		i_1 = 0;
		i_2 = 1;
	}
	if(invariant_M >= _invM[_ninvSlopePar - 2]) {
		i_1 = _ninvSlopePar - 2;
		i_2 = _ninvSlopePar - 1;
	}
	// case of linear interpolation
	if(i_1 < 0 || i_2 < 0) {
		// search for the matching two points
		for(int i = 0; i < _ninvSlopePar-2; i++) {
			if(_invM[i] <= invariant_M && invariant_M < _invM[i+1]) {
				i_1=i;
				i_2=i+1;
				break;
			}
		}
	}
	// extra/interpolate lineary
	double m_1 = _invM[i_1];
	double m_2 = _invM[i_2];
	double invt_1 = _invSlopePar[i_1];
	double invt_2 = _invSlopePar[i_2];
	result = invt_1 + (((invt_2-invt_1) / (m_2-m_1)) * (invariant_M-m_1));
	return result;
}


// based on Dima's prod_decay_split.f
unsigned int
diffractivePhaseSpace::event()
{
	unsigned long int attempts = 0;
  // construct primary vertex and beam
  // use the primary Vertex Generator if available
	if(_primaryVertexGen) {
		// as documented one may need several attempts to get a vertex position
		// which is valid also for the beam direction measurement
		while(attempts < 1000) {
			_vertex = _primaryVertexGen->Get_Vertex();
			TVector3 beam_dir = _primaryVertexGen->Get_beam_dir(_vertex);
			if(beam_dir.Mag() == 0) {
				++attempts;
				//cout << " skipping " << endl;
				continue;
			}
			_beamLab = _primaryVertexGen->Get_beam_part(beam_dir);
			break;
		}
		// Just a few events should contain a vertex position with no beam direction information
		if(attempts == 999) {
			cerr << " Error in beam construction. Please check the beam properties loaded correctly!" << endl;
			_beamLab = makeBeam();
		}
	} else {
		_vertex.SetXYZ(0, 0,
		               _targetZPos+gRandom->Uniform(-_targetZLength * 0.5,
		               _targetZLength * 0.5));
		_beamLab = makeBeam();
	}

	const TLorentzVector targetLab(0, 0, 0, _targetMass);
	const TLorentzVector overallCm = _beamLab + targetLab;  // beam-target center-of-mass system
	// check
	if(_xMassMax + _targetMass  > overallCm.M()) {
		printErr << "Max Mass out of kinematic range." <<  endl
			<< "Limit = " << overallCm.M()  - _targetMass << "GeV/c2" << endl
			<< " ABORTING " << flush<< endl;
		throw;
	}

	bool done = false;
	while(!done) {

		// _xMassMin == _xMassMax
		double xMass = _xMassMin;
		if(_xMassMin < _xMassMax) {
			xMass = gRandom->Uniform(_xMassMin, _xMassMax); // pick random X mass
		} else if(_xMassMin > _xMassMax) {
			xMass = gRandom->Gaus( _xMassMin, _xMassMax); // pick random X mass arround _xMassMin
		}

		double tPrime = _tprimeMin;
		if(_tprimeMax < _tprimeMin) {
			// calculate the slope parameter depending on the invariant mass
			const double calc_invSlopePar = Get_inv_SlopePar(xMass);
			tPrime = -gRandom->Exp(calc_invSlopePar);  // pick random t'
			//cout << " inv slope par " << _invSlopePar << " gradient " << _invSlopeParGradient << " t' is " << tPrime << endl;
		}

		// make sure that X mass is not larger than maximum allowed mass
		if(xMass + _recoilMass > overallCm.M()) {
			continue;
		}

		// calculate t from t' in center-of-mass system of collision
		const double s            = overallCm.Mag2();
		const double sqrtS        = sqrt(s);
		const double recoilMass2  = _recoilMass * _recoilMass;
		const double xMass2       = xMass * xMass;
		const double xEnergyCM    = (s - recoilMass2 + xMass2) / (2 * sqrtS);  // breakup energy
		const double xMomCM       = sqrt(xEnergyCM * xEnergyCM - xMass2);      // breakup momentum
		const double beamMass2    = _beamLab.M2();
		const double targetMass2  = _targetMass * _targetMass;
		const double beamEnergyCM = (s - targetMass2 + beamMass2) / (2 * sqrtS);    // breakup energy
		const double beamMomCM    = sqrt(beamEnergyCM * beamEnergyCM - beamMass2);  // breakup momentum
		const double t0           = (xEnergyCM - beamEnergyCM) * (xEnergyCM - beamEnergyCM) -
		                            (xMomCM - beamMomCM) * (xMomCM - beamMomCM);
		const double t            = t0 + tPrime;
		// reject events outside of allowed kinematic region
		if(t > t0) {
			continue;
		}

		// construct X Lorentz-vector in lab frame (= target RF)
		// convention used here: Reggeon = X - beam = target - recoil (momentum transfer from target to beam vertex)
		const double reggeonEnergyLab = (targetMass2 - recoilMass2 + t) / (2 * _targetMass);  // breakup energy
		const double xEnergyLab       = _beamLab.E() + reggeonEnergyLab;
		const double xMomLab          = sqrt(xEnergyLab * xEnergyLab - xMass2);
		const double xCosThetaLab     = (t - xMass2 - beamMass2 + 2 * _beamLab.E() * xEnergyLab) / (2 * _beamLab.P() * xMomLab);
		const double xSinThetaLab     = sqrt(1 - xCosThetaLab * xCosThetaLab);
		const double xPtLab           = xMomLab * xSinThetaLab;
		const double xPhiLab          = gRandom->Uniform(0., TMath::TwoPi());

		// xSystemLab is defined w.r.t. beam direction
		TLorentzVector xSystemLab = TLorentzVector(xPtLab  * cos(xPhiLab),
		                                           xPtLab  * sin(xPhiLab),
		                                           xMomLab * xCosThetaLab,
		                                           xEnergyLab);
		// rotate according to beam tilt
		TVector3 beamDir = _beamLab.Vect().Unit();
		xSystemLab.RotateUz(beamDir);
		// calculate the recoil proton properties
		_recoilprotonLab = (_beamLab + targetLab) - xSystemLab; // targetLab

		/* check for coplanarity
		   cout << " Energy balance is " << (_beamLab + targetLab).E() << " vs. " << (_recoilprotonLab+xSystemLab).E() << endl;
		   cout << " Momentum balance is " << (_beamLab + targetLab).Mag() << " vs. " << (_recoilprotonLab+xSystemLab).Mag() << endl;
		   cout << " Direction X balance is " << (_beamLab + targetLab).Px() << " vs. " << (_recoilprotonLab+xSystemLab).Px() << endl;
		   cout << " Direction Y balance is " << (_beamLab + targetLab).Py() << " vs. " << (_recoilprotonLab+xSystemLab).Py() << endl;
		   cout << " Direction Z balance is " << (_beamLab + targetLab).Pz() << " vs. " << (_recoilprotonLab+xSystemLab).Pz() << endl;

		// own rotation
		TVector3 vec_direction = _beamLab.Vect().Unit();
		TVector3 vec_origin(0.,0.,1.);
		// get the angle of the vector
		double angle = vec_origin.Angle(vec_direction);
		// get the rotation axis perpendicular to the plane between these both
		TVector3 vec_rotation = vec_origin.Cross(vec_direction);
		vec_rotation = vec_rotation.Unit();
		// rotate around this axis by the given angle
		//particle_null.Rotate  (-angle, vec_rotation);
		_recoilprotonLab.Rotate(-angle, vec_rotation);
		xSystemLab.Rotate(-angle, vec_rotation);

		cout << " delta phi is " << (_recoilprotonLab.Phi()-xSystemLab.Phi())/3.141592654 << endl;
		 */

		// recalculate t' for xcheck or save the generated
		// number directly if you change if (1) to (0) to
		// speed up the process a bit
		if(0) {
			_tprime = Calc_t_prime(_beamLab, xSystemLab);
		} else {
			_tprime = tPrime;
		}

		// apply t cut
		if(t > _tMin || _tprime > _tprimeMin || _tprime < _tprimeMax) {
			continue;
		}

		// generate n-body phase space for X system
		++attempts;
		{
			_phaseSpace.pickMasses(xMass);

			// correct weight for phase space splitting
			// and for 2-body phase space beam-target
			// (1 / 4pi) * q(sqrt(s), m_x, m_recoil) / sqrt(s)
			const double ps2bodyWMax = breakupMomentum(sqrtS,_xMassMax,_targetMass)/sqrtS;
			const double ps2bodyW = breakupMomentum(sqrtS,xMass,_targetMass)/sqrtS;

			const double maxPsWeight = _phaseSpace.maxWeight()  * _xMassMax * ps2bodyWMax;
			const double psWeight    = _phaseSpace.calcWeight() * xMass* ps2bodyW;

			if((psWeight / maxPsWeight) < _phaseSpace.random()) {
				continue;
			}
			_phaseSpace.pickAngles();
			_phaseSpace.calcEventKinematics(xSystemLab);
		}

		done = true;
	}
	// event was accepted

	return attempts;
}


unsigned int
diffractivePhaseSpace::event(ostream& stream)
{
	unsigned int attempts = event();
	//writePwa2000Ascii(stream, 9, -1);  // use pi^- beam
	// use the first particle as the beam particle
	writePwa2000Ascii(stream, _decayProducts[0]._gId, _decayProducts[0]._charge);
	return attempts;
}

unsigned int
diffractivePhaseSpace::event(ostream& stream, ostream& streamComGeant)
{
	unsigned int attempts = event();
	//writePwa2000Ascii(stream, 9, -1);  // use pi^- beam
	// use the first particle as the beam particle
	writePwa2000Ascii(stream, _decayProducts[0]._gId, _decayProducts[0]._charge);
	writeComGeantAscii(streamComGeant, true);
	return attempts;
}


void
diffractivePhaseSpace::SetBeam(double Mom,  double MomSigma,
		double DxDz, double DxDzSigma,
		double DyDz, double DyDzSigma)
{
	_beamMomSigma=MomSigma;
	_beamMom=Mom;
	_beamDxDz=DxDz;
	_beamDxDzSigma=DxDzSigma;
	_beamDyDz=DyDz;
	_beamDyDzSigma=DyDzSigma;
}

float
diffractivePhaseSpace::Calc_t_prime(const TLorentzVector& particle_In, const TLorentzVector& particle_Out){
	float result = 0.;
	result = (particle_Out.M2()-particle_In.M2());
	result = pow(result,2);
	result /= 4*pow(particle_In.P(),2);
	result = fabs((particle_In-particle_Out).M2())-fabs(result);
	return result;
}
