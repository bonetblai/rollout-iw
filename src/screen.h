// (c) 2017 Blai Bonet

#ifndef SCREEN_H
#define SCREEN_H

#include <cassert>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <ale_interface.hpp>

#include "utils.h"

struct MyALEScreen {
    const int type_; // type=0: no features, type=1: basic features, type=2: basic + B-PROS, type=3: basic + B-PROS + B-PROT
    const bool debug_;
    std::ostream &logos_;
    const ALEScreen &screen_;
    std::vector<bool> basic_features_bitmap_;
    std::vector<bool> bpros_features_bitmap_;
    std::vector<bool> bprot_features_bitmap_;

    static const size_t width_ = 160;
    static const size_t height_ = 210;
    static const size_t num_basic_features_ = 16 * 14 * 128; // 28,672
    static const size_t num_bpros_features_t0_ = 6803136; // (dc,dr,k1,k2) where k1 < k2, number equal to 31 * 27 * 128 * 127 / 2
    static const size_t num_bpros_features_t1_ = 53504; // (dc,dr,k,k) where dc != 0 or dr != 0, number equal to (31 * 27 - 1) * 128 / 2
    static const size_t num_bpros_features_t2_ = 128; // (dc,dr,k,k) where dc = dr = 0, number equal to 128
    static const size_t num_bpros_features_ = num_bpros_features_t0_ + num_bpros_features_t1_ + num_bpros_features_t2_; // 6,856,768
    static const size_t num_bprot_features_ = 31 * 27 * 128 * 128; // 13,713,408

    static ActionVect minimal_actions_;
    static size_t minimal_actions_size_;
    static std::vector<pixel_t> background_;
    static size_t num_background_pixels_;

    MyALEScreen(ALEInterface &ale,
                std::ostream &logos,
                int type,
                std::vector<int> *screen_state_atoms = nullptr,
                const std::vector<int> *prev_screen_state_atoms = nullptr,
                bool debug = false)
      : type_(type),
        debug_(debug),
        logos_(logos),
        screen_(ale.getScreen()) {

        if( debug_ ) {
            logos_ << "screen:"
                   << " type=" << type_
                   << ", height=" << screen_.height() << " (expecting " << height_ << ")"
                   << ", width=" << screen_.width() << " (expecting " << width_ << ")"
                   << std::endl;
        }
        assert((width_ == screen_.width()) && (height_ == screen_.height()));

        compute_features(type, screen_state_atoms, prev_screen_state_atoms, debug);
    }

