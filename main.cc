#include "quarto.h"

#include <assert.h>
#include <ctype.h>

#include <array>
#include <algorithm>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

using Move = State::Move;

const char *palette = "ab+-01xy";

const std::array<std::string, 16> piece_ids = []{
    std::array<std::string, 16> ids;
    for (int i = 0; i < 16; ++i) {
        ids[i].resize(4);
        for (int j = 0; j < 4; ++j) {
            ids[i][j] = palette[2*j + ((i >> (3 - j)) & 1)];
        }
    }
    return ids;
}();

std::string Rtrim(const std::string &s, char ch) {
    size_t i = s.size();
    while (i > 0 && s[i - 1] == ch) --i;
    return s.substr(0, i);
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

void DrawState(std::ostream &os, const State &state) {
    std::vector<std::string> grid(14, std::string(80, ' '));
    auto draw_piece = [&grid](int r, int c, int i) {
        grid[r + 0][c + 0] = i < 0 ? '.' : piece_ids[i][0];
        grid[r + 0][c + 1] = i < 0 ? '.' : piece_ids[i][1];
        grid[r + 1][c + 0] = i < 0 ? '.' : piece_ids[i][2];
        grid[r + 1][c + 1] = i < 0 ? '.' : piece_ids[i][3];
    };
    // Draw board.
    {
        int r1 = 0, c1 = 4, r2 = 12, c2 = 21;
        // Draw border.
        for (int c = c1 + 1; c < c2; ++c) grid[r1][c] = grid[r2][c] = '-';
        for (int r = r1 + 1; r < r2; ++r) grid[r][c1] = grid[r][c2] = '|';
        grid[r1][c1] = grid[r1][c2] = grid[r2][c1] = grid[r2][c2] = '+';
        // Draw column/row labels.
        for (int n = 0; n < 4; ++n) grid[r1 + 3*n + 1][c1 - 2] = '4' - n;
        for (int n = 0; n < 4; ++n) grid[r2 + 1][c1 + 4*n + 3] = 'A' + n;
        // Draw fields.
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                draw_piece(r1 + 3*r + 1, c1 + 4*c + 2, state.PieceAt(4*r + c));
            }
        }
        int i = state.LastField();
        if (i >= 0) {
            int r = i / 4;
            int c = i % 4;
            grid[r1 + 3*r + 1][c1 + 4*c + 1] = grid[r1 + 3*r + 2][c1 + 4*c + 1] = '[';
            grid[r1 + 3*r + 1][c1 + 4*c + 4] = grid[r1 + 3*r + 2][c1 + 4*c + 4] = ']';
        }
    }
    // Draw available pieces.
    {
        int r1 = 1, c1 = 30;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                int i = 4*r + c;
                draw_piece(r1 + 3*r, c1 + 4*c, state.Available(i) ? i : -1);
            }
        }
    }
    // Draw action arrows.
    auto draw_arrow = [&grid](int r, int c1, int c2, char head, char mid, char tail) {
        grid[r][c1] = head;
        for (int c = c1 + 1; c < c2; ++c) grid[r][c] = mid;
        grid[r][c2] = tail;
    };
    NextAction next_action = state.NextAction();
    if (next_action == NextAction::SELECT) {
        draw_arrow(7, 24, 27, '-', '-', '>');
    }
    if (next_action == NextAction::PLACE) {
        draw_arrow(4, 24, 27, '<', '-', '-');
    }

    // Draw piece to place.
    draw_piece(5, 25, next_action == NextAction::PLACE ? state.LastPiece() : -1);

    for (const auto &line : grid) os << Rtrim(line, ' ') << '\n';
}

std::optional<Move> ParseMove(const std::string &line) {
    std::string lower = ToLower(line);
    if (lower.size() == 2 &&
            (lower[0] >= 'a' && lower[0] <= 'd') &&
            (lower[1] >= '1' && lower[1] <= '4')) {
        return Move::Place(4*('4' - lower[1]) + (lower[0] - 'a'));
    }
    if (lower == "p" || lower == "pass") {
        return Move::Pass();
    }
    if (lower == "q" || lower == "quarto") {
        return Move::Quarto();
    }
    for (int i = 0; i < piece_ids.size(); ++i) {
        if (lower == ToLower(piece_ids[i])) {
            return Move::Select(i);
        }
    }
    return std::nullopt;
}

std::ostream &operator<<(std::ostream &os, Move move) {
    switch (move.GetType()) {
    case Move::Type::SELECT:
        return os << piece_ids.at(move.SelectedPiece());
    case Move::Type::PLACE:
        {
            int field = move.PlacedField();
            if (field >= 0) {
                char a = 'A' + (field % 4);
                char b = '4' - (field / 4);
                return os << a << b;
            }
            assert(false);
            return os;
        }
    case Move::Type::QUARTO:
        return os << "quarto";
    case Move::Type::PASS:
        return os << "pass";
    }
    assert(false);
    return os;
}

bool AllMovesValid(const State &state, const std::vector<Move> &moves) {
    for (Move move : moves) {
        if (!state.IsValid(move)) return false;
    }
    return true;
}

}  // namespace

int main() {
    State state = State::Initial();
    while (!state.Over()) {
        assert(AllMovesValid(state, state.ListValidMoves()));  // sanity check

        DrawState(std::cout, state);
        std::optional<Move> move;
        while (!move) {
            std::cout << "Player " << (state.NextPlayer() + 1) << " to move. ";
            std::cout << "(Q)uarto or ";
            NextAction next_action = state.NextAction();
            if (next_action == NextAction::SELECT) {
                std::cout << "select piece";
            } else if (next_action == NextAction::PLACE) {
                std::cout << "place on field";
            } else if (next_action == NextAction::PASS) {
                std::cout << "(P)ass";
            } else {
                assert(false);
            }
            std::cout << ": " << std::flush;

            std::string line;
            if (!std::getline(std::cin, line)) {
                std::cout << "\nEnd of input! Exiting." << std::endl;
                return 0;
            }
            move = ParseMove(line);
            if (!move) {
                std::cout << "Unrecognized move: \"" << line << "\"" << std::endl;
            } else if (!state.IsValid(*move)) {
                std::cout << "Move is not allowed: \"" << line << "\"" << std::endl;
                move = std::nullopt;
            }
            if (!move) {
                std::cout << "Valid moves are:";
                for (Move move : state.ListValidMoves()) {
                    std::cout << ' ' << move;
                }
                std::cout << std::endl;
            }
        }
        state.ExecuteValid(*move);
    }

    DrawState(std::cout, state);
    std::cout << "Game over. ";
    int winner = state.Winner();
    if (winner < 0) {
        std::cout << "It's a tie.";
    } else {
        std::cout << "Player " << (winner + 1) << " won!";
    }
    std::cout << std::endl;
}
