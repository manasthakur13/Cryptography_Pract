# Geffe-11 Stream Cipher

A high-performance file encryption tool developed in C. This application implements a custom **Stream Cipher** architecture using **11 Linear Feedback Shift Registers (LFSRs)** combined via **Non-Linear Geffe Generator logic** to encrypt and decrypt files of any format (binary, text, media) at the bit level.

## Project Overview

This tool demonstrates low-level cryptographic implementation and secure binary file handling. Unlike standard library wrappers, it manually manages bitwise state across 11 distinct registers to generate a high-entropy keystream. It processes files as raw binary streams, ensuring compatibility with large video files (`.mp4`), images, and executables without data corruption.

## Key Features

* **Universal File Compatibility:** Operates in binary mode (`rb`/`wb`), allowing encryption of any file type regardless of content or encoding.
* **11-Stage LFSR Engine:** Utilizes 11 independent shift registers, each initialized with unique primitive polynomials to maximize cycle length.
* **Non-Linear Combination:** Implements **Cascaded Geffe Generator** logic to combine register outputs, mitigating linear correlation attacks.
* **Atomic File Handling:** Uses a write-swap-replace mechanism (writing to `.tmp` first) to prevent data loss in the event of a crash or power failure.
* **Zero Dependencies:** Built entirely with standard C libraries (`stdio.h`, `stdlib.h`, `stdint.h`, `string.h`).

## Technical Architecture

The core cryptographic engine functions as follows:

1. **Initialization:** The user's key is hashed and used to seed 11 separate LFSRs.
2. **Keystream Generation:**
* The engine steps all 11 registers simultaneously.
* Outputs are fed into a **Geffe Combiner Function**: `(Control & InputA) ^ (!Control & InputB)`.
* This non-linear combination produces a complex keystream bit.


3. **XOR Encryption:** The generated keystream byte is XORed with the raw file byte (`Plaintext ⊕ Key = Ciphertext`).
4. **Symmetric Operation:** The same process is used for both encryption and decryption.

## Installation

Ensure you have a GCC compiler installed.

```bash
gcc cipher_geffe.c -o cipher

```

## Usage

The tool accepts the target file path as a command-line argument. It automatically handles file paths containing spaces.

**Syntax:**

```bash
./cipher <file_path>

```

**Example:**

```bash
./cipher "C:\Users\Data\video.mp4"

```

### workflow

1. Run the command with the target file.
2. Enter a secure password when prompted.
3. The file is encrypted in-place (safely via temp file).
4. To **decrypt**, run the exact same command with the same password.

## Safety Mechanisms

To ensure data integrity, the application strictly adheres to the **Atomic Save** pattern:

1. Opens the source file in `rb` (Read Binary) mode.
2. Creates a temporary destination file (`filename.tmp`).
3. Processes data in 64KB chunks from source to temp.
4. **On Success Only:** Removes the original file and renames the `.tmp` file to the original name.

## Disclaimer

This project is intended for educational and research purposes to demonstrate the implementation of stream ciphers and bitwise logic in C. It is not recommended for securing sensitive data in production environments where AES-256 or ChaCha20 standards are required.

## Author
Developed by Manas Thakur

Passionate about Low-Level C and Cryptography.