    static Action random_action() {
        return minimal_actions_[lrand48() % minimal_actions_size_];
    }
    static void reset(ALEInterface &ale) {
        ale.reset_game();
        for( size_t k = 0; k < 100; ++k )
            ale.act(random_action());
    }
    static void fill_image(ALEInterface &ale, std::vector<pixel_t> &image) {
        const ALEScreen &screen = ale.getScreen();
        for( size_t c = 0; c < width_; ++c ) {
            for( size_t r = 0; r < height_; ++r ) {
                image[r * width_ + c] = screen.get(r, c);
            }
        }
    }
    static void create_background_image() {
        background_ = std::vector<pixel_t>(width_ * height_, 0);
        num_background_pixels_ = width_ * height_;
    }
    static void compute_background_image(ALEInterface &ale, std::ostream &logos, size_t num_frames, bool debug = false) {
        assert((width_ == ale.getScreen().width()) && (height_ == ale.getScreen().height()));
        float start_time = Utils::read_time_in_seconds();
        if( debug ) {
            logos << Utils::green()
                  << "screen: computing background image (#frames=" << num_frames << ") ... "
                  << Utils::normal()
                  << std::flush;
        }

        minimal_actions_ = ale.getMinimalActionSet();
        minimal_actions_size_ = minimal_actions_.size();

        std::vector<bool> is_background(width_ * height_, true);
        std::vector<pixel_t> reference_image(width_ * height_);
        std::vector<pixel_t> image(width_ * height_);

        reset(ale);
        fill_image(ale, reference_image);
        int frameskip = ale.getInt("frame_skip");
        for( size_t k = 0; k < num_frames; k += frameskip ) {
            if( ale.game_over() ) reset(ale);
            fill_image(ale, image);
            for( size_t c = 0; c < width_; ++c ) {
                for( size_t r = 0; r < height_; ++r ) {
                    if( (reference_image[r * width_ + c] != image[r * width_ + c]) && is_background[r * width_ + c] ) {
                        is_background[r * width_ + c] = false;
                        --num_background_pixels_;
                    }
                }
            }
            ale.act(random_action());
        }

        for( size_t c = 0; c < width_; ++c ) {
            for( size_t r = 0; r < height_; ++r ) {
                if( is_background[r * width_ + c] ) {
                    background_[r * width_ + c] = reference_image[r * width_ + c];
                }
            }
        }

        float elapsed_time = Utils::read_time_in_seconds() - start_time;
        if( debug ) {
            logos << Utils::green()
                  << "done in " << elapsed_time << " seconds"
                  << std::endl
                  << "background: #pixels=" << num_background_pixels_ << "/" << width_ * height_
                  << Utils::normal()
                  << std::endl;
        }
    }
    static void ammend_background_image(size_t r, size_t c, bool debug = false, std::ostream *logos = nullptr) {
        assert(background_[r * width_ + c] > 0);
        assert(num_background_pixels_ > 0);
        background_[r * width_ + c] = 0;
        --num_background_pixels_;
        if( debug && (logos != nullptr) ) {
            *logos << Utils::blue() << "background:" << Utils::normal()
                   << " #pixels=" << num_background_pixels_ << "/" << width_ * height_
                   << std::endl;
        }
    }

    const ALEScreen& get_screen() const {
        return screen_;
    }

    void compute_features(int type, std::vector<int> *screen_state_atoms, const std::vector<int> *prev_screen_state_atoms, bool debug = false) {
        int num_basic_features = 0;
        int num_bpros_features = 0;
        int num_bprot_features = 0;

        if( type_ > 0 ) {
            basic_features_bitmap_ = std::vector<bool>(num_basic_features_, false);
            compute_basic_features(screen_state_atoms);
            num_basic_features = screen_state_atoms->size();
            if( (type_ > 1) && (screen_state_atoms != nullptr) ) {
                std::vector<int> basic_features(*screen_state_atoms);
                bpros_features_bitmap_ = std::vector<bool>(num_bpros_features_, false);
                compute_bpros_features(basic_features, *screen_state_atoms);
                num_bpros_features = screen_state_atoms->size() - num_basic_features;
                if( (type_ > 2) && (prev_screen_state_atoms != nullptr) ) {
                    bprot_features_bitmap_ = std::vector<bool>(num_bprot_features_, false);
                    compute_bprot_features(basic_features, *screen_state_atoms, *prev_screen_state_atoms);
                    num_bprot_features = screen_state_atoms->size() - num_basic_features - num_bpros_features;
                }
            }
        }

        if( debug_ ) {
            logos_ << "screen:"
                   << " #features=" << screen_state_atoms->size()
                   << ", #basic=" << num_basic_features
                   << ", #bpros=" << num_bpros_features
                   << ", #bprot=" << num_bprot_features
                   << std::endl;
        }
    }

    void compute_basic_features(size_t c, size_t r, std::vector<int> *screen_state_atoms = 0) {
        assert((c < 16) && (r < 14));
        for( size_t ic = 0; ic < 10; ++ic ) {
            for( size_t ir = 0; ir < 15; ++ir ) {
                assert((15*r + ir < height_) && (10*c + ic < width_));
                pixel_t p = screen_.get(15*r + ir, 10*c + ic);
                pixel_t b = background_[(15*r + ir) * width_ + (10*c + ic)];

                // subtract/ammend background pixel
                if( p < b )
                    ammend_background_image(15*r + ir, 10*c + ic, false, &logos_);
                else
                    p -= b;

                assert(p % 2 == 0); // per documentation, expecting 128 different colors!
                int pack = pack_basic_feature(c, r, p >> 1);
                if( !basic_features_bitmap_[pack] ) {
                    basic_features_bitmap_[pack] = true;
                    if( screen_state_atoms != nullptr )
                        screen_state_atoms->push_back(pack);
                }
            }
        }
    }
    void compute_basic_features(std::vector<int> *screen_state_atoms = 0) {
        for( size_t c = 0; c <= width_ - 10; c += 10 ) { // 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150
            for( size_t r = 0; r <= height_ - 15; r += 15) { // 0, 15, 30, 45, 60, 75, 90, 105, 120, 135, 150, 165, 180, 195
                compute_basic_features(c / 10, r / 15, screen_state_atoms);
            }
        }
    }

