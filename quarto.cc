#include "quarto.h"

bool State::Select(int piece) {
    CheckPiece(piece);
    if (NextAction() != NextAction::SELECT) return false;
    if (!Available(piece)) return false;
    last_piece = piece;
    pieces[piece] = false;
    num_moves++;
    return true;
}

bool State::Place(int field) {
    CheckField(field);
    if (NextAction() != NextAction::PLACE) return false;
    if (!Empty(field)) return false;
    CheckPiece(last_piece);
    last_field = field;
    fields[field] = last_piece;
    num_moves++;
    return true;
}

bool State::Pass() {
    if (num_moves < 32 || Over()) return false;
    ++num_moves;
    return true;
}

bool State::Quarto() {
    if (num_moves < 2 || Over()) return false;
    CheckField(last_field);
    CheckPiece(last_field);
    // TODO: validate quarto!
    quarto = true;
    ++num_moves;
    return true;
}
