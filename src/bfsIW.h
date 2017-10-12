// (c) 2017 Blai Bonet

#ifndef BFS_IW_H
#define BFS_IW_H

#include <cassert>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

#include "sim_planner.h"

struct BfsIW : SimPlanner {
    const int screen_features_;
    const float time_budget_;
    const bool novelty_subtables_;
    const bool random_actions_;
    const size_t max_rep_;
    const float discount_;
    const float alpha_;
    const bool use_alpha_to_update_reward_for_death_;
    const int nodes_threshold_;
    const bool break_ties_using_rewards_;
    const bool debug_;

    mutable size_t num_expansions_;
    mutable float total_time_;
    mutable float expand_time_;
    mutable size_t root_height_;
    mutable bool random_decision_;

    BfsIW(std::ostream &logos,
          ALEInterface &sim,
          size_t frameskip,
          bool use_minimal_action_set,
          size_t num_tracked_atoms,
          int screen_features,
          float simulator_budget,
          float time_budget,
          bool novelty_subtables,
          bool random_actions,
          size_t max_rep,
          float discount,
          float alpha,
          bool use_alpha_to_update_reward_for_death,
          int nodes_threshold,
          bool break_ties_using_rewards,
          bool debug = false)
      : SimPlanner(logos, sim, frameskip, use_minimal_action_set, simulator_budget, num_tracked_atoms, debug),
        screen_features_(screen_features),
        time_budget_(time_budget),
        novelty_subtables_(novelty_subtables),
        random_actions_(random_actions),
        max_rep_(max_rep),
        discount_(discount),
        alpha_(alpha),
        use_alpha_to_update_reward_for_death_(use_alpha_to_update_reward_for_death),
        nodes_threshold_(nodes_threshold),
        break_ties_using_rewards_(break_ties_using_rewards),
        debug_(debug) {
    }
    virtual ~BfsIW() { }

    virtual std::string name() const {
        return std::string("bfs(")
          + "frameskip=" + std::to_string(frameskip_)
          + ",minimal-action-set=" + std::to_string(use_minimal_action_set_)
          + ",features=" + std::to_string(screen_features_)
          + ",simulator-budget=" + std::to_string(simulator_budget_)
          + ",time-budget=" + std::to_string(time_budget_)
          + ",novelty-subtables=" + std::to_string(novelty_subtables_)
          + ",random-actions=" + std::to_string(random_actions_)
          + ",max-rep=" + std::to_string(max_rep_)
          + ",discount=" + std::to_string(discount_)
          + ",alpha=" + std::to_string(alpha_)
          + ",use-alpha-to-update-reward-for-death=" + std::to_string(use_alpha_to_update_reward_for_death_)
          + ",nodes-threshold=" + std::to_string(nodes_threshold_)
          + ",break-ties-using-rewards=" + std::to_string(break_ties_using_rewards_)
          + ",debug=" + std::to_string(debug_)
          + ")";
    }

    virtual bool random_decision() const {
        return random_decision_;
    }
    virtual size_t height() const {
        return root_height_;
    }
    virtual size_t expanded() const {
        return num_expansions_;
    }