    void compute_bpros_features(const std::vector<int> &basic_features, std::vector<int> &screen_state_atoms) {
        std::pair<std::pair<size_t, size_t>, pixel_t> f1, f2;
        for( size_t j = 0; j < basic_features.size(); ++j ) {
            unpack_basic_feature(basic_features[j], f1);
            for( size_t k = j; k < basic_features.size(); ++k ) {
                unpack_basic_feature(basic_features[k], f2);
                int pack = pack_bpros_feature(f1, f2);
                if( !bpros_features_bitmap_[pack - num_basic_features_] ) {
                    bpros_features_bitmap_[pack - num_basic_features_] = true;
                    screen_state_atoms.push_back(pack);
                }
            }
        }
    }

    void compute_bprot_features(const std::vector<int> &basic_features,
                                std::vector<int> &screen_state_atoms,
                                const std::vector<int> &prev_screen_state_atoms) {
        std::pair<std::pair<size_t, size_t>, pixel_t> f1, f2;
        for( size_t j = 0; j < basic_features.size(); ++j ) {
            unpack_basic_feature(basic_features[j], f1);
            for( size_t k = 0; k < prev_screen_state_atoms.size(); ++k ) {
                if( !is_basic_feature(prev_screen_state_atoms[k]) ) break; // no more basic features in vector
                unpack_basic_feature(prev_screen_state_atoms[k], f2);
                int pack = pack_bprot_feature(f1, f2);
                if( !bprot_features_bitmap_[pack - num_basic_features_ - num_bpros_features_] ) {
                    bprot_features_bitmap_[pack - num_basic_features_ - num_bpros_features_] = true;
                    screen_state_atoms.push_back(pack);
                }
            }
        }
    }

    // features
    typedef std::pair<size_t, size_t> patch_t;
    typedef std::pair<patch_t, pixel_t> basic_feature_t;
    typedef std::pair<size_t, size_t> offset_t;
    typedef std::pair<offset_t, std::pair<pixel_t, pixel_t> > bpros_feature_t;
    typedef std::pair<offset_t, std::pair<pixel_t, pixel_t> > bprot_feature_t;

    // basic features
    static bool is_basic_feature(int pack) {
        return (pack >= 0) && (pack < int(num_basic_features_));
    }
    static int pack_basic_feature(size_t c, size_t r, pixel_t p) {
        assert((c < 16) && (r < 14));
        assert((p >= 0) && (p < 128));
        int pack = ((14 * c + r) << 7) + p;
        assert(is_basic_feature(pack));
        return pack;
    }
    static int pack_basic_feature(const basic_feature_t &bf) {
        return pack_basic_feature(bf.first.first, bf.first.second, bf.second);
    }
    static void unpack_basic_feature(int pack, basic_feature_t &bf) {
        assert(is_basic_feature(pack));
        bf.first.first = (pack >> 7) / 14;
        bf.first.second = (pack >> 7) % 14;
        bf.second = pack & 127;
        assert(pack == pack_basic_feature(bf));
    }

