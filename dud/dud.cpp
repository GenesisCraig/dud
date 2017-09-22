/* dud.cpp : Defines the entry point for the console application.
 *
 */

 // TODO: Add option to parse paths as possible client numbers and quesry BST for client / project status for archivability hinting.
 // Junk.cpp : Defines the entry point for the console application.
 //

#include "stdafx.h"
#include <iostream>
#include<iomanip>
#include <locale>
#include <thread>
#include <string>
#include <mutex>
#include <vector>
#include <ctime>
#include <filesystem>
#include <chrono>

using namespace std;
namespace fs = std::experimental::filesystem;
using namespace std::chrono_literals;
using namespace std::chrono;

#define MIN_STALENESS 365 // Days
#define MIN_SIZE (1024*1024) * 500 //500MB
#define MAX_DIRS 1024
mutex queueLock;
mutex resultLock;
mutex coutLock;

vector<string> queue;
vector<string> result;

struct DirInfo {
	string path;
	long unsigned int dirSizesB = 0;
	int dirFileCount = 0;
	int dirDirCount = 0;
	int dirDaysStale = 0;
	string dirFlag = "";
};

void worker(short);
DirInfo getDirectoryInfo(string);
void printOutput(DirInfo, string);
float roundPlaces(float, int);

int main(int argc, char *argv[])
{
	bool csvOut = false;
	if (argc < 2) {
		std::cerr << "Error: Missing arguments\n";
		return 0;
	}
	else if (argc > MAX_DIRS) {
		std::cerr << "Error: Maximum number of directories (" << MAX_DIRS << ") exceeded.\n";
		return 0;
	}

	if (argv[1] == "--csv") csvOut = true;

	unsigned long const max_threads = 8;
	unsigned long const hardware_threads = std::thread::hardware_concurrency();
	unsigned long const num_threads = min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

	vector<thread> threads(num_threads - 1);

	for (int i = 1; i < argc; ++i) {
		if (fs::exists(argv[i])) {
			queue.push_back(argv[i]);
		}
	}

	setlocale(LC_NUMERIC, "");
	std::locale loc("");
	std::cout.imbue(loc);
	std::cout << "\nUsing " << num_threads << " threads\n";
	std::cout << "\nPath                                          Files    Dirs  Size (MB)  Days Stale\n";
	std::cout << "---------------------------------------- ---------- ------- ---------- -----------\n";

	high_resolution_clock::time_point tbegin = high_resolution_clock::now();

	for (int i = 0; i < (num_threads - 1); ++i) {
		threads[i] = thread(worker, i);
	}

	for (int i = 0; i < (num_threads - 1); ++i) {
		threads[i].join();
	}

	high_resolution_clock::time_point tend = high_resolution_clock::now();

	//auto tduration = duration_cast<microseconds>((t3 - tbegin)).count();
	auto tduration = duration_cast<milliseconds>(tend - tbegin).count();
	cout << "\n\nTotal time elapsed: " << tduration << "ms\n";

	//system("PAUSE");
	return 0;
}

void worker(short myThreadNumber) {
	bool gotIt = true;
	string myPath;
	while (gotIt == true) {
		queueLock.lock();
		if (queue.size() > 0) {
			myPath = queue.back();
			queue.pop_back();
		}
		else {
			gotIt = false;
		}
		queueLock.unlock();
		if (gotIt == true) {

			DirInfo thisDirectory = getDirectoryInfo(myPath);

			//cout << "\n Thread " << myThreadNumber << " got path " << myPath << endl;
			coutLock.lock();
			printOutput(thisDirectory, "txt");
			coutLock.unlock();
		}
	}
	return;
}

DirInfo getDirectoryInfo(string myPath) {
	DirInfo d;
	d.path = myPath;

	std::time_t dirNewestTime;
	std::time_t thisFileTime;
	time_t now;
	(time_t)0;
	dirNewestTime = (time_t)0;

	for (auto& p : fs::recursive_directory_iterator(myPath)) {
		if (fs::is_regular_file(p)) {
			d.dirSizesB += (long)fs::file_size(p);
			d.dirFileCount++;
			auto ftime = fs::last_write_time(p);
			thisFileTime = decltype(ftime)::clock::to_time_t(ftime);
			if (thisFileTime > dirNewestTime) {
				dirNewestTime = thisFileTime;
			}
		}
		else if (fs::is_directory(p)) {
			d.dirSizesB += 512;
			d.dirDirCount++;
			auto ftime = fs::last_write_time(p);
			thisFileTime = decltype(ftime)::clock::to_time_t(ftime);
			if (thisFileTime > dirNewestTime) {
				dirNewestTime = thisFileTime;
			}
		}
	}
	time(&now);
	d.dirDaysStale = (int)round(difftime(now, dirNewestTime) / 60 / 60 / 24);

	if (d.dirDaysStale > MIN_STALENESS && d.dirSizesB > MIN_SIZE) {
		d.dirFlag = "ARCHIVE";
	}
	return d;
};

void printOutput(DirInfo d, string format) {
	if (format == "txt") {
		cout.imbue(std::locale(""));
		cout << std::setprecision(2) << fixed;
		cout << left << setw(40) << d.path << right
			<< setw(11) << d.dirFileCount
			<< setw(8) << d.dirDirCount
			<< setw(11) << roundPlaces((d.dirSizesB / ((float)std::pow(1024, 2))), 2)
			<< setw(12) << d.dirDaysStale
			<< setw(10) << d.dirFlag << endl;
	}
	else {
		cout << "\"" << d.path << "\"," << d.dirFileCount << "," << d.dirDirCount << ","
			<< roundPlaces((d.dirSizesB / ((float)std::pow(1024, 2))), 2) << ","
			<< d.dirDaysStale << "," << d.dirFlag << endl;
	}
};

float roundPlaces(float number, int places) {
	float wholeNumber = round(number);
	float multiplier = (float)pow(10, places);
	float decimalNumber = round((number - wholeNumber) * multiplier)*(1 / multiplier);
	return (wholeNumber + decimalNumber);
}