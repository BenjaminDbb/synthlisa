#include "lisasim-tens.h"
#include "lisasim-wave.h"

#include <iostream>
using namespace std;

#include <math.h>

WaveArray::WaveArray(Wave **warray, int wnum) : wavenum(wnum) {
    if(wnum < 1) {
	cout << "WaveArray needs at least one wave object..." << endl;
	abort();
    }

    wavearray = new Wave*[wnum]; // syntax?

    for(int i=0;i<wnum;i++)
	wavearray[i] = warray[i];
}

WaveArray::~WaveArray() {
    delete [] wavearray;
}

Wave *WaveArray::firstwave() {
    wavecurrent = 0;
    return wavearray[0];
}

Wave *WaveArray::nextwave() {
    if(++wavecurrent < wavenum)
	return wavearray[wavecurrent];
    else
	return 0;
}

Wave::Wave(double b, double l, double p) {
    beta = b;
    lambda = l;
    pol = p;

    k[0] = -cos(lambda)*cos(beta);
    k[1] = -sin(lambda)*cos(beta);
    k[2] = -sin(beta);

    Tensor stdpp(0.0), stdpc(0.0);

    stdpp[0][0] = 1.0; stdpp[1][1] = -1.0;
    stdpc[0][1] = 1.0; stdpc[1][0] =  1.0;

    Tensor A, At;
    
    A.seteuler(beta,lambda,pol);
    At.settranspose(A);

    Tensor tmp;
    
    tmp.setproduct(stdpp,At);
    pp.setproduct(A,tmp);
    
    tmp.setproduct(stdpc,At);
    pc.setproduct(A,tmp);    
}

void Wave::putk(Vector &kout) {
    kout[0] = k[0];
    kout[1] = k[1];
    kout[2] = k[2];

    // ??? Is this needed or would a direct copy work?
}

void Wave::putwave(Tensor &h, double t) {
  double hp_temp = hp(t);
  double hc_temp = hc(t);
  
  for(int i=0;i<3;i++) {
    for(int j=0;j<3;j++) {
      h[i][j] = hp_temp * pp[i][j] + hc_temp * pc[i][j];
    }        
  }
}

// full constructor for SimpleBinary; takes frequency in Hertz
// b and l are SSB ecliptic latitude and longitude

SimpleBinary::SimpleBinary(double freq, double initphi, double inc, double amp, double b, double l, double p) : Wave(b,l,p) {
    f = freq;

    phi0 = initphi;

    i = inc;
    a = amp;
    
    ap = a * (1.0 + cos(i)*cos(i));
    ac = a * (2.0 * cos(i));
}

double SimpleBinary::hp(double t) {
    const double twopi = 2.0*M_PI;

    return ap * cos(twopi*f*t + phi0);
}

double SimpleBinary::hc(double t) {
    const double twopi = 2.0*M_PI;
    
    return ac * sin(twopi*f*t + phi0);
}            


// --- SimpleMonochromatic wave class --------------------------------------------------

// originally written to compare with John's fortran code

SimpleMonochromatic::SimpleMonochromatic(double freq, double phi, double gamma, double amp, double b, double l, double p) : Wave(b,l,p) {
    f = freq;

    ph = phi;
    gm = gamma;
    
    ap = amp*sin(gm);
    ac = amp*cos(gm);
}

double SimpleMonochromatic::hp(double t) {
    const double twopi = 2.0*M_PI;

    return ap * sin(twopi*f*t + ph);
}

double SimpleMonochromatic::hc(double t) {
    const double twopi = 2.0*M_PI;
    
    return ac * sin(twopi*f*t);
}

// --- GaussianPulse ---

GaussianPulse::GaussianPulse(double time, double decay, double gamma, double amp, double b, double l, double p) : Wave(b,l,p) {
  t0 = time;
  dc = decay;

  a = amp;
  gm = gamma;

  ap = a * sin(gm);
  ac = a * cos(gm);
}

const double GaussianPulse::sigma_cutoff = 10.0;

int GaussianPulse::inscope(double t) {
    double ex = (t - t0) / dc;

    return fabs(ex) < sigma_cutoff;
}

double GaussianPulse::hp(double t) {
    double ex = (t - t0) / dc;

    return ap * exp(-ex*ex);
}

double GaussianPulse::hc(double t) {
    double ex = (t - t0) / dc;

    return ac * exp(-ex*ex);
}

// --- NoiseWave ---

NoiseWave::NoiseWave(Noise *noisehp, Noise *noisehc, double b, double l, double p) : Wave(b,l,p) {
    np = noisehp;
    nc = noisehc;

    allocated = 0;
}

NoiseWave::NoiseWave(double sampletime, double prebuffer, double density, double exponent, int swindow, double b, double l, double p) : Wave(b,l,p) {
    np = new InterpolateNoise(sampletime,prebuffer,density,exponent,swindow);
    nc = new InterpolateNoise(sampletime,prebuffer,density,exponent,swindow);

    allocated = 1;
}

NoiseWave::NoiseWave(double *hpa, double *hca, long samples, double sampletime, double prebuffer, double density, double exponent, int swindow, double b, double l, double p) : Wave(b,l,p) {
    np = new InterpolateNoise(hpa,samples,sampletime,prebuffer,density,exponent,swindow);
    nc = new InterpolateNoise(hca,samples,sampletime,prebuffer,density,exponent,swindow);

    allocated = 1;
}

NoiseWave::~NoiseWave() {
    if(allocated) {
	delete nc;
	delete np;
    }
}

NoiseWave *SampledWave(double *hpa, double *hca, long samples, double sampletime, double prebuffer, double density, double exponent, int swindow, double d, double a, double p) {
    return new NoiseWave(hpa,hca,samples,sampletime,prebuffer,density,exponent,swindow,d,a,p);
}
