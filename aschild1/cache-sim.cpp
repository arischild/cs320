#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <cmath>

using namespace std;

struct trace {
	unsigned long long address;
	string action;
};

int directMapped(int kiloBytes);
int setAssociative(int associativity);
int setAssociativeHCR(int associativity);
int setAssociativeNoAlloc(int associativity);
int setAssociativePreFetch(int associativity);
int setAssociativePreFetchOnMiss(int associativity);

vector<trace> traces;

int main(int argc, char *argv[]) {
	if (argc != 3) {
		cout << "incorrect number of arguments" << endl;
		exit(1);
	}

	// read and process data
	ifstream infile(argv[1]);
	string line;
	int numAccess = 0;
	while(getline(infile, line)) {
		numAccess++;
		trace temp;
		istringstream iss(line);
		string address;
		// extract value
		iss >> temp.action;
		if(temp.action == "") break;
		// extract address and convert to int
		iss >> address;
		stringstream ss;
		ss << hex << address.substr(2);
		ss >> temp.address;
		traces.push_back(temp);
	}
	infile.close();

	// compute and print results
	ofstream outfile(argv[2]);
	outfile << directMapped(1) << "," << numAccess << "; ";
	outfile << directMapped(4) << "," << numAccess << "; ";
	outfile << directMapped(16) << "," << numAccess << "; ";
	outfile << directMapped(32) << "," << numAccess << ";" << endl;
	outfile << setAssociative(2) << "," << numAccess << "; ";
	outfile << setAssociative(4) << "," << numAccess << "; ";
	outfile << setAssociative(8) << "," << numAccess << "; ";
	outfile << setAssociative(16) << "," << numAccess << ";" << endl;
	outfile << setAssociative(512) << "," << numAccess << ";" << endl;
	outfile << setAssociativeHCR(512) << "," << numAccess << ";" << endl;
	outfile << setAssociativeNoAlloc(2) << "," << numAccess << "; ";
	outfile << setAssociativeNoAlloc(4) << "," << numAccess << "; ";
	outfile << setAssociativeNoAlloc(8) << "," << numAccess << "; ";
	outfile << setAssociativeNoAlloc(16) << "," << numAccess << ";" << endl;
	outfile << setAssociativePreFetch(2) << "," << numAccess << "; ";
	outfile << setAssociativePreFetch(4) << "," << numAccess << "; ";
	outfile << setAssociativePreFetch(8) << "," << numAccess << "; ";
	outfile << setAssociativePreFetch(16) << "," << numAccess << ";" << endl;
	outfile << setAssociativePreFetchOnMiss(2) << "," << numAccess << "; ";
	outfile << setAssociativePreFetchOnMiss(4) << "," << numAccess << "; ";
	outfile << setAssociativePreFetchOnMiss(8) << "," << numAccess << "; ";
	outfile << setAssociativePreFetchOnMiss(16) << "," << numAccess << ";" << endl;
	outfile.close();
}

int directMapped(int kiloBytes) {
	int hits = 0;
	int rows = kiloBytes * 32;
	int pageTable[rows];
	int offset = log2(rows) + log2(32);
	for(unsigned int i = 0; i < traces.size(); i++) {
		int blockAddress = traces[i].address/32;
		int index = blockAddress % rows;
		int tag = traces[i].address >> offset;
		if(tag == pageTable[index]) hits++;
		else pageTable[index] = tag;
	}
	return hits;
}

int setAssociative(int associativity) {
	int hits = 0;
	int rows = 512;
	int sets = rows / associativity;
	int rowLength = 3 * associativity;
	int **pageTable = new int*[sets];
	for (int i = 0; i < sets; i++) {
		pageTable[i] = new int[rowLength];
		for (int j = 0; j < rowLength; j++) pageTable[i][j] = 0;
	}
	int offset = log2(sets) + log2(32); 
	for(unsigned int i = 0; i < traces.size(); i++) {
		int blockAddress = traces[i].address / 32;
		int index = (blockAddress % sets);
		int tag = traces[i].address >> offset;
		int hit = 0;
		for (int j = 0; j < rowLength && hit == 0; j += 3) {
			if (pageTable[index][j] == tag) {
				hits++;
				hit = 1;
				pageTable[index][j+1] = i;
			}
		}
		int toReplace = 0;
		int foundEmpty = 0;
		for(int j = 0; j < rowLength && (hit == 0 && foundEmpty == 0); j += 3) {
			if (pageTable[index][j + 2] == 0) {
				toReplace = j;
				foundEmpty = 1;
				pageTable[index][j + 2] = 1;
			}
		}
		for(int j = 0; j < rowLength && (foundEmpty == 0 && hit == 0); j += 3) {
			if(pageTable[index][j + 1] < pageTable[index][toReplace + 1]) toReplace = j;
		}
		if(hit == 0) {
			pageTable[index][toReplace] = tag;
			pageTable[index][toReplace + 1] = i;
		}
	}
	return hits;
}

