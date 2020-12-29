#ifndef AQUILA_NODE_HPP
#define AQUILA_NODE_HPP

#include "aq_mesh.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

namespace aq {

    class Node {
    public:
        class Iterator;
        class HierarchyIterator;

        Node(glm::vec3 position=glm::vec3(0.0f), 
            glm::quat rotation=glm::quat(1.0f,0.0f,0.0f,0.0f), 
            glm::vec3 scale=glm::vec3(1.0f));
        virtual ~Node();

        virtual void add_node(std::shared_ptr<Node> node);
        virtual void remove_node(Node* node);

        virtual void add_mesh(std::shared_ptr<Mesh> mesh);

        virtual glm::mat4 get_model_matrix();
    
    // private:
        std::vector<std::shared_ptr<Mesh>> child_meshes;
        std::vector<std::shared_ptr<Node>> child_nodes;

        glm::vec3 position;
        glm::quat rotation;
        glm::vec3 scale;
        
    };

    class Node::Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::shared_ptr<Node>;
        using pointer           = value_type*;
        using reference         = value_type&;

        reference operator*() const { return *ptr; }
        pointer operator->() { return ptr; }

        // Prefix increment
        Iterator& operator++() {
            if (current_index >= (*ptr)->child_nodes.size()) {
                if (parents.empty()) {
                    ptr = nullptr;
                }
                else {
                    std::tie(ptr, current_index) = parents.back();
                    parents.pop_back();
                    ++(*this);
                }
            } else {
                parents.push_back(std::pair<pointer, size_t>(ptr, current_index+1));
                ptr = &(*ptr)->child_nodes[current_index];
                current_index = 0;
            }

            return *this;
        }

        // Postfix increment
        Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator== (const Iterator& a, const Iterator& b) { return a.ptr == b.ptr; };
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a.ptr != b.ptr; };

    private:
        Iterator(pointer ptr) : ptr(ptr) {} // Use `ptr = nullptr` for end iterator

        pointer ptr = nullptr;
        size_t current_index = 0;
        std::vector<std::pair<pointer, size_t>> parents;
    
    friend class Node;
    friend Node::Iterator begin(std::shared_ptr<Node>&);
    friend Node::Iterator end(std::shared_ptr<Node>&);
    };

    inline Node::Iterator begin(std::shared_ptr<Node>& node) {
        return Node::Iterator(&node);
    }

    inline Node::Iterator end(std::shared_ptr<Node>& node) {
        return Node::Iterator(nullptr);
    }


    class Node::HierarchyIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = std::shared_ptr<Node>;
        using pointer           = value_type*;
        using reference         = value_type&;

        reference operator*() const { return *ptr; }
        pointer operator->() { return ptr; }

        // Prefix increment
        HierarchyIterator& operator++() {
            if (current_index >= (*ptr)->child_nodes.size()) {
                if (parents.empty()) {
                    ptr = nullptr;
                }
                else {
                    std::tie(ptr, current_index, current_transform) = parents.back();
                    parents.pop_back();
                    ++(*this);
                }
            } else {
                parents.push_back(std::tuple<pointer, size_t, glm::mat4>(ptr, current_index+1, current_transform));
                ptr = &(*ptr)->child_nodes[current_index];
                current_index = 0;
                current_transform = current_transform * (*ptr)->get_model_matrix();
            }

            return *this;
        }

        // Postfix increment
        HierarchyIterator operator++(int) { HierarchyIterator tmp = *this; ++(*this); return tmp; }

        glm::mat4 get_transform() { return current_transform; }

        friend bool operator== (const HierarchyIterator& a, const HierarchyIterator& b) { return a.ptr == b.ptr; };
        friend bool operator!= (const HierarchyIterator& a, const HierarchyIterator& b) { return a.ptr != b.ptr; };

    private:
        // Use `ptr = nullptr` for end iterator
        HierarchyIterator(pointer ptr) : ptr(ptr) {
            if (ptr)
                current_transform = (*ptr)->get_model_matrix();
        } 

        pointer ptr = nullptr;
        size_t current_index = 0;
        glm::mat4 current_transform;
        std::vector<std::tuple<pointer, size_t, glm::mat4>> parents;
    
    friend class Node;
    friend Node::HierarchyIterator hbegin(std::shared_ptr<Node>&);
    friend Node::HierarchyIterator hend(std::shared_ptr<Node>&);
    };

    // Iterate over node hierarchy; returns both the node and its model
    // matrix calculated from its parents
    inline Node::HierarchyIterator hbegin(std::shared_ptr<Node>& node) {
        return Node::HierarchyIterator(&node);
    }

    inline Node::HierarchyIterator hend(std::shared_ptr<Node>& node) {
        return Node::HierarchyIterator(nullptr);
    }



}

#endif