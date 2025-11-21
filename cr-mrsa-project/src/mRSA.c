/*
 * Copyright(c) 2020-2024 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 자는 이 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */
#ifdef __linux__
#include <bsd/stdlib.h>
#elif __APPLE__
#include <stdlib.h>
#else
#include <stdlib.h>
#endif
#include "mRSA.h"
#include <stdbool.h>



/*
 * mod_add() - computes a + b mod m
 */
uint64_t mod_add(uint64_t a, uint64_t b, uint64_t m)
{
    if (m == 0) return 0;  // 방어
    a %= m;
    b %= m;

    if (a >= m - b)
        return a - (m - b);
    else
        return a + b;
}
/*
 * mod_mul() - computes a * b mod m
 */
uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t m)
{
    uint64_t r = 0;

    while (b) {
        if (b & 1)
            r = mod_add(r, a, m);   // r += a (mod m)
        b >>= 1;
        if (b)                      // 마지막 루프에서 불필요한 한 번의 mod_add 회피
            a = mod_add(a, a, m);   // a = 2a (mod m)
    }
    return r;
}

/*  
 * mod_pow() - computes a^b mod m
 */
uint64_t mod_pow(uint64_t a, uint64_t b, uint64_t m)
{
        uint64_t r = 1;
        
        while(b){
            if(b & 1) r= mod_mul(r, a , m);
            b = b >> 1;
            a = mod_mul(a, a, m);
        }

        return r;
}

/*
 * gcd() - Euclidean algorithm
 */
static uint64_t gcd(uint64_t a, uint64_t b)
{
     int64_t new_a = a;
     int64_t new_b = b;

 while (new_b > 0) {
     int64_t temp = new_b;
     new_b = new_a % new_b;
     new_a = temp;
 }

 return new_a;
}

/*
 * umul_inv() - computes multiplicative inverse a^-1 mod m
 * It returns 0 if no inverse exist.
 */
static uint64_t umul_inv(uint64_t a, uint64_t m)
{
     int64_t d_0 = a, d_1 = m;
     int64_t x_0 = 1, x_1 = 0;
     int64_t q, tmp;

 while (d_1 != 0) {
     q = d_0 / d_1;
     tmp = d_0 - q * d_1;
     d_0 = d_1;
     d_1 = tmp;

     tmp = x_0 - q * x_1;
     x_0 = x_1;
     x_1 = tmp;
 }

 
 if (d_0 == 1)  return (x_0 > 0 ? x_0 : x_0 + m);
 else return 0; 
}

/*
 * Miller-Rabin Primality Testing against small sets of bases
 *
 * if n < 2^64,
 * it is enough to test a = 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, and 37.
 *
 * if n < 3317044064679887385961981,
 * it is enough to test a = 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, and 41.
 */
static const uint64_t a[BASELEN] = {2,3,5,7,11,13,17,19,23,29,31,37};

/*
 * miller_rabin() - Miller-Rabin Primality Test (deterministic version)
 *
 * n > 3, an odd integer to be tested for primality
 * It returns 1 if n is prime, 0 otherwise.
 */
static int miller_rabin(uint64_t n)
{
    if (n == 2 || n == 3) return PRIME;
    if ((n & 1ULL) == 0ULL || n < 2) return COMPOSITE;

    // 0) 작은 소수로 빠르게 거르기 (베이스 배열 재활용)
    //    n이 작은 소수면 즉시 PRIME, 그 외 나누어떨어지면 COMPOSITE
    for (int i = 0; i < BASELEN; ++i) {
        uint64_t p = a[i];
        if (n == p) return PRIME;
        if (n % p == 0) return COMPOSITE;
    }

    // 1) n-1 = d * 2^s  (d는 홀수)  — 비트 트레일링 제로 카운트로 즉시 계산
    uint64_t n_minus_1 = n - 1;
#if defined(__GNUC__) || defined(__clang__)
    uint64_t s = __builtin_ctzll(n_minus_1); // trailing zeros
#else
    // fallback (루프) — GCC/Clang 아니면 이 블록이 쓰임
    uint64_t s = 0;
    uint64_t tmp = n_minus_1;
    while ((tmp & 1ULL) == 0ULL) { tmp >>= 1; ++s; }
#endif
    uint64_t d = n_minus_1 >> s;

    // 2) 결정적 베이스 테스트
    for (int i = 0; i < BASELEN; ++i) {
        uint64_t a_i = a[i] % n;
        if (a_i == 0) continue;                 // a_i ≡ 0 (mod n)이면 건너뜀
        uint64_t x = mod_pow(a_i, d, n);        // x = a^d mod n
        if (x == 1 || x == n_minus_1) continue; // 이 베이스는 통과

        bool witness = true;
        // r번 제곱하며 n-1 나오는지 확인 (최대 s-1번)
        for (uint64_t r = 1; r < s; ++r) {
            x = mod_mul(x, x, n);
            if (x == n_minus_1) { witness = false; break; }
        }
        if (witness) return COMPOSITE;          // 합성수 증인 발견
    }

    return PRIME;
}

/*
 * mRSA_generate_key() - generates mini RSA keys e, d and n
 *
 * Carmichael's totient function Lambda(n) is used.
 */
void mRSA_generate_key(uint64_t *e, uint64_t *d, uint64_t *n)
{
    uint32_t p, q;

    do {
        do { 
            p = arc4random() | 1u;
            p |= (1u << 31);              // 32비트 최상위 비트 설정
        } 
        while (miller_rabin((uint64_t)p) == COMPOSITE);

        do { 
            q = arc4random() | 1u;
            q |= (1u << 31);              // 32비트 최상위 비트 설정
        } 
        while (q == p || miller_rabin((uint64_t)q) == COMPOSITE);

        *n = (uint64_t)p * (uint64_t)q;
    } while (*n < MINIMUM_N);

    uint64_t phi   = (uint64_t)(p - 1) * (uint64_t)(q - 1);
    uint64_t g     = gcd(p - 1, q - 1);
    uint64_t c_phi = phi / g; // ← λ(n)과 동일!

     while (1) {
        *e = arc4random() | 1ULL;
        if(gcd(*e, c_phi) == 1) break;
    }
    
    *d = umul_inv(*e, c_phi);
    
        
    
}


/*
 * mRSA_cipher() - compute m^k mod n
 *
 * If data >= n then returns 1 (error), otherwise 0 (success).
 */
int mRSA_cipher(uint64_t *m, uint64_t k, uint64_t n)
{
    if (!m || n < 2) return 1;      // 잘못된 인자

    // 평문/암문은 Z_n에 있어야 함 → 필요 시 감소
    if (*m >= n) *m %= n;

    *m = mod_pow(*m, k, n);         
    return 0;                       
}

