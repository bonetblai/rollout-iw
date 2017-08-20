// (c) 2017 Blai Bonet

#ifndef PLANNER_H
#define PLANNER_H

#include <deque>
#include <string>
#include <vector>
#include <ale_interface.hpp>
#include "node.h"

typedef int OBS;

struct Planner {
    virtual ~Planner() { }
    virtual std::string name() const = 0;
    virtual size_t simulator_calls() const = 0;
    virtual bool random_decision() const = 0;
    virtual size_t height() const = 0;
    virtual size_t expanded() const = 0;
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
    virtual ~RandomPlanner() { }

    virtual std::string name() const {
        return std::string("random()");
    }
    virtual size_t simulator_calls() const {
        return 0;
    }
    virtual bool random_decision() const {
        return true;
    }
    virtual size_t height() const {
        return 0;
    }
    virtual size_t expanded() const {
        return 0;
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

struct FixedPlanner : public Planner {
    mutable std::deque<Action> actions_;

    FixedPlanner(const std::vector<Action> &actions) {
        actions_ = std::deque<Action>(actions.begin(), actions.end());
    }
    virtual ~FixedPlanner() { }

    Action pop_first_action() const {
        Action action = actions_.front();
        actions_.pop_front();
        return action;
    }

    virtual std::string name() const {
        return std::string("fixed(sz=") + std::to_string(actions_.size()) + ")";
    }
    virtual size_t simulator_calls() const {
        return 0;
    }
    virtual bool random_decision() const {
        return true;
    }
    virtual size_t height() const {
        return 0;
    }
    virtual size_t expanded() const {
        return 0;
    }

    virtual Action random_action() const {
        assert(!actions_.empty());
        return pop_first_action();
    }

    virtual Node* get_branch(ALEInterface &env,
                             const std::vector<Action> &prefix,
                             Node *root,
                             float last_reward,
                             std::deque<Action> &branch) const {
        assert(branch.empty());
        if( !actions_.empty() ) {
            Action action = pop_first_action();
            branch.push_back(action);
        }
        return nullptr;
    }
};

#endif

