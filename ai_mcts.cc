#include "ai_mcts.h"

#include <assert.h>

#include <algorithm>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <memory>
#include <optional>

namespace {

using ai_internal::Node;
using ai_internal::random_engine_t;

constexpr int exploration_factor = 2;
constexpr int iterations_per_move = 1000000;
constexpr bool debug_print_moves = true;
constexpr bool debug_print_expected_value = true;

static signed char lines_per_field[16][4] = {
    {0, 4, 8, -1},
    {0, 5, -1},
    {0, 6, -1},
    {0, 7, 9, -1},
    {1, 4, -1},
    {1, 5, 8, -1},
    {1, 6, 9, -1},
    {1, 7, -1},
    {2, 4, -1},
    {2, 5, 9, -1},
    {2, 6, 8, -1},
    {2, 7, -1},
    {3, 4, 9, -1},
    {3, 5, -1},
    {3, 6, -1},
    {3, 7, 8, -1}};

enum class Result : signed char { LOSS = -1, TIE = 0, WIN = +1 };

struct Outcome {
    Result result;
    bool fixed;

    bool operator==(const Outcome &o) { return result == o.result && fixed == o.fixed; }
};

struct LineInfo {
    unsigned char common_values = 0xff;
    unsigned char spaces_left = 4;
};

struct EnhancedState {
    // Number of next piece to place, or -1 if we need to select the next piece.
    signed char next_piece = -1;

    // Bitmask of available pieces.
    unsigned short pieces = 0xffff;

    // For each field, the number of a piece, or -1 if the field is empty.
    std::array<signed char, 16> fields = {
        -1, -1, -1, -1,
        -1, -1, -1, -1,
        -1, -1, -1, -1,
        -1, -1, -1, -1};

    // State of each line on the board.
    std::array<LineInfo, 10> lines;
};

random_engine_t SeedRandomEngine() {
    // Seed with 8 random integers, or 256 bits, which should be enough.
    std::random_device dev;
    std::seed_seq seed = {dev(), dev(), dev(), dev(), dev(), dev(), dev(), dev()};
    return random_engine_t(seed);
}

constexpr unsigned AttributeValues(int piece) {
    return (piece << 4) | (piece ^ 0xf);
}

Result Invert(Result r) { return static_cast<Result>(-static_cast<int>(r)); }

int GameValue(Result r) { return static_cast<int>(r); }

Outcome Invert(Outcome o) {
    return Outcome{Invert(o.result), o.fixed};
};


int RandomIndex(int size, random_engine_t &random_engine) {
    assert(size > 0);
    std::uniform_int_distribution<> dist(0, size - 1);
    return dist(random_engine);
}

Move RandomMove(const std::vector<Move> &moves, random_engine_t &random_engine) {
    return moves[RandomIndex(moves.size(), random_engine)];
}

EnhancedState EnhanceState(const State &state) {
    EnhancedState est;
    switch (state.NextAction()) {
    case NextAction::SELECT:
        est.next_piece = -1;
        break;
    case NextAction::PLACE:
        est.next_piece = state.LastPiece();
        est.pieces -= 1 << est.next_piece;
        break;
    default:
        assert(false);
    }
    for (int i = 0; i < 16; ++i) {
        int piece = state.PieceAt(i);
        if (piece < 0) {
            est.fields[i] = -1;
        } else {
            assert(est.pieces & (1 << piece));
            est.fields[i] = piece;
            est.pieces -= 1 << piece;
            unsigned values = AttributeValues(piece);
            for (signed char *p = lines_per_field[i]; *p >= 0; ++p) {
                LineInfo &line = est.lines[*p];
                line.spaces_left -= 1;
                line.common_values &= values;
            }
        }
    }
    return est;
}

int ListNonlosingMoves(const EnhancedState &est, std::array<int, 16> &moves) {
    unsigned char winning_values = 0;
    for (const LineInfo &line : est.lines) {
        if (line.spaces_left == 1) {
            winning_values |= line.common_values;
        }
    }
    int n = 0;
    if (est.next_piece < 0) {
        // We must select the piece to place.
        // Consider only moves that don't allow the opponent to win immediately.
        for (int i = 0; i < 16; ++i) {
            if ((est.pieces & (1 << i)) == 0) continue;
            if ((winning_values & AttributeValues(i)) != 0) continue;
            moves[n++] = i;
        }
    } else {
        // We must place a piece. Any empty field works.
        for (int i = 0; i < 16; ++i) {
            if (est.fields[i] < 0) {
                moves[n++] = i;
            }
        }
    }
    return n;
}

void Select(EnhancedState &est, int piece) {
    assert(piece >= 0 && piece < 16);
    assert(est.next_piece == -1);
    assert(est.pieces & (1 << piece));
    est.next_piece = piece;
    est.pieces -= (1 << piece);
}

void Place(EnhancedState &est, int field) {
    assert(field >= 0 && field < 16);
    assert(est.next_piece >= 0);
    assert(est.fields[field] < 0);
    int piece = est.next_piece;
    est.next_piece = -1;
    est.fields[field] = piece;
    unsigned values = AttributeValues(piece);
    for (signed char *p = lines_per_field[field]; *p >= 0; ++p) {
        LineInfo &line = est.lines[*p];
        line.spaces_left -= 1;
        line.common_values &= values;
    }
}

}  // namespace

