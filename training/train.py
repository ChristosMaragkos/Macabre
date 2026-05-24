# Training script for MACABRE
# This reads a corpus and identifies prevalent word and character bigrams
# (words that often follow specific words etc.), and emits a C file
# for use in a Sharpie cartridge.

import os
import re
import sys
from collections import defaultdict

# This is the complete list of characters we will allow the model to use.
# The position of each character in this string is its index:
# its ID number that we use everywhere in the table.
# 'a' is index 0, 'b' is index 1, and so on and so forth.

CHARSET = (
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"  # indices 0-25: letters
    "0123456789"  # indices 26-35: digits
    " "  # index  36:     space (word boundary)
    ".,!?'-:;()#"  # indices 37-47: punctuation + hash
    "\n"  # index  48:     newline
    # indices 49-63: reserved / padding
)

# Pad to exactly 64 characters with null bytes.
# This makes the table exactly 64x64 = 4096 bytes.
# Null bytes are never generated, they're just padding
# so our array size is a power of two, which will make the
# index arithmetic simpler for to calculate down the line.

CHARSET = CHARSET.ljust(64, "\x00")
CHARSET_SIZE = 64

# Count how many real (non-padding) characters we have
REAL_CHARSET_SIZE = len(CHARSET.rstrip("\x00"))
print(
    f"Character set: {REAL_CHARSET_SIZE} real characters, "
    f"{CHARSET_SIZE - REAL_CHARSET_SIZE} padding slots."
)

char_to_index = {}
for i, ch in enumerate(CHARSET):
    if ch != "\x00":
        char_to_index[ch] = i

# MACABRE will be able to operate in two modes:
#   - Character mode: generate one letter at a time
#   - Word mode: pick whole words at a time
#
# We'll determine the vocabulary automatically from the corpus
# later (most frequent words win), but we set the maximum
# vocabulary size here. Each word gets an index just like
# each character does.
#
# Memory cost: VOCAB_SIZE x VOCAB_SIZE bytes for the word table.
# At 128 words: 128 x 128 = 16,384 bytes = half a Sharpie bank.
# At 200 words: 200 x 200 = 40,000 bytes = more than one bank.
# 128 is a comfortable limit that covers the vast majority of
# common words in any corpus. We're going to place all that in bank 0.

MAX_VOCAB_SIZE = 128

# We have to do some sanitizing:
#   - Convert Windows line endings (\r\n) to Unix (\n)
#   - Convert smart quotes to straight quotes
#   - Convert em dashes and en dashes to hyphens
#   - Strip any characters outside our charset
#
corpus_file = (
    sys.argv[1]  # NOTE FOR SELF: in python, argv[0] is the script name.
    if len(sys.argv) > 1
    else os.path.join(os.path.dirname(__file__), "corpus.txt")
)

if not os.path.exists(corpus_file):
    print(f"Error: could not find '{corpus_file}'.")
    print("Put your training data in a file called corpus.txt")
    print("next to this script, or pass the filename as an argument:")
    print("  python macabre_train.py my_script.txt")
    sys.exit(1)

with open(corpus_file, "r", encoding="utf-8") as f:
    raw_text = f.read().upper()

print(f"Read {len(raw_text):,} characters from '{corpus_file}'.")


# We replace characters that aren't in our charset with
# their closest equivalent, or skip them entirely
def clean_text(text):
    # Normalize line endings
    text = text.replace("\r\n", "\n").replace("\r", "\n")

    replacements = {
        "\u2018": "'",  # left single quote  -> apostrophe
        "\u2019": "'",  # right single quote -> apostrophe
        "\u201c": "'",  # left double quote  -> apostrophe
        "\u201d": "'",  # right double quote -> apostrophe
        "\u2013": "-",  # en dash            -> hyphen
        "\u2014": "-",  # em dash            -> hyphen
        "\u2026": ".",  # ellipsis           -> period
        "\t": " ",  # tab                -> space
    }
    for original, replacement in replacements.items():
        text = text.replace(original, replacement)

    # Collapse multiple spaces into one.
    # Multiple consecutive spaces don't add useful information
    # to the transition table and just waste training signal.
    text = re.sub(r" +", " ", text)

    # Collapse multiple newlines into one as well
    text = re.sub(r"\n+", "\n", text)

    return text


text = clean_text(raw_text)
print(f"After cleaning: {len(text):,} characters.")

# Count how many characters in the cleaned text are outside
# our charset. If this is high, we might want to expand the
# charset or do more cleaning. For example, after converting the text to uppercase entirely,
# it dropped from ~50% to 40 characters (so nearly 0%)
outside_charset = sum(1 for ch in text if ch not in char_to_index)
print(
    f"Characters outside charset (will be skipped): "
    f"{outside_charset:,} "
    f"({100 * outside_charset / len(text):.1f}%)"
)

