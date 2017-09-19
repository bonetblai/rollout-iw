// (c) 2017 Blai Bonet

#ifndef SIM_PLANNER_H
#define SIM_PLANNER_H

#include <deque>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "planner.h"
#include "node.h"
#include "screen.h"
#include "utils.h"

struct SimPlanner : Planner {
    ALEInterface &sim_;

    const size_t frameskip_;
    const bool use_minimal_action_set_;
    const int simulator_budget_;
    const size_t num_tracked_atoms_;
    const bool debug_;

    mutable size_t simulator_calls_;
    mutable float sim_time_;
    mutable float sim_reset_time_;
    mutable float sim_get_set_state_time_;

    mutable size_t get_atoms_calls_;
    mutable float get_atoms_time_;
    mutable float novel_atom_time_;
    mutable float update_novelty_time_;

    ALEState initial_sim_state_;
    ActionVect action_set_;

    SimPlanner(std::ostream &logos,
               ALEInterface &sim,
               size_t frameskip,
               bool use_minimal_action_set,
               int simulator_budget,
               size_t num_tracked_atoms,
               bool debug = false)
      : Planner(logos),
        sim_(sim),
        frameskip_(frameskip),
        use_minimal_action_set_(use_minimal_action_set),
        simulator_budget_(simulator_budget),
        num_tracked_atoms_(num_tracked_atoms),
        debug_(debug) {
        //static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 required");
        assert(sim_.getInt("frame_skip") == frameskip_);
        if( use_minimal_action_set_ )
            action_set_ = sim_.getMinimalActionSet();
        else
            action_set_ = sim_.getLegalActionSet();
        assert(sim_.getInt("frame_skip") == frameskip_);
        reset_game(sim_);
        get_state(sim_, initial_sim_state_);
    }
    virtual ~SimPlanner() { }

    void reset_stats() const {
        simulator_calls_ = 0;
        sim_time_ = 0;
        sim_reset_time_ = 0;
        sim_get_set_state_time_ = 0;
        update_novelty_time_ = 0;
        get_atoms_calls_ = 0;
        get_atoms_time_ = 0;
        novel_atom_time_ = 0;
    }

    virtual float simulator_time() const {
        return sim_time_ + sim_reset_time_ + sim_get_set_state_time_;
    }
    virtual size_t simulator_calls() const {
        return simulator_calls_;
    }
    virtual Action random_action() const {
        return action_set_[lrand48() % action_set_.size()];
    }

    Action random_zero_value_action(const Node *root, float discount) const {
        assert(root != 0);
        assert((root->num_children_ > 0) && (root->first_child_ != nullptr));
        std::vector<Action> zero_value_actions;
        for( Node *child = root->first_child_; child != nullptr; child = child->sibling_ ) {
            if( child->qvalue(discount) == 0 )
                zero_value_actions.push_back(child->action_);
        }
        assert(!zero_value_actions.empty());
        return zero_value_actions[lrand48() % zero_value_actions.size()];
    }

    float call_simulator(ALEInterface &ale, Action action) const {
        ++simulator_calls_;
        float start_time = Utils::read_time_in_seconds();
        float reward = ale.act(action);
        assert(reward != -std::numeric_limits<float>::infinity());
        sim_time_ += Utils::read_time_in_seconds() - start_time;
        return reward;
    }

    void reset_game(ALEInterface &ale) const {
        float start_time = Utils::read_time_in_seconds();
        ale.reset_game();
        sim_reset_time_ += Utils::read_time_in_seconds() - start_time;
    }
    void get_state(ALEInterface &ale, ALEState &ale_state) const {
        float start_time = Utils::read_time_in_seconds();
        ale_state = ale.cloneState();
        sim_get_set_state_time_ += Utils::read_time_in_seconds() - start_time;
    }
    void set_state(ALEInterface &ale, const ALEState &ale_state) const {
        float start_time = Utils::read_time_in_seconds();
        ale.restoreState(ale_state);
        sim_get_set_state_time_ += Utils::read_time_in_seconds() - start_time;
    }

    int get_lives(ALEInterface &ale) const {
        return ale.lives();
    }
    bool terminal_state(ALEInterface &ale) const {
        return ale.game_over();
    }

