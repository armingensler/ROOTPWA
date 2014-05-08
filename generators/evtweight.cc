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
//-----------------------------------------------------------
//
// Description:
//      calculates weights (= intensities) from .amp files and writes
//      them to output file
//
//      limitations:
//
//      * at the moment the event data files are used to define the
//        looping over the events. in the case where the data are not
//        copied to the output file this is unnecessary. instead the
//        looping should be based on the amplitude files which are the
//        sole input needed to calculate the weight(s). this change
//        should be implemented along with the support for .root
//        amplitude files. also the use of friend trees should be
//        investigated in order to avoid copying of event data.
//
//      * the parsing of wave names to extract quantum numbers is not
//        implemented in an error safe way
//
//      * a better handling of wave quantum numbers would allow to
//        have a more flexible way of calculating weights for model
//        components; at the moment the weights for individual waves
//        and the weights for positive and negative reflectivity
//        totals are hard-coded
//
//
// Author List:
//      Sebastian Neubert    TUM            (original author)
//
//
//-----------------------------------------------------------


#include <unistd.h>
#include <vector>

#include <boost/progress.hpp>

#include "TFile.h"
#include "TTree.h"
#include "TClonesArray.h"
#include "TROOT.h"
#include "TString.h"
#include "TStopwatch.h"

#include "ampIntegralMatrix.h"
#include "amplitudeTreeLeaf.h"
#include "fitResult.h"
#include "fileUtils.hpp"
#include "evtTreeHelper.h"


using namespace std;
using namespace boost;
using namespace rpwa;


#ifdef USE_STD_COMPLEX_TREE_LEAFS
unsigned long
openRootAmpFiles(const string&               ampDirName,
                 const vector<string>&       waveNames,
                 vector<TTree*>&             ampRootTrees,
                 vector<amplitudeTreeLeaf*>& ampRootLeafs,
                 const string&               ampLeafName = "amplitude")
{
	// force loading predefined std::complex dictionary
	// see http://root.cern.ch/phpBB3/viewtopic.php?f=5&t=9618&p=50164
	gROOT->ProcessLine("#include <complex>");

	ampRootTrees.clear();
	ampRootLeafs.assign(waveNames.size(), NULL);
	unsigned long nmbAmpValues = 0;
	for (size_t iWave = 0; iWave < waveNames.size(); ++iWave) {
		// no amplitude file for the flat wave
		if (waveNames[iWave] == "flat") {
			ampRootTrees.push_back(NULL);
			continue;
		}

		// try to open amplitude file
		const string ampFilePath = changeFileExtension(ampDirName + "/" + waveNames[iWave], ".root");
		printInfo << "opening amplitude file '" << ampFilePath << "'" << endl;
		TFile* ampFile = TFile::Open(ampFilePath.c_str(), "READ");
		if (not ampFile or ampFile->IsZombie()) {
			printErr << "cannot open amplitude file '" << ampFilePath << "'. skipping wave." << endl;
			continue;
		}

		// find amplitude tree
		TTree* ampTree = NULL;
		const string ampTreeName = changeFileExtension(waveNames[iWave], ".amp");
		ampFile->GetObject(ampTreeName.c_str(), ampTree);
		if (not ampTree) {
			printErr << "cannot find tree '" << ampTreeName << "' in file "
			         << "'" << ampFilePath << "'. skipping wave." << endl;
			continue;
		}

		// check that all tree have the same number of entries
		const unsigned long nmbEntries = ampTree->GetEntriesFast();
		if (nmbEntries == 0) {
			printErr << "amplitude tree '" << ampTree->GetName() << "' has zero entries. "
			         << "skipping wave." << endl;
			continue;
		}
		if (nmbAmpValues == 0)
			nmbAmpValues = nmbEntries;
		else if (nmbEntries != nmbAmpValues) {
			printErr << "amplitude tree '" << ampTree->GetName() << "' has different number "
			         << "of entries than previous tree. skipping wave." << endl;
			continue;
		}

		// connect tree leaf
		ampRootTrees.push_back(ampTree);
		ampTree->SetBranchAddress(ampLeafName.c_str(), &ampRootLeafs[iWave]);
	}

	return nmbAmpValues;
}
#endif