    virtual Node* get_branch(ALEInterface &env,
                             const std::vector<Action> &prefix,
                             Node *root,
                             float last_reward,
                             std::deque<Action> &branch) const {
        assert(!prefix.empty());

        logos_ << Utils::red() << "**** bfs: get branch ****" << Utils::normal() << std::endl;
        logos_ << "prefix: sz=" << prefix.size() << ", actions=";
        print_prefix(logos_, prefix);
        logos_ << std::endl;
        logos_ << "input:"
               << " #nodes=" << (root == nullptr ? "na" : std::to_string(root->num_nodes()))
               << ", #tips=" << (root == nullptr ? "na" : std::to_string(root->num_tip_nodes()))
               << ", height=" << (root == nullptr ? "na" : std::to_string(root->height_))
               << std::endl;

        // reset stats and start timer
        reset_stats();
        float start_time = Utils::read_time_in_seconds();

        // novelty table
        std::map<int, std::vector<int> > novelty_table_map;

        // construct root node
        assert((root == nullptr) || (root->action_ == prefix.back()));
        if( root == nullptr ) {
            Node *root_parent = new Node(nullptr, PLAYER_A_NOOP, -1);
            root_parent->state_ = new ALEState;
            apply_prefix(sim_, initial_sim_state_, prefix, root_parent->state_);
            root = new Node(root_parent, prefix.back(), 0);
        }
        assert(root->parent_ != nullptr);
        root->parent_->parent_ = nullptr;

        // if root has some children, make sure it has all children
        if( root->num_children_ > 0 ) {
            assert(root->first_child_ != nullptr);
            std::set<Action> root_actions;
            for( Node *child = root->first_child_; child != nullptr; child = child->sibling_ )
                root_actions.insert(child->action_);

            // complete children
            assert(root->num_children_ <= action_set_.size());
            if( root->num_children_ < action_set_.size() ) {
                for( size_t k = 0; k < action_set_.size(); ++k ) {
                    if( root_actions.find(action_set_[k]) == root_actions.end() )
                        root->expand(action_set_[k]);
                }
            }
            assert(root->num_children_ == action_set_.size());
        } else {
            // make sure this root node isn't marked as frame rep
            root->parent_->feature_atoms_.clear();
        }

        // normalize depths and recompute path rewards
        root->parent_->depth_ = -1;
        root->normalize_depth();
        root->reset_frame_rep_counters(frameskip_);
        root->recompute_path_rewards(root);

        // construct/extend lookahead tree
        if( root->num_nodes() < nodes_threshold_ ) {
            bfs(prefix, root, novelty_table_map);
        }

        // if nothing was expanded, return random actions (it can only happen with small time budget)
        if( root->num_children_ == 0 ) {
            assert(root->first_child_ == nullptr);
            assert(time_budget_ != std::numeric_limits<float>::infinity());
            random_decision_ = true;
            branch.push_back(random_action());
        } else {
            assert(root->first_child_ != nullptr);

            // backup values and calculate heights
            root->backup_values(discount_);
            root->calculate_height();
            root_height_ = root->height_;

            // print info about root node
            if( true || debug_ ) {
                logos_ << Utils::green()
                       << "root:"
                       << " value=" << root->value_
                       << ", imm-reward=" << root->reward_
                       << ", children=[";
                for( Node *child = root->first_child_; child != nullptr; child = child->sibling_ )
                    logos_ << child->value_ << ":" << child->action_ << " ";
                logos_ << "]" << Utils::normal() << std::endl;
            }

            // compute branch
            if( root->value_ != 0 ) {
                root->best_branch(branch, discount_);
            } else {
                if( random_actions_ ) {
                    random_decision_ = true;
                    branch.push_back(random_zero_value_action(root, discount_));
                } else {
                    root->longest_zero_value_branch(discount_, branch);
                    assert(!branch.empty());
                }
            }

            // make sure states along branch exist (only needed when doing partial caching)
            generate_states_along_branch(root, branch, screen_features_, alpha_, use_alpha_to_update_reward_for_death_);

            // print branch
            assert(!branch.empty());
            if( true || debug_ ) {
                logos_ << "branch:"
                       << " value=" << root->value_
                       << ", size=" << branch.size()
                       << ", actions:"
                       << std::endl;
                //root->print_branch(logos_, branch);
            }
        }

        // stop timer and print stats
        total_time_ = Utils::read_time_in_seconds() - start_time;
        if( true || debug_ )
            print_stats(logos_, *root, novelty_table_map);

        // return root node
        return root;
    }

    // breadth-first search with ties broken in favor of bigger path reward
    struct NodeComparator {
        bool break_ties_using_rewards_;
        NodeComparator(bool break_ties_using_rewards) : break_ties_using_rewards_(break_ties_using_rewards) {
        }
        bool operator()(const Node *lhs, const Node *rhs) const {
            return
              (lhs->depth_ > rhs->depth_) ||
              (break_ties_using_rewards_ && (lhs->depth_ == rhs->depth_) && (lhs->path_reward_ < rhs->path_reward_));
        }
    };