namespace ai_internal {

class Node {
public:
    explicit Node(const EnhancedState &est);
    Node(const EnhancedState &est, int move);
    Outcome Fix(Result result);
    Node &ExpandChild();

    EnhancedState est;

    // Number of times this node was visited.
    int visits = 0;

    // Number of visits that resulted in a win/loss.
    //
    // If fixed_value is set, these fields will not be updated anymore
    // and should no longer be used to estimate the node's value.
    int wins = 0, losses = 0;

    // Exact value of the node, if known.
    std::optional<Result> fixed_value = std::nullopt;

    // Successor states.
    int num_moves;
    int num_expanded = 0;  // 0 <= num_expanded <= num_moves
    std::array<int, 16> moves;  // only first num_moves elements are valid
    std::array<std::unique_ptr<Node>, 16> children;  // only first num_expanded elements are valid
};

Node::Node(const EnhancedState &est) : est(est) {
    num_moves = ListNonlosingMoves(est, moves);
}

Node::Node(const EnhancedState &parent_est, int move) : est(parent_est) {
    if (est.next_piece < 0) Select(est, move); else Place(est, move);
    num_moves = ListNonlosingMoves(est, moves);
}

Outcome Node::Fix(Result result) {
    assert(!fixed_value);
    fixed_value = result;
    // For debugging:
    wins = result == Result::WIN;
    losses = result == Result::LOSS;
    // We might want to clear child nodes to reclaim memory, but we should not
    // do that for the root node, since we want to look at its children later
    // to find the best move. Also, it would prevent reusing the tree for later
    // searches.
    //while (num_expanded > 0) children[--num_expanded] = nullptr;
    return Outcome{result, true};
}

Node &Node::ExpandChild() {
    assert(num_expanded < num_moves);
    int i = num_expanded++;
    std::unique_ptr<Node> &child = children[i];
    child = std::make_unique<Node>(est, moves[i]);
    return *child;
}

}  // namespace ai_internal

