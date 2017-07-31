// (c) 2017 Blai Bonet

#ifndef NODE_H
#define NODE_H

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <ale_interface.hpp>

class Node;
inline void remove_tree(Node *node);

class Node {
  public:
    bool visited_;                           // label
    bool solved_;                            // label
    Node *parent_;                           // pointer to parent node
    Action action_;                          // action mapping parent to this node
    size_t depth_;                           // node's depth
    size_t height_;                          // node's height (calculated)
    float reward_;                           // reward for this node
    bool is_info_valid_;                     // is info valid?
    bool terminal_;                          // is node a terminal node?
    float value_;                            // backed up value
    int ale_lives_;                          // remaining ALE lives
    std::vector<Node*> children_;            // children

    mutable ALEState *state_;                // state for this node
    mutable std::vector<int> feature_atoms_; // features made true by this node
    mutable int frame_rep_;                  // frame counter for number identical feature atoms through ancestors

    Node(Node *parent, Action action, size_t depth)
      : visited_(false),
        solved_(false),
        parent_(parent),
        action_(action),
        depth_(depth),
        height_(0),
        reward_(0),
        is_info_valid_(false),
        terminal_(false),
        value_(0),
        ale_lives_(-1),
        state_(nullptr),
        frame_rep_(0) {
    }

    void remove_children() {
        while( !children_.empty() ) {
            remove_tree(children_.back());
            children_.pop_back();
        }
    }

    void expand(Action action) {
        children_.push_back(new Node(this, action, 1 + depth_));
    }
    void expand(const ActionVect &actions) {
        assert(children_.empty());
        for( size_t k = 0; k < actions.size(); ++k )
            expand(actions[k]);
        assert(children_.size() == actions.size());
    }

    Node* advance(Action action) {
        assert((parent_ == nullptr) || (parent_->parent_ == nullptr));
        if( parent_ != nullptr ) {
            delete parent_;
            parent_ = nullptr;
        }

        Node *child = nullptr;
        for( size_t k = 0; k < children_.size(); ++k ) {
            if( children_[k]->action_ == action )
                child = children_[k];
            else
                remove_tree(children_[k]);
        }
        assert(child != nullptr);

        children_.clear();
        children_.push_back(child);
        return child;
    }

    void clear_solved_labels() {
        solved_ = false;
        for( size_t k = 0; k < children_.size(); ++k )
            children_[k]->clear_solved_labels();
    }

    void normalize_depth(int depth = 0) {
        depth_ = depth;
        for( size_t k = 0; k < children_.size(); ++k )
            children_[k]->normalize_depth(1 + depth);
    }

    void solve_and_backpropagate_label() {
        assert(!solved_);
        if( !solved_ ) {
            solved_ = true;
            if( parent_ != nullptr ) {
                assert(!parent_->solved_);
                bool unsolved_siblings = false;
                for( size_t k = 0; k < parent_->children_.size(); ++k ) {
                    if( !parent_->children_[k]->solved_ ) {
                        unsolved_siblings = true;
                        break;
                    }
                }
                if( !unsolved_siblings )
                    parent_->solve_and_backpropagate_label();
            }
        }
    }

    float backup_values(float discount) {
        assert(children_.empty() || is_info_valid_);
        value_ = reward_;
        if( !children_.empty() ) {
            float max_child_value = -std::numeric_limits<float>::infinity();
            for( size_t k = 0; k < children_.size(); ++k ) {
                float child_value = children_[k]->backup_values(discount);
                max_child_value = std::max(max_child_value, child_value);
            }
            value_ += discount * max_child_value;
        }
        return value_;
    }

    float backup_values_along_branch(const std::deque<Action> &branch, float discount, size_t index = 0) {
        //assert(is_info_valid_ && !children_.empty()); // tree now grows past branch
        if( index == branch.size() ) {
            backup_values(discount);
            return value_;
        } else {
            assert(index < branch.size());
            float value_along_branch = 0;
            const Action &action = branch[index];
            float max_child_value = -std::numeric_limits<float>::infinity();
            for( size_t k = 0; k < children_.size(); ++k ) {
                if( children_[k]->action_ == action )
                    value_along_branch = children_[k]->backup_values_along_branch(branch, discount, ++index);
                max_child_value = std::max(max_child_value, children_[k]->value_);
            }
            value_ = reward_ + discount * max_child_value;
            return reward_ + discount * value_along_branch;
        }
    }

    float qvalue(const Node &child, float discount) const {
        return reward_ + discount * child.value_;
    }

