#include <mpi.h>
#include <iostream>
#include <vector>
#include <fstream>

#include "OWLQN.h"
#include "featureSel.h"
using namespace std;

void printVector(const DblVec &vec, const char* filename) {
	ofstream outfile(filename);
	if (!outfile.good()) {
		cerr << "error opening matrix file " << filename << endl;
		exit(1);
	}

	for (size_t i=0; i<vec.size(); i++) {
		outfile << vec[i] << endl;
	}
	outfile.close();
}

void printUsageAndExit() {
	cout << "options:" << endl;
	cout << "  -tol <value>   sets convergence tolerance (default is 1e-4)" << endl;
 	cout << "  -l2weight <value>" << endl;
 	cout << "  -l12weight <value>" << endl;
 	cout << "  -K <value>" << endl;
	cout << endl;
	exit(0);
}
double calSparsity(const DblVec& vec, size_t dimLatent){
	int num = vec.size() / dimLatent;
	int sparseNum = 0;
	double sum = 0;
	for(int i = 0; i < num; i++){ 
		sum = 0;
		for(int j = 0; j < dimLatent; j++)
		sum += vec[i*dimLatent+j] * vec[i*dimLatent+j];
		if(sum < 1e-10) sparseNum++;
	}
	return (double)sparseNum / num;
} 


