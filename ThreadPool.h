#ifndef TIETOEVRYTASK_THREADPOOL_H
#define TIETOEVRYTASK_THREADPOOL_H
#include<condition_variable>
#include<filesystem>
#include<string>
#include<thread>
#include<mutex>
#include<queue>
#include<map>

namespace fs = std::filesystem;

class ThreadPool {
private:
    long noThreads;
    unsigned int searchedFiles=0, filesContainingPattern=0, patternsNumber=0;

    std::string stringToFind;
    std::string startingDirectory;
    std::string resultFileName;
    std::string startDirectory;

    std::vector<std::thread> threads;
    std::thread directorySearcher;
    std::queue<fs::path> paths;

    std::map<std::thread::id, std::vector<fs::path>> threadIdToPathsMap;
    std::map<fs::path, std::vector<std::pair<unsigned int, std::string>>> filePathToLineMap;

    std::mutex mutexQueue;
    std::mutex mutexFilePathToLineMap;

    std::condition_variable emptyQueueCondition;

    bool allFilesFound=false;

public:

    ThreadPool(long noThreads, std::string& stringToFind, std::string& resultFileName, std::string& startDirectory);
    void resetResultFile();
    void searchDirectory();
    void addPathToQueue(const fs::path& pathToFile);
    void beginWork();
    void startWorkWithFile();
    void searchWithinFile(fs::path &pathToFile);

    unsigned int getSearchedFiles() const;
    unsigned int getFilesWithPattern() const;
    unsigned int getPatternsNumber() const;
    std::map<std::thread::id, std::vector<fs::path>> getThreadIdToPathsMap() const;
    std::map<fs::path, std::vector<std::pair<unsigned int, std::string>>> getfilePathToLineMap() const;

};

#endif //TIETOEVRYTASK_THREADPOOL_H
