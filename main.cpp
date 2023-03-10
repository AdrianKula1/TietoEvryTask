#include <iostream>
#include <filesystem>
#include <fstream>
#include <thread>
#include <mutex>
#include <string>
#include <queue>
#include <map>

namespace fs=std::filesystem;

bool compareLogData(std::pair<std::thread::id, std::vector<fs::path>>& a,
        std::pair<std::thread::id, std::vector<fs::path>>& b)
{
    return a.second.size() > b.second.size();
}

class threadPool{
private:

    long noThreads;
    unsigned int searchedFiles=0, filesWithPattern=0, patternsNumber=0;

    std::string stringToFind;
    std::string startingDirectory;

    std::vector<std::thread> threads;
    std::thread directorySearcher;
    std::queue<fs::path> paths;

    std::map<fs::path, std::vector<std::pair<unsigned int, std::string>>> resultData;
    std::map<std::thread::id, std::vector<fs::path>> logData;

    std::mutex *searchMutexes;
    std::mutex m;


public:
    threadPool(long noThreads, std::string& stringToFind){
        this->noThreads=noThreads;
        this->threads.reserve(noThreads);
        this->stringToFind=stringToFind;

        this->searchMutexes = new std::mutex[noThreads];
    }

    void addPathToQueue(const fs::path& pathToFile){
        this->paths.push(pathToFile);
    }

    void beginWork(){
        this->searchedFiles+=this->paths.size();
        for(int i=0; i<noThreads; i++)
            this->threads.emplace_back(&threadPool::fileSearcher, this, i);


        for(int i=0; i<noThreads; i++)
            this->threads[i].join();

    }

    void fileSearcher(int id){
        logData[std::this_thread::get_id()];
        while(!paths.empty()){
            std::lock_guard<std::mutex> lock(searchMutexes[id]);

            if(paths.empty())
                return;

            fs::path pathToFile = paths.front();
            paths.pop();

            std::ifstream file;
            file.open(pathToFile);

            if(!file.good())
                continue;

            logData[std::this_thread::get_id()].push_back(pathToFile.filename());

            bool fileHasPattern = false;
            std::string line;
            unsigned int lineRow = 0, lineColumn = 0;

            while (std::getline(file, line)) {
                lineColumn = line.find(stringToFind);

                if (lineColumn != std::string::npos) {
                    if (!fileHasPattern) {
                        fileHasPattern = true;
                        filesWithPattern++;
                    }

                    patternsNumber++;
                    resultData[pathToFile].emplace_back(std::make_pair(lineColumn, line));
                }
                lineRow++;
            }
            file.close();

        }
    }

    unsigned int getSearchedFiles() const{
        return this->searchedFiles;
    }

    unsigned int getDilesWithPattern() const{
        return this->filesWithPattern;
    }

    unsigned int getPatternsNumber() const{
        return this->patternsNumber;
    }

    std::map<fs::path, std::vector<std::pair<unsigned int, std::string>>> getResultData() const{
        return resultData;
    }
    std::map<std::thread::id, std::vector<fs::path>> getLogData() const{
        return logData;
    }

};



int main(int args, char *argv[]){
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    if(args<2){
        std::cout << "The program has one obligatory parameter which is <pattern(string)>" << std::endl;
        return 0;
    }

    //default parameters
    std::string STRING_TO_FIND = (std::string)argv[1];
    std::string START_DIRECTORY = fs::current_path().string();
    std::string LOG_FILE_NAME = std::filesystem::path(argv[0]).filename().string()+".log";
    std::string RESULT_FILE_NAME = std::filesystem::path(argv[0]).filename().string()+".txt";
    long NUMBER_OF_THREADS = 4;

    if(args>2){
        for(int i=2; i<args; i+=2){
            if((std::string)argv[i]=="-d" || (std::string)argv[i]=="--dir" ){
                START_DIRECTORY = (std::string) argv[i+1];
            }
            if((std::string)argv[i]== "-l" || (std::string)argv[i]=="--log_file" ){
                LOG_FILE_NAME = (std::string) argv[i+1]+".log";
            }
            if((std::string)(argv[i])=="-r" || (std::string)argv[i]=="--result_file" ){
                RESULT_FILE_NAME = (std::string) argv[i+1]+".txt";
            }
            if((std::string)(argv[i])=="-t" || (std::string)argv[i]=="--threads" ){
                char* p;
                long arg = strtol(argv[i+1], &p, 10);
                NUMBER_OF_THREADS = arg;
            }
        }
    }

    threadPool pool(NUMBER_OF_THREADS, STRING_TO_FIND);

    for(const fs::directory_entry& entry : fs::recursive_directory_iterator(START_DIRECTORY)){
        if(entry.is_regular_file()){
            pool.addPathToQueue(entry.path());
        }
    }

    pool.beginWork();

    unsigned int searchedFiles = pool.getSearchedFiles();
    unsigned int filesWithPattern = pool.getDilesWithPattern();
    unsigned int patternsNumber = pool.getPatternsNumber();

    std::map<fs::path, std::vector<std::pair<unsigned int, std::string>>> resultData = pool.getResultData();
    std::map<std::thread::id, std::vector<fs::path>> logData = pool.getLogData();

    std::ofstream resultFile(RESULT_FILE_NAME);
    if(resultFile.is_open()){
        for(auto &path: resultData){
            for (auto &numberAndLine: path.second){
                resultFile << path.first.string() << ':' << numberAndLine.first << ": " << numberAndLine.second << std::endl;
            }
        }
    }
    resultFile.close();


    std::vector<std::pair<std::thread::id, std::vector<fs::path>>> logDataVector;
    for(auto &data : logData){
        logDataVector.emplace_back(data);
    }

    std::sort(logDataVector.begin(), logDataVector.end(), compareLogData);

    std::ofstream logFile(LOG_FILE_NAME);
    if(logFile.is_open()){
        for(auto &threadData: logDataVector){
            logFile << threadData.first << ": ";
            for (auto &filename: threadData.second){
                logFile << filename << ',' ;
            }
            logFile << std::endl;
        }
    }
    logFile.close();

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    long long int elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(end-begin).count();

    std::cout << "Searched files: " << searchedFiles << std::endl;
    std::cout << "Files with pattern: " << filesWithPattern << std::endl;
    std::cout << "Patterns number: " << patternsNumber << std::endl;
    std::cout << "Result file: " << RESULT_FILE_NAME << std::endl;
    std::cout << "Log file: " << LOG_FILE_NAME << std::endl;
    std::cout << "Used threads: " << NUMBER_OF_THREADS << std::endl;
    std::cout << "Elapsed time: " << elapsedTime  << "[ms]"<< std::endl;

    return 0;
}