int setAssociativeHCR(int associativity) {
	int hits = 0;
	int rows = (512 * 2) - 1;
	int pageTable[rows];
	for (int i = 0; i < rows; i++) {
		pageTable[i] = 0;
	}
	int offset = log2(32); 
	for(unsigned int i = 0; i < traces.size(); i++) {
		int tag = traces[i].address >> offset;
		int hit = 0;
		for (int j = 511; j < rows && hit == 0; j++) {
			if (pageTable[j] == tag) {
				hits++;
				hit = 1;
			}
		}
		int j = 0;
		while (j < 511 && hit == 0) {
			if (pageTable[j] == 0) {
				pageTable[j] = 1;
				j = (j * 2) + 1;
			} else {
				pageTable[j] = 0;
				j = 2 * (j + 1);
			}
		}
		if(hit == 0) {
			pageTable[j] = tag;
		}
	}
	return hits;
}

int setAssociativeNoAlloc(int associativity) {
	int hits = 0;
	int rows = 512;
	int sets = rows / associativity;
	int rowLength = 3 * associativity;
	int **pageTable = new int*[sets];
	for (int i = 0; i < sets; i++) {
		pageTable[i] = new int[rowLength];
		for (int j = 0; j < rowLength; j++) pageTable[i][j] = 0;
	}
	int offset = log2(sets) + log2(32); 
	for(unsigned int i = 0; i < traces.size(); i++) {
		int blockAddress = traces[i].address / 32;
		int index = (blockAddress % sets);
		int tag = traces[i].address >> offset;
		int hit = 0;
		for (int j = 0; j < rowLength && hit == 0; j += 3) {
			if (pageTable[index][j] == tag) {
				hits++;
				hit = 1;
				pageTable[index][j+1] = i;
			}
		}
		if(traces[i].action == "S") hit = 1;
		int toReplace = 0;
		int foundEmpty = 0;
		for(int j = 0; j < rowLength && (hit == 0 && foundEmpty == 0); j += 3) {
			if (pageTable[index][j + 2] == 0) {
				toReplace = j;
				foundEmpty = 1;
				pageTable[index][j + 2] = 1;
			}
		}
		for(int j = 0; j < rowLength && (foundEmpty == 0 && hit == 0); j += 3) {
			if(pageTable[index][j + 1] < pageTable[index][toReplace + 1]) toReplace = j;
		}
		if(hit == 0) {
			pageTable[index][toReplace] = tag;
			pageTable[index][toReplace + 1] = i;
		}
	}
	return hits;
}

