#ifndef AI_H_INCLUDED
#define AI_H_INCLUDED

#include "quarto.h"

#include <memory>
#include <random>

namespace ai_internal {
class Node;
using random_engine_t = std::mt19937;
}  // namespace ai_internal

class Ai {
public:
    Ai(const State &state);
    ~Ai();

    bool Execute(Move move);

    Move CalculateMove();

private:
    using random_t = std::mt19937;
    State state;
    std::unique_ptr<ai_internal::Node> root;
    ai_internal::random_engine_t random_engine;
};

#endif /* ndef AI_H_INCLUDED */
