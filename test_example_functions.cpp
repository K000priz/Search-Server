#include"test_example_functions.h"

void PrintDocument(const Document& document) {
    std::cout << "{ "
        << "document_id = " << document.id << ", "
        << "relevance = " << document.relevance << ", "
        << "rating = " << document.rating << " }" << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view> words, DocumentStatus status) {
    std::cout << "{ "
        << "document_id = " << document_id << ", "
        << "status = " << static_cast<int>(status) << ", "
        << "words =";
    for (std::string_view word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}" << std::endl;
}

void AddDocument(SearchServer& search_server, int document_id, std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Error adding document " << document_id << ": " << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, std::string_view raw_query) {
    LOG_DURATION_STREAM("Operation time", std::cout);
    std::cout << "Search results on demand: " << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Search error: " << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, std::string_view query) {
    LOG_DURATION_STREAM("Operation time", std::cout);
    try {
        std::cout << "Document matching on request: " << query << std::endl;
        for (const int document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    }
    catch (const std::invalid_argument& e) {
        std::cout << "Error matching documents to query " << query << ": " << e.what() << std::endl;
    }
}