namespace {

// Simulates a random playout.
Result PlayOut(EnhancedState est, random_engine_t &random_engine) {
    Result win = Result::WIN;
    while (est.next_piece >= 0 || est.pieces != 0) {
        std::array<int, 16> moves;
        int num_moves = ListNonlosingMoves(est, moves);
        if (est.next_piece < 0) {
            if (num_moves == 0) {
                return Invert(win);
            }
            int piece = moves[RandomIndex(num_moves, random_engine)];
            Select(est, piece);
            win = Invert(win);
        } else {
            assert(num_moves > 0);
            int field = moves[RandomIndex(num_moves, random_engine)];
            Place(est, field);
        }
    }
    // All pieces have been placed, but nobody won. It's a tie!
    return Result::TIE;
}

Outcome ExpandTree(Node &node, random_engine_t &random_engine) {
    ++node.visits;
    if (node.fixed_value) {
        return Outcome{*node.fixed_value, true};
    }
    if (node.visits == 1) {
        Result result = PlayOut(node.est, random_engine);
        if (result == Result::WIN) ++node.wins;
        if (result == Result::LOSS) ++node.losses;
        return Outcome{result, false};
    }
    assert(node.num_moves > 0);
    Node *child_ptr = nullptr;
    bool child_value_fixed_before = false;
    if (node.num_expanded < node.num_moves) {
        // Expand new child node.
        Node &child = node.ExpandChild();
        if (child.num_moves == 0) {
            assert(child.est.next_piece < 0);
            child.Fix(child.est.pieces ? Result::LOSS : Result::TIE);
        }
        child_ptr = &child;
    } else {
        // Select child node to revisit.
        double best_v = -1e99;
        int best_i = -1;
        for (int i = 0; i < node.num_moves; ++i) {
            const Node &child = *node.children[i];
            assert(child.fixed_value || child.visits > 0);
            double expected_value =
                child.fixed_value ? GameValue(*child.fixed_value) :
                1.0 * (child.wins - child.losses) / child.visits;
            // TODO: maybe add some randomness for tie-breaking here?
            // better shuffle moves when generating them.
            double variance = sqrt(exploration_factor * log(node.visits) / child.visits);
            double v = expected_value + variance;
            if (v > best_v) {
                best_v = v;
                best_i = i;
            }
        }
        assert(best_i >= 0);
        child_ptr = node.children[best_i].get();
        child_value_fixed_before = child_ptr->fixed_value.has_value();
    }
    assert(child_ptr != nullptr);
    Node &child = *child_ptr;
    Outcome child_outcome = ExpandTree(child, random_engine);

    const Outcome outcome = node.est.next_piece < 0 ? Invert(child_outcome) : child_outcome;
    if (!child_value_fixed_before && outcome.fixed) {
        assert(child_outcome.result == child.fixed_value);
        // Child's value just became fixed. Try to fix parent's value, too.
        if (outcome.result == Result::WIN) {
            // Win for current player!
            return node.Fix(outcome.result);
        }
        // See if all child nodes values are fixed. If so, we can fix the parent
        // node's value too. In that case, the value must be <= 0 because if
        // there is any child node that could cause the parent to win, it would
        // have been handled by the if-statement above.
        if (node.num_expanded == node.num_moves) {
            bool all_children_fixed = true;
            int min_child_value = 1;
            int max_child_value = -1;
            for (int j = 0; j < node.num_expanded; ++j) {
                if (!node.children[j]->fixed_value) {
                    all_children_fixed = false;
                    break;
                }
                min_child_value = std::min(min_child_value, GameValue(*node.children[j]->fixed_value));
                max_child_value = std::max(max_child_value, GameValue(*node.children[j]->fixed_value));
            }
            if (all_children_fixed) {
                int max_value = node.est.next_piece < 0 ? -min_child_value : max_child_value;
                assert(max_value <= 0);
                return node.Fix(max_value < 0 ? Result::LOSS : Result::TIE);
            }
        }
    }
    if (outcome.result == Result::WIN) ++node.wins;
    if (outcome.result == Result::LOSS) ++node.losses;
    return Outcome{outcome.result, false};
}

Move GetBestMoveFromFixedNode(const Node &node, random_engine_t &random_engine) {
    assert(node.fixed_value);
    Result child_result = node.est.next_piece < 0 ? Invert(*node.fixed_value) : *node.fixed_value;
    std::vector<Move> possible_moves;
    for (int i = 0; i < node.num_expanded; ++i) {
        if (node.children[i]->fixed_value == child_result) {
            possible_moves.push_back(
                node.est.next_piece < 0 ?
                    Move::Select(node.moves[i]) :
                    Move::Place(node.moves[i]));
        }
    }
    assert(!possible_moves.empty());
    return RandomMove(possible_moves, random_engine);
}

Move GetBestMove(Node &node, random_engine_t &random_engine) {
    assert(node.num_moves > 0);
    // Run a large number of Monte Carlo simulations.
    for (int it = 0; it < iterations_per_move; ++it) {
        if (node.fixed_value) {
            std::cout << "(AI) Root node has fixed value: " << (int)*node.fixed_value << std::endl;
            return GetBestMoveFromFixedNode(node, random_engine);
        }
        ExpandTree(node, random_engine);
    }
    // Find the most-visited child node, and return the corresponding move.
    double expected_value = 0.0/0.0;  // for debug printing
    int max_visits = -1;
    int best_move = -1;
    for (int i = 0; i < node.num_expanded; ++i) {
        int move = node.moves[i];
        const Node& child = *node.children[i];
        if (child.visits > max_visits) {
            max_visits = child.visits;
            best_move = move;
            if (debug_print_expected_value) {
                expected_value = child.fixed_value ? GameValue(*child.fixed_value) :
                        1.0*(child.wins - child.losses)/child.visits;
            }
        }
        if (debug_print_moves) {
            std::cout << "(AI) Move " << move << ": ";
            std::cout << '(' << child.wins << " - " << child.losses << ") / " << child.visits << '\n';
        }
    }
    if (debug_print_moves) {
        for (int i = node.num_expanded; i < node.num_moves; ++i) {
            std::cout << "(AI) Move " << node.moves[i] << " unexpanded\n";
        }
    }
    if (debug_print_expected_value) {
        if (node.est.next_piece < 0) expected_value = -expected_value;
        std::cout << "(AI) Expected value: "
                << std::fixed << std::setprecision(3) << expected_value
                << std::endl;
    }
    return node.est.next_piece < 0 ? Move::Select(best_move) : Move::Place(best_move);
}

}  // namespace

