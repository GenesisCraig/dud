// dud.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <experimental/filesystem>
#include <string>
#include <locale>
#include <chrono>

namespace fs = std::experimental::filesystem;

using std::string;
using namespace std::chrono_literals;
using namespace std::chrono;
#define MAX_DIRS 1024

string float_to_money(float input) {
	float dollarsPortion = round(input);
	float centsPortion = round((input - dollarsPortion) * 100);
	string retValue = std::to_string(dollarsPortion) + '.' + std::to_string(centsPortion);
	return retValue;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Error: Missing arguments\n";
		return 0;
	}
	else if (argc > MAX_DIRS ) {
		std::cerr << "Error: Maximum number of directories (1024) exceeded.\n";
		return 0;
	}

	setlocale(LC_NUMERIC, "");
	float dirSizesB[MAX_DIRS];
	int dirFileCount[MAX_DIRS];
	int dirDirCount[MAX_DIRS];
	std::time_t dirNewestTime[MAX_DIRS];
	std::time_t thisFileTime;
	std::locale loc("");
	std::cout.imbue(loc);
	time_t now;
	time(&now);
	high_resolution_clock::time_point tbegin = high_resolution_clock::now();
	for (int i = 1; i < argc; ++i) {
		dirSizesB[i] = 0.0;
		dirFileCount[i] = 0;
		dirDirCount[i] = 0;
		dirNewestTime[i] = (time_t)0;
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		std::cout << argv[i] << ", ";
		for (auto& p : fs::recursive_directory_iterator(argv[i])) {
			if (fs::is_regular_file(p)) {
				dirSizesB[i] = dirSizesB[i] + fs::file_size(p);
				dirFileCount[i]++;
				auto ftime = fs::last_write_time(p);
				thisFileTime = decltype(ftime)::clock::to_time_t(ftime);
				if (thisFileTime > dirNewestTime[i]) {
					dirNewestTime[i] = thisFileTime;
				}
			}
			else if (fs::is_directory(p)) {
				dirSizesB[i] = dirSizesB[i] + 512;
				dirDirCount[i]++;
			}
		}
		high_resolution_clock::time_point t2 = high_resolution_clock::now();
		auto duration = duration_cast<microseconds>(t2 - t1).count();
		std::cout << dirFileCount[i] << "-files, " << dirDirCount[i] << "-dirs, " << (dirSizesB[i] / (std::pow(1024, 2))) << "MB, most recent modification is " << round(difftime(now, dirNewestTime[i]) / 60 / 60 / 24) << " days old ("<< duration/1000<<"ms)\n";
	}
	
	high_resolution_clock::time_point t3 = high_resolution_clock::now();

	auto tduration = duration_cast<microseconds>(t3 - tbegin).count();
	
	std::cout << "\n\nTotal time elapsed: "<< tduration / 1000 << "msl\n";
}

