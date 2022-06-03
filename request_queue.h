#pragma once

#include"search_server.h"
#include "document.h"

#include <string>
#include <vector>
#include <algorithm>
#include <deque>
class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(std::string_view  raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(std::string_view  raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(std::string_view  raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::string_view query;
        std::vector<Document> doc;
    };
    std::deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    const SearchServer& search_server_;
    int time = 0;
};

template <typename DocumentPredicate>
std::vector<Document>RequestQueue::AddFindRequest(std::string_view  raw_query, DocumentPredicate document_predicate){
    std::vector<Document> DOC = search_server_.FindTopDocuments(raw_query, document_predicate);
    QueryResult query_res;
    if (DOC.empty()){
        time += 1;
        query_res.query = raw_query;
        requests_.push_back(query_res);
    }
    else{
        time += 1;
        query_res.query = raw_query;
        query_res.doc = DOC;
        requests_.push_back(query_res);
    }
    if (time > sec_in_day_){
        requests_.pop_front();
        --time;
    }
    return DOC;
}
