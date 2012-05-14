// Fast byte comparison. Inspired by Igor Sysoev's Nginx.
// Works with LE systems that can handle 32-bit integers in one register.
#ifndef HUE__FASTCMP_H
#define HUE__FASTCMP_H

// bool hue_<T>cmp<N>(const T*, T1, ..) -- compare vector of T items of N length for equality

#define hue_i8cmp2(m, c0, c1) ((m)[0] == c0 && (m)[1] == c1)

#define hue_i8cmp3(m, c0, c1, c2) ((m)[0] == c0 && (m)[1] == c1 && (m)[2] == c2)

#define hue_i8cmp4(m, c0, c1, c2, c3)                                        \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0))

#define hue_i8cmp5(m, c0, c1, c2, c3, c4)                                    \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0) && (m)[4] == c4)

#define hue_i8cmp6(m, c0, c1, c2, c3, c4, c5)                                \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)             \
        && (((uint32_t *) (m))[1] & 0xffff) == ((c5 << 8) | c4))

#define hue_i8cmp7(m, c0, c1, c2, c3, c4, c5, c6)                       \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)             \
        && (m)[4] == c4 && (m)[5] == c5 && (m)[6] == c6)

#define hue_i8cmp8(m, c0, c1, c2, c3, c4, c5, c6, c7)                        \
    (*(uint32_t *) (m) == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)             \
        && ((uint32_t *) (m))[1] == ((c7 << 24) | (c6 << 16) | (c5 << 8) | c4))


#define hue_i32cmp2(m, c0, c1) \
     ( (m)[0] == ((uint32_t)(c0)) \
    && (m)[1] == ((uint32_t)(c1)) )
                                     
#define hue_i32cmp3(m, c0, c1, c2) \
     ( (m)[0] == ((uint32_t)(c0)) \
    && (m)[1] == ((uint32_t)(c1)) \
    && (m)[2] == ((uint32_t)(c2)) )
                                     
#define hue_i32cmp4(m, c0, c1, c2, c3) \
     ( (m)[0] == ((uint32_t)(c0)) \
    && (m)[1] == ((uint32_t)(c1)) \
    && (m)[2] == ((uint32_t)(c2)) \
    && (m)[3] == ((uint32_t)(c3)) )
                                     
#define hue_i32cmp5(m, c0, c1, c2, c3, c4) \
     ( (m)[0] == ((uint32_t)(c0)) \
    && (m)[1] == ((uint32_t)(c1)) \
    && (m)[2] == ((uint32_t)(c2)) \
    && (m)[3] == ((uint32_t)(c3)) \
    && (m)[4] == ((uint32_t)(c4)) )
                                     
#define hue_i32cmp6(m, c0, c1, c2, c3, c4, c5) \
     ( (m)[0] == ((uint32_t)(c0)) \
    && (m)[1] == ((uint32_t)(c1)) \
    && (m)[2] == ((uint32_t)(c2)) \
    && (m)[3] == ((uint32_t)(c3)) \
    && (m)[4] == ((uint32_t)(c4)) \
    && (m)[5] == ((uint32_t)(c5)) )
                                     
#define hue_i32cmp7(m, c0, c1, c2, c3, c4, c5, c6) \
     ( (m)[0] == ((uint32_t)(c0)) \
    && (m)[1] == ((uint32_t)(c1)) \
    && (m)[2] == ((uint32_t)(c2)) \
    && (m)[3] == ((uint32_t)(c3)) \
    && (m)[4] == ((uint32_t)(c4)) \
    && (m)[5] == ((uint32_t)(c5)) \
    && (m)[6] == ((uint32_t)(c6)) )
                                     
#define hue_i32cmp8(m, c0, c1, c2, c3, c4, c5, c6, c7) \
     ( (m)[0] == ((uint32_t)(c0)) \
    && (m)[1] == ((uint32_t)(c1)) \
    && (m)[2] == ((uint32_t)(c2)) \
    && (m)[3] == ((uint32_t)(c3)) \
    && (m)[4] == ((uint32_t)(c4)) \
    && (m)[5] == ((uint32_t)(c5)) \
    && (m)[6] == ((uint32_t)(c6)) \
    && (m)[7] == ((uint32_t)(c7)) )

#endif // HUE__FASTCMP_H

// uint8_t *m = new uint8_t[sizeof("abcdefghijklmnopqrstuvwxyz")];
// memcpy(m, "abcdefghijklmnopqrstuvwxyz", sizeof("abcdefghijklmnopqrstuvwxyz"));
// cout << "hue_i8cmp2: " << hue_i8cmp2(m, 'a','b') << " " << hue_i8cmp2(m, 'z','y') << endl;
// cout << "hue_i8cmp3: " << hue_i8cmp3(m, 'a','b','c') << " " << hue_i8cmp3(m, 'z','y','x') << endl;
// cout << "hue_i8cmp4: " << hue_i8cmp4(m, 'a','b','c','d') << " " << hue_i8cmp4(m, 'z','y','y','y') << endl;
// cout << "hue_i8cmp5: " << hue_i8cmp5(m, 'a','b','c','d','e') << " " << hue_i8cmp5(m, 'z','y','y','y','y') << endl;
// cout << "hue_i8cmp6: " << hue_i8cmp6(m, 'a','b','c','d','e','f') << " " << hue_i8cmp6(m, 'z','y','c','d','e','x') << endl;
// cout << "hue_i8cmp7: " << hue_i8cmp7(m, 'a','b','c','d','e','f','g') << " " << hue_i8cmp7(m, 'z','y','c','d','e','f','g') << endl;
// cout << "hue_i8cmp8: " << hue_i8cmp8(m, 'a','b','c','d','e','f','g','h') << " " << hue_i8cmp8(m, 'z','y','c','d','e','f','g','h') << endl;
// cout << "hue_i8cmp9: " << hue_i8cmp9(m, 'a','b','c','d','e','f','g','h','i') << " " << hue_i8cmp9(m, 'z','y','c','d','e','f','g','h','i') << endl;
