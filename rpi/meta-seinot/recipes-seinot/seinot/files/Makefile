TARGET := seinot
OBJ := main.o mifare.o soundctrl.o playback.o nfc.o led.o
CFLAGS += -Wall -Wextra -Werror
LIBS := $(shell pkg-config --libs alsa libmpg123 libout123) -lnfc -lm

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJ)

.PHONE: clean
