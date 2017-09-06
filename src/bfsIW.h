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
#include <ale_interface.hpp>

#include "planner.h"
#include "node.h"
#include "screen.h"
#include "utils.h"

struct BfsIW : Planner {
    ALEInterface &sim_;

    const size_t frameskip_;
    const bool use_minimal_action_set_;
    const size_t num_tracked_atoms_;
    const int screen_features_;
    const int simulator_budget_;
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

    ALEState initial_sim_state_;
    ActionVect minimal_action_set_;
    ActionVect legal_action_set_;

    mutable float best_path_reward_;
    mutable size_t simulator_calls_;
    mutable size_t num_expansions_;
    mutable float total_time_;
    mutable float sim_time_;
    mutable float sim_reset_time_;
    mutable float sim_get_set_state_time_;
    mutable float update_novelty_time_;
    mutable size_t get_atoms_calls_;
    mutable float get_atoms_time_;
    mutable float novel_atom_time_;
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
      : Planner(logos),
        sim_(sim),
        frameskip_(frameskip),
        use_minimal_action_set_(use_minimal_action_set),
        num_tracked_atoms_(num_tracked_atoms),
        screen_features_(screen_features),
        simulator_budget_(simulator_budget),
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
        //static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 required");
        assert(sim_.getInt("frame_skip") == frameskip_);
        minimal_action_set_ = sim_.getMinimalActionSet();
        legal_action_set_ = sim_.getLegalActionSet();
        reset_game(sim_);
        get_state(sim_, initial_sim_state_);
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