    const Node *best_tip_node(float discount) const {
        if( children_.empty() ) {
            return this;
        } else {
            size_t num_best_children = 0;
            for( size_t k = 0; k < children_.size(); ++k ) {
                const Node &child = *children_[k];
                num_best_children += qvalue(child, discount) == value_;
            }
            assert(num_best_children > 0);
            size_t index_best_child = lrand48() % num_best_children;
            for( size_t k = 0; k < children_.size(); ++k ) {
                const Node &child = *children_[k];
                if( qvalue(child, discount) == value_ ) {
                    if( index_best_child == 0 )
                        return child.best_tip_node(discount);
                    --index_best_child;
                }
            }
            assert(0);
        }
    }

    void best_branch(std::deque<Action> &branch, float discount) const {
        if( !children_.empty() ) {
            size_t num_best_children = 0;
            for( size_t k = 0; k < children_.size(); ++k ) {
                const Node &child = *children_[k];
                num_best_children += qvalue(child, discount) == value_;
            }
            assert(num_best_children > 0);
            size_t index_best_child = lrand48() % num_best_children;
            for( size_t k = 0; k < children_.size(); ++k ) {
                const Node &child = *children_[k];
                if( qvalue(child, discount) == value_ ) {
                    if( index_best_child == 0 ) {
                        branch.push_back(child.action_);
                        child.best_branch(branch, discount);
                        break;
                    }
                    --index_best_child;
                }
            }
        }
    }

    void longest_zero_value_branch(std::deque<Action> &branch) const {
        if( !children_.empty() ) {
            size_t max_height = 0;
            size_t num_best_children = 0;
            for( size_t k = 0; k < children_.size(); ++k ) {
                const Node &child = *children_[k];
                if( (qvalue(child, 1) == 0) && (child.height_ >= max_height) ) {
                    if( child.height_ > max_height ) {
                        max_height = child.height_;
                        num_best_children = 0;
                    }
                    ++num_best_children;
                }
            }
            assert(num_best_children > 0);
            size_t index_best_child = lrand48() % num_best_children;
            for( size_t k = 0; k < children_.size(); ++k ) {
                const Node &child = *children_[k];
                if( (qvalue(child, 1) == 0) && (child.height_ >= max_height) ) {
                    if( index_best_child == 0 ) {
                        branch.push_back(child.action_);
                        child.longest_zero_value_branch(branch);
                        break;
                    }
                    --index_best_child;
                }
            }
        }
    }

    size_t num_tip_nodes() const {
        if( children_.empty() ) {
            return 1;
        } else {
            size_t n = 0;
            for( size_t k = 0; k < children_.size(); ++k )
                n += children_[k]->num_tip_nodes();
            return n;
        }
    }

    size_t num_nodes() const {
        size_t n = 1;
        for( size_t k = 0; k < children_.size(); ++k )
            n += children_[k]->num_nodes();
        return n;
    }

    size_t calculate_height() {
        height_ = 0;
        if( !children_.empty() ) {
            for( size_t k = 0; k < children_.size(); ++k ) {
                size_t child_height = children_[k]->calculate_height();
                height_ = std::max(height_, child_height);
            }
            height_ += 1;
        }
        return height_;
    }

    void print_branch(std::ostream &os, const std::deque<Action> &branch, size_t index = 0) const {
        print(os);
        if( index < branch.size() ) {
            Action action = branch[index];
            bool child_found = false;
            for( size_t k = 0; k < children_.size(); ++k ) {
                if( children_[k]->action_ == action ) {
                    children_[k]->print_branch(os, branch, ++index);
                    child_found = true;
                    break;
                }
            }
            assert(child_found);
        }
    }

    void print(std::ostream &os) const {
        os << "node:"
           << " valid=" << is_info_valid_
           << ", solved=" << solved_
           << ", lives=" << ale_lives_
           << ", value=" << value_
           << ", reward=" << reward_
           << ", action=" << action_
           << ", depth=" << depth_
           << ", children=[";
        for( size_t k = 0; k < children_.size(); ++k )
            os << children_[k]->value_ << " ";
        os << "] (this=" << this << ", parent=" << parent_ << ")"
           << std::endl;
    }

    void print_tree(std::ostream &os) const {
        print(os);
        for( size_t k = 0; k < children_.size(); ++k )
            children_[k]->print_tree(os);
    }
};

inline void remove_tree(Node *node) {
    for( size_t k = 0; k < node->children_.size(); ++k )
        remove_tree(node->children_[k]);
    delete node;
}

#endif