unsigned long
openBinAmpFiles(const string&         ampDirName,
                const vector<string>& waveNames,
                vector <ifstream*>&   ampFiles)
{
	ampFiles.clear();
	streampos ampFileSize = 0;
	for (size_t iWave = 0; iWave < waveNames.size(); ++iWave) {
		// no amplitude file for the flat wave
		if (waveNames[iWave] == "flat") {
			ampFiles.push_back(0);
			continue;
		}

		// try to open amplitde file
		const string ampFilePath = ampDirName + "/" + waveNames[iWave];
		printInfo << "opening amplitude file '" << ampFilePath << "'" << endl;
		ifstream* ampFile = new ifstream(ampFilePath.c_str());
		if (not ampFile->good()) {
			printErr << "cannot open amplitude file '" << ampFilePath << "'. skipping file." << endl;
			continue;
		}

		// check that all files have the same size
		const streampos fileSize = rpwa::fileSize(*ampFile);
		if (fileSize == 0) {
			printErr << "amplitude file '" << ampFilePath << "' has zero size. skipping file." << endl;
			continue;
		}
		if (ampFileSize == 0)
			ampFileSize = fileSize;
		else if (fileSize != ampFileSize) {
			printErr << "amplitude file '" << ampFilePath << "' has different size "
			         << "than previous file. skipping file." << endl;
			continue;
		}

		ampFiles.push_back(ampFile);
	}

	const unsigned long nmbAmpValues = ampFileSize / sizeof(complex<double>);
	return nmbAmpValues;
}


void
usage(const string& progName,
      const int     errCode = 0)
{
	cerr << "weights (phase space) events according to production amplitudes given by fit result" << endl
	     << endl
	     << "usage:" << endl
	     << progName
	     << " [-o output file -e -s -w fit-result file -n # of samples -i integral file -d amplitude directory -R] -m mass [-b mass bin width] "
	     << "-t tree name -l leaf names -v -h] "
	     << "input data file(s) (.evt or .root format)" << endl
	     << "    where:" << endl
	     << "        -o file    ROOT output file (default: './genpw.root')"<< endl
	     << "        -e         do _not_ copy event data to output file"<< endl
	     << "        -s         write out weights for each single wave (caution: this vastly increase the size of the output file)" << endl
	     << "        -w file    fit-result file containing the fitResult tree to be used as input (default: './fitresult.root')"<< endl
	     << "        -n #       if > 1, additional production amplitudes are generated according to covariances (default: 1)"<< endl
	     << "        -i file    integral file (default: './norm.int')"<< endl
	     << "        -d dir     path to directory with decay amplitude files (default: '.')" << endl
#ifdef USE_STD_COMPLEX_TREE_LEAFS
	     << "        -R         use .root amplitude files (default: false)" << endl
#else
	     << "        -R         use .root amplitude files [not supported; ROOT version too low]" << endl
#endif
	     << "        -m #       central mass of mass bin [MeV/c^2]"<< endl
	     << "        -b #       width of mass bin [MeV/c^2] (default: 60 MeV/c^2)"<< endl
	     << "        -t name    name of tree in ROOT data files (default: rootPwaEvtTree)" << endl
	     << "        -l names   semicolon separated object/leaf names in input data" << endl
	     << "                   (default: 'prodKinParticles;prodKinMomenta;decayKinParticles;decayKinMomenta')" << endl
	     << "        -v         verbose; print debug output (default: false)" << endl
	     << "        -h         print help" << endl
	     << endl;
	exit(errCode);
}


