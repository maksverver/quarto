#ifndef QUARTO_H_INCLUDED
#define QUARTO_H_INCLUDED

#include <assert.h>

enum class NextAction {
    NONE,    // game is over -- no actions are possible
    SELECT,  // select a piece for the opponent to place (or call quarto)
    PLACE,   // place the given piece on a field (or call quarto)
    PASS,    // pass (or call quarto)
};

class State {
public:
    State() : num_moves(0), last_piece(0), last_field(0), quarto(false) {
        for (int i = 0; i < 16; ++i) {
            fields[i] = 0;
        }
        for (int i = 0; i < 16; ++i) {
            pieces[i] = true;
        }
    }

    State(const State&) = default;
    State &operator=(const State&) = default;

    // Query the game state.
    int PreviousPlayer() { return ((num_moves) >> 1) & 1; }
    int NextPlayer() { return ((num_moves + 1) >> 1) & 1; }
    inline ::NextAction NextAction();
    bool Over() { return quarto || num_moves >= 34; }
    int Winner() { return quarto ? PreviousPlayer() : -1; }
    bool Empty(int field) { return fields[CheckField(field)] < 0; }
    int PieceAt(int field) { return fields[CheckField(field)]; }
    bool Available(int piece) { return pieces[CheckPiece(piece)]; }

    // Update the game state.
    bool Select(int piece);
    bool Place(int field);
    bool Quarto();
    bool Pass();

private:
    static inline int CheckField(int field) {
        assert(field >= 0 && field < 16);
        return field;
    }

    static inline int CheckPiece(int piece) {
        assert(piece >= 0 && piece < 16);
        return piece;
    }

    // Number of moves played so far. 0 through 34 (inclusive).
    // Selecting a piece and placing it count as separate moves.
    // The number of moves determines whose turn it is and what their next
    // action should be (select a piece, or place on a field).
    // After 32 moves, each player can either pass or call Quarto.
    // After 34 moves the game ends in a draw.
    char num_moves;

    // Last piece selected (either on previous move, or the one before that).
    // -1 at the beginning of the game.
    signed char last_piece;

    // Last field selected (either on previous move, or the one before that).
    // -1 at the beginning of the game.
    signed char last_field;

    // True if quarto has been found.
    bool quarto;

    // Fields of the board. Each element is the number of he the piece occupying
    // this field (between 0 and 16, inclusive), or -1 if the field is empty.
    signed char fields[16];

    // Available pieces.
    bool pieces[16];
};

NextAction State::NextAction() {
    return Over() ? NextAction::NONE : num_moves >= 32 ? NextAction::PASS :
            ((num_moves + 1) & 1) ? NextAction::SELECT : NextAction::PLACE;
}

#endif /* ndef QUARTO_H_INCLUDED */
