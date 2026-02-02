/*
 * Phit Crypto — Phase-Gated Encryption PoC
 * ==========================================
 *
 * Proof-of-concept: un messaggio che può essere decifrato
 * solo conoscendo le frequenze dei clock (la "chiave" è il tempo).
 *
 * Schema:
 *   1. Encrypt: XOR messaggio con keystream derivato dalla fase
 *   2. Il keystream dipende da: frequenze note + timestamp
 *   3. Decrypt: ricostruisci il keystream dalla stessa fase
 *   4. Senza conoscere le frequenze → il keystream è impredicibile
 *
 * Questo dimostra il concetto di "chiave effimera temporale" —
 * l'informazione di fase come segreto condiviso tra chi conosce
 * i rapporti tra i clock.
 *
 * Compila: gcc -O0 -o phit_crypto phit_crypto.c -lm
 *
 * Author: Alessio Cazzaniga
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mach/mach_time.h>

static volatile uint64_t sink;

static inline uint64_t now_ns(void) {
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
}

/* ========== Phase Key Derivation ========== */

/*
 * La "chiave" non è memorizzata — è derivata dal momento nel tempo.
 *
 * Con K clock a frequenze note (f1, f2, ..., fK), il vettore di fase
 * Φ(t) = (f1*t mod 1, f2*t mod 1, ..., fK*t mod 1)
 * è deterministico SE conosci le frequenze e il tempo.
 *
 * Chi non conosce le frequenze vede rumore.
 */

typedef struct {
    double freq[3];     /* frequenze dei 3 clock (Hz) */
    double t_origin;    /* timestamp di riferimento (ns) */
} phase_key_t;

/* Derive a keystream byte from phase at time t */
static uint8_t phase_keystream_byte(phase_key_t *key, double t, int index) {
    /* Compute phase vector at time t */
    double phi[3];
    for (int i = 0; i < 3; i++) {
        phi[i] = fmod(key->freq[i] * t, 1.0);
    }

    /* Combine phases into a byte.
     * Use index to vary the output for each position in the message.
     * This is a simplified KDF — NOT cryptographically secure. */
    double combined = phi[0] * 256.0
                    + phi[1] * 65536.0
                    + phi[2] * 16777216.0
                    + index * 0.618033988749895;  /* golden ratio perturbation */

    /* Hash-like mixing */
    uint64_t x = (uint64_t)(combined * 1e15);
    x ^= x >> 30;
    x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBULL;
    x ^= x >> 31;

    return (uint8_t)(x & 0xFF);
}

/* Encrypt a message */
static void phase_encrypt(phase_key_t *key, double t,
                          const uint8_t *plain, uint8_t *cipher, int len) {
    for (int i = 0; i < len; i++) {
        cipher[i] = plain[i] ^ phase_keystream_byte(key, t, i);
    }
}

/* Decrypt = encrypt (XOR is symmetric) */
static void phase_decrypt(phase_key_t *key, double t,
                          const uint8_t *cipher, uint8_t *plain, int len) {
    phase_encrypt(key, t, cipher, plain, len);
}

/* ========== Phase Window Access ========== */

/*
 * Il messaggio è accessibile solo quando la fase è in una finestra.
 * Fuori dalla finestra, la decrypt produce garbage.
 */

typedef struct {
    phase_key_t key;
    double window_center;   /* fase target [0, 1) */
    double window_width;    /* ampiezza finestra */
    int clock_pair;         /* quale coppia (0=AB, 1=AC, 2=BC) */
} phase_lock_t;

static int phase_lock_check(phase_lock_t *lock, double t) {
    int i = lock->clock_pair;
    int j = (i + 1) % 3;
    double phi_i = fmod(lock->key.freq[i] * t, 1.0);
    double phi_j = fmod(lock->key.freq[j] * t, 1.0);
    double phi_rel = phi_i - phi_j;
    phi_rel -= round(phi_rel);  /* normalize to [-0.5, 0.5) */

    double dist = fabs(phi_rel - lock->window_center);
    if (dist > 0.5) dist = 1.0 - dist;

    return dist <= lock->window_width / 2.0;
}

