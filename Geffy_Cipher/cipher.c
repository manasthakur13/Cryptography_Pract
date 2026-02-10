#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NUM_LFSRS 11
#define BUFFER_SIZE 65536 // 64KB Buffer

// --- 1. LFSR STRUCTURE & SETUP ---

typedef struct {
    uint32_t state; // Current bits
    uint32_t mask;  // Polynomial taps
} LFSR;

/*
 * Function: step_lfsr
 * Advances the register by 1 bit.
 * Returns the "dropped" bit (LSB) as output.
 */
uint8_t step_lfsr(LFSR *lfsr) {
    uint8_t output_bit = lfsr->state & 1;
    lfsr->state >>= 1;
    if (output_bit) {
        lfsr->state ^= lfsr->mask;
    }
    return output_bit;
}

/*
 * Function: init_lfsrs
 * Sets up 11 unique polynomials and seeds them with the Master Key.
 */
void init_lfsrs(LFSR lfsrs[], const char *key) {
    // 11 Primitive Polynomials (Maximal Length Taps)
    uint32_t polys[NUM_LFSRS] = {
        0xB400, 0xE000, 0x80200003, 0x80000057,
        0x80000062, 0x40000018, 0x10000002, 0x20000029,
        0x0080000D, 0x00200001, 0x00080005
    };

    size_t key_len = strlen(key);

    for (int i = 0; i < NUM_LFSRS; i++) {
        lfsrs[i].mask = polys[i];

        // Seed Generation: Mix Key + Index + Constant
        // This ensures every register starts with a unique, non-zero state.
        uint32_t seed = 0x5A5A5A5A; // Initial salt
        for (size_t k = 0; k < key_len; k++) {
            seed = (seed << 5) ^ seed ^ key[k];
        }
        seed = seed ^ (i * 0x12345678); 

        // Important: LFSR state must never be 0
        if (seed == 0) seed = 1;
        
        lfsrs[i].state = seed;
    }
}

// --- 2. GEFFE GENERATOR LOGIC ---

/*
 * Geffe Function:
 * If (control == 1) output input1
 * Else output input0
 * Boolean Logic: (control AND input1) XOR ((NOT control) AND input0)
 */
uint8_t geffe_bit(uint8_t control, uint8_t input1, uint8_t input0) {
    return (control & input1) ^ ((!control) & input0);
}

/*
 * Function: get_keystream_byte
 * Generates 8 bits of keystream to encrypt 1 byte of file data.
 */
unsigned char get_keystream_byte(LFSR lfsrs[]) {
    unsigned char result_byte = 0;

    // Loop 8 times to build one byte
    for (int bit = 0; bit < 8; bit++) {
        // 1. Step all 11 registers to get 11 fresh bits
        uint8_t b[NUM_LFSRS];
        for (int i = 0; i < NUM_LFSRS; i++) {
            b[i] = step_lfsr(&lfsrs[i]);
        }

        // 2. Apply Geffe Logic
        // We split the first 9 bits into 3 Geffe Blocks
        uint8_t geffe_A = geffe_bit(b[0], b[1], b[2]); // L0 controls L1 vs L2
        uint8_t geffe_B = geffe_bit(b[3], b[4], b[5]); // L3 controls L4 vs L5
        uint8_t geffe_C = geffe_bit(b[6], b[7], b[8]); // L6 controls L7 vs L8
        
        // 3. Combine Blocks and remaining 2 registers (L9, L10)
        // We XOR them all to preserve entropy from all sources
        uint8_t final_bit = geffe_A ^ geffe_B ^ geffe_C ^ b[9] ^ b[10];

        // 4. Shift into result
        result_byte = (result_byte << 1) | final_bit;
    }
    return result_byte;
}

// --- 3. FILE HANDLING (SAFE TEMP METHOD) ---

void remove_quotes(char *path) {
    if (!path) return;
    size_t len = strlen(path);
    if (len < 2) return;
    if (path[0] == '"' && path[len - 1] == '"') {
        memmove(path, path + 1, len - 2);
        path[len - 2] = '\0';
    }
}

void process_file(char *filename, const char *key) {
    // Generate Temp Filename
    char temp_filename[1024];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename);

    FILE *fin = fopen(filename, "rb");
    FILE *fout = fopen(temp_filename, "wb");

    if (!fin) {
        perror("Error opening input file");
        return;
    }
    if (!fout) {
        perror("Error creating temp file");
        fclose(fin);
        return;
    }

    printf("Processing: %s\n", filename);

    // Initialize Encryption Engine
    LFSR generators[NUM_LFSRS];
    init_lfsrs(generators, key);

    unsigned char *buffer = (unsigned char *)malloc(BUFFER_SIZE);
    size_t bytes_read;

    // --- MAIN ENCRYPTION LOOP ---
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fin)) > 0) {
        
        // Process the buffer byte-by-byte
        for (size_t i = 0; i < bytes_read; i++) {
            // Get 1 byte of chaos from our Geffe Engine
            unsigned char k = get_keystream_byte(generators);
            
            // XOR with file data
            buffer[i] = buffer[i] ^ k;
        }

        fwrite(buffer, 1, bytes_read, fout);
    }

    free(buffer);
    fclose(fin);
    fclose(fout);

    // --- ATOMIC SWAP ---
    if (remove(filename) != 0) {
        printf("Error: Could not delete original. Data saved in .tmp\n");
    } else if (rename(temp_filename, filename) != 0) {
        printf("Error: Could not rename temp file.\n");
    } else {
        printf("Success! Operation complete on %s\n", filename);
    }
}

// --- 4. MAIN ENTRY POINT ---

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    char filename[1024];
    strncpy(filename, argv[1], sizeof(filename)-1);
    filename[sizeof(filename)-1] = '\0';
    
    remove_quotes(filename);

    // Ideally, prompt user for input rather than hardcoding
    char key[256];
    printf("Enter Password: ");
    if (fgets(key, sizeof(key), stdin)) {
        key[strcspn(key, "\n")] = 0; // Remove newline
    }

    process_file(filename, key);

    return 0;
}
