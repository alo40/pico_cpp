#include "vedirect_parser.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define FIELD_BATTERY_VOLTAGE (1u << 0)
#define FIELD_PANEL_VOLTAGE   (1u << 1)
#define FIELD_BATTERY_CURRENT (1u << 2)
#define FIELD_PANEL_POWER     (1u << 3)

enum
{
    REQUIRED_FIELDS =
        FIELD_BATTERY_VOLTAGE |
        FIELD_PANEL_VOLTAGE |
        FIELD_BATTERY_CURRENT |
        FIELD_PANEL_POWER
};

static const char checksum_label[] = "Checksum\t";

static void reset_block(vedirect_parser_t *parser)
{
    memset(&parser->pending, 0, sizeof(parser->pending));
    parser->checksum = 0;
    parser->field_mask = 0;
    parser->line_length = 0;
    parser->malformed = false;
}

static bool parse_int32(const char *text, int32_t *value)
{
    char *end;
    long parsed = strtol(text, &end, 10);

    if (
        text == end ||
        *end != '\0' ||
        parsed < INT32_MIN ||
        parsed > INT32_MAX
    )
    {
        return false;
    }

    *value = (int32_t)parsed;
    return true;
}

static void process_line(vedirect_parser_t *parser)
{
    char *tab = strchr(parser->line, '\t');

    if (tab == NULL)
    {
        parser->malformed = true;
        return;
    }

    *tab = '\0';

    const char *label = parser->line;
    const char *value = tab + 1;
    int32_t parsed;

    if (strcmp(label, "V") == 0)
    {
        if (parse_int32(value, &parsed))
        {
            parser->pending.battery_mv = parsed;
            parser->field_mask |= FIELD_BATTERY_VOLTAGE;
        }
        else
        {
            parser->malformed = true;
        }
    }
    else if (strcmp(label, "VPV") == 0)
    {
        if (parse_int32(value, &parsed))
        {
            parser->pending.panel_mv = parsed;
            parser->field_mask |= FIELD_PANEL_VOLTAGE;
        }
        else
        {
            parser->malformed = true;
        }
    }
    else if (strcmp(label, "I") == 0)
    {
        if (parse_int32(value, &parsed))
        {
            parser->pending.battery_ma = parsed;
            parser->field_mask |= FIELD_BATTERY_CURRENT;
        }
        else
        {
            parser->malformed = true;
        }
    }
    else if (strcmp(label, "PPV") == 0)
    {
        if (parse_int32(value, &parsed))
        {
            parser->pending.panel_w = parsed;
            parser->field_mask |= FIELD_PANEL_POWER;
        }
        else
        {
            parser->malformed = true;
        }
    }
}

void vedirect_parser_init(vedirect_parser_t *parser)
{
    memset(parser, 0, sizeof(*parser));
}

vedirect_result_t vedirect_parser_feed(
    vedirect_parser_t *parser,
    uint8_t byte,
    vedirect_measurement_t *measurement
)
{
    parser->checksum = (uint8_t)(parser->checksum + byte);

    if (
        parser->line_length == sizeof(checksum_label) - 1 &&
        memcmp(
            parser->line,
            checksum_label,
            sizeof(checksum_label) - 1
        ) == 0
    )
    {
        vedirect_result_t result;

        parser->received_blocks++;

        if (parser->checksum != 0)
        {
            parser->invalid_checksum_blocks++;
            result = VEDIRECT_INVALID_CHECKSUM;
        }
        else if (
            parser->malformed ||
            parser->field_mask != REQUIRED_FIELDS
        )
        {
            parser->incomplete_blocks++;
            result = VEDIRECT_INCOMPLETE_BLOCK;
        }
        else
        {
            parser->valid_blocks++;

            if (measurement != NULL)
            {
                *measurement = parser->pending;
            }

            result = VEDIRECT_VALID_BLOCK;
        }

        reset_block(parser);
        return result;
    }

    if (byte == '\r')
    {
        return VEDIRECT_IN_PROGRESS;
    }

    if (byte == '\n')
    {
        if (parser->line_length > 0 && !parser->malformed)
        {
            parser->line[parser->line_length] = '\0';
            process_line(parser);
        }

        parser->line_length = 0;
        return VEDIRECT_IN_PROGRESS;
    }

    if ((byte >= 32 && byte <= 126) || byte == '\t')
    {
        if (parser->line_length < sizeof(parser->line) - 1)
        {
            parser->line[parser->line_length++] = (char)byte;
        }
        else
        {
            parser->malformed = true;
        }
    }
    else
    {
        parser->malformed = true;
    }

    return VEDIRECT_IN_PROGRESS;
}