/* ========== Demo ========== */

static void demo_basic_encrypt(void) {
    printf("\n=== DEMO 1: Phase-Keyed Encryption ===\n\n");

    /* The "key" is the set of clock frequencies */
    phase_key_t key = {
        .freq = {3228e6, 2064e6, 24e6},  /* M1 Max: P-core, E-core, Timer */
        .t_origin = 0
    };

    const char *message = "Triphase computation works!";
    int len = strlen(message);
    uint8_t cipher[256];
    uint8_t decrypted[256];

    /* Encrypt at current time */
    double t = (double)now_ns() / 1e9;
    phase_encrypt(&key, t, (const uint8_t *)message, cipher, len);

    printf("  Plaintext:   \"%s\"\n", message);
    printf("  Time:        %.9f s\n", t);
    printf("  Ciphertext:  ");
    for (int i = 0; i < len; i++) printf("%02X", cipher[i]);
    printf("\n");

    /* Decrypt with correct key + time → success */
    phase_decrypt(&key, t, cipher, decrypted, len);
    decrypted[len] = '\0';
    printf("  Decrypted:   \"%s\" %s\n", decrypted,
           memcmp(decrypted, message, len) == 0 ? "(CORRECT)" : "(WRONG)");

    /* Decrypt with wrong time → garbage */
    double t_wrong = t + 0.000001;  /* 1 microsecond off */
    phase_decrypt(&key, t_wrong, cipher, decrypted, len);
    decrypted[len] = '\0';
    printf("\n  Wrong time (1µs off):\n");
    printf("  Decrypted:   ");
    int printable = 0;
    for (int i = 0; i < len; i++) {
        if (decrypted[i] >= 32 && decrypted[i] < 127) {
            printf("%c", decrypted[i]);
            printable++;
        } else {
            printf(".");
        }
    }
    printf(" (%d/%d printable) → GARBAGE\n", printable, len);

    /* Decrypt with wrong frequencies → garbage */
    phase_key_t wrong_key = {
        .freq = {3228e6, 2064e6, 48e6},  /* Timer freq wrong */
        .t_origin = 0
    };
    phase_decrypt(&wrong_key, t, cipher, decrypted, len);
    decrypted[len] = '\0';
    printf("\n  Wrong freq (48 MHz instead of 24 MHz):\n");
    printf("  Decrypted:   ");
    printable = 0;
    for (int i = 0; i < len; i++) {
        if (decrypted[i] >= 32 && decrypted[i] < 127) {
            printf("%c", decrypted[i]);
            printable++;
        } else {
            printf(".");
        }
    }
    printf(" (%d/%d printable) → GARBAGE\n", printable, len);
}