    void bfs(const std::vector<Action> &prefix, Node *root, std::map<int, std::vector<int> > &novelty_table_map) const {
        // priority queue
        NodeComparator cmp(break_ties_using_rewards_);
        std::priority_queue<Node*, std::vector<Node*>, NodeComparator> q(cmp);

        // add tip nodes to queue
        add_tip_nodes_to_queue(root, q);
        logos_ << "queue: sz=" << q.size() << std::endl;

        // explore in breadth-first manner
        float start_time = Utils::read_time_in_seconds();
        while( !q.empty() && (simulator_calls_ < simulator_budget_) && (Utils::read_time_in_seconds() - start_time < time_budget_) ) {
            Node *node = q.top();
            q.pop();

            // print debug info
            if( debug_ ) logos_ << node->depth_ << "@" << node->path_reward_ << std::flush;

            // update node info
            assert((node->num_children_ == 0) && (node->first_child_ == nullptr));
            assert(node->visited_ || (node->is_info_valid_ != 2));
            if( node->is_info_valid_ != 2 ) {
                update_info(node, screen_features_, alpha_, use_alpha_to_update_reward_for_death_);
                assert((node->num_children_ == 0) && (node->first_child_ == nullptr));
                node->visited_ = true;
            }

            // check termination at this node
            if( node->terminal_ ) {
                if( debug_ ) logos_ << "t" << "," << std::flush;
                continue;
            }

            // verify max repetitions of feature atoms (screen mode)
            if( node->frame_rep_ > max_rep_ ) {
                if( debug_ ) logos_ << "r" << node->frame_rep_ << "," << std::flush;
                continue;
            }

            // calculate novelty and prune
            if( node->frame_rep_ == 0 ) {
                // calculate novelty
                std::vector<int> &novelty_table = get_novelty_table(node, novelty_table_map, novelty_subtables_);
                int atom = get_novel_atom(node->depth_, node->feature_atoms_, novelty_table);
                assert((atom >= 0) && (atom < novelty_table.size()));

                // prune node using novelty
                if( novelty_table[atom] <= node->depth_ ) {
                    if( debug_ ) logos_ << "p" << "," << std::flush;
                    continue;
                }

                // update novelty table
                update_novelty_table(node->depth_, node->feature_atoms_, novelty_table);
            }
            if( debug_ ) logos_ << "+" << std::flush;

            // expand node
            if( node->frame_rep_ == 0 ) {
                ++num_expansions_;
                float start_time = Utils::read_time_in_seconds();
                node->expand(action_set_, false);
                expand_time_ += Utils::read_time_in_seconds() - start_time;
            } else {
                assert((node->parent_ != nullptr) && (screen_features_ > 0));
                node->expand(node->action_);
            }
            assert((node->num_children_ > 0) && (node->first_child_ != nullptr));
            if( debug_ ) logos_ << node->num_children_ << "," << std::flush;

            // add children to queue
            for( Node *child = node->first_child_; child != nullptr; child = child->sibling_ )
                q.push(child);
        }
        if( debug_ ) logos_ << std::endl;
    }

    void add_tip_nodes_to_queue(Node *node, std::priority_queue<Node*, std::vector<Node*>, NodeComparator> &pq) const {
        std::deque<Node*> q;
        q.push_back(node);
        while( !q.empty() ) {
            Node *n = q.front();
            q.pop_front();
            if( n->num_children_ == 0 ) {
                assert(n->first_child_ == nullptr);
                pq.push(n);
            } else {
                assert(n->first_child_ != nullptr);
                for( Node *child = n->first_child_; child != nullptr; child = child->sibling_ )
                    q.push_back(child);
            }
        }
    }

    void reset_stats() const {
        SimPlanner::reset_stats();
        num_expansions_ = 0;
        total_time_ = 0;
        expand_time_ = 0;
        root_height_ = 0;
        random_decision_ = false;
    }

    void print_stats(std::ostream &os, const Node &root, const std::map<int, std::vector<int> > &novelty_table_map) const {
        os << Utils::red()
           << "stats:"
           << " #entries=[";

        for( std::map<int, std::vector<int> >::const_iterator it = novelty_table_map.begin(); it != novelty_table_map.end(); ++it )
            os << it->first << ":" << num_entries(it->second) << "/" << it->second.size() << ",";

        os << "]";

        os << " #nodes=" << root.num_nodes()
           << " #tips=" << root.num_tip_nodes()
           << " height=[" << root.height_ << ":";

        for( Node *child = root.first_child_; child != nullptr; child = child->sibling_ )
            os << child->height_ << ",";

        os << "]";

        os << " #expansions=" << num_expansions_
           << " #sim=" << simulator_calls_
           << " total-time=" << total_time_
           << " simulator-time=" << sim_time_
           << " reset-time=" << sim_reset_time_
           << " get/set-state-time=" << sim_get_set_state_time_
           << " expand-time=" << expand_time_
           << " update-novelty-time=" << update_novelty_time_
           << " get-atoms-calls=" << get_atoms_calls_
           << " get-atoms-time=" << get_atoms_time_
           << " novel-atom-time=" << novel_atom_time_
           << Utils::normal() << std::endl;
    }
};

#endif

