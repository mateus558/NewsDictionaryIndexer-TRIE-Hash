#include "Dictionary.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

Dictionary::Dictionary(){ 
    char_translator[(unsigned char)'á'] = 'a';
    char_translator[(unsigned char)'Á'] = 'a';
    char_translator[(unsigned char)'à'] = 'a';
    char_translator[(unsigned char)'À'] = 'a';
    char_translator[(unsigned char)'ã'] = 'a';
    char_translator[(unsigned char)'Ã'] = 'a';
    char_translator[(unsigned char)'â'] = 'a';
    char_translator[(unsigned char)'Â'] = 'a';
    char_translator[(unsigned char)'é'] = 'e';
    char_translator[(unsigned char)'É'] = 'e';
    char_translator[(unsigned char)'è'] = 'e';
    char_translator[(unsigned char)'È'] = 'e';
    char_translator[(unsigned char)'ê'] = 'e';
    char_translator[(unsigned char)'Ê'] = 'e';
    char_translator[(unsigned char)'ẽ'] = 'e';
    char_translator[(unsigned char)'Ẽ'] = 'e';
    char_translator[(unsigned char)'í'] = 'i';
    char_translator[(unsigned char)'Í'] = 'i';
    char_translator[(unsigned char)'ì'] = 'i';
    char_translator[(unsigned char)'Ì'] = 'i';
    char_translator[(unsigned char)'î'] = 'i';
    char_translator[(unsigned char)'Î'] = 'i';
    char_translator[(unsigned char)'ĩ'] = 'i';
    char_translator[(unsigned char)'Ĩ'] = 'i';
    char_translator[(unsigned char)'ó'] = 'o';
    char_translator[(unsigned char)'Ó'] = 'o';
    char_translator[(unsigned char)'ò'] = 'o';
    char_translator[(unsigned char)'Ò'] = 'o';
    char_translator[(unsigned char)'ô'] = 'o';
    char_translator[(unsigned char)'Ô'] = 'o';
    char_translator[(unsigned char)'õ'] = 'o';
    char_translator[(unsigned char)'Õ'] = 'o';
    char_translator[(unsigned char)'ú'] = 'u';
    char_translator[(unsigned char)'Ú'] = 'u';
    char_translator[(unsigned char)'ù'] = 'u';
    char_translator[(unsigned char)'Ù'] = 'u';
    char_translator[(unsigned char)'û'] = 'u';
    char_translator[(unsigned char)'Û'] = 'u';
    char_translator[(unsigned char)'ũ'] = 'u';
    char_translator[(unsigned char)'Ũ'] = 'u';
    char_translator[(unsigned char)'ç'] = 'c';
    char_translator[(unsigned char)'Ç'] = 'c';
    char_translator[(unsigned char)'ñ'] = 'n';
    char_translator[(unsigned char)'Ñ'] = 'n';
}

void Dictionary::generatePlots(const std::string& strategy){
    gp << "set terminal svg" << std::endl;
    gp << "set output 'insertion_mem_plot_" << strategy << ".svg'" << std::endl;
    gp << "set xlabel 'Number of documents'" << std::endl;
    gp << "set ylabel 'Memory (Mb)'" << std::endl;
    gp << "plot" << gp.file1d(mem_measures) << "with lines title 'Virtual memory'" << std::endl;
 
    gp << "set output 'insertion_time_plot_" << strategy << ".svg'" << std::endl;
    gp << "set xlabel 'Number of documents'" << std::endl;
    gp << "set ylabel 'Time (ms)'" << std::endl;
    gp << "plot" << gp.file1d(time_measures) << "with lines title 'avg insertion time'" << std::endl;
    
    mem_measures.clear();
    time_measures.clear();
}

bool Dictionary::isValidTerm(const std::string& term){
    if((term[0] >= '0' && term[0] <= '9') || (term.size() == 1 || term.empty())) return false;
    if(!term.empty() && std::find_if(term.begin(), term.end(), [](unsigned char c) { return !std::isdigit(c); }) == term.end()) return false;
    return true;
}

