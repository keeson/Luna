#include <iostream>
#include <vector>
#include "featureSel.h"


int main(int argc, char** argv) {
	/*
	int K = 10; //latent factor dimension
	int m = 1000;
	int n = 1000;
	DblMatrix W = new vector<double>[m];
	DblMatrix V = new vector<double>[n];
	
	FeatureSelectionProblem fsp;
	
	numUserFeature
	numAdFeature
	numOtherFeature
	
	//
	Matrix W : mxk
	Matrix V : nxk
	DblVector P: (m+n+o)*1
	
	getWVector mxk x 1
	getVVector nxk x 1
	
	fsp.setW(W_init)
	fsp.setV(V_init)
	fsp.setP(P_init)
	
	Objective o0 FeatureSelectionInit()
	QWLQN.optimize(o0);
	fsp.setP(P)
	
	
	for(iter i < 10){
		
		Objective o1 FeatureSelectionFixAd(W,V,P)
		QWLQN.optimize(o1);
		fsp.setV
		
		Objective o2 FeatureSelectionFixUser(W,V,P)
		QWLQN.optimize(o2)
		fsp.setW
	}
	*/
	
	FeatureSelectionProblem("ins", "fea");
	return 0;
	
}