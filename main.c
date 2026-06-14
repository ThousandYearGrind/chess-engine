    /*
     * Thank you to Chess Programming on YouTube for teaching me
     */

    #include <stdio.h>
    #include <string.h>
    #define U64 unsigned long long

    // FEN dedug positions
    #define empty_board "8/8/8/8/8/8/8/8 w - - "
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

    char ascii_pieces[12] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'};

    int char_pieces[] = {
        ['P'] = wp,
        ['N'] = wn,
        ['B'] = wb,
        ['R'] = wr,
        ['Q'] = wq,
        ['K'] = wk,
        ['p'] = bp,
        ['n'] = bn,
        ['b'] = bb,
        ['r'] = br,
        ['q'] = bq,
        ['k'] = bk
    };

    // bitboards
    U64 bitboards[12];
    U64 occupancies[3];

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
        printf("     Side:     %s\n", (side == white) ? "white" : side == black ? "black" : "0");
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
                if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z')) {
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

    U64 find_magic_number(int square, int relevant_bits, int bishop) {
        U64 occupancies[4096];
        U64 attacks[4096];
        U64 used_attacks[4096];
        U64 attack_mask = bishop ? mask_bishop_attacks(square) : mask_rook_attacks(square);
        int occupancy_indices = 1 << relevant_bits;

        for (int index = 0; index < occupancy_indices; ++index) {
            occupancies[index] = set_occupancy(index, relevant_bits, attack_mask);
            attacks[index] = bishop ? generate_bishop_attacks(square, occupancies[index]) : generate_rook_attacks(square, occupancies[index]);
        }

        // test magic numbers
        for (int random_count = 0; random_count < 100000000; random_count++) {
            U64 magic_number = generate_magic_number();

            // skip bad magic numbers
            if (count_bits((attack_mask * magic_number) & 0xFF00000000000000) < 6) continue;

            memset(used_attacks, 0ULL, sizeof(used_attacks));
            int index, fail;

            for (index = 0, fail = 0; !fail && index < occupancy_indices; index++) {
                int magic_index = (int) ((occupancies[index] * magic_number) >> (64 - relevant_bits));
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
                    int magic_index = (occupancy * bishop_magic_numbers[square]) >> (64 - relevant_bits_count);
                    bishop_attacks_table[square][magic_index] = generate_bishop_attacks(square, occupancy);
                }
                else {
                    U64 occupancy = set_occupancy(index, relevant_bits_count, attack_mask);
                    int magic_index = (occupancy * rook_magic_numbers[square]) >> (64 - relevant_bits_count);
                    rook_attacks_table[square][magic_index] = generate_rook_attacks(square, occupancy);
                }
            }
        }
    }

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
    }

    int main(void) {
        init_attack_tables();
        init_sliders_attacks(bishop);
        init_sliders_attacks(rook);

        return 0;
    }