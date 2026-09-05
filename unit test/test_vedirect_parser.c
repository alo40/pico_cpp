#include "vedirect_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_CAPACITY 512

typedef struct
{
    uint8_t bytes[FRAME_CAPACITY];
    size_t length;
} frame_t;

static int failures = 0;

#define EXPECT(condition)                                                   \
    do                                                                      \
    {                                                                       \
        if (!(condition))                                                   \
        {                                                                   \
            fprintf(                                                        \
                stderr,                                                     \
                "FAIL %s:%d: %s\n",                                        \
                __FILE__,                                                   \
                __LINE__,                                                   \
                #condition                                                  \
            );                                                              \
            failures++;                                                     \
        }                                                                   \
    } while (0)

static void append_bytes(frame_t *frame, const void *bytes, size_t length)
{
    EXPECT(frame->length + length <= sizeof(frame->bytes));

    if (frame->length + length > sizeof(frame->bytes))
    {
        exit(EXIT_FAILURE);
    }

    memcpy(&frame->bytes[frame->length], bytes, length);
    frame->length += length;
}

static void append_text(frame_t *frame, const char *text)
{
    append_bytes(frame, text, strlen(text));
}

static uint8_t byte_sum(const frame_t *frame)
{
    uint8_t sum = 0;

    for (size_t i = 0; i < frame->length; i++)
    {
        sum = (uint8_t)(sum + frame->bytes[i]);
    }

    return sum;
}

static void append_required_fields(frame_t *frame)
{
    append_text(frame, "\r\nV\t12750");
    append_text(frame, "\r\nVPV\t18420");
    append_text(frame, "\r\nI\t-850");
    append_text(frame, "\r\nPPV\t16");
}

static void finish_valid_frame(frame_t *frame)
{
    append_text(frame, "\r\nChecksum\t");

    uint8_t checksum = (uint8_t)(0u - byte_sum(frame));
    append_bytes(frame, &checksum, 1);
}

static frame_t make_valid_frame(void)
{
    frame_t frame = {0};
    append_required_fields(&frame);
    finish_valid_frame(&frame);
    return frame;
}

static frame_t make_frame_with_checksum(uint8_t wanted_checksum)
{
    frame_t frame = {0};
    append_required_fields(&frame);
    append_text(&frame, "\r\nX\t");

    static const char suffix[] = "\r\nChecksum\t";
    uint8_t suffix_sum = 0;

    for (size_t i = 0; i < sizeof(suffix) - 1; i++)
    {
        suffix_sum = (uint8_t)(suffix_sum + (uint8_t)suffix[i]);
    }

    bool found = false;

    for (int a = 32; a <= 126 && !found; a++)
    {
        for (int b = 32; b <= 126 && !found; b++)
        {
            for (int c = 32; c <= 126; c++)
            {
                uint8_t total = (uint8_t)(
                    byte_sum(&frame) +
                    (uint8_t)a +
                    (uint8_t)b +
                    (uint8_t)c +
                    suffix_sum +
                    wanted_checksum
                );

                if (total == 0)
                {
                    uint8_t filler[] = {
                        (uint8_t)a,
                        (uint8_t)b,
                        (uint8_t)c
                    };

                    append_bytes(&frame, filler, sizeof(filler));
                    found = true;
                    break;
                }
            }
        }
    }

    EXPECT(found);
    append_text(&frame, suffix);
    append_bytes(&frame, &wanted_checksum, 1);
    EXPECT(byte_sum(&frame) == 0);
    return frame;
}

static vedirect_result_t feed_frame(
    vedirect_parser_t *parser,
    const frame_t *frame,
    vedirect_measurement_t *measurement
)
{
    vedirect_result_t final_result = VEDIRECT_IN_PROGRESS;

    for (size_t i = 0; i < frame->length; i++)
    {
        vedirect_result_t result = vedirect_parser_feed(
            parser,
            frame->bytes[i],
            measurement
        );

        if (result != VEDIRECT_IN_PROGRESS)
        {
            EXPECT(final_result == VEDIRECT_IN_PROGRESS);
            final_result = result;
        }
    }

    return final_result;
}

static void expect_measurement(const vedirect_measurement_t *measurement)
{
    EXPECT(measurement->battery_mv == 12750);
    EXPECT(measurement->panel_mv == 18420);
    EXPECT(measurement->battery_ma == -850);
    EXPECT(measurement->panel_w == 16);
}

static void test_valid_block(void)
{
    vedirect_parser_t parser;
    vedirect_measurement_t measurement = {0};
    frame_t frame = make_valid_frame();

    vedirect_parser_init(&parser);

    EXPECT(
        feed_frame(&parser, &frame, &measurement) ==
        VEDIRECT_VALID_BLOCK
    );
    expect_measurement(&measurement);
    EXPECT(parser.received_blocks == 1);
    EXPECT(parser.valid_blocks == 1);
    EXPECT(parser.invalid_checksum_blocks == 0);
    EXPECT(parser.incomplete_blocks == 0);
}

