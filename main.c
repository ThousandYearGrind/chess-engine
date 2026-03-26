/*
 * Thank you to Chess Programming on YouTube for teaching me
 */

#include <stdio.h>
#define U64 unsigned long long

// enum board squares
enum squares {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1,
};

// sides or colors
enum colors {white, black};

// bit manipulation section

// operation at bit[square] in the bitboard
#define setBit(bitboard, square) (bitboard |= (1ULL << square))
#define getBit(bitboard, square) (bitboard & (1ULL << square))
#define clearBit(bitboard, square) (bitboard &= ~(1ULL << square))

void printBitboard(U64 bitboard) {
    for (int rank = 0; rank < 8; ++rank) {
        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file; // square index
            // print ranks
            if (!file)
                printf("  %d ", 8 - rank);
            printf(" %d", getBit(bitboard, square) ? 1 : 0);
        }
        printf("\n");
    }
    // print files
    printf("     a b c d e f g h\n\n");
    // print the numerical value of the bitboard
    printf("Bitboard: %llu (decimal) \n\n", bitboard);
}

// attacks section

// file masks
const U64 notFileA = 18374403900871474942ULL;
const U64 notFileB = 18302063728033398269ULL;
const U64 notFileC = 18157383382357244923ULL;
const U64 notFileD = 17868022691004938231ULL;
const U64 notFileE = 17289301308300324847ULL;
const U64 notFileF = 16131858542891098079ULL;
const U64 notFileG = 13816973012072644543ULL;
const U64 notFileH = 9187201950435737471ULL;
const U64 notFileAB = notFileA & notFileB;
const U64 notFileGH = notFileG & notFileH;

// pawn attacks table [color][square]
U64 pawnAttacksTable[2][64];
// knight attacks table [square]
U64 knightAttacksTable[64];
// king attacks table [square]
U64 kingAttacksTable[64];
// bishops attacks table [square]
U64 bishopAttacksTable[64];

// generate pawn attacks
U64 maskPawnAttacks(int square, int color) {
    // result attacks bitboard
    U64 attacks = 0ULL;
    // piece bitboard
    U64 bitboard = 0ULL;
    // set piece on board
    setBit(bitboard, square);
    // white pawns
    if (color == white) {
        attacks |= (bitboard & notFileH) >> 7;
        attacks |= (bitboard & notFileA) >> 9;
    }
    // black pawns
    else {
        attacks |= (bitboard & notFileH) << 9;
        attacks |= (bitboard & notFileA) << 7;
    }
    return attacks;
}
// generate knight attacks
U64 maskKnightAttacks(int square) {
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;
    setBit(bitboard, square);
    if (bitboard & notFileH) {
        attacks |= (bitboard << 17); // down 2 right 1
        attacks |= (bitboard >> 15); // up 2 right 1
        if (bitboard & notFileGH) {
            attacks |= (bitboard << 10); // down 1 right 2
            attacks |= (bitboard >> 6); // up 1 right 2
        }
    }
    if (bitboard & notFileA) {
        attacks |= (bitboard << 15); // down 2 left 1
        attacks |= (bitboard >> 17); // up 2 left 1
        if (bitboard & notFileAB) {
            attacks |= (bitboard >> 10); // up 1 left 2
            attacks |= (bitboard << 6); // down 1 left 2
        }
    }
    return attacks;
}
// generate king attacks
U64 maskKingAttacks(int square) {
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;
    setBit(bitboard, square);
    if (bitboard & notFileH) {
        attacks |= bitboard << 9;
        attacks |= bitboard << 1;
        attacks |= bitboard >> 7;
    }
    if (bitboard & notFileA) {
        attacks |= bitboard >> 9;
        attacks |= bitboard >> 1;
        attacks |= bitboard << 7;
    }
    attacks |= bitboard << 8;
    attacks |= bitboard >> 8;
    return attacks;
}
// generate bishops attack
U64 maskBishopAttacks(int square) {
    U64 attacks = 0ULL;
    return attacks;
}

void initAttackTables() {
    for (int square = 0; square < 64; ++square) {
        // pawn attack tables
        pawnAttacksTable[white][square] = maskPawnAttacks(square, white);
        pawnAttacksTable[black][square] = maskPawnAttacks(square, black);
        // knight attack tables
        knightAttacksTable[square] = maskKnightAttacks(square);
        // bishop attack tables
        // rook attack tables
        // queen attack tables
        // king attack tables
        kingAttacksTable[square] = maskKingAttacks(square);
    }
}

int main(void) {
    initAttackTables();

    for (int square = 0; square < 64; ++square) {
        printBitboard(kingAttacksTable[square]);
    }
    return 0;
}