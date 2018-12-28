#include "quarto.h"

#include <iostream>

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
    if (num_moves < 2 || Over()) return false;
    CheckField(last_field);
    // TODO: validate quarto involving `last_field`.
    return true;
}

std::vector<State::Move> State::ListValidMoves() const {
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