static void test_corrupted_block(void)
{
    vedirect_parser_t parser;
    vedirect_measurement_t measurement = {0};
    frame_t frame = make_valid_frame();

    frame.bytes[5] ^= 1u;
    vedirect_parser_init(&parser);

    EXPECT(
        feed_frame(&parser, &frame, &measurement) ==
        VEDIRECT_INVALID_CHECKSUM
    );
    EXPECT(parser.received_blocks == 1);
    EXPECT(parser.valid_blocks == 0);
    EXPECT(parser.invalid_checksum_blocks == 1);
}

static void test_binary_checksum_bytes(void)
{
    const uint8_t checksum_values[] = {0x00, '\r', '\n'};

    for (
        size_t i = 0;
        i < sizeof(checksum_values) / sizeof(checksum_values[0]);
        i++
    )
    {
        vedirect_parser_t parser;
        vedirect_measurement_t measurement = {0};
        frame_t frame = make_frame_with_checksum(checksum_values[i]);

        vedirect_parser_init(&parser);

        EXPECT(
            feed_frame(&parser, &frame, &measurement) ==
            VEDIRECT_VALID_BLOCK
        );
        expect_measurement(&measurement);
    }
}

static void test_truncated_block(void)
{
    vedirect_parser_t parser;
    vedirect_measurement_t measurement = {0};
    frame_t frame = make_valid_frame();

    frame.length--;
    vedirect_parser_init(&parser);

    EXPECT(
        feed_frame(&parser, &frame, &measurement) ==
        VEDIRECT_IN_PROGRESS
    );
    EXPECT(parser.received_blocks == 0);
    EXPECT(parser.valid_blocks == 0);
}

static void test_recovery_after_invalid_block(void)
{
    vedirect_parser_t parser;
    vedirect_measurement_t measurement = {0};
    frame_t invalid = make_valid_frame();
    frame_t valid = make_valid_frame();

    invalid.bytes[5] ^= 1u;
    vedirect_parser_init(&parser);

    EXPECT(
        feed_frame(&parser, &invalid, &measurement) ==
        VEDIRECT_INVALID_CHECKSUM
    );
    EXPECT(
        feed_frame(&parser, &valid, &measurement) ==
        VEDIRECT_VALID_BLOCK
    );
    expect_measurement(&measurement);
    EXPECT(parser.received_blocks == 2);
    EXPECT(parser.valid_blocks == 1);
    EXPECT(parser.invalid_checksum_blocks == 1);
}

static void test_missing_required_field(void)
{
    vedirect_parser_t parser;
    vedirect_measurement_t measurement = {0};
    frame_t frame = {0};

    append_text(&frame, "\r\nV\t12750");
    append_text(&frame, "\r\nVPV\t18420");
    append_text(&frame, "\r\nI\t-850");
    finish_valid_frame(&frame);
    vedirect_parser_init(&parser);

    EXPECT(
        feed_frame(&parser, &frame, &measurement) ==
        VEDIRECT_INCOMPLETE_BLOCK
    );
    EXPECT(parser.received_blocks == 1);
    EXPECT(parser.valid_blocks == 0);
    EXPECT(parser.incomplete_blocks == 1);
}

static void test_multiple_blocks(void)
{
    vedirect_parser_t parser;
    vedirect_measurement_t measurement = {0};
    frame_t frame = make_valid_frame();

    vedirect_parser_init(&parser);

    EXPECT(
        feed_frame(&parser, &frame, &measurement) ==
        VEDIRECT_VALID_BLOCK
    );
    EXPECT(
        feed_frame(&parser, &frame, &measurement) ==
        VEDIRECT_VALID_BLOCK
    );
    EXPECT(parser.received_blocks == 2);
    EXPECT(parser.valid_blocks == 2);
}

static void test_overlong_line(void)
{
    vedirect_parser_t parser;
    vedirect_measurement_t measurement = {0};
    frame_t frame = {0};

    append_required_fields(&frame);
    append_text(&frame, "\r\nX\t");

    for (size_t i = 0; i < VEDIRECT_LINE_CAPACITY + 10; i++)
    {
        append_text(&frame, "A");
    }

    finish_valid_frame(&frame);
    vedirect_parser_init(&parser);

    EXPECT(
        feed_frame(&parser, &frame, &measurement) ==
        VEDIRECT_INCOMPLETE_BLOCK
    );
    EXPECT(parser.valid_blocks == 0);
    EXPECT(parser.incomplete_blocks == 1);
}

int main(void)
{
    test_valid_block();
    test_corrupted_block();
    test_binary_checksum_bytes();
    test_truncated_block();
    test_recovery_after_invalid_block();
    test_missing_required_field();
    test_multiple_blocks();
    test_overlong_line();

    if (failures != 0)
    {
        fprintf(stderr, "%d test assertion(s) failed\n", failures);
        return EXIT_FAILURE;
    }

    puts("All VE.Direct parser tests passed");
    return EXIT_SUCCESS;
}