# We slide a window of size 2 across the entire text.
# At each position we look at the current character and the
# next character. We find their indices and increment the
# counter at char_counts[current_index][next_index].
#
# After processing the entire text, char_counts[i][j] tells
# us "how many times did character j immediately follow
# character i in the training data?"
# This is essentially a markov chain in and of itself, but there's more work to be done.
char_counts = defaultdict(lambda: defaultdict(int))

pairs_counted = 0
pairs_skipped = 0

for i in range(len(text) - 1):
    current = text[i]
    next_ch = text[i + 1]

    # Skip if either character isn't in our charset.
    if current not in char_to_index or next_ch not in char_to_index:
        pairs_skipped += 1
        continue

    current_idx = char_to_index[current]
    next_idx = char_to_index[next_ch]
    char_counts[current_idx][next_idx] += 1
    pairs_counted += 1

print(f"\nCharacter bigrams: {pairs_counted:,} counted, {pairs_skipped:,} skipped.")


# For the word-level table we first need to:
#   1. Tokenize the text into words (split on spaces/newlines)
#   2. Count word frequencies to find the most common words
#   3. Build a vocabulary of the top MAX_VOCAB_SIZE words
#   4. Count word bigrams (pairs of consecutive words)
#   5. Normalize to bytes
#
# A word that isn't in the vocabulary is treated as an
# "unknown" token. MACABRE will fall back to character mode
# whenever it encounters an unknown word.
def tokenize(text):
    # Split on whitespace.
    raw_words = text.split()
    words = []
    for word in raw_words:
        # Strip leading and trailing punctuation.
        word = word.strip('.,!?;:-"()')
        if word:  # skip empty strings
            words.append(word)
    return words


words = tokenize(text)
print(f"Total words in corpus: {len(words):,}")

# Count word frequencies.
# word_freq[word] = how many times that word appears.
word_freq = defaultdict(int)
for word in words:
    word_freq[word] += 1

# Sort words by frequency, most common first.
# We take the top MAX_VOCAB_SIZE words as our vocabulary.
sorted_words = sorted(word_freq.items(), key=lambda x: x[1], reverse=True)

vocabulary = [word for word, _ in sorted_words[:MAX_VOCAB_SIZE]]
vocab_size = len(vocabulary)
print(f"Vocabulary: top {vocab_size} words selected.")
print(f"Most common: {sorted_words[:10]}")

# Build a word-to-index lookup, same idea as char_to_index: basically another layer to our Markov chain
word_to_index = {word: i for i, word in enumerate(vocabulary)}

# Count word bigrams
word_counts = defaultdict(lambda: defaultdict(int))

word_pairs_counted = 0
word_pairs_skipped = 0

for i in range(len(words) - 1):
    current = words[i]
    next_word = words[i + 1]

    # Skip if either word isn't in our vocabulary.
    if current not in word_to_index or next_word not in word_to_index:
        word_pairs_skipped += 1
        continue

    current_idx = word_to_index[current]
    next_idx = word_to_index[next_word]
    word_counts[current_idx][next_idx] += 1
    word_pairs_counted += 1

print(f"Word bigrams: {word_pairs_counted:,} counted, {word_pairs_skipped:,} skipped.")


# Right now our counts might be in the thousands or tens of
# thousands. We need to squish each row down so the largest
# value becomes 255 and everything else scales proportionally.
#
# We also handle empty rows here: if a character or word
# never appeared in the training data, or never had a valid
# successor, we fill its row with a uniform distribution
# (all 1s) so the model never gets stuck.
def normalize_counts(counts, table_size, real_size):
    """
    Takes a 2D defaultdict of raw counts.
    Returns a 2D list (table_size x table_size) of byte values.
    """
    table = [[0] * table_size for _ in range(table_size)]

    empty_rows = 0

    for row_idx in range(real_size):
        row_counts = counts[row_idx]

        if not row_counts or max(row_counts.values()) == 0:
            # This character/word never had a valid successor.
            # Fill with uniform distribution so we never get stuck.
            for col_idx in range(real_size):
                table[row_idx][col_idx] = 1
            empty_rows += 1
            continue

        max_count = max(row_counts.values())

        for col_idx in range(table_size):
            count = row_counts.get(col_idx, 0)
            if count == 0:
                table[row_idx][col_idx] = 0
            else:
                # Scale to 1-255.
                # We use max(1, ...) so that any transition that
                # actually occurred in the training data gets at
                # least a weight of 1, even if it was very rare.
                # Without this, rare but valid transitions would
                # round down to 0 and become impossible.
                scaled = max(1, round((count / max_count) * 255))
                table[row_idx][col_idx] = scaled

    if empty_rows:
        print(f"  Warning: {empty_rows} empty rows filled with uniform distribution.")

    return table


print("\nNormalizing character table...")
char_table = normalize_counts(char_counts, CHARSET_SIZE, REAL_CHARSET_SIZE)

print("Normalizing word table...")
word_table = normalize_counts(word_counts, MAX_VOCAB_SIZE, vocab_size)

