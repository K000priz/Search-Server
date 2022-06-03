#include "search_server.h"
#include <iostream>
#include <cmath>
#include <numeric>

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}
//-----------------------------------------------AddDocument----------------------------------------------------------------------------
void SearchServer::AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw std::invalid_argument("Invalid document_id");
    }
    const auto words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view word : words) {
        word_to_document_freqs_[std::string( word )][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}
//---------------------------------------------FindTopDocuments--------------------------------------------------------------------------
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::par, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
        });
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::sequenced_policy&, std::string_view raw_query) const {
    return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}
std::vector<Document> SearchServer::FindTopDocuments(const std::execution::parallel_policy&, std::string_view raw_query) const {
    return FindTopDocuments(std::execution::par, raw_query, DocumentStatus::ACTUAL);
}
//---------------------------------------------GetDocumentCount--------------------------------------------------------------------------
int SearchServer::GetDocumentCount() const {
    return documents_.size();
}
//---------------------------------------------GetWordFrequencies--------------------------------------------------------------------------
const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (document_to_word_freqs_.count(document_id)) {
        return document_to_word_freqs_.at(document_id);
    }
    else {
        static std::map<std::string_view, double> MapFrequencies;
        return MapFrequencies;
    }
}
//---------------------------------------------RemoveDocument--------------------------------------------------------------------------
void SearchServer::RemoveDocument(int document_id) {
    if (document_ids_.find(document_id) == document_ids_.end()) {
        return;
    }
    std::string word_remove;
    for (auto& [word, id_freq] : word_to_document_freqs_) {
        if (id_freq.find(document_id) != id_freq.end()) {
            word_remove = word;
        }
    }
    word_to_document_freqs_.erase(word_remove);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}
//---------------------------------------------MatchDocument--------------------------------------------------------------------------
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
   return MatchDocument(std::execution::seq, raw_query, document_id);
}
//---------------------------------------------MatchDocument--------------------------------------------------------------------------
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const {
    if (document_to_word_freqs_.count(document_id) == 0) {
        throw std::out_of_range("There is no such id");
    }
    const auto query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;
    if (std::any_of(std::execution::par,
        query.minus_words.begin(),
        query.minus_words.end(),
        [&](const std::string_view word) { return word_to_document_freqs_.at(std::string(word)).count(document_id); }
    )) {
        return { matched_words, documents_.at(document_id).status };
    }
    std::copy_if(std::execution::par,
        query.plus_words.begin(),
        query.plus_words.end(),
        std::back_inserter(matched_words),
        [&](const std::string_view word) { return word_to_document_freqs_.at(std::string(word)).count(document_id); }
    );
    matched_words.shrink_to_fit();
    std::sort(matched_words.begin(), matched_words.end());
    auto pos = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(pos, matched_words.end());
    return { matched_words, documents_.at(document_id).status };
}
//---------------------------------------------MatchDocument--------------------------------------------------------------------------
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const {
    if (document_to_word_freqs_.count(document_id) == 0) {
        throw std::out_of_range("There is no such id");
    }
    const auto query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;
    if (std::any_of(std::execution::seq,
        query.minus_words.begin(),
        query.minus_words.end(),
        [&](const std::string_view& word) { return word_to_document_freqs_.at(std::string( word )).count(document_id); }
    )) {
        return { matched_words, documents_.at(document_id).status };
    }
    std::copy_if(std::execution::seq,
        query.plus_words.begin(),
        query.plus_words.end(),
        std::back_inserter(matched_words),
        [&](const std::string_view& word) { return word_to_document_freqs_.at(std::string( word )).count(document_id); }
    );
    return { matched_words, documents_.at(document_id).status };
}

//---------------------------------------------IsStopWord--------------------------------------------------------------------------
bool SearchServer::IsStopWord(std::string_view word) const {
    return stop_words_.count(word) > 0;
}
//---------------------------------------------IsValidWord--------------------------------------------------------------------------
bool SearchServer::IsValidWord(std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}
//---------------------------------------------SplitIntoWordsNoStop--------------------------------------------------------------------------
std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word is invalid");
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}
//---------------------------------------------ComputeAverageRating--------------------------------------------------------------------------
int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}
//---------------------------------------------ParseQueryWord--------------------------------------------------------------------------
SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty");
    }
    std::string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word is invalid");
    }
    return { word, is_minus, IsStopWord(word) };
}
//---------------------------------------------ParseQuery--------------------------------------------------------------------------
SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
    Query result;
    for (const std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            }
            else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}
//---------------------------------------------ComputeWordInverseDocumentFreq--------------------------------------------------------------------------
double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(std::string(word)).size());//{}
}
