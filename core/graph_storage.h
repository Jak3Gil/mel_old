/**
 * graph_storage.h
 * 
 * Unified graph storage structure for MELVIN
 * 
 * This defines the standard graph interface that all components use.
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <fstream>
#include <iostream>

struct Node {
    int id;
    std::vector<float> embedding;
};

class GraphStorage {
public:
    // Data structures
    std::unordered_map<int, Node> nodes;
    std::unordered_map<int, std::vector<std::pair<int, float>>> edges;
    std::unordered_map<std::string, int> token_to_id;
    std::unordered_map<int, std::string> id_to_token;
    
    // Get embeddings map
    std::unordered_map<int, std::vector<float>> get_embeddings() const {
        std::unordered_map<int, std::vector<float>> embeddings;
        for (const auto& [id, node] : nodes) {
            embeddings[id] = node.embedding;
        }
        return embeddings;
    }
    
    // Load from disk
    bool load(const std::string& data_dir = "data") {
        std::cout << "📂 Loading unified graph from " << data_dir << "..." << std::endl;
        
        // Load nodes
        std::ifstream nodes_file(data_dir + "/unified_nodes.bin", std::ios::binary);
        if (!nodes_file.is_open()) {
            std::cerr << "❌ Failed to load nodes!" << std::endl;
            return false;
        }
        
        size_t num_nodes;
        nodes_file.read(reinterpret_cast<char*>(&num_nodes), sizeof(size_t));
        
        for (size_t i = 0; i < num_nodes; i++) {
            int node_id;
            size_t emb_size;
            
            nodes_file.read(reinterpret_cast<char*>(&node_id), sizeof(int));
            nodes_file.read(reinterpret_cast<char*>(&emb_size), sizeof(size_t));
            
            std::vector<float> embedding(emb_size);
            nodes_file.read(reinterpret_cast<char*>(embedding.data()), emb_size * sizeof(float));
            
            Node node;
            node.id = node_id;
            node.embedding = embedding;
            nodes[node_id] = node;
        }
        
        nodes_file.close();
        
        // Load token map
        std::ifstream token_file(data_dir + "/token_map.bin", std::ios::binary);
        if (token_file.is_open()) {
            size_t num_tokens;
            token_file.read(reinterpret_cast<char*>(&num_tokens), sizeof(size_t));
            
            for (size_t i = 0; i < num_tokens; i++) {
                size_t token_len;
                token_file.read(reinterpret_cast<char*>(&token_len), sizeof(size_t));
                
                std::string token(token_len, '\0');
                token_file.read(&token[0], token_len);
                
                int node_id;
                token_file.read(reinterpret_cast<char*>(&node_id), sizeof(int));
                
                token_to_id[token] = node_id;
                id_to_token[node_id] = token;
            }
            
            token_file.close();
        }
        
        // Load edges
        std::ifstream edges_file(data_dir + "/unified_edges.bin", std::ios::binary);
        if (edges_file.is_open()) {
            size_t num_edges;
            edges_file.read(reinterpret_cast<char*>(&num_edges), sizeof(size_t));
            
            for (size_t i = 0; i < num_edges; i++) {
                int src, dst;
                float weight;
                
                edges_file.read(reinterpret_cast<char*>(&src), sizeof(int));
                edges_file.read(reinterpret_cast<char*>(&dst), sizeof(int));
                edges_file.read(reinterpret_cast<char*>(&weight), sizeof(float));
                
                edges[src].push_back({dst, weight});
            }
            
            edges_file.close();
        }
        
        std::cout << "✅ Loaded: " << nodes.size() << " nodes, " 
                  << edges.size() << " edge lists" << std::endl;
        
        return true;
    }
};

