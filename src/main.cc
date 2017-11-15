// (c) 2017 Blai Bonet

#include <iostream>
#include <fstream>
#include <limits>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/tokenizer.hpp>
#include <ale_interface.hpp>

#include "planner.h"
#include "bfsIW.h"
#include "rolloutIW.h"
#include "logger.h"
#include "utils.h"

#ifdef __USE_SDL
  #include <SDL.h>
#endif

using namespace std;
namespace fs = boost::filesystem;
namespace po = boost::program_options;

vector<pixel_t> MyALEScreen::background_;
size_t MyALEScreen::num_background_pixels_;
ActionVect MyALEScreen::minimal_actions_;
size_t MyALEScreen::minimal_actions_size_;

// global variables
float g_acc_simulator_time = 0;
float g_acc_reward = 0;
size_t g_acc_simulator_calls = 0;
size_t g_max_simulator_calls = 0;
size_t g_acc_decisions = 0;
size_t g_acc_frames = 0;
size_t g_acc_random_decisions = 0;
size_t g_acc_height = 0;
size_t g_acc_expanded = 0;


void reset_global_variables() {
    g_acc_simulator_time = 0;
    g_acc_reward = 0;
    g_acc_simulator_calls = 0;
    g_max_simulator_calls = 0;
    g_acc_decisions = 0;
    g_acc_frames = 0;
    g_acc_random_decisions = 0;
    g_acc_height = 0;
    g_acc_expanded = 0;
}

void run_episode(ALEInterface &env,
                 const Planner &planner,
                 int initial_noops,
                 int lookahead_caching,
                 float prefix_length_to_execute,
                 bool execute_single_action,
                 size_t frameskip,
                 size_t max_execution_length_in_frames,
                 vector<Action> &prefix) {
    assert(prefix.empty());
    reset_global_variables();

    // fill initial prefix
    if( initial_noops == 0 ) {
        prefix.push_back(planner.random_action());
    } else {
        assert(initial_noops > 0);
        while( initial_noops-- > 0 )
            prefix.push_back(Action(0));
    }

    // reset simulator
    env.reset_game();

    // execute actions in initial prefix
    float last_reward = numeric_limits<float>::infinity();
    for( size_t k = 0; k < prefix.size(); ++k ) {
        last_reward = env.act(prefix[k]);
        g_acc_reward += last_reward;
    }
    assert(last_reward != numeric_limits<float>::infinity());

    // play
    Node *node = nullptr;
    deque<Action> branch;
    for( size_t frame = 0; !env.game_over() && (frame < max_execution_length_in_frames); frame += frameskip ) {
        // if empty branch, get branch
        if( branch.empty() ) {
            ++g_acc_decisions;

            if( (node != nullptr) && (lookahead_caching == 1) ) {
                node->clear_cached_states();
                assert(node->state_ == nullptr);
                assert(node->parent_ != nullptr);
                assert(node->parent_->state_ != nullptr);
            }

            node = planner.get_branch(env, prefix, node, last_reward, branch);
            g_acc_simulator_time += planner.simulator_time();
            g_acc_simulator_calls += planner.simulator_calls();
            g_max_simulator_calls = std::max(g_max_simulator_calls, planner.simulator_calls());
            g_acc_random_decisions += planner.random_decision() ? 1 : 0;
            g_acc_height += planner.height();
            g_acc_expanded += planner.expanded();

            if( branch.empty() ) {
                Logger::Error << "no more available actions!" << endl;
                break;
            }

            // calculate executable prefix
            assert(prefix_length_to_execute >= 0);
            if( !execute_single_action && (prefix_length_to_execute > 0) ) {
                int target_branch_length = int(branch.size() * prefix_length_to_execute);
                if( target_branch_length == 0 ) target_branch_length = 1;
                assert(target_branch_length > 0);
                while( int(branch.size()) > target_branch_length )
                    branch.pop_back();
                assert(target_branch_length == int(branch.size()));
            } else if( execute_single_action ) {
                assert(branch.size() >= 1);
                while( branch.size() > 1 )
                    branch.pop_back();
            }

            Logger::Info << "executable-prefix: len=" << branch.size() << ", actions=[";
            for( size_t j = 0; j < branch.size(); ++j )
                Logger::Continuation(Logger::Info) << branch[j] << ",";
            Logger::Continuation(Logger::Info) << "]" << endl;
        }

        // select action to apply
        Action action = branch.front();
        branch.pop_front();

        // apply action
        last_reward = env.act(action);
        prefix.push_back(action);
        g_acc_reward += last_reward;
        g_acc_frames += frameskip;
        Logger::Stats << "step-stats: acc-reward=" << g_acc_reward << ", acc-frames=" << g_acc_frames << endl;

        // advance/destroy lookhead tree
        if( node != nullptr ) {
            if( (lookahead_caching == 0) || (node->num_children_ == 0) ) {
                remove_tree(node);
                node = nullptr;
            } else {
                assert(node->parent_->state_ != nullptr);
                node = node->advance(action);
            }
        }

        // prune branch if got positive reward
        if( execute_single_action || ((prefix_length_to_execute == 0) && (last_reward > 0)) )
            branch.clear();
    }

    // cleanup
    if( node != nullptr ) {
        assert(node->parent_ != nullptr);
        assert(node->parent_->parent_ == nullptr);
        remove_tree(node->parent_);
    }
}

