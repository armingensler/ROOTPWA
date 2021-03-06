#include <complex>
#include <vector>

#include "TGraph.h"
#include "TCanvas.h"
#include "TMath.h"

#include "complexMatrix.h"

#include "matrix.hpp"
#include "io.hpp"

//typedef TMatrixT<std::complex<double> > TMatrixC;

using namespace std;

 // simple single channel K function with several poles:
complex<double> K(double s, double f, double s0,
		  const vector<double>& mu2, const vector<double>& g){

  unsigned int npoles=mu2.size();
  double k=0;
 // pipi phasespace:
  double rho=sqrt((s-0.0784)/s);
  for(unsigned int i=0;i<npoles;++i){
    double rho0=sqrt((mu2[i]-0.0784)/s);
    k+=g[i]/(mu2[i]-s)*rho/rho0;
  }
  k+=f*(1.0+s0)/(s+s0);
  k*=(s-0.02)/(s+0.2);

  // K-Matrix formula:
  complex<double> denom(1.,-k);
  complex<double> nom(k,0);
  return nom/denom;
}
 // simple breit Wigner sum amplitude:
complex<double> BW(double s, double f, double s0,
		  const vector<double>& mu2, const vector<double>& g){

   // pipi phasespace:
  double rho=sqrt((s-0.784)/s);

  unsigned int npoles=mu2.size();
  complex<double> BW;
  for(unsigned int i=0;i<npoles;++i){
    double gamma=2*g[i]/sqrt(mu2[i]); //gamma=g/2m
    complex<double> nom(gamma,0);
    double rho0=sqrt((mu2[i]-0.0784)/s);
    complex<double> denom(mu2[i]-s,-fabs(gamma)*rho/rho0);
    BW+=nom/denom;
  }

  return BW;
}

///////////////////////// T-Matrix a'la Novoseller ///////////////

complexMatrix SMatrix(){

  //TMatrixT<complex<double> > Sc(2,2);
  //Sc[0][0]=complex<double>(1,0);
  //Sc[0][1]=complex<double>(2,1);
  //Sc[1][0]=complex<double>(3,2);
  //Sc[1][1]=complex<double>(-1,0);

  //TMatrixT<complex<double> > ScInv=Sc.Invert();

  using namespace boost::numeric::ublas;
  matrix<complex<double> > m (3, 3);
    for (unsigned i = 0; i < m.size1 (); ++ i)
        for (unsigned j = 0; j < m.size2 (); ++ j)
	  m (i, j) = complex<double>(3 * i, + j);
    std::cout << m << std::endl;



  complexMatrix S(2,2);
  complexMatrix SB(2,2); // Background
  complexMatrix SR(2,2); // Resonance

  // parameterize Background:
  // SB=(1+iKb)(1-iKb)^-1

  SR.set(0,0,TComplex(1,0));
  SR.set(0,1,TComplex(2,1));
  SR.set(1,0,TComplex(3,2));
  SR.set(1,1,TComplex(-1,0));

  SB.set(0,0,TComplex(1,0));
  SB.set(1,1,TComplex(1,-1));

  SB.set(1,0,TComplex(5,2));
  SB.set(0,1,TComplex(-5,2));

  complexMatrix SRT=SR.dagger();

  return SRT*SB*SR;

}





///////////////////////// Main ///////////////////////////////////

void testKMatrix(){

  SMatrix().Print();

  double mstart=0.9;
  double mstep=0.02;
  unsigned int nsteps=100;

  vector<double> mu2;
  mu2.push_back(2.43*2.43);
  mu2.push_back(1.77*1.77);
  // mu2.push_back(1.97*1.97);
  vector<double> g;
  g.push_back(0.2);
  g.push_back(-0.1);
  //g.push_back(1);

  double s0=4;
  double f=0.2;

  TGraph* gI=new TGraph(nsteps);
  TGraph* gPhase=new TGraph(nsteps);
  TGraph* gArgand=new TGraph(nsteps);

  TGraph* gIBW=new TGraph(nsteps);
  TGraph* gPhaseBW=new TGraph(nsteps);
  TGraph* gArgandBW=new TGraph(nsteps);

  for(unsigned int i=0;i<nsteps;++i){
    double m=mstart+(double)i*mstep;
    double s=m*m;
    complex<double> amp=K(s,f,s0,mu2,g);
    gI->SetPoint(i,m,norm(amp));
    double phi=arg(amp);
    if(i>0){
      double lastphi, lastm;gPhase->GetPoint(i-1,lastm,lastphi);
      if(fabs(phi-lastphi)>=fabs(phi+TMath::Pi()-lastphi))phi+=TMath::Pi();
      else if(fabs(phi-lastphi)>=fabs(phi-TMath::Pi()-lastphi))phi-=TMath::Pi();
    }
    gPhase->SetPoint(i,m,phi);
    gArgand->SetPoint(i,amp.real(),amp.imag());
    complex<double> ampBW=BW(s,f,s0,mu2,g);
    gIBW->SetPoint(i,m,norm(ampBW));
    gPhaseBW->SetPoint(i,m,arg(ampBW));
    gArgandBW->SetPoint(i,ampBW.real(),ampBW.imag());
  }

  TCanvas* c=new TCanvas("c","c",10,10,1000,600);
  c->Divide(3,2);
  c->cd(1);
  gI->Draw("APC");
  c->cd(2);
  gPhase->Draw("AP");
  c->cd(3);
  gArgand->Draw("AP");
 c->cd(4);
  gIBW->Draw("APC");
  c->cd(5);
  gPhaseBW->Draw("AP");
  c->cd(6);
  gArgandBW->Draw("AP");

}
