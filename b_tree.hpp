#ifndef __B_TREE_HPP__
#define __B_TREE_HPP__

#include <algorithm>
#include <cmath>
#include <fstream>

using size_t = decltype(sizeof(void*));

// Add given value to given arr.
// After function calling, length of arr is now_len + 1.
template<typename T>
void add_to_arr(T* arr, size_t now_len, T value, size_t idx)
{
    for(int i = now_len; i > idx; --i) {
        arr[i] = arr[i-1];
    }
    arr[idx] = value;
}

// Delete given arr's idx-th element, and return it.
// After function calling, length of arr is now_len - 1
template<typename T>
T delete_from_arr(T* arr, size_t now_len, size_t idx)
{
    T result = arr[idx];
    for(size_t i = idx; i < now_len - 1; ++i) {
        arr[i] = arr[i+1];
    }
    return result;
}

// typename T is type of key. int order is order of b-tree.
template<typename T, size_t order>
class B_node {
    using this_type = B_node<T, order>;

private:
    T* keys;
    this_type** children;
    size_t num_of_keys;

public:
    B_node() : keys(new T[order]), children(new this_type*[order + 1]), num_of_keys(0)
    {
        std::fill(children, children + order + 1, nullptr);
    }

    ~B_node()
    {
        delete[] keys;
        for(size_t i = 0; i <= num_of_keys; ++i) {
            delete children[i];
        }
        delete[] children;
    }

    static this_type* insert(this_type* root, T key)
    {
        bool last_overflow = root->insert_internal(key);
        if(!last_overflow) return root;
        this_type* new_root = new this_type();
        this_type* new_right_subtree = root->node_separation();
        new_root->children[0] = root;
        new_root->children[1] = new_right_subtree;
        new_root->keys[new_root->num_of_keys++] = root->keys[--root->num_of_keys];

        return new_root;
    }

    static void print_inorder(this_type* root, std::ofstream& output)
    {
        if(root == nullptr) return;
        for(size_t i = 0; i < root->num_of_keys; ++i) {
            print_inorder(root->children[i], output);
            output << root->keys[i] << '(' << i << ") ";
        }
        print_inorder(root->children[root->num_of_keys], output);
    }

private:

    // test rotatable for insertion from this node's src child to another child
    /* return rotatable test result that
        < 0 : can rotate with a left sibling.
        == 0 : can't rotate with any siblings.
        > 0 : can rotate with a right sibling.
        magnitude of return value is diffence between src index and dst index.
    */ 
    int insert_rotatable(size_t src) const
    {
        int result = 0;
        int dst;
        // test the left siblings
        for(dst = src - 1; dst >= 0; --dst) {
            if(children[dst]->num_of_keys < order - 1) {
                result = dst - src;
                break;
            }
        }
        // test the right siblings
        for(dst = src + 1; dst <= num_of_keys && (result == 0 || dst - src < result); ++dst) {
            if(children[dst]->num_of_keys < order - 1) {
                result = dst - src;
                break;
            }
        }
        return result;
    }

    // Function for rotation in insertion process.
    // Rotate from this node's src-th child to (src - difference)-th child.
    // Premise that all nodes between src-th child and dst-th child are full.
    void insert_rotate_with_left(size_t src, int difference)
    {
        int now_idx;
        int dst = src - difference;
        T key_temp;
        this_type* subtree_temp;

        for(now_idx = src; now_idx != dst; --now_idx) {
            key_temp = delete_from_arr(children[now_idx]->keys, children[now_idx]->num_of_keys, 0);
            subtree_temp = delete_from_arr(children[now_idx]->children, children[now_idx]->num_of_keys, 0);
            children[now_idx]->num_of_keys -= 1;

            std::swap(key_temp, keys[now_idx - 1]);

            children[now_idx - 1]->keys[children[now_idx - 1]->num_of_keys] = key_temp;
            children[now_idx - 1]->children[children[now_idx - 1]->num_of_keys + 1] = subtree_temp;
            children[now_idx - 1]->num_of_keys += 1;
        }
    }

    // Function for rotation in insertion process.
    // Rotate from this node's src-th child to (src + difference)-th child.
    // Premise that all nodes between src-th child and dst-th child are full.
    void insert_rotate_with_right(size_t src, int difference)
    {
        size_t now_idx;
        size_t dst = src + difference;
        T key_temp;
        this_type* subtree_temp;

        for(now_idx = src; now_idx != dst; ++now_idx) {
            key_temp = children[now_idx]->keys[children[now_idx]->num_of_keys - 1];
            subtree_temp = children[now_idx]->children[children[now_idx]->num_of_keys];
            children[now_idx]->num_of_keys -= 1;

            std::swap(key_temp, keys[now_idx]);

            add_to_arr(children[now_idx + 1]->keys, children[now_idx + 1]->num_of_keys, key_temp, 0);
            add_to_arr(children[now_idx + 1]->children, children[now_idx + 1]->num_of_keys + 1, subtree_temp, 0);
            children[now_idx + 1]->num_of_keys += 1;
        }
    }

    // For node split, separate this node with new node. And return new node.
    this_type* node_separation()
    {
        this_type* new_node = new this_type();
        size_t this_num = static_cast<size_t>(std::ceil((order - 1) / 2.0));
        for(size_t i = 0; i < (order - 1) / 2; ++i) {
            new_node->keys[i] = keys[i + 1 + this_num];
            new_node->children[i] = children[i + 1 + this_num];
        }
        new_node->children[(order - 1) / 2] = children[order];

        this->num_of_keys -= (order - 1) / 2;
        new_node->num_of_keys = (order - 1) / 2;

        return new_node;
    }

    // By calling node_separation func, perform node split.
    void node_split(size_t idx)
    {
        this_type* new_node = children[idx]->node_separation();
        add_to_arr(keys, num_of_keys, children[idx]->keys[--children[idx]->num_of_keys], idx);
        add_to_arr(children, num_of_keys + 1, new_node, idx + 1);
        num_of_keys += 1;
    }

    // Perform insert given value to b-tree recursively. And return if overflow occurs or not.
    bool insert_internal(T value)
    {
        size_t i;
        for(i = 0; i < num_of_keys && keys[i] < value; ++i);
        if(i < num_of_keys && keys[i] == value) {
            return false;
        }
        
        //if this node is leaf-node
        if(children[0] == nullptr) {
            add_to_arr(keys, num_of_keys++, value, i);
            return num_of_keys == order;
        }
        
        bool is_overflowed = children[i]->insert_internal(value);

        // if overflow occurred in child node
        if(is_overflowed) {
            int rotatable = insert_rotatable(i);

            if(rotatable < 0) {
                insert_rotate_with_left(i, -rotatable);
                return false;
            }
            if(rotatable > 0) {
                insert_rotate_with_right(i, rotatable);
                return false;
            }
            node_split(i);
            return num_of_keys == order;
        } else { // overflow didn't occur.
            return false;
        }
    }
};

#endif