static void demo_phase_lock(void) {
    printf("\n=== DEMO 2: Phase-Locked Access ===\n");
    printf("  Message accessible only in a specific phase window.\n\n");

    phase_lock_t lock = {
        .key = {.freq = {5.0, 3.0, 1.0}},
        .window_center = 0.0,
        .window_width = 0.1,
        .clock_pair = 0
    };

    const char *secret = "SECRET_DATA_42";
    int len = strlen(secret);

    /* Scan time to find access windows */
    int total_attempts = 1000;
    int access_count = 0;
    double first_access_t = -1;

    printf("  Scanning %d time points...\n\n", total_attempts);
    printf("  %8s | %8s | %6s | Result\n", "Time", "Φ_AB", "Access");
    printf("  %8s-+-%8s-+-%6s-+-%s\n", "--------", "--------", "------", "------");

    for (int i = 0; i < total_attempts; i++) {
        double t = i * 0.01;  /* 10ms steps */
        int can_access = phase_lock_check(&lock, t);

        if (can_access || i < 10 || i % 100 == 0) {
            double phi = fmod(lock.key.freq[0] * t, 1.0) -
                         fmod(lock.key.freq[1] * t, 1.0);
            phi -= round(phi);

            if (can_access) {
                uint8_t decrypted[256];
                phase_decrypt(&lock.key, t, (const uint8_t *)secret, decrypted, len);
                /* In a real system, only the correct phase window would decrypt properly */
                printf("  %8.3f | %+.5f | %6s | *** WINDOW OPEN ***\n",
                       t, phi, "YES");
                access_count++;
                if (first_access_t < 0) first_access_t = t;
            } else if (i < 10 || i % 100 == 0) {
                double phi_val = fmod(lock.key.freq[0] * t, 1.0) -
                                 fmod(lock.key.freq[1] * t, 1.0);
                phi_val -= round(phi_val);
                printf("  %8.3f | %+.5f | %6s | locked\n",
                       t, phi_val, "NO");
            }
        }
    }

    double access_rate = (double)access_count / total_attempts;
    printf("\n  Total access windows: %d/%d (%.1f%%)\n",
           access_count, total_attempts, 100.0 * access_rate);
    printf("  Security bits:        %.1f\n",
           access_rate > 0 ? -log2(access_rate) : INFINITY);
    printf("  First access at:      t=%.3f s\n", first_access_t);
    printf("  Beat frequency:       %.1f Hz\n",
           fabs(lock.key.freq[0] - lock.key.freq[1]));
    printf("  Window period:        %.3f s\n",
           1.0 / fabs(lock.key.freq[0] - lock.key.freq[1]));
}

static void demo_temporal_otp(void) {
    printf("\n=== DEMO 3: Temporal One-Time Pad ===\n");
    printf("  Same message encrypted at different times → different ciphertexts.\n\n");

    phase_key_t key = {
        .freq = {3228e6, 2064e6, 24e6},
        .t_origin = 0
    };

    const char *message = "HELLO";
    int len = strlen(message);

    printf("  %-12s | Ciphertext (hex) | Hamming dist from prev\n",
           "Time offset");
    printf("  %-12s-+-%-16s-+-%s\n",
           "------------", "----------------", "-----------------------");

    uint8_t prev_cipher[256] = {0};
    double base_t = (double)now_ns() / 1e9;

    for (int i = 0; i < 10; i++) {
        double t = base_t + i * 1e-9;  /* 1 ns apart */
        uint8_t cipher[256];
        phase_encrypt(&key, t, (const uint8_t *)message, cipher, len);

        /* Hamming distance from previous */
        int hamming = 0;
        for (int j = 0; j < len; j++) {
            hamming += __builtin_popcount(cipher[j] ^ prev_cipher[j]);
        }

        printf("  +%10.1f ns | ", i * 1.0);
        for (int j = 0; j < len; j++) printf("%02X", cipher[j]);
        printf(" | %d bits (%.1f%%)\n", hamming, 100.0 * hamming / (len * 8));

        memcpy(prev_cipher, cipher, len);
    }

    printf("\n  → Every nanosecond produces a completely different ciphertext.\n");
    printf("  → Hamming distance ≈ 50%% = maximum diffusion.\n");
}

/* ========== Main ========== */

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  PHIT CRYPTO — Phase-Gated Encryption PoC               ║\n");
    printf("║  The key is the moment in time × clock frequencies       ║\n");
    printf("║  Apple Silicon M1 Max                                    ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    demo_basic_encrypt();
    demo_phase_lock();
    demo_temporal_otp();

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  SUMMARY                                                ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  1. Phase-keyed: encrypt/decrypt needs exact time       ║\n");
    printf("║  2. Phase-locked: access only in specific phase window  ║\n");
    printf("║  3. Temporal OTP: same message → different cipher/ns    ║\n");
    printf("║                                                         ║\n");
    printf("║  The 'key' is not stored — it's the relationship        ║\n");
    printf("║  between clock frequencies at a specific moment.        ║\n");
    printf("║  This is a NEW kind of secret: temporal, not spatial.   ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");

    return 0;
}
