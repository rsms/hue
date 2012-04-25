// Fast byte comparison. Inspired by Igor Sysoev's Nginx.
#ifndef RSMS_FASTCMP_H
#define RSMS_FASTCMP_H

#define rsms_i8cmp2(m, c0, c1) ((m)[0] == c0 && (m)[1] == c1)

#define rsms_i8cmp3(m, c0, c1, c2) ((m)[0] == c0 && (m)[1] == c1 && (m)[2] == c2)

#define rsms_i8cmp4(m, c0, c1, c2, c3)                                        \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0))

#define rsms_i8cmp5(m, c0, c1, c2, c3, c4)                                    \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) && (m)[4] == c4)

#define rsms_i8cmp6(m, c0, c1, c2, c3, c4, c5)                                \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)             \
        && (((uint32_t *) (m))[1] & 0xffff) == ((c5 << 8) | c4))

#define rsms_i8cmp7(m, c0, c1, c2, c3, c4, c5, c6)                       \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)             \
        && (m)[4] == c4 && (m)[5] == c5 && (m)[6] == c6)

#define rsms_i8cmp8(m, c0, c1, c2, c3, c4, c5, c6, c7)                        \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)             \
        && ((uint32_t *) (m))[1] == ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4))

#define rsms_i8cmp9(m, c0, c1, c2, c3, c4, c5, c6, c7, c8)                    \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)             \
        && ((uint32_t *) (m))[1] == ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4) && (m)[8] == c8)


#endif // RSMS_FASTCMP_H

// uint8_t *m = new uint8_t[sizeof("abcdefghijklmnopqrstuvwxyz")];
// memcpy(m, "abcdefghijklmnopqrstuvwxyz", sizeof("abcdefghijklmnopqrstuvwxyz"));
// cout << "rsms_i8cmp2: " << rsms_i8cmp2(m, 'a','b') << " " << rsms_i8cmp2(m, 'z','y') << endl;
// cout << "rsms_i8cmp3: " << rsms_i8cmp3(m, 'a','b','c') << " " << rsms_i8cmp3(m, 'z','y','x') << endl;
// cout << "rsms_i8cmp4: " << rsms_i8cmp4(m, 'a','b','c','d') << " " << rsms_i8cmp4(m, 'z','y','y','y') << endl;
// cout << "rsms_i8cmp5: " << rsms_i8cmp5(m, 'a','b','c','d','e') << " " << rsms_i8cmp5(m, 'z','y','y','y','y') << endl;
// cout << "rsms_i8cmp6: " << rsms_i8cmp6(m, 'a','b','c','d','e','f') << " " << rsms_i8cmp6(m, 'z','y','c','d','e','x') << endl;
// cout << "rsms_i8cmp7: " << rsms_i8cmp7(m, 'a','b','c','d','e','f','g') << " " << rsms_i8cmp7(m, 'z','y','c','d','e','f','g') << endl;
// cout << "rsms_i8cmp8: " << rsms_i8cmp8(m, 'a','b','c','d','e','f','g','h') << " " << rsms_i8cmp8(m, 'z','y','c','d','e','f','g','h') << endl;
// cout << "rsms_i8cmp9: " << rsms_i8cmp9(m, 'a','b','c','d','e','f','g','h','i') << " " << rsms_i8cmp9(m, 'z','y','c','d','e','f','g','h','i') << endl;
