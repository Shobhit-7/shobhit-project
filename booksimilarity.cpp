#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <cctype>
#include <cstring>
#include <dirent.h>

const int NUM_BOOKS = 64;
const int TOP_WORDS = 100;
const int TOP_PAIRS = 10;

struct WordFreq {
    std::string word;
    double frequency;

    WordFreq() : word(""), frequency(0.0) {}
    WordFreq(const std::string& w, double f) : word(w), frequency(f) {}

    bool operator<(const WordFreq& other) const {
        return frequency > other.frequency;
    }
};

struct SimilarityPair {
    int book1;
    int book2;
    double similarity;

    SimilarityPair(int b1, int b2, double sim) : book1(b1), book2(b2), similarity(sim) {}

    bool operator<(const SimilarityPair& other) const {
        return similarity < other.similarity;
    }
};

std::vector<std::string> stopWords = {"A", "AND", "AN", "OF", "IN", "THE"};

bool isStopWord(const std::string& word) {
    return std::find(stopWords.begin(), stopWords.end(), word) != stopWords.end();
}

std::string normalizeWord(const std::string& word) {
    std::string normalized;
    for (char c : word) {
        if (std::isalnum(c)) {
            normalized += std::toupper(c);
        }
    }
    return normalized;
}

std::vector<WordFreq> getTopWords(const std::string& filename) {
    std::ifstream file(filename);
    std::unordered_map<std::string, int> wordCount;
    int totalWords = 0;

    std::string word;
    while (file >> word) {
        word = normalizeWord(word);
        if (!word.empty() && !isStopWord(word)) {
            wordCount[word]++;
            totalWords++;
        }
    }

    std::vector<WordFreq> topWords;
    topWords.reserve(wordCount.size());
    for (const auto& pair : wordCount) {
        topWords.emplace_back(pair.first, static_cast<double>(pair.second) / totalWords);
    }

    std::partial_sort(topWords.begin(), topWords.begin() + std::min(TOP_WORDS, static_cast<int>(topWords.size())), topWords.end());
    if (topWords.size() > TOP_WORDS) {
        topWords.resize(TOP_WORDS);
    }

    return topWords;
}

double calculateSimilarity(const std::vector<WordFreq>& words1, const std::vector<WordFreq>& words2) {
    double similarity = 0.0;
    std::unordered_map<std::string, double> wordMap;

    for (const auto& word : words1) {
        wordMap[word.word] = word.frequency;
    }

    for (const auto& word : words2) {
        auto it = wordMap.find(word.word);
        if (it != wordMap.end()) {
            similarity += std::min(it->second, word.frequency);
        }
    }

    return similarity;
}

int main() {
    std::vector<std::vector<WordFreq>> bookWords;
    std::vector<std::string> bookNames;

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir("books")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            std::string filename = ent->d_name;
            if (filename.length() > 4 && filename.substr(filename.length() - 4) == ".txt") {
                bookNames.push_back(filename);
                bookWords.push_back(getTopWords("books/" + filename));
            }
        }
        closedir(dir);
    } else {
        std::cerr << "Could not open directory" << std::endl;
        return 1;
    }

    int numBooks = bookNames.size();
    std::vector<std::vector<double>> similarityMatrix(numBooks, std::vector<double>(numBooks, 0.0));
    std::priority_queue<SimilarityPair> topPairs;

    for (int i = 0; i < numBooks; ++i) {
        for (int j = i + 1; j < numBooks; ++j) {
            double similarity = calculateSimilarity(bookWords[i], bookWords[j]);
            similarityMatrix[i][j] = similarityMatrix[j][i] = similarity;

            topPairs.emplace(i, j, similarity);
            if (topPairs.size() > TOP_PAIRS) {
                topPairs.pop();
            }
        }
    }

    std::cout << "Top " << TOP_PAIRS << " similar pairs of text books:" << std::endl;
    std::vector<SimilarityPair> results;
    while (!topPairs.empty()) {
        results.push_back(topPairs.top());
        topPairs.pop();
    }

    for (auto it = results.rbegin(); it != results.rend(); ++it) {
        std::cout << bookNames[it->book1] << " - " << bookNames[it->book2] << ": " << it->similarity << std::endl;
    }

return 0;
}