void parse_action_sequence(const string &fixed_action_sequence, vector<Action> &actions) {
    boost::tokenizer<> tok(fixed_action_sequence);
    for( boost::tokenizer<>::iterator it = tok.begin(); it != tok.end(); ++it )
        actions.push_back(static_cast<Action>(atoi(it->c_str())));
}

void print_options(ostream &os, const po::variables_map &opt_varmap) {
    os << "options:" << endl;
    bool something_printed = false;
    for( po::variables_map::const_iterator it = opt_varmap.begin(); it != opt_varmap.end(); ++it ) {
        if( something_printed ) os << " \\" << endl;
        os << "  --" << it->first;
        if( ((boost::any)it->second.value()).type() == typeid(bool) ) {
            os << " " << opt_varmap[it->first].as<bool>();
        } else if( ((boost::any)it->second.value()).type() == typeid(int) ) {
            os << " " << opt_varmap[it->first].as<int>();
        } else if( ((boost::any)it->second.value()).type() == typeid(float) ) {
            os << " " << opt_varmap[it->first].as<float>();
        } else if( ((boost::any)it->second.value()).type() == typeid(string) ) {
            os << " " << opt_varmap[it->first].as<string>();
        }
        something_printed = true;
    }
    if( something_printed ) os << endl;
}

void usage(ostream &os, const po::options_description &opt_desc) {
    os << Utils::magenta() << "Usage:" << Utils::normal()
       << " rom_planner <option>* <rom>" << endl
       << endl
       << opt_desc
       << endl;
}