int main(int argc, char** argv) {

	int my_rankid;
	int cnt_processors;
	char train_file[100] = "./data/train/ins";
	char fea_file[100] = "./FeaDict.dat";
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rankid);
	MPI_Comm_size(MPI_COMM_WORLD, &cnt_processors);
	
	
	int K = 10; //latent factor dimension
	double l21reg = 0.0;
	double l2weight = 0.0;
	double tol = 1e-6;
	for (int i=1; i<argc; i++) {
		 if (!strcmp(argv[i], "-tol")) {
			++i;
			if (i >= argc || (tol = atof(argv[i])) <= 0) {
				cout << "-tol (convergence tolerance) flag requires 1 positive real argument." << endl;
				exit(1);
			}
		} else if (!strcmp(argv[i], "-l2weight")) {
			++i;
			if (i >= argc || (l2weight = atof(argv[i])) < 0) {
				cout << "-l2weight flag requires 1 non-negative real argument." << endl;
				exit(1);
			}
		}
		else if (!strcmp(argv[i], "-l21weight")) {
			++i;
			if (i >= argc || (l21reg = atof(argv[i])) < 0) {
				cout << "-l21weight flag requires 1 non-negative real argument." << endl;
				exit(1);
			}
		}
		else if (!strcmp(argv[i], "-K")) {
			++i;
			if (i >= argc || (K = atoi(argv[i])) < 0) {
				cout << "-K flag requires 1 non-negative integer argument." << endl;
				exit(1);
			}
		}
	}
	FeatureSelectionProblem *fsp = new FeatureSelectionProblem(train_file, fea_file, K, my_rankid);
	DifferentiableFunction* o00  = new FeatureSelectionObjectiveInitP(*fsp);
	DifferentiableFunction* o0  = new FeatureSelectionObjectiveInit(*fsp, l2weight);
	DifferentiableFunction* o1  = new FeatureSelectionObjectiveFixAd(*fsp, l21reg);
	DifferentiableFunction* o2  = new FeatureSelectionObjectiveFixUser(*fsp, l21reg);
	
	DblVec& P = fsp->getP();
	DblVec& P1 = fsp->getP1();
	DblVec& P2 = fsp->getP2();
	
	size_t Psize = P.size();
	size_t P1size = P1.size();
	size_t P2size = P2.size();
	
	
	double l1regweight = 0;
	int m = 10;
	DblVec input0(Psize), gradient0(Psize);
	
	if(my_rankid == 0){
		OWLQN opt0;
		for(int i = 0; i < Psize; i++) input0[i] = P[i];
		opt0.Minimize(*o00, input0, input0, l1regweight, tol, m);
		o00->handler(0, 0); // inform all non-root worker finish
		for(int i = 0; i < Psize; i++) P[i] = input0[i];
	}
	else{
		int ret;
		int command = 0;
		while(1){
			ret = o00->handler(my_rankid, command);
			if(ret == 0){
				break;
			}
			else{
				o00->Eval(input0, gradient0);
			}
		}
		
	}
	//broadcast P to all slaver node
	MPI_Bcast(&(P[0]), Psize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&(P1[0]), P1size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&(P2[0]), P2size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	size_t size = Psize + P1size + P2size;
	DblVec input(size), gradient(size);
	
	if(my_rankid == 0){
		OWLQN opt;
		for(size_t i = 0; i < Psize; i++) input[i] = P[i];
		for(size_t i = 0; i < P1size; i++) input[i+Psize] = P1[i];
		for(size_t i = 0; i < P2size; i++) input[i+Psize+P1size] = P2[i];
		opt.Minimize(*o0, input, input, l1regweight, tol, m);
		o0->handler(0, 0); // inform all non-root worker finish
		for(size_t i = 0; i < Psize; i++) P[i] = input[i];
		for(size_t i = 0; i < P1size; i++) P1[i] = input[i+Psize];
		for(size_t i = 0; i < P2size; i++) P2[i] = input[i+Psize+P1size];
	}
	else{
		int ret;
		int command = 0;
		while(1){
			ret = o0->handler(my_rankid, command);
			if(ret == 0){
				break;
			}
			else{
				o0->Eval(input, gradient);
			}
		}
		
	}
	//broadcast P to all slaver node
	MPI_Bcast(&(P[0]), Psize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&(P1[0]), P1size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&(P2[0]), P2size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	if(my_rankid == 0)
		cout << "HAHAHAHAHAHA INIT finished" << endl;
		
	
	//INIT finish
	
	
	//ALternate optimization for user and ad parts
	
	DblVec& W = fsp->getW();
	DblVec& V = fsp->getV();
	size_t Wsize = W.size();
	size_t Vsize = V.size();
	size_t size1 = Wsize+size;
	size_t size2 = Vsize+size;
	DblVec input1(size1), gradient1(size1);
	DblVec input2(size2), gradient2(size2);
	
	
	MPI_Bcast(&(W[0]), Wsize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&(V[0]), Vsize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	double loss = 1e8;
	
	
	for(int iter = 0; iter < 5; iter++){
		for(size_t i = 0; i < Wsize; i++) input1[i] = W[i];
		for(size_t i = 0; i < Psize; i++) input1[i+Wsize] = P[i];
		for(size_t i = 0; i < P1size; i++) input1[i+Wsize+Psize] = P1[i];
		for(size_t i = 0; i < P2size; i++) input1[i+Wsize+Psize+P1size] = P2[i];
		if(my_rankid == 0){
			OWLQN opt1;	
			opt1.Minimize(*o1, input1, input1, l1regweight, tol, m, iter);
		//	opt1.Minimize(*o1, input1, input1, l1regweight, tol, m, iter);
			o1->handler(0, 0); // inform all non-root worker finish
			for(size_t i = 0; i<Wsize; i++) W[i] = input1[i];
			for(size_t i = 0; i<Psize; i++) P[i] = input1[i+Wsize];
			for(size_t i = 0; i<P1size; i++) P1[i] = input1[i+Wsize+Psize];
			for(size_t i = 0; i<P2size; i++) P2[i] = input1[i+Wsize+Psize+P1size];
				
		}
		else{
			int ret;
			int command = 0;
			while(1){
				ret = o1->handler(my_rankid, command);
				if(ret == 0){
					break;
				}
				else{
					o1->Eval(input1, gradient1);
				}
			}
		}
		
		if(my_rankid == 0) cout << ">>Iter" <<iter << " OPT1 END" << endl;
		
		MPI_Bcast(&(W[0]), Wsize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(P[0]), Psize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(P1[0]), P1size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(P2[0]), P2size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		
		
		for(size_t i = 0; i < Vsize; i++) input2[i] = V[i];
		for(size_t i = 0; i < Psize; i++) input2[i+Vsize] = P[i];
		for(size_t i = 0; i < P1size; i++) input2[i+Vsize+Psize] = P1[i];
		for(size_t i = 0; i < P2size; i++) input2[i+Vsize+Psize+P1size] = P2[i];
		
		
		if(my_rankid == 0){
			OWLQN opt2;
			double newloss = opt2.Minimize(*o2, input2, input2, l1regweight, tol, m, iter);
			o2->handler(0, 0); // inform all non-root worker finish
			for(size_t i = 0; i<Vsize; i++) V[i] = input2[i];
			for(size_t i = 0; i<Psize; i++) P[i] = input2[i+Vsize];
			for(size_t i = 0; i<P1size; i++) P1[i] = input2[i+Vsize+Psize];
			for(size_t i = 0; i<P2size; i++) P2[i] = input2[i+Vsize+Psize+P1size];
		}
		
		else{
			int ret;
			int command = 0;
			while(1){
				ret = o2->handler(my_rankid, command);
				if(ret == 0){
					break;
				}
				else{
					o2->Eval(input2, gradient2);
				}
			}
		}
		
		if(my_rankid == 0)  cout << ">>Iter" <<iter << " OPT2 END" << endl;
		MPI_Bcast(&(V[0]), Vsize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(P[0]), Psize, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(P1[0]), P1size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&(P2[0]), P2size, MPI_DOUBLE, 0, MPI_COMM_WORLD);
			
		
		if(my_rankid == 0){
//				if ((loss - newloss) / loss > 1e-8){
//					loss = newloss;
//				}
//				else{
//					cout << "LARGE ITERATION: " << iter << " END" << endl;
//					break;
//				}
		}
	
	}

	printVector(fsp->getP(), "./rank-00000/modelP");
	printVector(fsp->getP1(), "./rank-00000/modelP1");
	printVector(fsp->getP2(), "./rank-00000/modelP2");
	printVector(fsp->getW(), "./rank-00000/modelW");
	printVector(fsp->getV(), "./rank-00000/modelV");
	if(my_rankid == 0){
		cout <<"HAHAHHAHAHA GAME OVER\n";
		cout << "Sparsity of W is : " << calSparsity(fsp->getW(),K) << endl;
		cout << "Sparsity of V is : " << calSparsity(fsp->getV(),K) << endl;
	}
	MPI_Finalize();
	return 0;
	
}
