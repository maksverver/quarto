#ifndef AI_H_INCLUDED
#define AI_H_INCLUDED

#include "quarto.h"

class Ai {
public:
    virtual ~Ai() = default;
    virtual bool Execute(Move move) = 0;
    virtual Move CalculateMove() = 0;
};

#endif /* ndef AI_H_INCLUDED */