# Now we write the C file.
output_file = "src/macabre_table.c"

with open(output_file, "w", encoding="utf-8") as out:

    def w(line=""):
        out.write(line + "\n")

    w("// ================================================")
    w("// MACABRE - Markov-Assisted Character")
    w("//           Approximation-Based Recursion Engine")
    w("//")
    w("// Auto-generated by macabre_train.py")
    w("// Do not edit by hand - re-run the training script")
    w("// if you need to change anything.")
    w("// ================================================")
    w()
    w("// --- Constants ---")
    w(f"#define MACABRE_CHARSET_SIZE  {CHARSET_SIZE}")
    w(f"#define MACABRE_REAL_CHARS    {REAL_CHARSET_SIZE}")
    w(f"#define MACABRE_VOCAB_SIZE    {vocab_size}")
    w(f"#define MACABRE_SPACE_IDX     {char_to_index.get(' ', 36)}")
    w(f"#define MACABRE_NEWLINE_IDX   {char_to_index.get(chr(10), 47)}")
    w()

    w("// --- Character Set ---")
    w("// sprite_to_ascii gives you the ASCII value of")
    w("// the character at index i.")
    w("// Use this to convert a generated index back to a")
    w("// character you can display on screen.")
    w("const unsigned char sprite_to_ascii[64] = {")
    entries = []
    for i, ch in enumerate(CHARSET):
        entries.append(str(ord(ch)))
    # Write 16 values per line for readability.
    for line_start in range(0, 64, 16):
        chunk = entries[line_start : line_start + 16]
        w("    " + ", ".join(chunk) + ",")
    w("};")
    w()

    w("// ascii_to_sprite is the reverse; given a raw ASCII value,")
    w("// it gives you back the sprite index.")

    w("unsigned char ascii_to_sprite(unsigned char sprite_id) {")
    w("    switch (sprite_id) {")
    for i, ch in enumerate(CHARSET):
        if i <= 48:
            w("    case " + str(ord(ch)) + ":")
            w("        return " + str(i) + ";")
    w("    default:")
    w("        return 0;")
    w("    }")
    w("}")
    w()

    w("// --- Character Transition Table ---")
    w("// macabre_table[from][to] = probability weight (0-255)")
    w("// that character 'to' follows character 'from'.")
    w("// 0 means never, 255 means most likely transition")
    w("// from this character.")
    w("const unsigned char macabre_table[64][64] = {")
    for row_idx in range(CHARSET_SIZE):
        ch = CHARSET[row_idx]
        if ch == "\x00":
            label = f"padding {row_idx}"
        elif ch == "\n":
            label = "newline"
        elif ch == " ":
            label = "space"
        else:
            label = ch
        row_str = ", ".join(str(v) for v in char_table[row_idx])
        w(f"    {{ {row_str} }}, // [{row_idx:2d}] '{label}'")
    w("};")
    w()

    w("// --- Word Vocabulary ---")
    w("// macabre_vocab[i] is the word at index i.")
    w("// Use this to convert a generated word index back")
    w("// to a string you can emit character by character.")
    w(f"const char* macabre_vocab[{vocab_size}] = {{")
    for i, word in enumerate(vocabulary):
        safe_word = word.replace('"', '\\"')
        w(f'    "{safe_word}", // [{i:3d}] freq={word_freq[word]:,}')
    w("};")
    w()

    w("// --- Word Transition Table ---")
    w("// macabre_word_table[from][to] = probability weight")
    w("// that word 'to' follows word 'from'.")
    w(f"const unsigned char macabre_word_table[{vocab_size}][{vocab_size}] = {{")
    for row_idx in range(vocab_size):
        word = vocabulary[row_idx]
        row_str = ", ".join(
            str(word_table[row_idx][col_idx]) for col_idx in range(vocab_size)
        )
        w(f"    {{ {row_str} }}, // [{row_idx:3d}] '{word}'")
    w("};")

print(f"\nWritten to '{output_file}'.")

print("\nTraining results:\n")
print("\n - Top 5 transitions per character (most likely next char):")
for row_idx in range(REAL_CHARSET_SIZE):
    ch = CHARSET[row_idx]
    row = char_table[row_idx]
    max_val = max(row)
    if max_val == 0:
        continue
    # Find top 3 successors.
    indexed = [(v, i) for i, v in enumerate(row) if v > 0]
    indexed.sort(reverse=True)
    top = indexed[:3]
    top_str = ", ".join(f"'{CHARSET[i]}'={v}" for v, i in top)
    label = repr(ch)
    print(f"  {label:6s} -> {top_str}")

print("\n - Top 10 word transitions:")
for row_idx in range(min(20, vocab_size)):
    word = vocabulary[row_idx]
    row = word_table[row_idx]
    max_val = max(row)
    if max_val == 0:
        continue
    max_col = row.index(max_val)
    next_word = vocabulary[max_col]
    print(f"  '{word}' -> '{next_word}' (weight {max_val})")

print("\nDone.")
