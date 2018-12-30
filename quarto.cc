#include "quarto.h"

#include <iostream>

namespace {

using internal::CheckPiece;
using internal::CheckField;

// winning_lines[] encodes the horizontal, vertical and diagonal lines on the
// board along which a Quarto can be formed.
//
// Each winning line consists of exactly four fields. Each field participates in
// two or three winning lines, depending on whether the field lies on a diagonal
// or not.
//
// The array below encodes the winning lines for each of 16 fields into a 64 bit
// integer, where each group of 12 bits encodes the indices of three other
// fields that are part of the same line (in addition to the field itself).
//
// For example, field 0 can form a winning line with fields 1, 2 and 3
// (horizontally) or 4, 8, and 12 (vertically), or 5, 10, 15 (diagonally),
// so the first element is 0xfa5c84321.
static const unsigned long long winning_lines[16] = {
    0xfa5c84321, 0x000d95320, 0x000ea6310, 0xc96fb7210,
    0x000c80765, 0xfa0d91764, 0xc93ea2754, 0x000fb3654,
    0x000c40ba9, 0xc63d51ba8, 0xf50e62b98, 0x000f73a98,
    0x963840fed, 0x000951fec, 0x000a62fdc, 0xa50b73edc };

// Pieces have four attributes (color, height, etc.) each of which has two
// possible values (black/white, short/tall, etc.) This function takes a piece
// number (a 4-bit integer) and returns an 8-bit bitmask with exactly 4 bits
// set, corresponding with the attribute values for this piece.
constexpr unsigned GetAttributeValues(int piece) {
    assert(piece >= 0 && piece < 16);
    return (piece << 4) | (piece ^ 0xf);
}

}  //namespace

bool State::IsValid(Move move) const {
    switch (move.GetType()) {
    case Move::Type::SELECT:
        if (NextAction() != NextAction::SELECT) return false;
        if (!Available(move.SelectedPiece())) return false;
        return true;
    case Move::Type::PLACE:
        if (NextAction() != NextAction::PLACE) return false;
        if (!Empty(move.PlacedField())) return false;
        return true;

    case Move::Type::QUARTO:
        if (!IsQuartoPossible()) return false;
        return true;

    case Move::Type::PASS:
        if (num_moves < 32 || Over()) return false;
        return true;
    }
    assert(false);
    return false;
}

bool State::IsQuartoPossible() const {
    if (num_moves < 8 || Over()) return false;
    assert(last_field >= 0);
    int first_piece = fields[last_field];
    assert(first_piece >= 0);
    for (unsigned long long lines = winning_lines[last_field]; lines != 0;
            lines >>= 12) {
        unsigned line = lines & 0xfff;
        unsigned common_attributes = GetAttributeValues(first_piece);
        int n = 3;
        for (; n > 0; --n) {
            int next_piece = fields[line & 0xf];
            if (next_piece < 0) break;  // next_field unoccupied
            common_attributes &= GetAttributeValues(next_piece);
            if (common_attributes == 0) break;  // no attribute in common
            line >>= 4;
        }
        if (n == 0) {
            return true;
        }
    }
    return false;
}

std::vector<Move> State::ListValidMoves() const {
    std::vector<Move> result;
    switch (NextAction()) {
    case NextAction::SELECT:
        result.reserve(17 - (num_moves >> 1));
        for (int piece = 0; piece < 16; ++piece) {
            if (Available(piece)) {
                result.push_back(Move::Select(piece));
            }
        }
        break;
    case NextAction::PLACE:
        result.reserve(17 - (num_moves >> 1));
        for (int field = 0; field < 16; ++field) {
            if (Empty(field)) {
                result.push_back(Move::Place(field));
            }
        }
        break;
    case NextAction::PASS:
        result.reserve(2);
        result.push_back(Move::Pass());
        break;
    case NextAction::NONE:
        return result;
    }
    if (IsQuartoPossible()) {
        result.push_back(Move::Quarto());
    }
    return result;
}

bool State::Execute(Move move) {
    if (!IsValid(move)) return false;
    ExecuteValid(move);
    return true;
}

void State::ExecuteValid(Move move) {
    switch (move.GetType()) {
    case Move::Type::SELECT:
        last_piece = move.SelectedPiece();
        pieces[last_piece] = false;
        break;
    case Move::Type::PLACE:
        last_field = move.PlacedField();
        fields[last_field] = CheckPiece(last_piece);
        break;
    case Move::Type::QUARTO:
        quarto = true;
        break;
    case Move::Type::PASS:
        if (num_moves == 32) last_piece = -1;
        if (num_moves == 33) last_field = -1;
        break;
    }
    num_moves++;
}
