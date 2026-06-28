/*
 * Thank you to Chess Programming on YouTube for teaching me
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN64
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

#define U64 unsigned long long

// FEN dedug positions
#define empty_board "8/8/8/8/8/8/8/8 b - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "

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
    a1, b1, c1, d1, e1, f1, g1, h1, no_square,
};

const char * const square_coordinates[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",};

// sides or colors
enum {white, black, both};

enum {rook, bishop};
// castling
enum {wkc = 1, wqc = 2, bkc = 4, bqc = 8};
// piece types for each color
enum { wp, wn, wb, wr, wq, wk, bp, bn, bb, br, bq, bk };

char ascii_pieces[12] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b',
    'r', 'q', 'k' };

int char_pieces[] = { ['P'] = wp, ['N'] = wn, ['B'] = wb, ['R'] = wr,
    ['Q'] = wq, ['K'] = wk, ['p'] = bp, ['n'] = bn, ['b'] = bb, ['r']
    = br, ['q'] = bq, ['k'] = bk };

char promoted_pieces[] = { [wq] = 'Q', [wr] = 'R', [wb] = 'B', [wn] =
    'N', [wp] = 'P', [wk] = 'K', [bq] = 'q', [br] = 'r', [bb] = 'b',
    [bn] = 'n', [bp] = 'p', [bk] = 'k' };

// bitboards
U64 bitboards[12];
U64 occupancies[3];
static inline void occ() {
    occupancies[white] = bitboards[wp] | bitboards[wb] | bitboards[wr]
    | bitboards[wn] | bitboards[wq] | bitboards[wk];
    occupancies[black] = bitboards[bp] | bitboards[bb] | bitboards[br]
    | bitboards[bn] | bitboards[bq] | bitboards[bk]; occupancies[both]
    = occupancies[black] | occupancies[white];
}

// side to move
int side = -1;
int en_passant = no_square;
/*
 * 1 - king side castle for white
 * 2 - queen side castle for white
 * 4 - king side castle for black
 * 8 - queen side castle for black
 *
 * 1111 (bin) - all castles are valid
 */
int castle;

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
    printf("\n");
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
    printf("\n     a b c d e f g h\n\n");
    // print the numerical value of the bitboard
    printf("Bitboard: %llu (decimal) \n\n", bitboard);
}

void print_board() {
    printf("\n");
    for (int rank = 0; rank < 8; ++rank) {
        for (int file = 0; file < 8; ++file) {
            if (file == 0) {
                printf("  %d ", 8 - rank);
            }
            int square = sq(rank, file);
            int piece = -1;

            for (int bbs_index = wp; bbs_index <= bk; ++bbs_index) {
                if (get_bit(bitboards[bbs_index], square)) {
                    piece = bbs_index;
                    break;
                }
            }

            printf(" %c", (piece == -1) ? '.' : ascii_pieces[piece]);
        }
        printf("\n");
    }
    printf("\n     a b c d e f g h\n\n");
    printf(" Side: %s\n", (side == white) ? "white" : side == black ?
    "black" : "0");
    printf("     En passant:  %s\n", (en_passant == no_square) ? "no" : square_coordinates[en_passant]);
    printf("     Castling:  %c%c%c%c\n\n",
        (castle & wkc) ? 'K' : '-',
        (castle & wqc) ? 'Q' : '-',
        (castle & bkc) ? 'k' : '-',
        (castle & bqc) ? 'q' : '-');
}

void parse_fen(char *fen) {
    memset(bitboards, 0ULL, sizeof(bitboards));
    memset(occupancies, 0ULL, sizeof(occupancies));

    side = 0;
    en_passant = no_square;
    castle = 0;

    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8;) {
            int square = sq(rank, file);
            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen
            <= 'Z')) {
                int piece = char_pieces[*fen];
                set_bit(&bitboards[piece], square);
                file++;
                fen++;
            }
            else if (*fen >= '0' && *fen <= '9') {
                int offset = *fen - '0';
                file += offset;
                fen++;
            }
        }

        if (*fen == '/') fen++;
    }

    fen++;
    side = (*fen++ == 'w') ? white : black;

    fen++;
    while (*fen != ' ') {
        switch (*fen) {
            case 'K':
                castle |= wkc;
                break;
            case 'Q':
                castle |= wqc;
                break;
            case 'k':
                castle |= bkc;
                break;
            case 'q':
                castle |= bqc;
                break;
            default:
                break;
        }
        fen++;
    }

    fen++;
    if (*fen != '-') {
        int rank = 8 - (fen[1] - '0');
        int file = fen[0] - 'a';
        en_passant = sq(rank, file);
        fen += 2;
    }
    else {
        en_passant = no_square;
        fen++;
    }

    for (int piece = wp; piece <= wk; piece++) {
        occupancies[white] |= bitboards[piece];
    }

    for (int piece = bp; piece <= bk; piece++) {
        occupancies[black] |= bitboards[piece];
    }

    occupancies[both] = occupancies[white] | occupancies[black];
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

