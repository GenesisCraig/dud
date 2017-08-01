// dud.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <experimental/filesystem>
#include <string>
#include <locale>
#include <chrono>
#include <thread>         // std::thread

namespace fs = std::experimental::filesystem;

using std::string;
using namespace std::chrono_literals;
using namespace std::chrono;
#define MAX_DIRS 1024

float roundPlaces(float number, int places) {
	float retValue;
	float wholeNumber = round(number);
	int multiplier = pow(10,places);
	float decimalNumber = round((number - wholeNumber) * multiplier)*(1/multiplier);
	return retValue;
}

class DudInfo {
public:
	std::string path;
	float dirSizesB = 0;
	int dirFileCount = 0;
	int dirDirCount = 0;
	int dirDaysStale = 0;
};

DudInfo getDirInfo(string path) {

	DudInfo myDir;
	std::time_t dirNewestTime;
	std::time_t thisFileTime;
	time_t now;
	(time_t)0;
	dirNewestTime = (time_t)0;

	for (auto& p : fs::recursive_directory_iterator(path)) {
		if (fs::is_regular_file(p)) {
			myDir.dirSizesB = myDir.dirSizesB + fs::file_size(p);
			myDir.dirFileCount++;
			auto ftime = fs::last_write_time(p);
			thisFileTime = decltype(ftime)::clock::to_time_t(ftime);
			if (thisFileTime > dirNewestTime) {
				dirNewestTime = thisFileTime;
			}
		}
		else if (fs::is_directory(p)) {
			myDir.dirSizesB = myDir.dirSizesB + 512;
			myDir.dirDirCount++;
		}
	}
	time(&now);
	myDir.dirDaysStale = (int)round(difftime(now, dirNewestTime) / 60 / 60 / 24);
	std::cout << std::setw(30) << path
		<< std::setw(11) << myDir.dirFileCount
		<< std::setw(8) << myDir.dirDirCount
		<< std::setw(6) << (myDir.dirSizesB / (std::pow(1024, 2))) << "MB, most recent modification is " << myDir.dirDaysStale << " days old.\n";
	return myDir;
};

int main(int argc, char *argv[])
{
	bool isThreadTwo = false;

	if (argc < 2) {
		std::cerr << "Error: Missing arguments\n";
		return 0;
	}
	else if (argc > MAX_DIRS) {
		std::cerr << "Error: Maximum number of directories (1024) exceeded.\n";
		return 0;
	}
	setlocale(LC_NUMERIC, "");
	DudInfo d;

	std::locale loc("");
	std::cout.imbue(loc);

	high_resolution_clock::time_point tbegin = high_resolution_clock::now();
	for (int i = 1; i < argc; ++i) {
		//high_resolution_clock::time_point t1 = high_resolution_clock::now();
		
		std::thread threadOne(getDirInfo, argv[i]);  // spawn new thread that calls getDirInfo(path)
				
		threadOne.join();
		
		//high_resolution_clock::time_point t2 = high_resolution_clock::now();
		//auto duration = duration_cast<microseconds>(t2 - t1).count();
		//std::cout << argv[i] << ", " << d.dirFileCount << "-files, " << d.dirDirCount << "-dirs, " << (d.dirSizesB / (std::pow(1024, 2))) << "MB, most recent modification is " << d.dirDaysStale << " days old (" << duration / 1000 << "ms)\n";
	}
	high_resolution_clock::time_point t3 = high_resolution_clock::now();
	auto tduration = duration_cast<microseconds>(t3 - tbegin).count();
	std::cout << "\n\nTotal time elapsed: " << tduration / 1000 << "ms\n";
}

