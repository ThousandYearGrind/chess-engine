/*
 * Thank you to Chess Programming on YouTube for teaching me
 */

#include <stdio.h>
#define U64 unsigned long long

// enum board squares
// the squares are assigned according to the big endian convention
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

const char * const square_coordinates[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};

// sides or colors
enum colors {white, black};

// random numbers
// pseudo-random number state
unsigned int random_state = 1804289383;

unsigned int get_random_int32_number() {
    unsigned int num = random_state;

    // XOR shift 32 algorithm
    num ^= num << 13;
    num ^= num >> 17;
    num ^= num << 5;
    random_state = num;
    return num;
}

/*
 * 64-bit pseudo-legal numbers
 */
U64 get_random_U64_numbers() {
    U64 n1 = ((U64) get_random_int32_number()) & 0xFFFF;
    U64 n2 = ((U64) get_random_int32_number()) & 0xFFFF;
    U64 n3 = ((U64) get_random_int32_number()) & 0xFFFF;
    U64 n4 = ((U64) get_random_int32_number()) & 0xFFFF;

    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

U64 generate_magic_number() {
    return get_random_U64_numbers() & get_random_U64_numbers() & get_random_U64_numbers();
}

// bit manipulation section

// operation at bit[square] in the bitboard
static inline U64 set_bit(U64 *bitboard, int square) {
    return (*bitboard) |= (1ULL << square);
}

static inline U64 get_bit(U64 bitboard, int square) {
    return bitboard & (1ULL << square);
}

static inline U64 clear_bit(U64 *bitboard, int square) {
    return (*bitboard) &= ~(1ULL << square);
}

static inline int sq(int rank, int file) {
    return (rank << 3) + file;
}

static inline int count_bits(U64 bitboard) {
    int count = 0;
    while (bitboard) {
        count++;
        bitboard &= bitboard - 1;
    }
    return count;
}

static inline int get_lsb_index(U64 bitboard) {
    if (bitboard) {
        return count_bits((bitboard & -bitboard) - 1);
    } else {
        printf("illegal input 0ULL");
        return -1;
    }
}

void print_bitboard(U64 bitboard) {
    for (int rank = 0; rank < 8; ++rank) {
        for (int file = 0; file < 8; ++file) {
            int square = sq(rank, file); // square index
            // print ranks
            if (!file)
                printf("  %d ", 8 - rank);
            printf(" %d", get_bit(bitboard, square) ? 1 : 0);
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
const U64 not_file_a = 18374403900871474942ULL;
const U64 not_file_b = 18302063728033398269ULL;
const U64 not_file_g = 13816973012072644543ULL;
const U64 not_file_h = 9187201950435737471ULL;
const U64 not_file_ab = not_file_a & not_file_b;
const U64 not_file_gh = not_file_g & not_file_h;

const int bishop_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6,
};

const int rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12,
};

// pawn attacks table [color][square]
U64 pawn_attacks_table[2][64];
// knight attacks table [square]
U64 knight_attacks_table[64];
// king attacks table [square]
U64 king_attacks_table[64];
// bishops attacks table [square]
U64 bishop_attacks_table[64];

// generate pawn attacks
U64 mask_pawn_attacks(int square, int color) {
    // result attacks bitboard
    U64 attacks = 0ULL;
    // piece bitboard
    U64 bitboard = 0ULL;
    // set piece on board
    set_bit(&bitboard, square);
    // white pawns
    if (color == white) {
        attacks |= (bitboard & not_file_h) >> 7;
        attacks |= (bitboard & not_file_a) >> 9;
    }
    // black pawns
    else {
        attacks |= (bitboard & not_file_h) << 9;
        attacks |= (bitboard & not_file_a) << 7;
    }
    return attacks;
}

// generate knight attacks
U64 mask_knight_attacks(int square) {
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;
    set_bit(&bitboard, square);
    if (bitboard & not_file_h) {
        attacks |= (bitboard << 17); // down 2 right 1
        attacks |= (bitboard >> 15); // up 2 right 1
        if (bitboard & not_file_gh) {
            attacks |= (bitboard << 10); // down 1 right 2
            attacks |= (bitboard >> 6); // up 1 right 2
        }
    }
    if (bitboard & not_file_a) {
        attacks |= (bitboard << 15); // down 2 left 1
        attacks |= (bitboard >> 17); // up 2 left 1
        if (bitboard & not_file_ab) {
            attacks |= (bitboard >> 10); // up 1 left 2
            attacks |= (bitboard << 6); // down 1 left 2
        }
    }
    return attacks;
}

// generate king attacks
U64 mask_king_attacks(int square) {
    U64 attacks = 0ULL;
    U64 bitboard = 0ULL;
    set_bit(&bitboard, square);
    if (bitboard & not_file_h) {
        attacks |= bitboard << 9;
        attacks |= bitboard << 1;
        attacks |= bitboard >> 7;
    }
    if (bitboard & not_file_a) {
        attacks |= bitboard >> 9;
        attacks |= bitboard >> 1;
        attacks |= bitboard << 7;
    }
    attacks |= bitboard << 8;
    attacks |= bitboard >> 8;
    return attacks;
}

/* this function produces a mask of the relevant squares when determining how far the bishop can slide
 *
 * square - the square that the bishop is on
 */
U64 mask_bishop_attacks(int square) {
    U64 attacks = 0ULL;
    int curRank = square / 8;
    int curFile = square % 8;

    int r, f;
    for (r = curRank + 1, f = curFile + 1; r <= 6 && f <= 6; ++r, ++f) set_bit(&attacks, sq(r, f));
    for (r = curRank + 1, f = curFile - 1; r <= 6 && f >= 1; ++r, --f) set_bit(&attacks, sq(r, f));
    for (r = curRank - 1, f = curFile + 1; r >= 1 && f <= 6; --r, ++f) set_bit(&attacks, sq(r, f));
    for (r = curRank - 1, f = curFile - 1; r >= 1 && f >= 1; --r, --f) set_bit(&attacks, sq(r, f));

    return attacks;
}

/* this function returns a mask of the squares that a bishop at a certain square can attack
 *
 * square - the square that the bishop is on
 * board - the pieces on the board around the bishop
 */
U64 generate_bishop_attacks(int square, U64 board) {
    U64 attacks = 0ULL;
    int curRank = square / 8;
    int curFile = square % 8;

    int r, f;
    for (r = curRank + 1, f = curFile + 1; r <= 7 && f <= 7; ++r, ++f) {
        set_bit(&attacks, sq(r, f));
        if (get_bit(board, sq(r, f))) break;
    }
    for (r = curRank + 1, f = curFile - 1; r <= 7 && f >= 0; ++r, --f) {
        set_bit(&attacks, sq(r, f));
        if (get_bit(board, sq(r, f))) break;
    }
    for (r = curRank - 1, f = curFile + 1; r >= 0 && f <= 7; --r, ++f) {
        set_bit(&attacks, sq(r, f));
        if (get_bit(board, sq(r, f))) break;
    }
    for (r = curRank - 1, f = curFile - 1; r >= 0 && f >= 0; --r, --f) {
        set_bit(&attacks, sq(r, f));
        if (get_bit(board, sq(r, f))) break;
    }

    return attacks;
}

/* this function produces a mask of the relevant squares when
 * determining how far the rook can slide
 *
 * square - the square that the rook is on
 */
U64 mask_rook_attacks(int square) {
    U64 attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    int r, f;
    for (r = rank + 1, f = file; r <= 6; ++r) set_bit(&attacks, sq(r, f));
    for (r = rank - 1, f = file; r >= 1; --r) set_bit(&attacks, sq(r, f));
    for (r = rank, f = file + 1; f <= 6; ++f) set_bit(&attacks, sq(r, f));
    for (r = rank, f = file - 1; f >= 1; --f) set_bit(&attacks, sq(r, f));

    return attacks;
}

/* this function returns a mask of the squares that a rook at a certain square can attack
 *
 * square - the square that the rook is on
 * board - the pieces on the board around the rook
 */
U64 generate_rook_attacks(int square, U64 board) {
    U64 attacks = 0ULL;
    int rank = square / 8;
    int file = square % 8;

    int r, f;
    for (r = rank + 1, f = file; r <= 7; ++r) {
        set_bit(&attacks, sq(r, f));
        if (get_bit(board, sq(r, f))) break;
    }
    for (r = rank - 1, f = file; r >= 0; --r) {
        set_bit(&attacks, sq(r, f));
        if (get_bit(board, sq(r, f))) break;
    }
    for (r = rank, f = file + 1; f <= 7; ++f) {
        set_bit(&attacks, sq(r, f));
        if (get_bit(board, sq(r, f))) break;
    }
    for (r = rank, f = file - 1; f >= 0; --f) {
        set_bit(&attacks, sq(r, f));
        if (get_bit(board, sq(r, f))) break;
    }

    return attacks;
}

/*e
 * this function is a map from the binary representation of the parameter <<index>>
 * to the possible bit arrangements along the relevant bits of the attack mask
 *
 * the range of values for index is [0, 2^(bits_in_mask) - 1]
 */
U64 set_occupancy(int index, int bits_in_mask, U64 attack_mask) {
    U64 occupancy = 0ULL;

    for (int count = 0; count < bits_in_mask; ++count) {
        // get lsb index of attack mask
        int square = get_lsb_index(attack_mask);
        clear_bit(&attack_mask, square);
        if (index & (1 << count)) set_bit(&occupancy, square);
    }
    return occupancy;
}

void init_attack_tables() {
    for (int square = 0; square < 64; ++square) {
        // pawn attack tables
        pawn_attacks_table[white][square] = mask_pawn_attacks(square, white);
        pawn_attacks_table[black][square] = mask_pawn_attacks(square, black);
        // knight attack tables
        knight_attacks_table[square] = mask_knight_attacks(square);
        // bishop attack tables
        // rook attack tables
        // queen attack tables
        // king attack tables
        king_attacks_table[square] = mask_king_attacks(square);
    }
}

int main(void) {
    init_attack_tables();
    return 0;
}