U64 rook_magic_numbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL,
};

U64 bishop_magic_numbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL,
};

U64 pawn_attacks_table[2][64];
U64 knight_attacks_table[64];
U64 king_attacks_table[64];
U64 bishop_masks[64];
U64 rook_masks[64];
U64 bishop_attacks_table[64][512];
U64 rook_attacks_table[64][4096];

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
    for (r = curRank + 1, f = curFile + 1; r <= 6 && f <= 6; ++r, ++f)
        set_bit(&attacks, sq(r, f));
    for (r = curRank + 1, f = curFile - 1; r <= 6 && f >= 1; ++r, --f)
        set_bit(&attacks, sq(r, f));
    for (r = curRank - 1, f = curFile + 1; r >= 1 && f <= 6; --r, ++f)
        set_bit(&attacks, sq(r, f));
    for (r = curRank - 1, f = curFile - 1; r >= 1 && f >= 1; --r, --f)
        set_bit(&attacks, sq(r, f));

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
    for (r = rank + 1, f = file; r <= 6; ++r)
        set_bit(&attacks, sq(r, f));
    for (r = rank - 1, f = file; r >= 1; --r)
        set_bit(&attacks, sq(r, f));
    for (r = rank, f = file + 1; f <= 6; ++f)
        set_bit(&attacks, sq(r, f));
    for (r = rank, f = file - 1; f >= 1; --f)
        set_bit(&attacks, sq(r, f));

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

U64 find_magic_number(int square, int relevant_bits, int bishop) {
    U64 occupancies[4096];
    U64 attacks[4096];
    U64 used_attacks[4096];
    U64 attack_mask = bishop ? mask_bishop_attacks(square) :
        mask_rook_attacks(square);
    int occupancy_indices = 1 << relevant_bits;

    for (int index = 0; index < occupancy_indices; ++index) {
        occupancies[index] = set_occupancy(index, relevant_bits,
            attack_mask);
        attacks[index] = bishop ?
            generate_bishop_attacks(square, occupancies[index]) :
            generate_rook_attacks(square, occupancies[index]);
    }

    // test magic numbers
    for (int random_count = 0; random_count < 100000000;
         random_count++) {
        U64 magic_number = generate_magic_number();

        // skip bad magic numbers
        if (count_bits((attack_mask * magic_number) &
                       0xFF00000000000000) < 6)
            continue;

        memset(used_attacks, 0ULL, sizeof(used_attacks));
        int index, fail;

        for (index = 0, fail = 0; !fail && index < occupancy_indices;
             index++) {
            int magic_index = (int) ((occupancies[index] *
                                      magic_number) >> (64 - relevant_bits));
            if (used_attacks[magic_index] == 0ULL)
                used_attacks[magic_index] = attacks[index];
            else if (used_attacks[magic_index] != attacks[index])
                fail = 1;
        }

        if (!fail) return magic_number;
    }
    printf("magic number fail");
    return 0ULL;
}

// init magic numbers
void init_magic_numbers() {
    for (int square = 0; square < 64; square++) {
        rook_magic_numbers[square] = find_magic_number(square, rook_relevant_bits[square], rook);
    }
    printf("\n\n");
    for (int square = 0; square < 64; square++) {
        bishop_magic_numbers[square] = find_magic_number(square, bishop_relevant_bits[square], bishop);
    }
}

void init_sliders_attacks(int bishop) {
    for (int square = 0; square < 64; square++) {
        bishop_masks[square] = mask_bishop_attacks(square);
        rook_masks[square] = mask_rook_attacks(square);

        U64 attack_mask = bishop ? bishop_masks[square] : rook_masks[square];
        int relevant_bits_count = count_bits(attack_mask);
        int occupancy_indices = (1 << relevant_bits_count);

        for (int index = 0; index < occupancy_indices; index++) {
            if (bishop) {
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                int magic_index = (int) ((occupancy * bishop_magic_numbers[square]) >> (64 - relevant_bits_count));
                bishop_attacks_table[square][magic_index] = generate_bishop_attacks(square, occupancy);
            }
            else {
                U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                int magic_index = (int) ((occupancy * rook_magic_numbers[square]) >> (64 - relevant_bits_count));
                rook_attacks_table[square][magic_index] = generate_rook_attacks(square, occupancy);
            }
        }
    }
}

