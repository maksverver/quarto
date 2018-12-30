#ifndef QUARTO_H_INCLUDED
#define QUARTO_H_INCLUDED

#include <assert.h>

#include <array>
#include <vector>

namespace internal {

constexpr int CheckField(int field) {
    assert(field >= 0 && field < 16);
    return field;
}

constexpr int CheckPiece(int piece) {
    assert(piece >= 0 && piece < 16);
    return piece;
}

}  // namespace internal

enum class NextAction {
    NONE,    // game is over -- no actions are possible
    SELECT,  // select a piece for the opponent to place (or call quarto)
    PLACE,   // place the given piece on a field (or call quarto)
    PASS,    // pass (or call quarto)
};

class Move {
public:
    enum class Type {
        SELECT,
        PLACE,
        QUARTO,
        PASS,
    };
    static Move Select(int piece) {
        Move move(Type::SELECT);
        move.piece = internal::CheckPiece(piece);
        return move;
    }
    static Move Place(int field) {
        Move move(Type::PLACE);
        move.field = internal::CheckField(field);
        return move;
    }
    static Move Quarto() { return Move(Type::QUARTO); }
    static Move Pass() { return Move(Type::PASS); }

    Move(const Move&) = default;
    Move& operator=(const Move&) = default;

    Type GetType() { return type; }
    int SelectedPiece() { return type == Type::SELECT ? piece : -1; }
    int PlacedField() { return type == Type::PLACE ? field : -1; }

private:
    Move(Type type) : type(type) {}

    Type type;
    union {
        int piece;  // if type == SELECT
        int field;  // if type == PLACE
    };
};

class State {
public:
    static State Initial() { return State(); }

    State(const State&) = default;
    State &operator=(const State&) = default;

    // Query the game state.
    int PreviousPlayer() const { return ((num_moves) >> 1) & 1; }
    int NextPlayer() const { return ((num_moves + 1) >> 1) & 1; }
    bool Over() const { return quarto || num_moves >= 34; }
    int Winner() const { return quarto ? PreviousPlayer() : -1; }
    bool Empty(int field) const { return fields[internal::CheckField(field)] < 0; }
    int PieceAt(int field) const { return fields[internal::CheckField(field)]; }
    bool Available(int piece) const { return pieces[internal::CheckPiece(piece)]; }
    int LastField() const { return last_field; }
    int LastPiece() const { return last_piece; }
    inline ::NextAction NextAction() const;
    bool IsValid(Move move) const;
    bool IsQuartoPossible() const;
    std::vector<Move> ListValidMoves() const;

    // Update the game state.
    void ExecuteValid(Move move);
    bool Execute(Move move);

private:
    inline State();

    // Number of moves played so far. 0 through 34 (inclusive).
    // Selecting a piece and placing it count as separate moves.
    // The number of moves determines whose turn it is and what their next
    // action should be (select a piece, or place on a field).
    // After 32 moves, each player can either pass or call Quarto.
    // After 34 moves the game ends in a draw.
    char num_moves = 0;

    // Last piece selected (either on previous move, or the one before that).
    // -1 at the beginning of the game, and after the first pass.
    signed char last_piece = -1;

    // Last field selected (either on previous move, or the one before that).
    // -1 at the beginning of the game, and after the second pass.
    signed char last_field = -1;

    // True if quarto has been found.
    bool quarto = false;

    // Fields of the board. Each element is the number of the piece occupying
    // this field (between 0 and 16, inclusive), or -1 if the field is empty.
    std::array<signed char, 16> fields;

    // Available pieces.
    std::array<bool, 16> pieces;
};

NextAction State::NextAction() const {
    return Over() ? NextAction::NONE : num_moves >= 32 ? NextAction::PASS :
            ((num_moves + 1) & 1) ? NextAction::SELECT : NextAction::PLACE;
}

State::State() {
    fields.fill(-1);
    pieces.fill(true);
}

#endif /* ndef QUARTO_H_INCLUDED */
