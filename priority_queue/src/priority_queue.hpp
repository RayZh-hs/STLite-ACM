#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {
    /**
     * @brief a container like std::priority_queue which is a heap internal.
     * **Exception Safety**: The `Compare` operation might throw exceptions for certain data.
     * In such cases, any ongoing operation should be terminated, and the priority queue should be restored to its original state before the operation began.
     */
    template<typename T, class Compare = std::less<T> >
    class priority_queue {
    private:
        struct pq_node {
            T val;
            pq_node *child = nullptr, *sibling = nullptr;

            pq_node(T val, pq_node *child, pq_node *sibling)
                : val(val), child(child), sibling(sibling) {
            }

            // Duplicates everything at (right) and below the level of the node, INCLUDING SELF
            pq_node *recursive_dup() const {
                auto *new_self_ptr = new pq_node(val, nullptr, nullptr);
                if (child != nullptr)
                    new_self_ptr->child = child->recursive_dup();
                if (sibling != nullptr)
                    new_self_ptr->sibling = sibling->recursive_dup();
                return new_self_ptr;
            }

            // Recursively deletes everything that at (right) and below the level of the node, EXCLUDING SELF.
            void recursive_del() {
                if (child != nullptr) {
                    child->recursive_del();
                    delete child;
                }
                if (sibling != nullptr) {
                    sibling->recursive_del();
                    delete sibling;
                }
            }
        };

        // meld two nodes together
        static pq_node *meld_nodes(pq_node *node_a, pq_node *node_b) {
            if (node_a == nullptr) return node_b;
            if (node_b == nullptr) return node_a;
            try {
                if (Compare()(node_a->val, node_b->val)) {
                    // node_b should come first
                    node_a->sibling = node_b->child;
                    node_b->child = node_a;
                    return node_b;
                } else {
                    node_b->sibling = node_a->child;
                    node_a->child = node_b;
                    return node_a;
                }
            } catch (...) {
                throw runtime_error();
            }
        }

        // iterate through a node's siblings and compare them in advance
        // used to capture exceptions before conducting merges
        static void compare_siblings(pq_node *begin) {
            try {
                if (begin == nullptr)
                    return;
                while(begin->sibling != nullptr) {
                    Compare()(begin->val, begin->sibling->val);   // possibly throwing errors here
                    begin = begin->sibling;
                }
            }
            catch (...) {
                throw runtime_error();
            }
        }

        // merge all siblings of a node, starting from begin
        static pq_node *merge_siblings(pq_node *begin) {
            if (begin == nullptr || begin->sibling == nullptr)
                return begin;
            auto begin_sib = begin->sibling;
            auto begin_sib_sib = begin->sibling->sibling;
            begin->sibling = nullptr;
            begin_sib->sibling = nullptr;
            const auto self_and_sib = meld_nodes(begin, begin_sib);
            const auto sib_of_sib = merge_siblings(begin_sib_sib);
            return meld_nodes(self_and_sib, sib_of_sib);
        }

        pq_node *root = nullptr;
        size_t raw_size_ = 0;

    public:
        /**
         * @brief default constructor
         */
        priority_queue() = default;

        /**
         * @brief copy constructor
         * @param other the priority_queue to be copied
         */
        priority_queue(const priority_queue &other) {
            if (root == other.root)
                return;
            // First delete self.root
            if (root != nullptr) {
                root->recursive_del();
                delete root;
            }
            root = other.root->recursive_dup();
            raw_size_ = other.raw_size_;
        }

        /**
         * @brief deconstructor
         */
        ~priority_queue() {
            if (root != nullptr) {
                root->recursive_del();
                delete root;
            }
        }

        /**
         * @brief Assignment operator
         * @param other the priority_queue to be assigned from
         * @return a reference to this priority_queue after assignment
         */
        priority_queue &operator=(const priority_queue &other) {
            if (root == other.root)
                return *this;
            // First delete self.root
            if (root != nullptr) {
                root->recursive_del();
                delete root;
            }
            root = other.root->recursive_dup();
            raw_size_ = other.raw_size_;
            return *this;
        }

        /**
         * @brief get the top element of the priority queue.
         * @return a reference of the top element.
         * @throws container_is_empty if empty() returns true
         */
        const T &top() const {
            if (empty()) {
                throw container_is_empty();
            }
            return root->val;
        }

        /**
         * @brief push new element to the priority queue.
         * @param e the element to be pushed
         */
        void push(const T &e) {
            pq_node *new_node_ptr = new pq_node(e, nullptr, nullptr);
            try {
                root = meld_nodes(root, new_node_ptr);
                ++raw_size_;   // this must come after med_nodes in case of an exception
            }
            catch (runtime_error&) {
                delete new_node_ptr;    // sanitise on error
                throw runtime_error();
            }
        }

        /**
         * @brief delete the top element from the priority queue.
         * @throws container_is_empty if empty() returns true
         */
        void pop() {
            if (root == nullptr) {
                throw container_is_empty();
            }
            compare_siblings(root->child);
            // After this, supposedly there should be no exceptions!
            const auto new_root = merge_siblings(root->child);
            delete root;
            root = new_root;
            --raw_size_;
        }

        /**
         * @brief return the number of elements in the priority queue.
         * @return the number of elements.
         */
        size_t size() const {
            return raw_size_;
        }

        /**
         * @brief check if the container is empty.
         * @return true if it is empty, false otherwise.
         */
        bool empty() const {
            return root == nullptr;
        }

        /**
         * @brief merge another priority_queue into this one.
         * The other priority_queue will be cleared after merging.
         * The complexity is at most O(logn).
         * @param other the priority_queue to be merged.
         */
        void merge(priority_queue &other) {
            root = meld_nodes(root, other.root);
            other.root = nullptr;
            raw_size_ += other.raw_size_;   // this must come after med_nodes in case of an exception
            other.raw_size_ = 0;
        }
    };
}

#endif
