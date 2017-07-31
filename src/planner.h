// (c) 2017 Blai Bonet

#ifndef PLANNER_H
#define PLANNER_H

#include <string>
#include <vector>
#include <ale_interface.hpp>
#include "node.h"

typedef int OBS;

struct Planner {
    virtual std::string name() const = 0;
    virtual Action random_action() const = 0;
    virtual Node* get_branch(ALEInterface &env,
                             const std::vector<Action> &prefix,
                             Node *root,
                             float last_reward,
                             std::deque<Action> &branch) const = 0;
};

struct RandomPlanner : Planner {
    ActionVect minimal_actions_;
    size_t minimal_actions_size_;

    RandomPlanner(ALEInterface &env) {
        minimal_actions_ = env.getMinimalActionSet();
        minimal_actions_size_ = minimal_actions_.size();
    }

    virtual std::string name() const {
        return std::string("random()");
    }

    virtual Action random_action() const {
        return minimal_actions_[lrand48() % minimal_actions_size_];
    }

    virtual Node* get_branch(ALEInterface &env,
                             const std::vector<Action> &prefix,
                             Node *root,
                             float last_reward,
                             std::deque<Action> &branch) const {
        assert(branch.empty());
        branch.push_back(random_action());
        return nullptr;
    }
};

#endif