/* my explanation of the so-called "magic" numbers
 *
 * at a given square, a sliding piece has only so many squares
 * the occupancy of which is relevant to its sliding
 * however these squares are scattered across the 64 bits of
 * the bitboard even though the number of configurations of
 * the relevant bits is far smaller than the number of configurations
 * of 64 bits
 *
 * hence, we multiply a number with the relevant occupancy square
 * bitboard in order to make the information in the bitboard more dense
 * and look at only the number of relevant occupancy square bits
 * to form a hashing function that maps every configuration of occupancy
 * bits to a unique number
 *
 * there is no particular meaning to finding such a number,
 * so we just find it using trial and error, generating random numbers
 * until we find one that satisfies our needs
 */

static inline U64 get_bishop_attacks(int square, U64 occupancy) {
    occupancy &= bishop_masks[square];
    occupancy *= bishop_magic_numbers[square];
    occupancy >>= 64 - bishop_relevant_bits[square];
    return bishop_attacks_table[square][occupancy];
}

static inline U64 get_rook_attacks(int square, U64 occupancy) {
    occupancy &= rook_masks[square];
    occupancy *= rook_magic_numbers[square];
    occupancy >>= 64 - rook_relevant_bits[square];
    return rook_attacks_table[square][occupancy];
}

static inline U64 get_queen_attacks(int square, U64 occupancy) {
    return get_bishop_attacks(square, occupancy) | get_rook_attacks(square, occupancy);
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
    init_sliders_attacks(bishop);
    init_sliders_attacks(rook);
}

// move generator

static inline int is_sq_attacked(int square, int side) {
    if (side == white) {
        // if an enemy piece on the given square would attack one of our own pieces
        // of the same type, then our piece is attacking that square
        if (pawn_attacks_table[black][square] & bitboards[wp]) return 1;
        if (knight_attacks_table[square] & bitboards[wn]) return 1;
        if (king_attacks_table[square] & bitboards[wk]) return 1;
        if (get_bishop_attacks(square, occupancies[both]) & (bitboards[wb] | bitboards[wq])) return 1;
        if (get_rook_attacks(square, occupancies[both]) & (bitboards[wr] | bitboards[wq])) return 1;
    }
    else if (side == black) {
        if (pawn_attacks_table[white][square] & bitboards[bp]) return 1;
        if (knight_attacks_table[square] & bitboards[bn]) return 1;
        if (king_attacks_table[square] & bitboards[bk]) return 1;
        if (get_bishop_attacks(square, occupancies[both]) & (bitboards[bb] | bitboards[bq])) return 1;
        if (get_rook_attacks(square, occupancies[both]) & (bitboards[br] | bitboards[bq])) return 1;
    }
    return 0;
}

void print_attacked_squares(int side) {
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            int square = sq(rank, file);
            if (!file) printf("  %d ", 8 - rank);
            printf(" %d", is_sq_attacked(square, side));
        }
        printf("\n");
    }

    printf("\n     a b c d e f g h\n\n");
}

/* OMNL JIHG FEDC BA98 7654 3210    bit #                    hex
 * 0000 0000 0000 0000 0011 1111    source square               0x3f
 * 0000 0000 0000 1111 1100 0000    target square               0xfc0
 * 0000 0000 1111 0000 0000 0000    piece                       0xf000
 * 0000 1111 0000 0000 0000 0000    promoted piece              0xf0000
 * 0001 0000 0000 0000 0000 0000    capture flag                0x100000
 * 0010 0000 0000 0000 0000 0000    double push flag            0x200000
 * 0100 0000 0000 0000 0000 0000    en passant flag             0x400000
 * 1000 0000 0000 0000 0000 0000    castling flag               0x800000
 */

enum {
    SOURCE = 0x3f, TARGET = 0xfc0, PIECE = 0xf000, PROMOTED = 0xf0000, CAPTURE = 0x100000, DOUBLE_PUSH = 0x200000, EN_PASSANT = 0x400000, CASTLING = 0x800000
};

static inline int encode_move(int source, int target, int piece, int promoted, int capture, int double_push, int enpassant, int castling) {
    return (source) | (target << 6) | (piece << 12) | (promoted << 16) | (capture << 20) | (double_push << 21) | (enpassant << 22) | (castling << 23);
}

static inline int get_move_source(int move) {
    return move & SOURCE;
}

static inline int get_move_target(int move) {
    return (move & TARGET) >> 6;
}

static inline int get_move_piece(int move) {
    return (move & PIECE) >> 12;
}

static inline int get_move_promoted(int move) {
    return (move & PROMOTED) >> 16;
}

static inline int get_move_capture(int move) {
    return (move & CAPTURE) >> 20;
}