AiMcts::AiMcts(const State &state) : state(state), random_engine(SeedRandomEngine()) {}

AiMcts::~AiMcts() = default;

bool AiMcts::Execute(Move move) {
    if (!state.Execute(move)) {
        return false;
    }

    // Update `root` to selected child node, or reset it to nullptr if we don't
    // have a matching expanded child (e.g. because the move was losing).
    std::unique_ptr<Node> new_root = nullptr;
    if (root && (
            move.GetType() == Move::Type::SELECT ||
            move.GetType() == Move::Type::PLACE)) {
        int move_index =
                std::find(
                        root->moves.begin(),
                        root->moves.begin() + root->num_expanded,
                        move.GetType() == Move::Type::SELECT ?
                                move.SelectedPiece() :
                                move.PlacedField()) -
                root->moves.begin();
        if (move_index < root->num_expanded) {
            new_root = std::move(root->children[move_index]);
            assert(new_root);
        }
    }
    root = std::move(new_root);
    return true;
}

Move AiMcts::CalculateMove() {
    assert(!state.Over());
    if (state.IsQuartoPossible()) {
        return Move::Quarto();
    }
    NextAction next_action = state.NextAction();
    if (next_action == NextAction::PASS) {
        return Move::Pass();
    }
    if (next_action == NextAction::PLACE) {
        // See if we can place somewhere to win.
        std::vector<Move> winning_moves;
        for (Move move : state.ListValidMoves()) {
            State next_state = state;
            next_state.ExecuteValid(move);
            if (next_state.IsQuartoPossible()) {
                std::cout << "(AI) Found winning move: place at " << move.PlacedField() << '\n';
                winning_moves.push_back(move);
            }
        }
        if (!winning_moves.empty()) {
            return RandomMove(winning_moves, random_engine);
        }
    }

    if (!root) {
        std::cout << "(AI) Recreating root node...\n";
        root = std::make_unique<ai_internal::Node>(EnhanceState(state));
    }
    if (root->num_moves == 0) {
        // All moves are losing. Pick one at random.
        std::cout << "(AI) Loss is imminent! :-(\n";
        return RandomMove(state.ListValidMoves(), random_engine);
    }
    return GetBestMove(*root, random_engine);
}
