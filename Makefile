CC = bin/sharpie
CFLAGS = -O
TARGET = out/macabre.shr
SRC = src/main.c src/font.c
ASSETS = assets/font.png

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) $(CFLAGS) -o $(TARGET)

export_assets: $(ASSETS)
	$(CC) $(ASSETS) -o src/

clean:
	rm -f $(TARGET)

.PHONY: all export_assets clean