int setAssociativePreFetch(int associativity) {
	int hits = 0;
	int rows = 512;
	int LRUCount = 0;
	int sets = rows / associativity;
	int rowLength = 3 * associativity;
	int **pageTable = new int*[sets];
	for (int i = 0; i < sets; i++) {
		pageTable[i] = new int[rowLength];
		for (int j = 0; j < rowLength; j++) pageTable[i][j] = 0;
	}
	int offset = log2(sets) + log2(32); 
	for(unsigned int i = 0; i < traces.size(); i++) {
		int blockAddress = traces[i].address / 32;
		int index = (blockAddress % sets);
		int pfindex = (blockAddress + 1) % sets;
		int tag = traces[i].address >> offset;
		int pftag = (traces[i].address + 32) >> offset;
		int hit = 0;
		int pfhit = 0;
		for (int j = 0; j < rowLength && hit == 0; j += 3) {
			if (pageTable[index][j] == tag) {
				hits++;
				hit = 1;
				pageTable[index][j+1] = LRUCount;
				LRUCount++;
			}
		}
		int toReplace = 0;
		int foundEmpty = 0;
		for(int j = 0; j < rowLength && (hit == 0 && foundEmpty == 0); j += 3) {
			if (pageTable[index][j + 2] == 0) {
				toReplace = j;
				foundEmpty = 1;
				pageTable[index][j + 2] = 1;
			}
		}
		for(int j = 0; j < rowLength && (foundEmpty == 0 && hit == 0); j += 3) {
			if(pageTable[index][j + 1] < pageTable[index][toReplace + 1]) toReplace = j;
		}
		if(hit == 0) {
			pageTable[index][toReplace] = tag;
			pageTable[index][toReplace + 1] = LRUCount;
			LRUCount++;
		}
		// pre-fetch
		for (int j = 0; j < rowLength && pfhit == 0; j += 3) {
			if (pageTable[pfindex][j] == pftag) {
				pfhit = 1;
				pageTable[pfindex][j+1] = LRUCount;
				LRUCount++;
			}
		}
		toReplace = 0;
		foundEmpty = 0;
		for(int j = 0; j < rowLength && (pfhit == 0 && foundEmpty == 0); j += 3) {
			if (pageTable[pfindex][j + 2] == 0) {
				toReplace = j;
				foundEmpty = 1;
				pageTable[pfindex][j + 2] = 1;
			}
		}
		for(int j = 0; j < rowLength && (foundEmpty == 0 && pfhit == 0); j += 3) {
			if(pageTable[pfindex][j + 1] < pageTable[pfindex][toReplace + 1]) toReplace = j;
		}
		if(pfhit == 0) {
			pageTable[pfindex][toReplace] = pftag;
			pageTable[pfindex][toReplace + 1] = LRUCount;
			LRUCount++;
		}
	}
	return hits;
}

int setAssociativePreFetchOnMiss(int associativity) {
	int hits = 0;
	int rows = 512;
	int LRUCount = 0;
	int sets = rows / associativity;
	int rowLength = 3 * associativity;
	int **pageTable = new int*[sets];
	for (int i = 0; i < sets; i++) {
		pageTable[i] = new int[rowLength];
		for (int j = 0; j < rowLength; j++) pageTable[i][j] = 0;
	}
	int offset = log2(sets) + log2(32); 
	for(unsigned int i = 0; i < traces.size(); i++) {
		int blockAddress = traces[i].address / 32;
		int index = (blockAddress % sets);
		int pfindex = (blockAddress + 1) % sets;
		int tag = traces[i].address >> offset;
		int pftag = (traces[i].address + 32) >> offset;
		int hit = 0;
		int pfhit = 0;
		for (int j = 0; j < rowLength && hit == 0; j += 3) {
			if (pageTable[index][j] == tag) {
				hits++;
				hit = 1;
				pfhit = 1;
				pageTable[index][j+1] = LRUCount;
				LRUCount++;
			}
		}
		int toReplace = 0;
		int foundEmpty = 0;
		for(int j = 0; j < rowLength && (hit == 0 && foundEmpty == 0); j += 3) {
			if (pageTable[index][j + 2] == 0) {
				toReplace = j;
				foundEmpty = 1;
				pageTable[index][j + 2] = 1;
			}
		}
		for(int j = 0; j < rowLength && (foundEmpty == 0 && hit == 0); j += 3) {
			if(pageTable[index][j + 1] < pageTable[index][toReplace + 1]) toReplace = j;
		}
		if(hit == 0) {
			pageTable[index][toReplace] = tag;
			pageTable[index][toReplace + 1] = LRUCount;
			LRUCount++;
		}
		// pre-fetch
		for (int j = 0; j < rowLength && pfhit == 0; j += 3) {
			if (pageTable[pfindex][j] == pftag) {
				pfhit = 1;
				pageTable[pfindex][j+1] = LRUCount;
				LRUCount++;
			}
		}
		toReplace = 0;
		foundEmpty = 0;
		for(int j = 0; j < rowLength && (pfhit == 0 && foundEmpty == 0); j += 3) {
			if (pageTable[pfindex][j + 2] == 0) {
				toReplace = j;
				foundEmpty = 1;
				pageTable[pfindex][j + 2] = 1;
			}
		}
		for(int j = 0; j < rowLength && (foundEmpty == 0 && pfhit == 0); j += 3) {
			if(pageTable[pfindex][j + 1] < pageTable[pfindex][toReplace + 1]) toReplace = j;
		}
		if(pfhit == 0) {
			pageTable[pfindex][toReplace] = pftag;
			pageTable[pfindex][toReplace + 1] = LRUCount;
			LRUCount++;
		}
	}
	return hits;
}


