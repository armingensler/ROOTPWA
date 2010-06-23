/*
 * author: Prometeusz (Promme) jasinski
 * 		jasinski@kph.uni-mainz.de Promme@web.de
 *
 * This file contains some methods simulating the Primary Vertex distribution
 * in the target cell as well as the incoming beam properties
 * You might use this script directly via executing in root
 *
 * root -l Beam_simulation.C+
 *
 * or use the methods here to adapt it in your event generator
 *
 * (2010.03.03)
 * - implementing 2008 h- beam simulation
 *
 * (2010.06.16)
 * - moved to rootpwa svn repository and changed to class structure
 *
 */

/** @addtogroup generators
 * @{
 */

#ifndef TPRIMARYVERTEXGEN_HH
#define TPRIMARYVERTEXGEN_HH

#include <iostream>
#include <string>
#include "TH1.h"
#include "TH2.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TStyle.h"
#include "TColor.h"
#include "TROOT.h"
#include "TVector3.h"
#include "TLorentzVector.h"
#include "TRandom.h"


using namespace std;

class TPrimaryVertexGen {

private:

// ********* some global constants and variables **********
TFile* histogramfile;
//static const string histogramfilename("properties_2008/primary_vertex_properties.root");
TH2* hist_angles_vert_mean;
TH2* hist_angles_horiz_mean;
TH2* hist_angles_vert_sigma;
TH2* hist_angles_horiz_sigma;
TH2* hist_vertex_distr_xy;
TH1* hist_vertex_distr_z;

//const double beam_part_mass(0.13957018); // [GeV] 2008's PDG pi-/+ value
//const double beam_part_mass(0.493677); // [GeV] 2008's PDG K-/+ value
//const double beam_energy_mean(191.29); // [GeV] fit on pi- pi+ pi- energy
//const double beam_energy_sigma(1.94476); // [GeV] fit on pi- pi+ pi- energy
double _beam_part_mass;
double _beam_energy_mean;
double _beam_energy_sigma;
bool   _histograms_loaded;
// in principle the fitted values are missing the momentum transfer to the
// recoil particle that should be taken into account in the future

public:

// loads histograms containing the measure vertex information
// initialization of default values
// see also above the out commented constants
// please TPrimaryVertexGen::Check() afterwards if loading went correctly
TPrimaryVertexGen(
		string histfilename 			= "properties_2008/primary_vertex_properties.root",
		double beam_part_mass 			= 0.13957018, // by default pion
		double mean_beam_energy 		= 191.29, 	  // measured 2008
		double mean_beam_energy_spread 	= 1.94476);   // measured sigma 2008

// free histograms
~TPrimaryVertexGen();

// check if histograms were loaded properly
bool Check();

// ************* the prototypes ************
private:
// Loading of distribution histograms specified by histogram filename
// if you wish a window showing the loaded histograms then plot = true
// returns false if something goes wrong
// Please do not use any other methods then
bool Load_histograms(
		string filename = "properties_2008/primary_vertex_properties.root",
		bool plot = true);

public:
// Get one Vertex point
// the position is distributed randomly based on hist_vertex_distr_xy
// and hist_vertex_distr_z
// you may change the cuts but it is not recommended since the cuts
// are tuned for the valid ranges in the histograms
// Call Load_histograms() before!
TVector3& Get_Vertex(const float cutR = 1.47,
				const float cutZ_low = -70.,
				const float cutZ_high = -30.);

// Based on the position of the Vertex retrieve the beam
// direction properties
// given by distributions of
// hist_angles_vert_mean
// hist_angles_horiz_mean
// hist_angles_vert_sigma
// hist_angles_horiz_sigma
// Call Load_histograms() before!
// returns a vector with Mag() == 0 if vertex is out of
// range of stored values
TVector3& Get_beam_dir(const TVector3 vertex);

// Simulate the beam particle based on fitted
// values given by
// beam_energy_mean
// beam_energy_sigma
// beam_part_mass
// in principal one should also measure the energy(spread) depending on
// X Y in the target since we know that there is a dependence
// but this is neglected for now
TLorentzVector& Get_beam_part(const TVector3 beam_dir);

private:

/*
 * big thanks to Sergei Gerassimov for this
 * smooth palette settings
 */
void ges_palette(int i=0);

};

#endif
/* @} **/
