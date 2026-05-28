CC = bin/sharpie
CFLAGS = -O
ASMFLAGS = -OS
TARGET_SHR = out/macabre.shr
TARGET_ASM = out/macabre.asm
SRC = src/main.c src/font.c src/macabre_table.c src/text_rendering.c src/text_generation.c
ASSETS = assets/font.png

all: $(TARGET_SHR)

$(TARGET_SHR): $(SRC)
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET_SHR)

export_assets: $(ASSETS)
	$(CC) $(ASSETS) -o src/include
	sed -i "s/font.h/include\/font.h/" src/include/font.c
	mv src/include/font.c src/

clean:
	rm -f $(TARGET_SHR) $(TARGET_ASM)

asm:
	$(CC) $(SRC) $(ASMFLAGS) -o $(TARGET_ASM)

.PHONY: all export_assets asm clean