    // B-PROS features
    static bool is_bpros_feature(int pack) {
        return (pack >= int(num_basic_features_)) && (pack < int(num_basic_features_ + num_bpros_features_));
    }
    static int pack_bpros_feature(int dc, int dr, pixel_t p1, pixel_t p2) {
        assert((-15 <= dc) && (dc <= 15));
        assert((-13 <= dr) && (dr <= 13));
        assert((p1 >= 0) && (p1 < 128));
        assert((p2 >= 0) && (p2 < 128));
        assert(p1 <= p2);
        int pack = 0;
        if( p1 < p2 ) {
            pack = ((15 + dc) * 27 + (13 + dr)) * 128 * 127 / 2;
            pack += p1 * 127 - p1 * (1 + p1) / 2 + p2 - 1;
            assert((pack >= 0) && (pack < int(num_bpros_features_t0_)));
        } else if( (dc != 0) || (dr != 0) ) {
            assert(p1 == p2);
            if( (dc < 0) || ((dc == 0) && (dr < 0)) ) {
                dc = -dc;
                dr = -dr;
            }
            assert((dc > 0) || ((dc == 0) && (dr > 0)));

            if( dc > 0 ) {
                pack = ((dc - 1) * 27 + (13 + dr)) * 128 + p1;
                assert((pack >= 0) && (pack < 15 * 27 * 128));
            } else {
                assert(dr > 0);
                pack = 15 * 27 * 128 + (dr - 1) * 128 + p1;
            }

            assert((pack >= 0) && (pack < int(num_bpros_features_t1_)));
            pack += num_bpros_features_t0_;
        } else {
            assert((p1 == p2) && (dc == 0) && (dr == 0));
            pack = p1;
            assert((pack >= 0) && (pack < int(num_bpros_features_t2_)));
            pack += num_bpros_features_t0_ + num_bpros_features_t1_;
        }
        pack += num_basic_features_;
        assert(is_bpros_feature(pack));
        return pack;
    }
    static int pack_bpros_feature(const bpros_feature_t &cf) {
        return pack_bpros_feature(cf.first.first, cf.first.second, cf.second.first, cf.second.second);
    }
    static int pack_bpros_feature(const basic_feature_t &bf1, const basic_feature_t &bf2) {
        int dc = bf1.first.first - bf2.first.first;
        int dr = bf1.first.second - bf2.first.second;
        if( bf1.second <= bf2.second )
            return pack_bpros_feature(dc, dr, bf1.second, bf2.second);
        else
            return pack_bpros_feature(-dc, -dr, bf2.second, bf1.second);
    }
    static void unpack_bpros_feature(int pack, bpros_feature_t &cf) {
        assert(0);
    }

    // B-PROT features
    static bool is_bprot_feature(int pack) {
        return (pack >= int(num_basic_features_ + num_bpros_features_)) && (pack < int(num_basic_features_ + num_bpros_features_ + num_bprot_features_));
    }
    static int pack_bprot_feature(int dc, int dr, pixel_t p1, pixel_t p2) {
        assert((-15 <= dc) && (dc <= 15));
        assert((-13 <= dr) && (dr <= 13));
        assert((((15 + dc) * 27 + (13 + dr)) * 128 + p1) * 128 + p2 < int(num_bprot_features_));
        return num_basic_features_ + num_bpros_features_ + (((15 + dc) * 27 + (13 + dr)) * 128 + p1) * 128 + p2;
    }
    static int pack_bprot_feature(const bprot_feature_t &cf) {
        return pack_bprot_feature(cf.first.first, cf.first.second, cf.second.first, cf.second.second);
    }
    static int pack_bprot_feature(const basic_feature_t &bf1, const basic_feature_t &bf2) {
        int dc = bf1.first.first - bf2.first.first;
        int dr = bf1.first.second - bf2.first.second;
        return pack_bprot_feature(dc, dr, bf1.second, bf2.second);
    }
    static void unpack_bprot_feature(int pack, bprot_feature_t &cf) {
        assert(pack >= 0);
        pixel_t p2 = pack % 128;
        pixel_t p1 = (pack >> 7) % 128;
        int dr = ((pack >> 14) % 27) - 13;
        assert((pack >> 14) / 27 < 31);
        int dc = ((pack >> 14) / 27) - 15;
        cf = std::make_pair(std::make_pair(dc, dr), std::make_pair(p1, p2));
        assert(pack == pack_bprot_feature(cf));
    }
};

#endif