static inline int get_move_double_push(int move) {
    return (move & DOUBLE_PUSH) >> 21;
}

static inline int get_move_en_passant(int move) {
    return (move & EN_PASSANT) >> 22;
}

static inline int get_move_castling(int move) {
    return (move & CASTLING) >> 23;
}

typedef struct {
    int moves[256];
    int count;
} moves;

static inline void add_move(moves *move_list, int move) {
    move_list->moves[move_list->count++] = move;
}

void print_move(int move) {
    if (get_move_promoted(move)) {
        printf("%s%s%c",
               square_coordinates[get_move_source(move)],
               square_coordinates[get_move_target(move)],
               promoted_pieces[get_move_promoted(move)]);
    }
    else {
        printf("%s%s",
               square_coordinates[get_move_source(move)],
               square_coordinates[get_move_target(move)]);
    }
}

void print_move_list(moves *move_list) {
    if (move_list->count == 0) {
        printf("No moves in move list \n");
        return;
    }

    printf("\n    move   piece  capture  double  enpassant  castling\n");
    for (int move_count = 0; move_count < move_list->count; move_count++) {
        int move = move_list->moves[move_count];
        printf("    %s%s%c  %c      %d        %d       %d          %d\n",
            square_coordinates[get_move_source(move)],
            square_coordinates[get_move_target(move)],
            get_move_promoted(move) ? promoted_pieces[get_move_promoted(move)] : ' ',
            ascii_pieces[get_move_piece(move)],
            get_move_capture(move),
            get_move_double_push(move),
            get_move_en_passant(move),
            get_move_castling(move));
    }

    printf("\n\n    Total number of moves: %d\n\n", move_list->count);
}

// sizeof(bitboards) = 96 
// sizeof(occupancies) = 24 

#define copy_board() \
    U64 bitboards_copy[12], occupancies_copy[3]; \
    int side_copy = side, en_passant_copy = en_passant, castle_copy = castle; \
    memcpy(bitboards_copy, bitboards, 96); \
    memcpy(occupancies_copy, occupancies, 24); \

#define restore_board() \
    memcpy(bitboards, bitboards_copy, 96); \
    memcpy(occupancies, occupancies_copy, 24); \
    side = side_copy, en_passant = en_passant_copy, castle = castle_copy; \

// mvoe types
enum { all_moves, only_captures };

const int castling_rights[64] = {
    7, 15, 15, 15, 3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15, 
    15, 15, 15, 15, 15, 15, 15, 15, 
    15, 15, 15, 15, 15, 15, 15, 15, 
    15, 15, 15, 15, 15, 15, 15, 15, 
    15, 15, 15, 15, 15, 15, 15, 15, 
    15, 15, 15, 15, 15, 15, 15, 15, 
    13, 15, 15, 15, 12, 15, 15, 14, 
};

static inline int make_move(int move, int move_flag) {
    if (move_flag == all_moves) {
        copy_board();

        int this_source_square = get_move_source(move);
        int this_target_square = get_move_target(move);
        int this_piece = get_move_piece(move);
        int this_promoted_piece = get_move_promoted(move);
        int this_capture = get_move_capture(move);
        int this_double_push = get_move_double_push(move);
        int this_en_passant = get_move_en_passant(move);
        int this_castling = get_move_castling(move);

        clear_bit(&bitboards[this_piece], this_source_square);
        set_bit(&bitboards[this_piece], this_target_square);

        if (this_capture) {
            int start_piece, end_piece;
            if (side == white) {
                start_piece = bp;
                end_piece = bk;
            }
            else {
                start_piece = wp;
                end_piece = wk;
            }

            for (int i_piece = start_piece; i_piece < end_piece; i_piece++) {
                if (get_bit(bitboards[i_piece], this_target_square)) {
                    clear_bit(&bitboards[i_piece], this_target_square);
                    break;
                }
            }
        }

        if (this_promoted_piece) {
            clear_bit(&bitboards[(side == white) ? wp : bp], this_target_square);
            set_bit(&bitboards[this_promoted_piece], this_target_square);
        }

        if (this_en_passant) {
            if (side == white) clear_bit(&bitboards[bp], this_target_square + 8) ;
            else clear_bit(&bitboards[wp], this_target_square - 8);
        }

        en_passant = no_square;

        if (this_double_push) {
            if (side == white) en_passant = this_target_square + 8;
            else en_passant = this_target_square - 8;
        }

        if (this_castling) {
            switch (this_target_square) {
            case g1:
                clear_bit(&bitboards[wr], h1);
                set_bit(&bitboards[wr], f1);
                break;
            case c1:
                clear_bit(&bitboards[wr], a1);
                set_bit(&bitboards[wr], d1);
                break;
            case g8:
                clear_bit(&bitboards[br], h8);
                set_bit(&bitboards[br], f8);
                break;
            case c8:
                clear_bit(&bitboards[br], a8);
                set_bit(&bitboards[br], d8);
                break;
            default:
                break;
            }
        }
        castle &= castling_rights[this_source_square];
        castle &= castling_rights[this_target_square];
        occ();

        side ^= 1;

        if (is_sq_attacked(side == white ? get_lsb_index(bitboards[bk]) : get_lsb_index(bitboards[wk]), side)) {
            restore_board();
            return 0;
        }
        else return 1;
    }
    // capture moves
    else {
        if (get_move_capture(move) || get_move_promoted(move)) {
            return make_move(move, all_moves);
        }
        else {
            return 0;
        }
    }
}