    const ALERAM& get_ram(ALEInterface &ale) const {
        return ale.getRAM();
    }
    void get_ram(ALEInterface &ale, std::string &ram_str) const {
        ram_str = std::string(256, '0');
        const ALERAM &ale_ram = get_ram(ale);
        for( size_t k = 0; k < 128; ++k ) {
            byte_t byte = ale_ram.get(k);
            ram_str[2 * k] = "01234567890abcdef"[byte >> 4];
            ram_str[2 * k + 1] = "01234567890abcdef"[byte & 0xF];
        }
    }

    // update info for node
    void update_info(Node *node, int screen_features, float alpha, bool use_alpha_to_update_reward_for_death) const {
        assert(node->is_info_valid_ != 2);
        assert(node->state_ == nullptr);
        assert(node->parent_ != nullptr);
        assert((node->parent_->is_info_valid_ == 1) || (node->parent_->state_ != nullptr));
        if( node->parent_->state_ == nullptr ) {
            // do recursion on parent
            update_info(node->parent_, screen_features, alpha, use_alpha_to_update_reward_for_death);
        }
        assert(node->parent_->state_ != nullptr);
        set_state(sim_, *node->parent_->state_);
        float reward = call_simulator(sim_, node->action_);
        node->state_ = new ALEState;
        get_state(sim_, *node->state_);
        if( node->is_info_valid_ == 0 ) {
            node->reward_ = reward;
            node->terminal_ = terminal_state(sim_);
            if( node->reward_ < 0 ) node->reward_ *= alpha;
            get_atoms(node, screen_features);
            node->ale_lives_ = get_lives(sim_);
            if( use_alpha_to_update_reward_for_death && (node->parent_ != nullptr) && (node->parent_->ale_lives_ != -1) ) {
                if( node->ale_lives_ < node->parent_->ale_lives_ ) {
                    node->reward_ = -10 * alpha;
                    //logos_ << "L" << std::flush;
                }
            }
            node->path_reward_ = node->parent_ == nullptr ? 0 : node->parent_->path_reward_;
            node->path_reward_ += node->reward_;
        }
        node->is_info_valid_ = 2;
    }

    // get atoms from ram or screen
    void get_atoms(const Node *node, int screen_features) const {
        assert(node->feature_atoms_.empty());
        ++get_atoms_calls_;
        if( screen_features == 0 ) { // RAM mode
            get_atoms_from_ram(node);
        } else {
            get_atoms_from_screen(node, screen_features);
            if( (node->parent_ != nullptr) && (node->parent_->feature_atoms_ == node->feature_atoms_) ) {
                node->frame_rep_ = node->parent_->frame_rep_ + frameskip_;
                assert((node->num_children_ == 0) && (node->first_child_ == nullptr));
            }
        }
        assert((node->frame_rep_ == 0) || (screen_features > 0));
    }
    void get_atoms_from_ram(const Node *node) const {
        assert(node->feature_atoms_.empty());
        node->feature_atoms_ = std::vector<int>(128, 0);
        float start_time = Utils::read_time_in_seconds();
        const ALERAM &ram = get_ram(sim_);
        for( size_t k = 0; k < 128; ++k ) {
            node->feature_atoms_[k] = (k << 8) + ram.get(k);
            assert((k == 0) || (node->feature_atoms_[k] > node->feature_atoms_[k-1]));
        }
        get_atoms_time_ += Utils::read_time_in_seconds() - start_time;
    }
    void get_atoms_from_screen(const Node *node, int screen_features) const {
        assert(node->feature_atoms_.empty());
        float start_time = Utils::read_time_in_seconds();
        if( (screen_features < 3) || (node->parent_ == nullptr) ) {
            MyALEScreen screen(sim_, logos_, screen_features, &node->feature_atoms_);
        } else {
            assert((screen_features == 3) && (node->parent_ != nullptr));
            MyALEScreen screen(sim_, logos_, screen_features, &node->feature_atoms_, &node->parent_->feature_atoms_);
        }
        get_atoms_time_ += Utils::read_time_in_seconds() - start_time;
    }

    // novelty tables
    int get_index_for_novelty_table(const Node *node, bool use_novelty_subtables) const {
        if( !use_novelty_subtables ) {
            return 0;
        } else {
            assert(node->path_reward_ != std::numeric_limits<float>::infinity());
            if( node->path_reward_ <= 0 ) {
                return 0;
            } else {
                int logr = int(floorf(log2f(node->path_reward_)));
                assert((logr != 0) || (node->path_reward_ >= 1));
                return node->path_reward_ < 1 ? logr : 1 + logr;
            }
        }
    }