std::string Dictionary::processString(const std::string& str){
    std::string clean_str, nopunct;

    clean_str.resize(str.size());
    std::transform(str.begin(),str.end(), clean_str.begin(), [this](unsigned char c){
        if(c <= 122 && c >= 97) return (char)c;
        if(c >= 65 && c <= 90) return (char)(c+32);
        if(c >= 48 && c <= 57) return (char)c;
        if(c == '-') return ' ';
        if(std::ispunct(c)) return (char)'.';
        if(char_translator.find(c) != char_translator.end()){
            return (char)char_translator[c];
        }
        if(c == 195 || c == 197 || c == 225) return '.';
        if(c == 196) return 'i';
        return (char)' ';
    });
    clean_str.erase(std::remove(clean_str.begin(), clean_str.end(), '.'), clean_str.end());

    return clean_str;
}

std::vector<Document*> Dictionary::findByTerms(const std::string& terms, const size_t &n_results){
    std::string term, clean_terms = processString(terms);
    std::stringstream input_stringstream(clean_terms);
    std::vector<Document*> results(n_results, nullptr);
    heap = new DocumentHeap(n_results);

    size_t n_terms = 0;
    size_t n_comparisons = 0;
    while(std::getline(input_stringstream, term, ' ')){
        if(!this->isValidTerm(term)) continue;
        n_terms++;
        if(n_terms > 2){
            std::cerr << "Error: too many terms for search. [Only 2 allowed]" << std::endl;
            return std::vector<Document*>();
        }
        auto search_result = this->find(term);
        n_comparisons += this->numberComparisons();
        if(!search_result) continue;
        for(auto it = search_result->docs_counts.begin(); it != search_result->docs_counts.end(); it++){
            auto id = (*it).first->id;
            if(!documents[id]->unique_terms) continue;
            double rank = ((double)1.0/documents[id]->unique_terms)*search_result->weight_i[id];
            documents[id]->rank += rank;
            if(rank != 0){
                heap->insertOrUpdate(documents[id]);
            }
        }
    }
    comparisons = n_comparisons;

    size_t i = 0;
    while(!heap->empty()){
        results[i] = heap->pop();
        i++;
    }

    delete heap;
    heap = nullptr;

    if(i < n_results){
        size_t k = 0;
        for(size_t j = i; j < results.size(); j++){
            while((k < documents.size()) && (documents[k]->rank > 0)){  k++; }
            results[j] = documents[k];
            k++;
        }
    }
    return results;
}

bool Dictionary::insert(const json& document){
    std::string term, input = document["headline"];
    Document* doc_info = new Document(document["category"], document["headline"], document["authors"], document["link"], 
                            document["short_description"], document["date"]);
    
    input += ' ';
    input += document["short_description"];
    doc_info->id = documents.size();
    documents.push_back(doc_info);
    
    std::string clean_input = processString(input);
    std::stringstream input_stringstream(clean_input);
    while(std::getline(input_stringstream, term, ' ')){
        if(!isValidTerm(term)) continue;
        if(!insert(term, doc_info)) return false;
    }
    return true;
}

bool Dictionary::insert(const std::string& path){
    std::ifstream file(path);
    if(file.is_open()){
        std::string raw_doc;
        duration<double> time_elapsed;
        
        while(std::getline(file, raw_doc)){
            if(documents.size() % 20000 == 0){
                time_measures.push_back(std::make_pair(documents.size(), (time_elapsed.count() * 1E3)/20000));
                double vm, rss;
                process_mem_usage(vm, rss);
                mem_measures.push_back(std::make_pair(documents.size(), vm/1024));
            }
            if(verbose){
                std::clog << "Parsing document " << documents.size()+1 << std::endl;
            }
            bool status;
            auto start = high_resolution_clock::now();
            status = insert(json::parse(raw_doc));
            auto end = high_resolution_clock::now();
            time_elapsed += (end-start);
            if(!status){ 
                std::cerr << "Error inserting document " << documents.size()+1 << ". Error code: " << status << std::endl;
                return false;
            }
        }
    }else{
        std::cerr << "The file could not be opened." << std::endl;
        return false;
    }
    file.close();

    if(verbose){
            std::cout << "Number of unique terms: " << distinct_terms << std::endl;
    }
    computeTermsParameters();
    return true;
}