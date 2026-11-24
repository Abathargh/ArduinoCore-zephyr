/*
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Scrolls "HELLO WORLD" on the 13x8 charlieplexed LED matrix of the Ventuno Q.
 * The matrix is driven directly from a timer interrupt (see loader/matrix.inc).
 */

#include <Arduino.h>
#include <zephyr/drivers/counter.h>
#include <string.h>
#include <array>
#include <unordered_map>
#include <vector>

#define MATRIX_W 13
#define MATRIX_H 8
#define NUM_LEDS 104
#define COUNTER  DT_NODELABEL(counter_matrix)

// Charlieplexing pin pairs {anode, cathode} on GPIOH, one entry per LED
static const uint8_t pins[NUM_LEDS][2] = {
	{2, 3},   {3, 2},   {2, 4},  {4, 2},  {3, 4},  {4, 3},  {2, 5},   {5, 2},   {3, 5},   {5, 3},
	{4, 5},   {5, 4},   {2, 6},  {6, 2},  {3, 6},  {6, 3},  {4, 6},   {6, 4},   {5, 6},   {6, 5},
	{2, 7},   {7, 2},   {3, 7},  {7, 3},  {4, 7},  {7, 4},  {5, 7},   {7, 5},   {6, 7},   {7, 6},
	{2, 9},   {9, 2},   {3, 9},  {9, 3},  {4, 9},  {9, 4},  {5, 9},   {9, 5},   {6, 9},   {9, 6},
	{7, 9},   {9, 7},   {2, 10}, {10, 2}, {3, 10}, {10, 3}, {4, 10},  {10, 4},  {5, 10},  {10, 5},
	{6, 10},  {10, 6},  {7, 10}, {10, 7}, {9, 10}, {10, 9}, {2, 13},  {13, 2},  {3, 13},  {13, 3},
	{4, 13},  {13, 4},  {5, 13}, {13, 5}, {6, 13}, {13, 6}, {7, 13},  {13, 7},  {9, 13},  {13, 9},
	{10, 13}, {13, 10}, {2, 14}, {14, 2}, {3, 14}, {14, 3}, {4, 14},  {14, 4},  {5, 14},  {14, 5},
	{6, 14},  {14, 6},  {7, 14}, {14, 7}, {9, 14}, {14, 9}, {10, 14}, {14, 10}, {13, 14}, {14, 13},
	{2, 15},  {15, 2},  {3, 15}, {15, 3}, {4, 15}, {15, 4}, {5, 15},  {15, 5},  {6, 15},  {15, 6},
	{7, 15},  {15, 7},  {9, 15}, {15, 9},
};

static volatile uint8_t framebuffer[NUM_LEDS / 8];

static void turnLed(int idx, bool on) {
	GPIOH->MODER &= 0x03C3000FU; // keep pins 0, 1, 8, 11, 12 untouched
	if (on) {
		uint8_t pin0 = pins[idx][0];
		uint8_t pin1 = pins[idx][1];

		GPIOH->BSRR = (1U << pin0) | (1U << (pin1 + 16));
		GPIOH->MODER |= (1U << (pin0 << 1)) | (1U << (pin1 << 1));
	}
}

// Lights one LED per tick, cycling through the whole matrix
static void matrixIsr(const struct device *dev, void *user_data) {
	static int i = 0;
	turnLed(i, framebuffer[i >> 3] & (1 << (i % 8)));
	i = (i + 1) % NUM_LEDS;
}

static void matrixBegin() {
	const struct device *dev = DEVICE_DT_GET(COUNTER);
	struct counter_top_cfg cfg = {};

	counter_start(dev);
	cfg.ticks = counter_us_to_ticks(dev, 10);
	cfg.callback = matrixIsr;
	counter_set_top_value(dev, &cfg);
}

// Each glyph is 5 columns wide, 7 rows tall: one byte per column, bit 0 is the top row
using Glyph = std::array<uint8_t, 5>;

static const std::unordered_map<char, Glyph> font = {
	{' ', {0x00, 0x00, 0x00, 0x00, 0x00}}, {'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}},
	{'B', {0x7F, 0x49, 0x49, 0x49, 0x36}}, {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
	{'D', {0x7F, 0x41, 0x41, 0x22, 0x1C}}, {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
	{'F', {0x7F, 0x09, 0x09, 0x09, 0x01}}, {'G', {0x3E, 0x41, 0x49, 0x49, 0x7A}},
	{'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}}, {'I', {0x00, 0x41, 0x7F, 0x41, 0x00}},
	{'J', {0x20, 0x40, 0x41, 0x3F, 0x01}}, {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
	{'L', {0x7F, 0x40, 0x40, 0x40, 0x40}}, {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
	{'N', {0x7F, 0x04, 0x08, 0x10, 0x7F}}, {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
	{'P', {0x7F, 0x09, 0x09, 0x09, 0x06}}, {'Q', {0x3E, 0x41, 0x51, 0x21, 0x5E}},
	{'R', {0x7F, 0x09, 0x19, 0x29, 0x46}}, {'S', {0x46, 0x49, 0x49, 0x49, 0x31}},
	{'T', {0x01, 0x01, 0x7F, 0x01, 0x01}}, {'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}},
	{'V', {0x1F, 0x20, 0x40, 0x20, 0x1F}}, {'W', {0x3F, 0x40, 0x38, 0x40, 0x3F}},
	{'X', {0x63, 0x14, 0x08, 0x14, 0x63}}, {'Y', {0x07, 0x08, 0x70, 0x08, 0x07}},
	{'Z', {0x61, 0x51, 0x49, 0x45, 0x43}},
};

// Edit this to change what is scrolled (letters are case-insensitive, unknown chars are blank)
static const char message[] = "ZEPHYR ON VENTUNOQ";

// The full message as a strip of columns (1 blank column between glyphs)
static std::vector<uint8_t> strip;
static size_t stripLen = 0;

static void buildStrip() {
	for (const char *p = message; *p; p++) {
		auto it = font.find(toupper(*p));
		Glyph g = it != font.end() ? it->second : Glyph{};
		strip.insert(strip.end(), g.begin(), g.end());
		strip.push_back(0);
	}
	stripLen = strip.size();
}

// Renders the strip starting at column `offset`, wrapping around with a blank gap
static void render(size_t offset) {
	uint8_t fb[NUM_LEDS / 8] = {};
	size_t period = stripLen + MATRIX_W;

	for (int x = 0; x < MATRIX_W; x++) {
		size_t s = (offset + x) % period;
		uint8_t column = s < stripLen ? strip[s] : 0;
		for (int y = 0; y < MATRIX_H; y++) {
			if (column & (1 << y)) {
				int idx = y * MATRIX_W + x;
				fb[idx >> 3] |= 1 << (idx % 8);
			}
		}
	}
	memcpy((void *)framebuffer, fb, sizeof(fb));
}

void setup() {
	buildStrip();
	matrixBegin();
}

void loop() {
	static size_t offset = 0;
	render(offset);
	offset = (offset + 1) % (stripLen + MATRIX_W);
	delay(100);
}
