CC      ?= cc
CFLAGS  ?= -O2 -Wall -Wextra
LIBS     = -lcups
BUILD    = build

all: $(BUILD)/rastertodetong

$(BUILD)/rastertodetong: src/rastertodetong.c
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

test: all tests/mkraster.c
	$(CC) -o $(BUILD)/mkraster tests/mkraster.c $(LIBS)
	$(BUILD)/mkraster > $(BUILD)/t.ras
	DETONG_BAND_MS=0 $(BUILD)/rastertodetong 1 u t 1 "" $(BUILD)/t.ras | wc -c

clean:
	rm -rf $(BUILD)

.PHONY: all test clean