int
main(int    argc,
     char** argv)
{
	printCompilerInfo();
	printLibraryInfo ();
	printGitHash     ();
	cout << endl;

	// parse command line options
	const string   progName          = argv[0];
	string         outFileName       = "./genpw.root";
	bool           doCopyEventData   = true;
	string         fitResultFileName = "./fitresult.root";
	const string   fitResultTreeName = "pwa";
	const string   fitResultLeafName = "fitResult_v2";
	string         intFileName       = "./norm.int";
	string         ampDirName        = ".";
	bool           useRootAmps       = false;                   // if true .root amplitude files are read
	unsigned int   nmbProdAmpSamples = 1;
	bool           writeSingleWaveWeights        = false;
	double         massBinCenter     = 0;   // [MeV/c^2]
	double         massBinWidth      = 60;  // [MeV/c^2]
	string         inTreeName        = "rootPwaEvtTree";
	string         leafNames         = "prodKinParticles;prodKinMomenta;decayKinParticles;decayKinMomenta";
	bool           debug             = false;
	const long int treeCacheSize     = 1000000;  // 1 MByte ROOT tree read cache size

	int c;
	while ((c = getopt(argc, argv, "o:esw:n:i:d:Rm:b:t:l:vh")) != -1) {
		switch (c) {
		case 'o':
			outFileName = optarg;
			break;
		case 'e':
			doCopyEventData = false;
			break;
		case 's':
			writeSingleWaveWeights = true;
			break;
		case 'w':
			fitResultFileName = optarg;
			break;
		case 'n':
			nmbProdAmpSamples = atoi(optarg);
			break;
		case 'i':
			intFileName = optarg;
			break;
		case 'd':
			ampDirName = optarg;
			break;
		case 'R':
#ifdef USE_STD_COMPLEX_TREE_LEAFS
			useRootAmps = true;
#endif
			break;
		case 'm':
			massBinCenter = atof(optarg);
			break;
		case 'b':
			massBinWidth = atof(optarg);
			break;
		case 't':
			inTreeName = optarg;
			break;
		case 'l':
			leafNames = optarg;
			break;
		case 'v':
			debug = true;
			break;

		case 'h':
		default:
			usage(progName);
			break;
		}
	}

	// the mass bin has to be specified
	if (massBinCenter == 0) {
		printErr << "central mass of mass bin to be processed has to be different from 0. aborting." << endl;
		usage(progName, 1);
	}

	// get input file names
	if (optind >= argc) {
		printErr << "you need to specify at least one data file to process. aborting." << endl;
		usage(progName, 1);
	}
	vector<string> rootInFileNames;
	vector<string> evtInFileNames;
	while (optind < argc) {
		const string fileName = argv[optind++];
		const string fileExt  = extensionFromPath(fileName);
		if (fileExt == "root")
			rootInFileNames.push_back(fileName);
		else if (fileExt == "evt")
			evtInFileNames.push_back(fileName);
		else
			printWarn << "input file '" << fileName << "' is neither a .root nor a .evt file. "
			          << "skipping." << endl;
	}
	if ((rootInFileNames.size() == 0) and (evtInFileNames.size() == 0)) {
		printErr << "none of the specified input files is a .root or .evt file. aborting.";
		usage(progName, 1);
	}

	// get object and leaf names for event data
	string prodKinPartNamesObjName,  prodKinMomentaLeafName;
	string decayKinPartNamesObjName, decayKinMomentaLeafName;
	parseLeafAndObjNames(leafNames, prodKinPartNamesObjName, prodKinMomentaLeafName,
	                     decayKinPartNamesObjName, decayKinMomentaLeafName);

	// open .root and .evt files for event data
	vector<TTree*> inTrees;
	TClonesArray*  prodKinPartNames  = 0;
	TClonesArray*  decayKinPartNames = 0;
	if (not openRootEvtFiles(inTrees, prodKinPartNames, decayKinPartNames,
	                         rootInFileNames, evtInFileNames,
	                         inTreeName, prodKinPartNamesObjName, prodKinMomentaLeafName,
	                         decayKinPartNamesObjName, decayKinMomentaLeafName, debug)) {
		printErr << "problems opening input file(s). aborting." << endl;
		exit(1);
	}

	// load integrals
	ampIntegralMatrix normInt;
	if (not normInt.readAscii(intFileName)) {
		printErr << "cannot read normalization integral from file '"
		         << intFileName << "'. aborting." << endl;
		exit(1);
	}

	// load production amplitudes and wave information
	vector<string>                    waveNames;                    // wave names [wave index]
	vector<unsigned int>              waveIndex;                    // wave index [production amplitude index]
	                                                                // mapping from production amplitude to wave
	vector<string>                    prodAmpNames;                 // names of production amplitudes [production amplitude index]
	vector<vector<complex<double> > > prodAmps(nmbProdAmpSamples);  // production amplitudes [sample index][production amplitude index]
	vector<int>                       reflectivities;               // [production amplitude index]
	vector<int>                       ranks;                        // [production amplitude index]
	int                               maxRank = 0;
	{
		// open fit-result file
		TFile* fitResultFile = TFile::Open(fitResultFileName.c_str(), "READ");
		if (not fitResultFile or fitResultFile->IsZombie()) {
			printErr << "cannot open fit-result file '" << fitResultFileName << "'. aborting." << endl;
			exit(1);
		}
		// find fit-result tree
		TTree* fitResultTree;
		fitResultFile->GetObject(fitResultTreeName.c_str(), fitResultTree);
		if (not fitResultTree) {
			printErr << "cannot find fit-result tree '" << fitResultTreeName << "' in file '"
			         << fitResultFileName << "'. aborting." << endl;
			exit(1);
		}
		// loop over fit results and the ones which are closest to the given mass-bin center
		// if there are more than one such fit results, take the one which has the best likelihood
		printInfo << "searching for the best fit result with mass-bin center closest to m = "
		          << massBinCenter << " MeV/c^2." << endl;
		fitResult* resultBest = 0;
		{
			unsigned int indexBest         = 0;
			double       massBinCenterBest = 0;
			double       logLikeBest       = 0;
			fitResult*   result            = 0;
			fitResultTree->SetBranchAddress(fitResultLeafName.c_str(), &result);
			for (unsigned int iTree = 0; iTree < fitResultTree->GetEntries(); ++iTree) {
				fitResultTree->GetEntry(iTree);
				const double mass    = result->massBinCenter();
				const double logLike = result->logLikelihood();  // its actually -ln L
				if ((iTree == 0) or (fabs(massBinCenter - mass) < fabs(massBinCenter - massBinCenterBest))) {
					// this fit result is closer to the given mass-bin center
					indexBest         = iTree;
					massBinCenterBest = mass;
					logLikeBest       = logLike;
				} else if (fabs(massBinCenter - mass) == fabs(massBinCenter - massBinCenterBest)) {
					// this fit result is as close to the given mass-bin center as before
					// so pick the one with better likelihood
					if (logLike < logLikeBest) {  // smaller values are better
						indexBest         = iTree;
						massBinCenterBest = mass;
						logLikeBest       = logLike;
					}
				}
			}  // end loop over fit results

			// check that mass-bin center of found fit result lies in given bin width
			if (   (massBinCenterBest < (massBinCenter - massBinWidth / 2))
			       or (massBinCenterBest > (massBinCenter + massBinWidth / 2))) {
				printErr << "no fit found for mass region "  << massBinCenter << " +- "
				         << massBinWidth / 2. << " MeV/c^2" << endl;
				exit(1);
			}
			printInfo << "found best matching fit result centered at m = " << massBinCenterBest
			          << " MeV/c^2 at tree index [" << indexBest << "]." << endl;
			fitResultTree->GetEntry(indexBest);
			resultBest = result;
		}

		// resize wave names to number of waves in fit result
		waveNames.resize(resultBest->nmbWaves(), "");

		// read production amplitudes, wave names, reflectivities, and rank
		// if nmbProdAmpSamples > 1 vary production amplitudes according to covariances
		for (unsigned int iSample = 0; iSample < nmbProdAmpSamples; ++iSample) {

			fitResult* result = 0;
			if (iSample == 0) {
				result = resultBest;
			} else {
				result = resultBest->variedProdAmps();
			}

			for (unsigned int iProdAmp = 0; iProdAmp < result->nmbProdAmps(); ++iProdAmp) {
				const string prodAmpName = result->prodAmpName(iProdAmp).Data();
				const string waveName    = result->waveNameForProdAmp(iProdAmp).Data();
				const int iWave          = result->waveIndex(waveName);

				// extract rank and reflectivity from wave name
				int rank = result->rankOfProdAmp(iProdAmp);
				int refl = 0;
				if (prodAmpName != "V_flat") {
					// check reflectivity to sort into correct production vector
					refl = ((waveName[6] == '+') ? +1 : -1);
				}
				// read production amplitude
				const complex<double> prodAmp = result->prodAmp(iProdAmp);
				if (prodAmp == 0.)
					continue;
				prodAmps[iSample].push_back(prodAmp);
				// for first fit result store wave info
				if (iSample == 0) {
					waveNames[iWave] = waveName;
					waveIndex.push_back     (iWave);
					prodAmpNames.push_back  (prodAmpName);
					reflectivities.push_back(refl);
					ranks.push_back         (rank);
					if (maxRank < rank)
						maxRank = rank;
				}

				printDebug << "read production amplitude '" << prodAmpName << "'"
				           << " [" << iProdAmp << "] = " << prodAmp
				           << " for wave '" << waveName << "'; rank = " << rank
				           << ", reflectivity = " << refl << endl;
			}

			if (iSample > 0)
				delete result;

			++maxRank;
			printDebug << "rank of fit is " << maxRank << endl;
		}  // end loop over variations of production amplitudes
	}

	const unsigned int nmbWaves = waveNames.size();
	const unsigned int nmbProdAmps = prodAmpNames.size();

	// open decay amplitude files
#ifdef USE_STD_COMPLEX_TREE_LEAFS
	vector<TTree*> ampRootTrees;
	vector<amplitudeTreeLeaf*> ampRootLeafs;
#endif
	vector<ifstream*> ampBinFiles;
#ifdef USE_STD_COMPLEX_TREE_LEAFS
	if (useRootAmps) {
		openRootAmpFiles(ampDirName, waveNames, ampRootTrees, ampRootLeafs);
		// test that an amplitude file was opened for each wave
		// note that ampRootLeafs cannot be used for this check
		if (waveNames.size() != ampRootTrees.size()) {
			printErr << "error opening ROOT amplitude files." << endl;
			exit(1);
		}
	} else
#endif
	{
		openBinAmpFiles(ampDirName, waveNames, ampBinFiles);
		//const unsigned long nmbBinEvents = openBinAmpFiles(ampDirName, waveNames, ampFiles);
		// test that an amplitude file was opened for each wave
		if (waveNames.size() != ampBinFiles.size()) {
			printErr << "error opening binary amplitude files." << endl;
			exit(1);
		}
	}

	// create output file and tree
	TFile* outFile = TFile::Open(outFileName.c_str(), "RECREATE");
	TTree* outTree = new TTree(inTreeName.c_str(), inTreeName.c_str());
	// leaf variables
	double         weight;
	double         weightPosRef;
	double         weightNegRef;
	double         weightFlat;
	vector<double> weightWaves(nmbWaves);                    // branches will take pointer to elements
	vector<double> weightProdAmps(nmbProdAmps);              // branches will take pointer to elements
	vector<double> weightProdAmpSamples(nmbProdAmpSamples);  // branches will take pointer to elements
	// book branches
	outTree->Branch("weight",       &weight,       "weight/D");
	outTree->Branch("weightPosRef", &weightPosRef, "weightPosRef/D");
	outTree->Branch("weightNegRef", &weightNegRef, "weightNegRef/D");
	outTree->Branch("weightFlat",   &weightFlat,   "weightFlat/D");
	if (writeSingleWaveWeights) {
		// create weight branches for each individual wave
		for (unsigned int iWave = 0; iWave < nmbWaves; ++iWave) {
			TString weightName("weightWave_");
			weightName += waveNames[iWave];
			outTree->Branch(weightName.Data(), &weightWaves[iWave], (weightName + "/D").Data());
		}
		// if not a rank-1 fit, also create weights for each rank
		if (maxRank > 1) {
			for (unsigned int iProdAmp = 0; iProdAmp < nmbProdAmps; ++iProdAmp) {
				TString weightName("weightProdAmp_");
				weightName += prodAmpNames[iProdAmp];
				outTree->Branch(weightName.Data(), &weightProdAmps[iProdAmp], (weightName + "/D").Data());
			}
		}
	}
	// create branches for the weights calculated from the varied production amplitudes
	// weightProdAmpSamples[0] == weight
	for (unsigned int iSample = 0; iSample < nmbProdAmpSamples; ++iSample) {
		TString weightName("W");
		weightName += iSample;
		outTree->Branch(weightName.Data(), &weightProdAmpSamples[iSample], (weightName + "/D").Data());
	}
	TBranch* outProdKinMomentaBr  = 0;
	TBranch* outDecayKinMomentaBr = 0;

	// read data from tree(s) and calculate weight for each event
	TStopwatch timer;
	timer.Reset();
	timer.Start();
	unsigned int eventCounter = 0;
	for (unsigned int iTree = 0; iTree < inTrees.size(); ++iTree) {
		printInfo << "processing ";
		if ((rootInFileNames.size() > 0) and (iTree == 0))
			cout << "chain of .root files";
		else
			cout << ".evt tree[" << ((rootInFileNames.size() > 0) ? iTree : iTree + 1) << "]";
		cout << endl;

		// create leaf variables and branch pointers
		TClonesArray* prodKinMomenta    = 0;
		TClonesArray* decayKinMomenta   = 0;
		TBranch*      prodKinMomentaBr  = 0;
		TBranch*      decayKinMomentaBr = 0;

		if (doCopyEventData) {
			// connect leaf variables to tree branches
			inTrees[iTree]->SetBranchAddress(prodKinMomentaLeafName.c_str(),  &prodKinMomenta,  &prodKinMomentaBr );
			inTrees[iTree]->SetBranchAddress(decayKinMomentaLeafName.c_str(), &decayKinMomenta, &decayKinMomentaBr);
			inTrees[iTree]->SetCacheSize(treeCacheSize);
			inTrees[iTree]->AddBranchToCache(prodKinMomentaLeafName.c_str(),  true);
			inTrees[iTree]->AddBranchToCache(decayKinMomentaLeafName.c_str(), true);
			inTrees[iTree]->StopCacheLearningPhase();
			// connect TClonesArrays of input tree to those of output tree
			const int splitLevel = 99;
			const int bufSize    = 256000;
			if (not outProdKinMomentaBr)
				outProdKinMomentaBr  = outTree->Branch(prodKinMomentaLeafName.c_str(),  "TClonesArray", &prodKinMomenta,  bufSize, splitLevel);
			else
				outProdKinMomentaBr->SetAddress(&prodKinMomenta);
			if (not outDecayKinMomentaBr)
				outDecayKinMomentaBr = outTree->Branch(decayKinMomentaLeafName.c_str(), "TClonesArray", &decayKinMomenta, bufSize, splitLevel);
			else
				outDecayKinMomentaBr->SetAddress(&decayKinMomenta);
		}

		// loop over data events and calculate weight
		const long int   nmbEventsTree = inTrees[iTree]->GetEntries();
		const long int   maxNmbEvents  = 0;
		const long int   nmbEvents     = ((maxNmbEvents > 0) ? min(maxNmbEvents, nmbEventsTree) : nmbEventsTree);
		progress_display progressIndicator(nmbEvents, cout, "");
		for (long int iEvent = 0; iEvent < nmbEvents; ++iEvent) {
			++eventCounter;
			++progressIndicator;

			if (inTrees[iTree]->LoadTree(iEvent) < 0)
				break;
			if (doCopyEventData) {
				// read only required branches
				prodKinMomentaBr->GetEntry (iEvent);
				decayKinMomentaBr->GetEntry(iEvent);
			}

			// read decay amplitudes for this event
			vector<complex<double> > decayAmps(nmbWaves);
			for (unsigned int iWave = 0; iWave < nmbWaves; ++iWave) {
#ifdef USE_STD_COMPLEX_TREE_LEAFS
				if (useRootAmps) {
					if (not ampRootTrees[iWave])  // e.g. flat wave
						decayAmps[iWave] = complex<double>(0);
					else {
						ampRootTrees[iWave]->GetEntry(eventCounter);
						assert(ampRootLeafs[iWave]->nmbIncohSubAmps() == 1);
						decayAmps[iWave] = ampRootLeafs[iWave]->incohSubAmp(0);
					}
				} else
#endif
				{
					if (not ampBinFiles[iWave])  // e.g. flat wave
						decayAmps[iWave] = complex<double>(0);
					else {
						complex<double> decayAmp;
						ampBinFiles[iWave]->read((char*) &decayAmp, sizeof(complex<double>));
						decayAmps[iWave] = decayAmp;
					}
				}
			}

			// calculate weight for each sample of production amplitudes
			for (unsigned int iSample = 0; iSample < nmbProdAmpSamples; ++iSample) {

				vector<complex<double> > posReflAmpSums(maxRank, 0);  // positive-reflectivity amplitude sums [rank]
				vector<complex<double> > negReflAmpSums(maxRank, 0);  // negative-reflectivity amplitude sums [rank]
				complex<double>          flatAmp = 0;

				for (unsigned int iProdAmp = 0; iProdAmp < nmbProdAmps; ++iProdAmp) {
					const unsigned int    iWave     = waveIndex        [iProdAmp];
					const complex<double> decayAmp  = decayAmps        [iWave];
					const complex<double> prodAmp   = prodAmps[iSample][iProdAmp];
					const string          waveName  = waveNames        [iWave];
					const double          nmbEvents = normInt.nmbEvents();
					if (waveName == "flat") {
						flatAmp = prodAmp / sqrt(nmbEvents);
						if (iSample == 0)  // set weights of individual production amplitudes
							weightProdAmps[iProdAmp] = norm(flatAmp);
					} else {
						const complex<double> amp = decayAmp * prodAmp / sqrt(normInt.element(waveName, waveName).real() * nmbEvents);
						if (iSample == 0)  // set weights of individual production amplitudes
							weightProdAmps[iProdAmp] = norm(amp);
						if (reflectivities[iProdAmp] == +1)
							posReflAmpSums[ranks[iProdAmp]] += amp;
						else if (reflectivities[iProdAmp] == -1)
							negReflAmpSums[ranks[iProdAmp]] += amp;
					}
				}  // end loop over waves

				// incoherent sum over rank
				double sampleWeightPosRef = 0;
				double sampleWeightNegRef = 0;
				for (int iRank = 0; iRank < maxRank; ++iRank) {
					sampleWeightPosRef += norm(posReflAmpSums[iRank]);
					sampleWeightNegRef += norm(negReflAmpSums[iRank]);
				}
				// total weight is incoherent sum of the two reflectivities and the flat wave
				double sampleWeightFlat = norm(flatAmp);
				double sampleWeight     = sampleWeightPosRef + sampleWeightNegRef + sampleWeightFlat;

				if (iSample == 0) {
					for (unsigned int iWave = 0; iWave < nmbWaves; ++iWave)
						weightWaves[iWave] = 0;
					// in the end the following corresponds to a sum over ranks,
					// which is an incoherent sum
					for (unsigned int iProdAmp = 0; iProdAmp < nmbProdAmps; ++iProdAmp)
						weightWaves[waveIndex[iProdAmp]] += weightProdAmps[iProdAmp];

					weightPosRef = sampleWeightPosRef;
					weightNegRef = sampleWeightNegRef;
					weightFlat   = sampleWeightFlat;
					weight       = sampleWeight;
				}

				weightProdAmpSamples[iSample] = sampleWeight;

			}  // end loop over production-amplitude samples
			outTree->Fill();
		}

		inTrees[iTree]->PrintCacheStats();
	}
	printSucc << "calculated weight for " << eventCounter << " events" << endl;
	timer.Stop();
	printInfo << "this job consumed: ";
	timer.Print();

	outFile->cd();
	prodKinPartNames ->Write(prodKinPartNamesObjName.c_str(),  TObject::kSingleKey);
	decayKinPartNames->Write(decayKinPartNamesObjName.c_str(), TObject::kSingleKey);
	outTree->Write();
	outFile->Close();

	// cleanup
	for (unsigned int iWave = 0; iWave < nmbWaves; ++iWave) {
#ifdef USE_STD_COMPLEX_TREE_LEAFS
		if (useRootAmps) {
		} else
#endif
		{
			if (ampBinFiles[iWave]) {
				ampBinFiles[iWave]->close();
				delete ampBinFiles[iWave];
			}
		}
	}
#ifdef USE_STD_COMPLEX_TREE_LEAFS
	if (useRootAmps) {
		ampRootTrees.clear();
		ampRootLeafs.clear();
	} else
#endif
	{
		ampBinFiles.clear();
	}

	prodAmps.clear();

	return 0;
}
