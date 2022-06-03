#include<algorithm>
#include<execution>
#include<list>
#include"request_queue.h"

using namespace std;

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result_(queries.size());
    transform(execution::par, queries.begin(), queries.end(), result_.begin(),
        [&search_server](const string& c) { return search_server.FindTopDocuments(c); });
    return result_;
}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries){
    vector<std::vector<Document>> result_big = move(ProcessQueries(search_server, queries));
    list<Document> result_;
    for (vector<Document> tmp : result_big){
        for (const Document& s : tmp){
            result_.push_back(s);
        }
    }
    return result_;
}
