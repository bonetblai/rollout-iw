// (c) 2017 Blai Bonet

#ifndef ROLLOUT_IW_H
#define ROLLOUT_IW_H

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "sim_planner.h"

struct RolloutIW : SimPlanner {
    const int screen_features_;
    const float time_budget_;
    const bool novelty_subtables_;
    const bool random_actions_;
    const size_t max_rep_;
    const float discount_;
    const float alpha_;
    const bool use_alpha_to_update_reward_for_death_;
    const int nodes_threshold_;
    const size_t max_depth_;
    const bool debug_;

    mutable size_t num_rollouts_;
    mutable size_t num_expansions_;
    mutable size_t num_cases_[4];
    mutable float total_time_;
    mutable float expand_time_;
    mutable size_t root_height_;
    mutable bool random_decision_;

    RolloutIW(std::ostream &logos,
              ALEInterface &sim,
              size_t frameskip,
              bool use_minimal_action_set,
              size_t num_tracked_atoms,
              int screen_features,
              int simulator_budget,
              float time_budget,
              bool novelty_subtables,
              bool random_actions,
              size_t max_rep,
              float discount,
              float alpha,
              bool use_alpha_to_update_reward_for_death,
              int nodes_threshold,
              size_t max_depth,
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
        max_depth_(max_depth),
        debug_(debug) {
    }
    virtual ~RolloutIW() { }

    virtual std::string name() const {
        return std::string("rollout(")
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
          + ",max-depth=" + std::to_string(max_depth_)
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

        logos_ << Utils::red() << "**** rollout: get branch ****" << Utils::normal() << std::endl;
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

        // novelty table and other vars
        std::map<int, std::vector<int> > novelty_table_map;
        std::set<std::pair<bool, bool> > rewards_seen;

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
        if( !root->children_.empty() ) {
            std::set<Action> root_actions;
            for( size_t k = 0; k < root->children_.size(); ++k )
                root_actions.insert(root->children_[k]->action_);

            // complete children
            assert(root->children_.size() <= action_set_.size());
            if( root->children_.size() < action_set_.size() ) {
                for( size_t k = 0; k < action_set_.size(); ++k ) {
                    if( root_actions.find(action_set_[k]) == root_actions.end() )
                        root->expand(action_set_[k]);
                }
            }
            assert(root->children_.size() == action_set_.size());
        } else {
            // make sure this root node isn't marked as frame rep
            root->parent_->feature_atoms_.clear();
        }

        // normalize depths, reset rep counters, and recompute path rewards
        root->parent_->depth_ = -1;
        root->normalize_depth();
        root->reset_frame_rep_counters(frameskip_);
        root->recompute_path_rewards(root);

        // construct/extend lookahead tree
        if( root->num_nodes() < nodes_threshold_ ) {
            float elapsed_time = Utils::read_time_in_seconds() - start_time;

            // clear solved labels
            clear_solved_labels(root);
            root->parent_->solved_ = false;
            while( !root->solved_ && (simulator_calls_ < simulator_budget_) && (elapsed_time < time_budget_) ) {
                if( debug_ ) logos_ << '.' << std::flush;
                std::pair<bool, bool> rewards_seen_in_rollout;
                rollout(prefix,
                        root,
                        screen_features_,
                        max_depth_,
                        max_rep_,
                        alpha_,
                        use_alpha_to_update_reward_for_death_,
                        novelty_table_map,
                        rewards_seen_in_rollout);
                rewards_seen.insert(rewards_seen_in_rollout);
                elapsed_time = Utils::read_time_in_seconds() - start_time;
            }
            if( debug_ ) logos_ << std::endl;
        }

        // if nothing was expanded, return random actions (it can only happen with small time budget)
        if( root->children_.empty() ) {
            random_decision_ = true;
            branch.push_back(random_action());
        } else {
            // backup values and calculate heights
            root->backup_values(discount_);
            root->calculate_height();
            root_height_ = root->height_;

            // print info about root node
            if( true || debug_ ) {
                logos_ << Utils::green()
                       << "root:"
                       << " solved=" << root->solved_
                       << ", value=" << root->value_
                       << ", imm-reward=" << root->reward_
                       << ", children=[";
                for( size_t k = 0; k < root->children_.size(); ++k )
                    logos_ << root->children_[k]->value_ << ":" << root->children_[k]->action_ << " ";
                logos_ << "]" << Utils::normal() << std::endl;
            }

            // compute branch
            if( root->value_ > 0 ) {
                root->best_branch(branch, discount_);
            } else if( random_actions_ && (root->value_ == 0) ) {
                random_decision_ = true;
                branch.push_back(random_action());
            } else if( (root->value_ == 0) && (root->reward_ == 0) ) {
                root->longest_zero_branch(branch);
                if( branch.empty() ) {
                    random_decision_ = true;
                    branch.push_back(random_action());
                }
            } else {
                root->best_branch(branch, discount_);
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

    void rollout(const std::vector<Action> &prefix,
                 Node *root,
                 int screen_features,
                 size_t max_depth,
                 size_t max_rep,
                 float alpha,
                 bool use_alpha_to_update_reward_for_death,
                 std::map<int, std::vector<int> > &novelty_table_map,
                 std::pair<bool, bool> &rewards_seen) const {
        ++num_rollouts_;
        rewards_seen = std::pair<bool, bool>(false, false);

        // apply prefix
        //apply_prefix(sim_, initial_sim_state_, prefix);

        // update root info
        if( root->is_info_valid_ != 2 )
            update_info(root, screen_features, alpha, use_alpha_to_update_reward_for_death);

        // perform rollout
        Node *node = root;
        while( !node->solved_ ) {
            assert(node->is_info_valid_ == 2);

            // if first time at this node, expand node
            if( node->children_.empty() ) {
                if( node->frame_rep_ == 0 ) {
                    ++num_expansions_;
                    float start_time = Utils::read_time_in_seconds();
                    node->expand(action_set_);
                    expand_time_ += Utils::read_time_in_seconds() - start_time;
                } else {
                    assert((node->parent_ != nullptr) && (screen_features > 0));
                    node->expand(node->action_);
                }
                assert(!node->children_.empty());
            }

            // pick random unsolved child
            size_t num_unsolved_children = 0;
            for( size_t k = 0; k < node->children_.size(); ++k )
                num_unsolved_children += node->children_[k]->solved_ ? 0 : 1;
            assert(num_unsolved_children > 0);
            size_t index = lrand48() % num_unsolved_children;
            for( size_t k = 0; k < node->children_.size(); ++k ) {
                if( !node->children_[k]->solved_ ) {
                    if( index == 0 ) {
                        node = node->children_[k];
                        break;
                    }
                    --index;
                }
            }

            // update info
            if( node->is_info_valid_ != 2 )
                update_info(node, screen_features, alpha, use_alpha_to_update_reward_for_death);

            // if terminal, label as solved and terminate rollout
            if( node->terminal_ ) {
                node->visited_ = true;
                assert(node->children_.empty());
                node->solve_and_backpropagate_label();
                //logos_ << "T[reward=" << node->reward_ << "]" << std::flush;
                break;
            }

            // verify repetitions of feature atoms (screen mode)
            if( node->frame_rep_ > max_rep ) {
                node->visited_ = true;
                assert(node->children_.empty());
                node->solve_and_backpropagate_label();
                //logos_ << "R" << std::flush;
                break;
            } else if( node->frame_rep_ > 0 ) {
                node->visited_ = true;
                //logos_ << "r" << std::flush;
                continue;
            }

            // report non-zero rewards
            if( node->reward_ > 0 ) {
                rewards_seen.first = true;
                if( debug_ ) logos_ << Utils::yellow() << "+" << Utils::normal() << std::flush;
            } else if( node->reward_ < 0 ) {
                rewards_seen.second = true;
                if( debug_ ) logos_ << "-" << std::flush;
            }

            // calculate novelty
            std::vector<int> &novelty_table = get_novelty_table(node, novelty_table_map, novelty_subtables_);
            int atom = get_novel_atom(node->depth_, node->feature_atoms_, novelty_table);
            assert((atom >= 0) && (atom < novelty_table.size()));

            // five cases
            if( node->depth_ > max_depth ) {
                node->visited_ = true;
                assert(node->children_.empty());
                node->solve_and_backpropagate_label();
                //logos_ << "D" << std::flush;
                break;
            } else if( novelty_table[atom] > node->depth_ ) { // novel => not(visited)
                //assert(!node->visited_);
                if( !node->visited_ ) {
                    ++num_cases_[0];
                    node->visited_ = true;
                    update_novelty_table(atom, node->depth_, node->feature_atoms_, novelty_table);
                    //logos_ << Utils::green() << "n" << Utils::normal() << std::flush;
                }
                continue;
            } else if( !node->visited_ && (novelty_table[atom] <= node->depth_) ) { // not(novel) and not(visited) => PRUNE
                ++num_cases_[1];
                node->visited_ = true;
                assert(node->children_.empty());
                node->solve_and_backpropagate_label();
                //logos_ << "x" << node->depth_ << std::flush;
                break;
            } else if( node->visited_ && (novelty_table[atom] < node->depth_) ) { // not(novel) and visited => PRUNE
                ++num_cases_[2];
                node->remove_children();
                node->reward_ = -std::numeric_limits<float>::infinity();
                rewards_seen.second = true;
                if( debug_ ) logos_ << "-" << std::flush;
                node->solve_and_backpropagate_label();
                //logos_ << "X" << node->depth_ << std::flush;
                break;
            } else { // optimal and visited => CONTINUE
                assert(node->visited_ && (novelty_table[atom] == node->depth_));
                ++num_cases_[3];
                //logos_ << "c" << std::flush;
                continue;
            }
        }
    }

    void clear_solved_labels(Node *node) const {
        node->solved_ = false;
        for( size_t k = 0; k < node->children_.size(); ++k )
            clear_solved_labels(node->children_[k]);
    }

    void reset_stats() const {
        SimPlanner::reset_stats();
        num_rollouts_ = 0;
        num_expansions_ = 0;
        num_cases_[0] = 0;
        num_cases_[1] = 0;
        num_cases_[2] = 0;
        num_cases_[3] = 0;
        total_time_ = 0;
        expand_time_ = 0;
        root_height_ = 0;
        random_decision_ = false;
    }

    void print_stats(std::ostream &os, const Node &root, const std::map<int, std::vector<int> > &novelty_table_map) const {
        os << Utils::red()
           << "stats:"
           << " #rollouts=" << num_rollouts_
           << " #entries=[";

        for( std::map<int, std::vector<int> >::const_iterator it = novelty_table_map.begin(); it != novelty_table_map.end(); ++it )
            os << it->first << ":" << num_entries(it->second) << "/" << it->second.size() << ",";
        os << "]";

        os << " #nodes=" << root.num_nodes()
           << " #tips=" << root.num_tip_nodes()
           << " height=[" << root.height_ << ":";

        for( size_t k = 0; k < root.children_.size(); ++k )
            os << root.children_[k]->height_ << ",";
        os << "]";

        os << " #expansions=" << num_expansions_
           << " #cases=[" << num_cases_[0] << "," << num_cases_[1] << "," << num_cases_[2] << "," << num_cases_[3] << "]"
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

