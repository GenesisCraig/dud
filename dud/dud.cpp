/* dud.cpp
 * Craig H. Anderson - 2017 ©
 * 
 * List the size and days since last modified of directories specified on the command line.  Sizes and
 * modification dates are calculated by recursively traversing the directories and accumulating the
 * size and recording the newest file/directory modified anywhere under the parent directory.
 */

 // TODO: Add option to parse paths as possible client numbers and quesry BST for client / project status for archivability hinting.

#include "stdafx.h"
#include <iostream>
#include <iomanip>
#include <locale>
#include <thread>
#include <string>
#include <mutex>
#include <vector>
#include <ctime>
#include <filesystem>
#include <chrono>
#include "dud.h"

using namespace std;
namespace fs = std::experimental::filesystem;
using namespace std::chrono_literals;
using namespace std::chrono;

mutex queueLock;
mutex resultLock;
mutex coutLock;

vector<string> queue;
vector<string> result;

// Accessed by worker threads to compare directory ages against.  Set once in main(),
//		read many times in getDirectoryInfo()
time_t now;

struct DirInfo {
	string path;
	long unsigned int dirSizesB = 0;
	int unsigned dirFileCount = 0;
	int unsigned dirDirCount = 0;
	int dirDaysStale = 0;

	string dirFlag = "";
};

class DirEntry {
public:
	string path;
	long unsigned int dirSizesB = 0;
	int unsigned dirFileCount = 0;
	int unsigned dirDirCount = 0;
	int dirDaysStale = 0;
	string dirFlag = "";
	short outputType = 0;
	void printData(void);
};

void DirEntry::printData() {
	switch (this->outputType) {
	case 0: //TEXT
		cout << std::setprecision(2) << fixed;
		cout << left << setw(40) << this->path << right
			<< setw(11) << this->dirFileCount
			<< setw(8) << this->dirDirCount
			<< setprecision(2) << fixed << setw(11) << this->dirSizesB / (1024.0* 1024.0)
			<< setw(12) << this->dirDaysStale
			<< setw(10) << this->dirFlag << endl;
		break;
	case 2: //CSV
		cout << "\"" << this->path << "\","
			<< this->dirFileCount
			<< "," << this->dirDirCount
			<< "," << this->dirSizesB / (1024.0* 1024.0)
			<< "," << this->dirDaysStale
			<< "," << this->dirFlag
			<< endl;
		break;
	}
}

void worker(short);
DirInfo getDirectoryInfo(string);
void printOutput(DirInfo, string);

int main(int argc, char *argv[])
{
	unsigned int i;

	bool csvOut = false;
	if (argc < 2) {
		cerr << "Error: Missing arguments\n";
		return 0;
	}
	else if (argc > MAX_DIRS) {
		cerr << "Error: Maximum number of directories (" << MAX_DIRS << ") exceeded.\n";
		return 0;
	}

	//Store current time in global variable "now" for use in directory age calulation in all threads.
	time(&now);
	
	if (argv[1] == "--csv") csvOut = true;

	unsigned long  const max_threads = 8;
	unsigned long  const hardware_threads = std::thread::hardware_concurrency();
	unsigned long  const num_threads = min(hardware_threads != 0 ? hardware_threads : 2, max_threads);

	vector<thread> threads(num_threads - 1);

	for (short k = 1; k < argc; ++k) {
		string item = argv[k];
		size_t found = item.find("*");
		if (found != std::string::npos) {
			cout << "Argument [" << k << "] '" << item << "',  has an unexpended wildcard and will be ignored\n";
		}
		else {
			queue.push_back(item);
		}
	}

	setlocale(LC_NUMERIC, "");
	locale loc("");
	cout.imbue(loc);
	cout << "\nUsing " << num_threads << " threads\n"
		<< "\nPath                                          Files    Dirs  Size (MB)  Days Stale\n"
		<< "---------------------------------------- ---------- ------- ---------- -----------\n";

	high_resolution_clock::time_point tbegin = high_resolution_clock::now();

	// Generate an appropriate number of worker threads
	for (i = 0; i < (num_threads - 1); ++i) {
		threads[i] = thread(worker, i);
	}

	// Wait for threads to finish their work
	for (i = 0; i < (num_threads - 1); ++i) {
		threads[i].join();
	}
	high_resolution_clock::time_point tend = high_resolution_clock::now();

	//auto tduration = duration_cast<microseconds>((t3 - tbegin)).count();
	auto tduration = duration_cast<milliseconds>(tend - tbegin).count();
	cout << "\n\nTotal time elapsed: " << tduration << "ms\n";

	system("PAUSE");
	return 0;
}

/** This is the worker thread that will grab work off the queue, perform it, then return for the next job
*/
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
			if (fs::exists(myPath)) {
				DirInfo thisDirectory = getDirectoryInfo(myPath);
				//cout << "\n Thread " << myThreadNumber << " got path " << myPath << endl;
				coutLock.lock();
				printOutput(thisDirectory, "txt");
				coutLock.unlock();
			}
		}
	}
	return;
}

/** This is the primary effort generator that iterates through a directory structure and gathers information.
* @TODO move collecting the current time to a global variable, and then use it here for determining age.
*/
DirInfo getDirectoryInfo(string myPath) {
	DirInfo d;
	d.path = myPath;

	time_t dirNewestTime;
	time_t thisFileTime;
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
	
	d.dirDaysStale = (int)round(difftime(now, dirNewestTime) / 60 / 60 / 24);

	if (d.dirDaysStale > MIN_STALENESS && d.dirSizesB > MIN_SIZE) {
		d.dirFlag = "ARCHIVE";
	}
	return d;
};

/** Prints output to the console
*/
void printOutput(DirInfo d, string format) {
	if (format == "txt") {
		cout.imbue(std::locale(""));
		cout << std::setprecision(2) << fixed;
		cout << left << setw(40) << d.path << right
			<< setw(11) << d.dirFileCount
			<< setw(8) << d.dirDirCount;
		cout << setw(11) << setprecision(2) << fixed << (d.dirSizesB / (1024.0 * 1024.0));
		cout << setw(12) << d.dirDaysStale
			<< setw(10) << d.dirFlag << endl;
	}
	else {
		cout << "\"" << d.path << "\"," << d.dirFileCount << "," << d.dirDirCount << ",";
		cout << setprecision(2) << fixed << (d.dirSizesB / (1024.0 * 1024.0)) << ",";
		cout << d.dirDaysStale << "," << d.dirFlag << endl;
	}
};