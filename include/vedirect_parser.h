#ifndef VEDIRECT_PARSER_H
#define VEDIRECT_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VEDIRECT_LINE_CAPACITY 64

typedef struct
{
    int32_t battery_mv;
    int32_t panel_mv;
    int32_t battery_ma;
    int32_t panel_w;
} vedirect_measurement_t;

typedef enum
{
    VEDIRECT_IN_PROGRESS = 0,
    VEDIRECT_VALID_BLOCK,
    VEDIRECT_INVALID_CHECKSUM,
    VEDIRECT_INCOMPLETE_BLOCK
} vedirect_result_t;

typedef struct
{
    vedirect_measurement_t pending;
    uint8_t checksum;
    uint8_t field_mask;
    char line[VEDIRECT_LINE_CAPACITY];
    size_t line_length;
    bool malformed;
    uint32_t received_blocks;
    uint32_t valid_blocks;
    uint32_t invalid_checksum_blocks;
    uint32_t incomplete_blocks;
} vedirect_parser_t;

void vedirect_parser_init(vedirect_parser_t *parser);

vedirect_result_t vedirect_parser_feed(
    vedirect_parser_t *parser,
    uint8_t byte,
    vedirect_measurement_t *measurement
);

#endif
