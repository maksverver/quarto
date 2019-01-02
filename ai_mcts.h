#ifndef AI_MCTS_H_INCLUDED
#define AI_MCTS_H_INCLUDED

#include "quarto.h"
#include "ai.h"

#include <memory>
#include <random>

namespace ai_internal {
class Node;
using random_engine_t = std::mt19937;
}  // namespace ai_internal

class AiMcts : public Ai {
public:
    AiMcts(const State &state);
    ~AiMcts();
    bool Execute(Move move) override;
    Move CalculateMove() override;

private:
    using random_t = std::mt19937;
    State state;
    std::unique_ptr<ai_internal::Node> root;
    ai_internal::random_engine_t random_engine;
};

#endif /* ndef AI_MCTS_INCLUDED */