static inline void generate_moves(moves *move_list) {
    int source_square, target_square;
    // current piece's bitboard and attacks
    U64 bitboard, attacks;
    for (int piece = wp; piece <= bk; piece++) {
        bitboard = bitboards[piece];

        // generate white pawns and white king castling moves
        if (side == white) {
            if (piece == wp) {
                while (bitboard) {
                    source_square = get_lsb_index(bitboard);
                    target_square = source_square - 8;
                    if (!(target_square < a8) && !get_bit(occupancies[both], target_square)) {
                        // promotion
                        if (source_square >= a7 && source_square <= h7) {
                            // add move into a move list
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, wq, 0, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, wr, 0, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, wb, 0, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, wn, 0, 0, 0, 0));
                        } else {
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                            if ((source_square >= a2 && source_square <= h2) && !get_bit(occupancies[both], target_square - 8))
                                add_move(move_list,
                                    encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0));
                        }
                    }
                    attacks = pawn_attacks_table[side][source_square] & occupancies[black];
                    while (attacks) {
                        target_square = get_lsb_index(attacks);
                        if (source_square >= a7 && source_square <= h7) {
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, wq, 1, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, wr, 1, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, wb, 1, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, wn, 1, 0, 0, 0));
                        } else {
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        clear_bit(&attacks, target_square);
                    }
                    if (en_passant != no_square) {
                        U64 en_passant_attacks = pawn_attacks_table[side][source_square] & (1ULL << en_passant);
                        if (en_passant_attacks) {
                            target_square = get_lsb_index(en_passant_attacks);
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, 0, 1, 0, 1, 0));
                        }
                    }
                    clear_bit(&bitboard, source_square);
                }
            }
            if (piece == wk) {
                if (castle & wkc) {
                    if (!get_bit(occupancies[both], f1) && !get_bit(occupancies[both], g1)) {
                        if (!is_sq_attacked(e1, black) && !is_sq_attacked(f1, black) && !is_sq_attacked(g1, black)) {
                            add_move(move_list,
                                encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
                if (castle & wqc) {
                    if (!get_bit(occupancies[both], b1) && !get_bit(occupancies[both], c1)
                        && !get_bit(occupancies[both], d1)) {
                        if (!is_sq_attacked(e1, black) && !is_sq_attacked(d1, black)
                            && !is_sq_attacked(c1, black)) {
                            add_move(move_list,
                                encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
        }
        // generate black pawns and black king castling moves
        else {
            if (piece == bp) {
                while (bitboard) {
                    source_square = get_lsb_index(bitboard);
                    target_square = source_square + 8;
                    if (!(target_square > h1) && !get_bit(occupancies[both], target_square)) {
                        // promotion
                        if (source_square >= a2 && source_square <= h2) {
                            // add move into a move list
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, bq, 0, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, br, 0, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, bb, 0, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, bn, 0, 0, 0, 0));
                        } else {
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                            if ((source_square >= a7 && source_square <= h7) && !get_bit(occupancies[both], target_square + 8))
                                add_move(move_list,
                                    encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0));
                        }
                    }
                    attacks = pawn_attacks_table[side][source_square] & occupancies[white];
                    while (attacks) {
                        target_square = get_lsb_index(attacks);
                        if (source_square >= a2 && source_square <= h2) {
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, bq, 1, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, br, 1, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, bb, 1, 0, 0, 0));
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, bn, 1, 0, 0, 0));
                        } else {
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                        }
                        clear_bit(&attacks, target_square);
                    }
                    if (en_passant != no_square) {
                        U64 en_passant_attacks = pawn_attacks_table[side][source_square] & (1ULL << en_passant);
                        if (en_passant_attacks) {
                            target_square = get_lsb_index(en_passant_attacks);
                            add_move(move_list,
                                encode_move(source_square, target_square, piece, 0, 1, 0, 1, 0));
                        }
                    }
                    clear_bit(&bitboard, source_square);
                }
            }
            if (piece == bk) {
                if (castle & bkc) {
                    if (!get_bit(occupancies[both], f8) && !get_bit(occupancies[both], g8)) {
                        if (!is_sq_attacked(e8, white) && !is_sq_attacked(f8, white)
                            && !is_sq_attacked(g8, white)) {
                                add_move(move_list,
                                    encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
                if (castle & bqc) {
                    if (!get_bit(occupancies[both], b8) && !get_bit(occupancies[both], c8) && !get_bit(occupancies[both], d8)) {
                        if (!is_sq_attacked(e8, white) && !is_sq_attacked(d8, white) &&
                            !is_sq_attacked(c8, white)) {
                                add_move(move_list,
                                    encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
                            }
                    }
                }
            }
        }
        if (side == white ? piece == wn : piece == bn) {
            while (bitboard) {
                source_square = get_lsb_index(bitboard);
                attacks = knight_attacks_table[source_square] & ~occupancies[side == white ? white : black];
                while (attacks) {
                    target_square = get_lsb_index(attacks);
                    if (!get_bit(side == white ? occupancies[black] : occupancies[white], target_square))
                        add_move(move_list,
                            encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        add_move(move_list,
                            encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    clear_bit(&attacks, target_square);
                }
                clear_bit(&bitboard, source_square);
            }
        }
        if (side == white ? piece == wb : piece == bb) {
            while (bitboard) {
                source_square = get_lsb_index(bitboard);
                attacks = get_bishop_attacks(source_square, occupancies[both]) & ~occupancies[side == white ? white : black];
                while (attacks) {
                    target_square = get_lsb_index(attacks);
                    if (!get_bit(side == white ? occupancies[black] : occupancies[white], target_square))
                        add_move(move_list,
                            encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        add_move(move_list,
                            encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    clear_bit(&attacks, target_square);
                }
                clear_bit(&bitboard, source_square);
            }
        }
        if (side == white ? piece == wr : piece == br) {
            while (bitboard) {
                source_square = get_lsb_index(bitboard);
                attacks = get_rook_attacks(source_square, occupancies[both]) & ~occupancies[side == white ? white : black];
                while (attacks) {
                    target_square = get_lsb_index(attacks);
                    if (!get_bit(side == white ? occupancies[black] : occupancies[white], target_square))
                        add_move(move_list,
                            encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        add_move(move_list,
                            encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    clear_bit(&attacks, target_square);
                }
                clear_bit(&bitboard, source_square);
            }
        }
        if (side == white ? piece == wq : piece == bq) {
            while (bitboard) {
                source_square = get_lsb_index(bitboard);
                attacks = get_queen_attacks(source_square, occupancies[both]) & ~occupancies[side == white ? white : black];
                while (attacks) {
                    target_square = get_lsb_index(attacks);
                    if (!get_bit(side == white ? occupancies[black] : occupancies[white], target_square))
                        add_move(move_list,
                            encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        add_move(move_list,
                            encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    clear_bit(&attacks, target_square);
                }
                clear_bit(&bitboard, source_square);
            }
        }
        if (side == white ? piece == wk : piece == bk) {
            while (bitboard) {
                source_square = get_lsb_index(bitboard);
                attacks = king_attacks_table[source_square] & ~occupancies[side == white ? white : black];
                while (attacks) {
                    target_square = get_lsb_index(attacks);
                    if (!get_bit(side == white ? occupancies[black] : occupancies[white], target_square))
                        add_move(move_list,
                            encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));
                    else
                        add_move(move_list,
                            encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));
                    clear_bit(&attacks, target_square);
                }
                clear_bit(&bitboard, source_square);
            }
        }
    }
}

int get_time_ms() {
#ifdef WIN64
    return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int) tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

// leaf nodes (number of position reached during move gen test)
// caller should set nodes = 0 before running this function
U64 nodes = 0;
static inline void perft_driver(int depth) {
    moves move_list[1];
    move_list->count = 0;
    if (depth == 0) {
        nodes++;
        return;
    }
    generate_moves(move_list);

    for (int move_count = 0; move_count < move_list->count; move_count++) {
        int move = move_list->moves[move_count];
        copy_board();
        if (!make_move(move, all_moves))
            continue;
        perft_driver(depth - 1);
        restore_board();
    }
}

void perft_test(int depth) {
    int start = get_time_ms();
    printf("\n    Performance test\n\n");

    moves move_list[1];
    move_list->count = 0;

    if (depth == 0) {
        nodes++;
        return;
    }
    generate_moves(move_list);

    for (int move_count = 0; move_count < move_list->count; move_count++) {
        copy_board();
        if (!make_move(move_list->moves[move_count], all_moves))
            continue;

        U64 cumulative_nodes = nodes;
        perft_driver(depth - 1);
        U64 old_nodes = nodes - cumulative_nodes;

        restore_board();
        printf("    move: %s%s%c  nodes: %llu\n",
               square_coordinates[get_move_source(move_list->moves[move_count])],
               square_coordinates[get_move_target(move_list->moves[move_count])],
               get_move_promoted(move_list->moves[move_count]) == 0 ? ' ' : promoted_pieces[get_move_promoted(move_list->moves[move_count])],
               old_nodes);

    }

    printf("\n    Depth: %d\n", depth);
    printf("    Nodes: %llu\n", nodes);
    printf("     Time: %d\n", get_time_ms() - start);
}

int material_score[12] = {
    [wp] = 100,
    [wn] = 300,
    [wb] = 350,
    [wr] = 500,
    [wq] = 1000,
    [wk] = 10000,
    [bp] = -100,
    [bn] = -300,
    [bb] = -350,
    [br] = -500,
    [bq] = -1000,
    [bk] = -10000,
};

const int pawn_score[64] = 
{
    90,  90,  90,  90,  90,  90,  90,  90,
    30,  30,  30,  40,  40,  30,  30,  30,
    20,  20,  20,  30,  30,  30,  20,  20,
    10,  10,  10,  20,  20,  10,  10,  10,
     5,   5,  10,  20,  20,   5,   5,   5,
     0,   0,   0,   5,   5,   0,   0,   0,
     0,   0,   0, -10, -10,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0
};

const int knight_score[64] = 
{
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,  10,  10,   0,   0,  -5,
    -5,   5,  20,  20,  20,  20,   5,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,  10,  20,  30,  30,  20,  10,  -5,
    -5,   5,  20,  10,  10,  20,   5,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5, -10,   0,   0,   0,   0, -10,  -5
};

const int bishop_score[64] = 
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,  10,  10,   0,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,  10,   0,   0,   0,   0,  10,   0,
     0,  30,   0,   0,   0,   0,  30,   0,
     0,   0, -10,   0,   0, -10,   0,   0

};

const int rook_score[64] =
{
    50,  50,  50,  50,  50,  50,  50,  50,
    50,  50,  50,  50,  50,  50,  50,  50,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,  10,  20,  20,  10,   0,   0,
     0,   0,   0,  20,  20,   0,   0,   0

};

const int king_score[64] = 
{
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   5,   5,   5,   5,   0,   0,
     0,   5,   5,  10,  10,   5,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   5,  10,  20,  20,  10,   5,   0,
     0,   0,   5,  10,  10,   5,   0,   0,
     0,   5,   5,  -5,  -5,   0,   5,   0,
     0,   0,   5,   0, -15,   0,  10,   0
};

const int mirror_score[128] =
{
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

static inline int evaluate() {
    int score = 0;
    U64 bitboard;
    int piece, square;
    for (int i_piece= wp; i_piece <= bk; i_piece++) {
        bitboard = bitboards[i_piece];
        while (bitboard) {
            piece = i_piece;
            square = get_lsb_index(bitboard);
            score += material_score[piece];
            switch (piece)
                {
                case wp:
                    score += pawn_score[square];
                    break;
                case wn:
                    score += knight_score[square];
                    break;
                case wb:
                    score += bishop_score[square];
                    break;
                case wr:
                    score += rook_score[square];
                    break;
                /* case wq: */
                /*     score += queen_score[square]; */
                /*     break; */
                case wk:
                    score += king_score[square];
                    break;
                case bp:
                    score -= pawn_score[mirror_score[square]];
                    break;
                case bn:
                    score -= knight_score[mirror_score[square]];
                    break;
                case bb:
                    score -= bishop_score[mirror_score[square]];
                    break;
                case br:
                    score -= rook_score[mirror_score[square]];
                    break;
                /* case bq: */
                /*     score -= queen_score[mirror_score[square]]; */
                /*     break; */
                case bk:
                    score -= king_score[mirror_score[square]];
                    break;
                }
            clear_bit(&bitboard, square);
        }
    }
    return side == white ? score : -score;
}

// search

int ply = 0; // half move counter
int best_move;

static inline int quiescence(int alpha, int beta) {
    int evaluation = evaluate();
    if (evaluation >= beta) {
        return beta;
    }
    if (evaluation > alpha) {
        alpha = evaluation;
    }

    moves move_list[1];
    move_list->count = 0;
    generate_moves(move_list);

    for (int count = 0; count < move_list->count; count++) {
        copy_board();
        ply++;
        if (make_move(move_list->moves[count], only_captures) == 0) {
            ply--;
            continue;
        }
        int score = -quiescence(-beta, -alpha);
        ply--;
        restore_board();

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

static inline int negamax(int alpha, int beta, int depth) {
    if (depth == 0)
        return quiescence(alpha, beta);

    nodes++;
    int legal_moves = 0;
    int in_check = is_sq_attacked( get_lsb_index(bitboards[side == white ? wk : bk]), side ^ 1);

    int best_yet;
    int old_alpha = alpha;

    moves move_list[1];
    move_list->count = 0;
    generate_moves(move_list);

    for (int count = 0; count < move_list->count; count++) {
        copy_board();

        ply++;
        if (make_move(move_list->moves[count], all_moves) == 0) {
            ply--;
            continue;
        }

        legal_moves++;

        int score = -negamax(-beta, -alpha, depth - 1);
        ply--;
        restore_board();

        if (score >= beta) {
            return beta;
        }

        if (score > alpha) {
            alpha = score;
            if (ply == 0)
                best_yet = move_list->moves[count];
        }
    }

    if (legal_moves == 0) {
        if (in_check)
            return -49000 + ply;
        else
            return 0;
    }

    if (old_alpha != alpha) {
        best_move = best_yet;
    }

    return alpha;
}

void search_position(int depth) {
    int score = negamax(-50000, 50000, depth);
    if (best_move) {
        printf("bestmove ");
        print_move(best_move);
        printf("\n");
    }
}

// universal chess interface

int parse_move(char * move_string) {
    moves move_list[1];
    move_list->count = 0;
    generate_moves(move_list);

    int source_square = sq((8 - (move_string[1] - '0')), move_string[0] - 'a');
    int target_square = sq((8 - (move_string[3] - '0')), move_string[2] - 'a');

    for (int i = 0; i < move_list->count; i++) {
        int move = move_list->moves[i];
        int this_promoted_piece = 0;
        if (strlen(move_string) == 5) {
            this_promoted_piece = char_pieces[move_string[4]];
        }
        if (get_move_source(move) == source_square && get_move_target(move) == target_square && get_move_promoted(move) == this_promoted_piece) {
            return move;
        }
    }

    return 0;
}

void parse_position(char* command) {
    command += 9;
    char *cur_char = command;
    if (strncmp(command, "startpos", 8) == 0)
        parse_fen(start_position);
    else {
        cur_char = strstr(command, "fen");
        if (cur_char == NULL)
            parse_fen(start_position);
        else {
            cur_char += 4;
            parse_fen(cur_char);
        }
    }

    cur_char = strstr(command, "moves");
    if (cur_char == NULL);
    else {
        cur_char += 6;
        while (*cur_char != '\0') {
            make_move(parse_move(cur_char), all_moves);
            while (*cur_char != ' ' && *cur_char != '\0') cur_char++;
            if (*cur_char != '\0') cur_char++;
        }
    }

    print_board();
}

void parse_go(char* command) {
    int depth = -1;
    char* current_depth = NULL;
    if ((current_depth = strstr(command, "depth")) != NULL) {
        depth = atoi(current_depth + 6);
    }
    else {
        depth = 6;
    }

    printf("depth: %d\n", depth);
    search_position(depth);
}

void uci_loop() {
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    char input[2000];

    printf("id name bitboardchessengine\n");
    printf("id name TYG\n");
    printf("uciok\n");

    while (1) {
        memset(input, 0, sizeof(input));
        fflush(stdout);
        if (!fgets(input, 2000, stdin))
            continue;
        if (input[0] == '\n')
            continue;

        if (strncmp(input, "isready", 7) == 0) {
            printf("readyok\n");
            continue;
        }

        if (strncmp(input, "position", 8) == 0)
            parse_position(input);

        else if (strncmp(input, "ucinewgame", 10) == 0)
            parse_position("position startpos");

        else if (strncmp(input, "go", 2) == 0)
            parse_go(input);

        else if (strncmp(input, "quit", 4) == 0)
            break;

        else if (strncmp(input, "uci", 3) == 0) {
            printf("id name bitboardchessengine\n");
            printf("id name TYG\n");
            printf("uciok\n");
        }
    }
}
    
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    init_attack_tables();
    /*
     * next goasl:
     * understand alpha beta pruning
     * go back to understand magic bitboards
     * implement checkmate and stalemate detection
     */
    // connect to gui
    // if (0) {
    //     parse_position("position startpos");
    //     print_board();
    //     search_position(2);
    // }
    // else {
        uci_loop();
    // }
    return -1;
}