    std::vector<int>& get_novelty_table(const Node *node, std::map<int, std::vector<int> > &novelty_table_map, bool use_novelty_subtables) const {
        int index = get_index_for_novelty_table(node, use_novelty_subtables);
        std::map<int, std::vector<int> >::iterator it = novelty_table_map.find(index);
        if( it == novelty_table_map.end() ) {
            novelty_table_map.insert(std::make_pair(index, std::vector<int>()));
            std::vector<int> &novelty_table = novelty_table_map.at(index);
            novelty_table = std::vector<int>(num_tracked_atoms_, std::numeric_limits<int>::max());
            return novelty_table;
        } else {
            return it->second;
        }
    }

    size_t update_novelty_table(size_t depth, const std::vector<int> &feature_atoms, std::vector<int> &novelty_table) const {
        float start_time = Utils::read_time_in_seconds();
        size_t first_index = 0;
        size_t number_updated_entries = 0;
        for( size_t k = first_index; k < feature_atoms.size(); ++k ) {
            assert((feature_atoms[k] >= 0) && (feature_atoms[k] < novelty_table.size()));
            if( depth < novelty_table[feature_atoms[k]] ) {
                novelty_table[feature_atoms[k]] = depth;
                ++number_updated_entries;
            }
        }
        update_novelty_time_ += Utils::read_time_in_seconds() - start_time;
        return number_updated_entries;
    }

    int get_novel_atom(size_t depth, const std::vector<int> &feature_atoms, const std::vector<int> &novelty_table) const {
        float start_time = Utils::read_time_in_seconds();
        for( size_t k = 0; k < feature_atoms.size(); ++k ) {
            assert(feature_atoms[k] < novelty_table.size());
            if( novelty_table[feature_atoms[k]] > depth ) {
                novel_atom_time_ += Utils::read_time_in_seconds() - start_time;
                return feature_atoms[k];
            }
        }
        for( size_t k = 0; k < feature_atoms.size(); ++k ) {
            if( novelty_table[feature_atoms[k]] == depth ) {
                novel_atom_time_ += Utils::read_time_in_seconds() - start_time;
                return feature_atoms[k];
            }
        }
        novel_atom_time_ += Utils::read_time_in_seconds() - start_time;
        assert(novelty_table[feature_atoms[0]] < depth);
        return feature_atoms[0];
    }

    size_t num_entries(const std::vector<int> &novelty_table) const {
        assert(novelty_table.size() == num_tracked_atoms_);
        size_t n = 0;
        for( size_t k = 0; k < novelty_table.size(); ++k )
            n += novelty_table[k] < std::numeric_limits<int>::max();
        return n;
    }

    // prefix
    void apply_prefix(ALEInterface &ale, const ALEState &initial_state, const std::vector<Action> &prefix, ALEState *last_state = nullptr) const {
        assert(!prefix.empty());
        reset_game(ale);
        set_state(ale, initial_state);
        for( size_t k = 0; k < prefix.size(); ++k ) {
            if( (last_state != nullptr) && (1 + k == prefix.size()) )
                get_state(ale, *last_state);
            call_simulator(ale, prefix[k]);
        }
    }

    void print_prefix(std::ostream &os, const std::vector<Action> &prefix) const {
        os << "[";
        for( size_t k = 0; k < prefix.size(); ++k )
            os << prefix[k] << ",";
        os << "]" << std::flush;
    }

    // generate states along given branch
    void generate_states_along_branch(Node *node,
                                      const std::deque<Action> &branch,
                                      int screen_features,
                                      float alpha,
                                      bool use_alpha_to_update_reward_for_death) const {
        for( size_t pos = 0; pos < branch.size(); ++pos ) {
            if( node->state_ == nullptr ) {
                assert(node->is_info_valid_ == 1);
                update_info(node, screen_features, alpha, use_alpha_to_update_reward_for_death);
            }

            Node *selected = nullptr;
            for( Node *child = node->first_child_; child != nullptr; child = child->sibling_ ) {
                if( child->action_ == branch[pos] ) {
                    selected = child;
                    break;
                }
            }
            assert(selected != nullptr);
            node = selected;
        }
    }
};

#endif