int main(int argc, char **argv) {
    // setup logger
    ostream *default_log_file = &cout;
    Logger::set_output_stream(*default_log_file);
    Logger::set_mode(Logger::Info);
    Logger::set_use_color(true);
    Logger::set_debug_threshold(0);

    // rom and log files
    string opt_logger_mode;
    string opt_log_file;
    string opt_rom;

    // planner
    string opt_planner_str;

    // general options
    int opt_random_seed;
    int opt_debug_threshold;
    string opt_ale_logger_mode;
    int opt_frameskip;
    bool opt_display = true;
    bool opt_sound = false;
    string opt_rec_dir;
    string opt_rec_sound_filename;
    bool opt_use_minimal_action_set = false;

    // episodes and execution length
    int opt_episodes;
    int opt_max_execution_length_in_frames;

    // simulate previous execution
    string opt_fixed_action_sequence;

    // features
    int opt_screen_features;
    int opt_frames_for_background_image;

    // online execution
    int opt_initial_random_noops;
    bool opt_execute_single_action = false;
    int opt_lookahead_caching;
    float opt_prefix_length_to_execute;
    int opt_simulator_budget;
    float opt_time_budget;

    // common options for planners
    float opt_alpha;
    float opt_discount;
    int opt_max_rep;
    int opt_nodes_threshold;
    bool opt_novelty_subtables = false;
    bool opt_random_actions = false;
    bool opt_use_alpha_to_update_reward_for_death = false;

    // options for rollout planner
    int opt_max_depth;

    // options for bfs planner
    bool opt_break_ties_using_rewards = false;

    // declare supported options
    po::options_description opt_desc("Allowed options");
    opt_desc.add_options()
      // rom and log files
      ("logger-mode", po::value<string>(&opt_logger_mode)->default_value("info"), "Change logger mode to 'debug', 'info', 'warning', 'error', 'stats', or 'silent' (default is 'info')")
      ("log-file", po::value<string>(&opt_log_file), "Set path to log file (default is \"\" for std::cout)")
      ("rom", po::value<string>(&opt_rom), "Set Atari ROM")

      // general options
      ("help", "Help message")
      ("seed", po::value<int>(&opt_random_seed)->default_value(0), "Set random seed (default is 0)")
      ("debug-threshold", po::value<int>(&opt_debug_threshold)->default_value(0), "Set threshold for debug mode (default is 0)")
      ("ale-logger-mode", po::value<string>(&opt_ale_logger_mode)->default_value("error"), "Change ALE logger mode to 'info', 'warning', 'error', or 'silent' (default is 'error')")
      ("frameskip", po::value<int>(&opt_frameskip)->default_value(15), "Set frame skip rate (default is 15)")
      ("nodisplay", "Turn off display (default is display)")
      ("sound", "Turn on sound (default is no sound)")
      ("rec-dir", po::value<string>(&opt_rec_dir), "Set folder for recording (default is \"\" for no recording)")
      ("rec-sound-filename", po::value<string>(&opt_rec_sound_filename), "Set filename for recording sound (default is \"\" for no recording)")
      ("minimal-action-set", "Turn on minimal action set instead of larger legal action set")

      // number of episodes and execution length
      ("num-episodes", po::value<int>(&opt_episodes)->default_value(1), "Set number of episodes (default is 1)")
      ("max-execution-length", po::value<int>(&opt_max_execution_length_in_frames)->default_value(18000), "Set max number of frames in single execution (default is 18k frames)")

      // simulate previous execution
      ("fixed-action-sequence", po::value<string>(&opt_fixed_action_sequence)->default_value("none"), "Pass fixed action sequence that provides actions (default is \"none\" for no such sequence)")

      // features
      ("features", po::value<int>(&opt_screen_features)->default_value(3), "Set feature set: 0=RAM, 1=basic, 2=basic+B-PROS, 3=basic+B-PROS+B-PROT (default is 3)")
      ("frames-background-image", po::value<int>(&opt_frames_for_background_image)->default_value(100), "Set number of random frames to compute background image (default is 100)")

      // options for online execution
      ("initial-random-noops", po::value<int>(&opt_initial_random_noops)->default_value(30), "Set max number of initial noops, actual # is sampled (default is 30)")
      ("lookahead-caching", po::value<int>(&opt_lookahead_caching)->default_value(2), "Set lookahead caching: 0=none, 1=partial, 2=full (default is 2)")
      ("simulator-budget", po::value<int>(&opt_simulator_budget)->default_value(150000), "Set budget for #calls to simulator for online decision making (default is 150k)")
      ("time-budget", po::value<float>(&opt_time_budget)->default_value(numeric_limits<float>::infinity()), "Set time budget for online decision making (default is infinite)")
      ("execute-single-action", "Execute only one action from best branch in lookahead (default is to execute prefix until first reward)")
      ("prefix-length-to-execute", po::value<float>(&opt_prefix_length_to_execute)->default_value(0.0), "Set \% of prefix to execute (default is 0 = execute until positive reward)")

      // planners
      ("planner", po::value<string>(&opt_planner_str)->default_value(string("rollout")), "Set planner, either 'rollout' or 'bfs' (default is 'rollout')")
      ("novelty-subtables", "Turn on use of novelty subtables (default is to use single table)")
      ("random-actions", "Use random action when there are no rewards in look-ahead tree (default is off)")
      ("max-rep", po::value<int>(&opt_max_rep)->default_value(30), "Set max rep(etition) of screen features during lookahead (default is 30)")
      ("discount", po::value<float>(&opt_discount)->default_value(1.00), "Set discount factor for lookahead (default is 1.00)")
      ("alpha", po::value<float>(&opt_alpha)->default_value(50000.0), "Set alpha value for lookahead (default is 50k)")
      ("use-alpha-to-update-reward-for-death", "Assign a big negative reward, depending on alpha's value, for deaths (default is off)")
      ("nodes-threshold", po::value<int>(&opt_nodes_threshold)->default_value(50000), "Set threshold in #nodes for expanding look-ahead tree (default is 50k)")

      // options for rollout planner
      ("max-depth", po::value<int>(&opt_max_depth)->default_value(1500), "Set max depth for lookahead (default is 1500)")

      // optiosn for bfs planner
      ("break-ties-using-rewards", "Break ties in favor of better rewards during bfs (default is no tie breaking)")
    ;

    po::positional_options_description opt_pos;
    opt_pos.add("rom", -1);

    // parse options
    po::variables_map opt_varmap;
    try {
        po::store(po::command_line_parser(argc, argv).options(opt_desc).positional(opt_pos).run(), opt_varmap);
        po::notify(opt_varmap);
    } catch( po::error &e ) {
        Logger::Error << e.what() << endl;
        exit(1);
    }

    // set values for boolean options
    opt_display = !opt_varmap.count("nodisplay");
    opt_sound = opt_varmap.count("sound");
    opt_use_minimal_action_set = opt_varmap.count("use-minimal-action-set");
    opt_execute_single_action = opt_varmap.count("execute-single-action");
    opt_novelty_subtables = opt_varmap.count("novelty-subtables");
    opt_random_actions = opt_varmap.count("random-actions");
    opt_use_alpha_to_update_reward_for_death = opt_varmap.count("use-alpha-to-update-reward-for-death");
    opt_break_ties_using_rewards = opt_varmap.count("break-ties-using-rewards");

    // set logger mode and log mode
    Logger::mode_t logger_mode = Logger::Silent;
    if( opt_logger_mode == "debug" ) {
        logger_mode = Logger::Debug;
    } else if( opt_logger_mode == "info" ) {
        logger_mode = Logger::Info;
    } else if( opt_logger_mode == "warning" ) {
        logger_mode = Logger::Warning;
    } else if( opt_logger_mode == "error" ) {
        logger_mode = Logger::Error;
    } else if( opt_logger_mode == "stats" ) {
        logger_mode = Logger::Stats;
    } else if( opt_logger_mode == "silent" ) {
        logger_mode = Logger::Silent;
    } else {
        Logger::Error << "invalid logger mode '" << opt_logger_mode << "'" << endl;
        exit(-1);
    }
    Logger::set_mode(logger_mode);
    Logger::set_debug_threshold(opt_debug_threshold);

    if( opt_varmap.count("log-file") && (opt_log_file != "") ) {
        ostream *logger_output_stream = new ofstream(opt_log_file);
        Logger::set_output_stream(*logger_output_stream);
        Logger::set_use_color(false);
    }

    // check whether there is something to be done
    if( opt_varmap.count("help") || (opt_rom == "") ) {
        usage(cout, opt_desc);
        exit(1);
    }

    // print command-line options
    print_options(Logger::output_stream(), opt_varmap);

    // set random seed for lrand48() and drand48()
    srand48(opt_random_seed);

    // set logger mode for ALE
    ale::Logger::mode ale_logger_mode = ale::Logger::Silent;
    if( opt_ale_logger_mode == "info" ) {
        ale_logger_mode = ale::Logger::Info;
    } else if( opt_ale_logger_mode == "warning" ) {
        ale_logger_mode = ale::Logger::Warning;
    } else if( opt_ale_logger_mode == "error" ) {
        ale_logger_mode = ale::Logger::Error;
    } else if( opt_ale_logger_mode == "silent" ) {
        ale_logger_mode = ale::Logger::Silent;
    } else {
        Logger::Error << "invalid ALE logger mode '" << opt_ale_logger_mode << "'" << endl;
        exit(-1);
    }

    // create ALEs
    ALEInterface env(ale_logger_mode), sim(ale::Logger::Silent);

    // get/set desired settings
    env.setInt("frame_skip", opt_frameskip);
    env.setInt("random_seed", opt_random_seed);
    env.setFloat("repeat_action_probability", 0.00);
    sim.setInt("frame_skip", opt_frameskip);
    sim.setInt("random_seed", opt_random_seed);
    sim.setFloat("repeat_action_probability", 0.00);
    fs::path rom_path(opt_rom);

#ifdef __USE_SDL
    env.setBool("display_screen", opt_display);
    env.setBool("sound", opt_sound);
    if( opt_rec_dir != "" ) {
        string full_rec_dir = opt_rec_dir + "/" + rom_path.filename().string();
        env.setString("record_screen_dir", full_rec_dir.c_str());
        if( opt_rec_sound_filename != "" )
            env.setString("record_sound_filename", (full_rec_dir + "/" + opt_rec_sound_filename).c_str());
        fs::create_directories(full_rec_dir);
    }
    sim.setBool("display_screen", false);
    sim.setBool("sound", false);
#endif

    // Load the ROM file. (Also resets the system for new settings to take effect.)
    env.loadROM(rom_path.string().c_str());
    sim.loadROM(rom_path.string().c_str());

    // initialize static members for screen features
    if( opt_screen_features > 0 ) {
        MyALEScreen::create_background_image();
        MyALEScreen::compute_background_image(sim, opt_frames_for_background_image);
    }

    // construct planner
    Planner *planner = nullptr;
    if( opt_fixed_action_sequence != "none" ) {
        vector<Action> actions;
        parse_action_sequence(opt_fixed_action_sequence, actions);
        planner = new FixedPlanner(actions);
    } else {
        size_t num_tracked_atoms = 0;
        if( opt_screen_features == 0 ) { // RAM mode
            num_tracked_atoms = 128 * 256; // this is for RAM: 128 8-bit entries
        } else {
            num_tracked_atoms = 16 * 14 * 128; // 28,672
            num_tracked_atoms += opt_screen_features > 1 ? 6856768 : 0;
            num_tracked_atoms += opt_screen_features > 2 ? 13713408 : 0;
        }

        if( opt_planner_str == "rollout" ) {
            planner = new RolloutIW(sim,
                                    opt_frameskip,
                                    opt_use_minimal_action_set,
                                    num_tracked_atoms,
                                    opt_screen_features,
                                    opt_simulator_budget,
                                    opt_time_budget,
                                    opt_novelty_subtables,
                                    opt_random_actions,
                                    opt_max_rep,
                                    opt_discount,
                                    opt_alpha,
                                    opt_use_alpha_to_update_reward_for_death,
                                    opt_nodes_threshold,
                                    opt_max_depth);
        } else if( opt_planner_str == "bfs" ) {
            planner = new BfsIW(sim,
                                opt_frameskip,
                                opt_use_minimal_action_set,
                                num_tracked_atoms,
                                opt_screen_features,
                                opt_simulator_budget,
                                opt_time_budget,
                                opt_novelty_subtables,
                                opt_random_actions,
                                opt_max_rep,
                                opt_discount,
                                opt_alpha,
                                opt_use_alpha_to_update_reward_for_death,
                                opt_nodes_threshold,
                                opt_break_ties_using_rewards);
        } else {
            Logger::Error << "inexistent planner '" << opt_planner_str << "'" << endl;
            exit(1);
        }
    }
    assert(planner != nullptr);
    Logger::Info << "planner=" << planner->name() << endl;

    // set number of initial noops
    assert(opt_initial_random_noops > 0);
    int initial_noops = lrand48() % opt_initial_random_noops;

    // play
    for( int k = 0; k < opt_episodes; ++k ) {
        vector<Action> prefix;
        float start_time = Utils::read_time_in_seconds();
        run_episode(env, *planner, initial_noops, opt_lookahead_caching, opt_prefix_length_to_execute, opt_execute_single_action, opt_frameskip, opt_max_execution_length_in_frames, prefix);
        float elapsed_time = Utils::read_time_in_seconds() - start_time;
        Logger::Stats
          << "episode-stats:"
          // rom and log files
          << " rom=" << opt_rom
          // planner
          << " planner=" << opt_planner_str
          // general options
          << " debug-threshold=" << opt_debug_threshold
          << " frameskip=" << opt_frameskip
          << " seed=" << opt_random_seed
          << " use-minimal-action-set=" << opt_use_minimal_action_set
          // episodes and execution length
          << " episodes=" << opt_episodes
          << " max-execution-length=" << opt_max_execution_length_in_frames
          // simulate previous execution
          << " fixed-action-sequence=\"" << opt_fixed_action_sequence << "\""
          // features
          << " features=" << opt_screen_features
          << " frames-background-image=" << opt_frames_for_background_image
          // online execution
          << " initial-noops=" << opt_initial_random_noops
          << " execute-single-action=" << opt_execute_single_action
          << " caching=" << opt_lookahead_caching
          << " prefix-length-to-execute=" << opt_prefix_length_to_execute
          << " simulator-budget=" << opt_simulator_budget
          << " time-budget=" << opt_time_budget
          // common options for planners
          << " alpha=" << opt_alpha
          << " discount=" << opt_discount
          << " max-rep=" << opt_max_rep
          << " nodes-threshold=" << opt_nodes_threshold
          << " novelty-subtables=" << opt_novelty_subtables
          << " random-actions=" << opt_random_actions
          << " use-alpha-to-update-reward-for-death=" << opt_use_alpha_to_update_reward_for_death
          // rollout planner
          << " max-depth=" << opt_max_depth
          // bfs planner
          << " break-ties-using-rewards=" << opt_break_ties_using_rewards
          // data
          << " score=" << g_acc_reward
          << " frames=" << g_acc_frames
          << " decisions=" << g_acc_decisions
          << " simulator-calls=" << g_acc_simulator_calls
          << " max-simulator-calls=" << g_max_simulator_calls
          << " total-time=" << elapsed_time
          << " simulator-time=" << g_acc_simulator_time
          << " sum-expanded=" << g_acc_expanded
          << " sum-height=" << g_acc_height
          << " random-decisions=" << g_acc_random_decisions
          << endl;
    }

    // cleanup
    delete planner;
    if( &Logger::output_stream() != default_log_file ) {
        static_cast<ofstream*>(&Logger::output_stream())->close();
    }

    return 0;
}