    virtual size_t simulator_calls() const {
        return simulator_calls_;
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

    virtual Action random_action() const {
        if( use_minimal_action_set_ )
            return minimal_action_set_[lrand48() % minimal_action_set_.size()];
        else
            return legal_action_set_[lrand48() % legal_action_set_.size()];
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
        if( !root->children_.empty() ) {
            std::set<Action> root_actions;
            for( size_t k = 0; k < root->children_.size(); ++k )
                root_actions.insert(root->children_[k]->action_);

            // complete children
            const ActionVect &actions = use_minimal_action_set_ ? minimal_action_set_ : legal_action_set_;
            assert(root->children_.size() <= actions.size());
            if( root->children_.size() < actions.size() ) {
                for( size_t k = 0; k < actions.size(); ++k ) {
                    if( root_actions.find(actions[k]) == root_actions.end() )
                        root->expand(actions[k]);
                }
            }
            assert(root->children_.size() == actions.size());
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
            bfs(prefix,
                root,
                simulator_budget_,
                time_budget_,
                screen_features_,
                max_rep_,
                alpha_,
                use_alpha_to_update_reward_for_death_,
                novelty_table_map);
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
                       << " value=" << root->value_
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

    // breadth-first search with ties broken in favor of bigger path reward
    struct NodeComparator {
        bool break_ties_using_rewards_;
        NodeComparator(bool break_ties_using_rewards) : break_ties_using_rewards_(break_ties_using_rewards) {
        }
        bool operator()(const Node *n1, const Node *n2) const {
            return
              (n1->depth_ > n2->depth_) ||
              (break_ties_using_rewards_ && (n1->depth_ == n2->depth_) && (n1->path_reward_ < n2->path_reward_));
        }
    };

    void bfs(const std::vector<Action> &prefix,
             Node *root,
             int simulator_budget,
             float time_budget,
             int screen_features,
             size_t max_rep,
             float alpha,
             bool use_alpha_to_update_reward_for_death,
             std::map<int, std::vector<int> > &novelty_table_map) const {
        // priority queue
        NodeComparator cmp(break_ties_using_rewards_);
        std::priority_queue<Node*, std::vector<Node*>, NodeComparator> q(cmp);

        // add tip nodes to queue
        add_tip_nodes_to_queue(root, q);
        logos_ << "queue: sz=" << q.size() << std::endl;

        // explore in breadth-first manner
        float start_time = Utils::read_time_in_seconds();
        while( !q.empty() && (simulator_calls_ < simulator_budget) && (Utils::read_time_in_seconds() - start_time < time_budget) ) {
            Node *node = q.top();
            q.pop();

            // print debug info
            if( debug_ ) logos_ << node->depth_ << "@" << node->path_reward_ << std::flush;

            // update node info
            assert(node->children_.empty());
            assert(node->visited_ || (node->is_info_valid_ != 2));
            if( node->is_info_valid_ != 2 ) {
                update_info(node, screen_features, alpha, use_alpha_to_update_reward_for_death);
                assert(node->children_.empty());
                node->visited_ = true;
            }

            // check termination at this node
            if( node->terminal_ ) {
                if( debug_ ) logos_ << "t" << "," << std::flush;
                continue;
            }

            // verify max repetitions of feature atoms (screen mode)
            if( node->frame_rep_ > max_rep ) {
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
                update_novelty_table(atom, node->depth_, node->feature_atoms_, novelty_table);
            }
            if( debug_ ) logos_ << "+" << std::flush;

            // expand node
            if( node->frame_rep_ == 0 ) {
                ++num_expansions_;
                float start_time = Utils::read_time_in_seconds();
                if( use_minimal_action_set_ )
                    node->expand(minimal_action_set_, false);
                else
                    node->expand(legal_action_set_, false);
                expand_time_ += Utils::read_time_in_seconds() - start_time;
            } else {
                assert((node->parent_ != nullptr) && (screen_features > 0));
                node->expand(node->action_);
            }
            assert(!node->children_.empty());
            if( debug_ ) logos_ << node->children_.size() << "," << std::flush;

            // add children to queue
            for( size_t k = 0; k < node->children_.size(); ++k )
                q.push(node->children_[k]);
        }
        if( debug_ ) logos_ << std::endl;
    }

    void add_tip_nodes_to_queue(Node *node, std::priority_queue<Node*, std::vector<Node*>, NodeComparator> &pq) const {
        std::deque<Node*> q;
        q.push_back(node);
        while( !q.empty() ) {
            Node *n = q.front();
            q.pop_front();
            if( n->children_.empty() ) {
                pq.push(n);
            } else {
                for( size_t k = 0; k < n->children_.size(); ++k )
                    q.push_back(n->children_[k]);
            }
        }
    }

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

            Node *child = nullptr;
            for( size_t k = 0; k < node->children_.size(); ++k ) {
                if( node->children_[k]->action_ == branch[pos] ) {
                    child = node->children_[k];
                    break;
                }
            }
            assert(child != nullptr);
            node = child;
        }
    }

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

            // update best global reward
            if( best_path_reward_ < node->path_reward_ )
                best_path_reward_ = node->path_reward_;
        }
        node->is_info_valid_ = 2;
    }

    void get_atoms(const Node *node, int screen_features) const {
        assert(node->feature_atoms_.empty());
        ++get_atoms_calls_;
        if( screen_features == 0 ) { // RAM mode
            get_atoms_from_ram(node);
        } else {
            get_atoms_from_screen(node, screen_features);
            if( (node->parent_ != nullptr) && (node->parent_->feature_atoms_ == node->feature_atoms_) ) {
                node->frame_rep_ = node->parent_->frame_rep_ + frameskip_;
                assert(node->children_.empty());
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

    int get_index_for_novelty_table(const Node *node, bool use_novelty_subtables) const {
        if( !use_novelty_subtables ) {
            return 0;
        } else {
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

    void update_novelty_table(int novel_atom, size_t depth, const std::vector<int> &feature_atoms, std::vector<int> &novelty_table) const {
        assert(novelty_table[novel_atom] > depth);
        float start_time = Utils::read_time_in_seconds();
        size_t first_index = 0;
        for( size_t k = first_index; k < feature_atoms.size(); ++k ) {
            assert((feature_atoms[k] >= 0) && (feature_atoms[k] < novelty_table.size()));
            if( depth < novelty_table[feature_atoms[k]] )
                novelty_table[feature_atoms[k]] = depth;
        }
        update_novelty_time_ += Utils::read_time_in_seconds() - start_time;
    }
    size_t num_entries(const std::vector<int> &novelty_table) const {
        assert(novelty_table.size() == num_tracked_atoms_);
        size_t n = 0;
        for( size_t k = 0; k < novelty_table.size(); ++k )
            n += novelty_table[k] < std::numeric_limits<int>::max();
        return n;
    }

    float call_simulator(ALEInterface &ale, Action action) const {
        ++simulator_calls_;
        float start_time = Utils::read_time_in_seconds();
        float reward = ale.act(action);
        assert(reward != -std::numeric_limits<float>::infinity());
        sim_time_ += Utils::read_time_in_seconds() - start_time;
        return reward;
    }

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

    int get_lives(ALEInterface &ale) const {
        return ale.lives();
    }

    bool terminal_state(ALEInterface &ale) const {
        return ale.game_over();
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

    void reset_game(ALEInterface &ale) const {
        float start_time = Utils::read_time_in_seconds();
        ale.reset_game();
        sim_reset_time_ += Utils::read_time_in_seconds() - start_time;
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

    void reset_stats() const {
        best_path_reward_ = 0;
        simulator_calls_ = 0;
        num_expansions_ = 0;
        total_time_ = 0;
        sim_time_ = 0;
        sim_reset_time_ = 0;
        sim_get_set_state_time_ = 0;
        update_novelty_time_ = 0;
        get_atoms_calls_ = 0;
        get_atoms_time_ = 0;
        novel_atom_time_ = 0;
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

        for( size_t k = 0; k < root.children_.size(); ++k )
            os << root.children_[k]->height_ << ",";
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

    void print_prefix(std::ostream &os, const std::vector<Action> &prefix) const {
        os << "[";
        for( size_t k = 0; k < prefix.size(); ++k )
            os << prefix[k] << ",";
        os << "]" << std::flush;
    }
};

#endif

