#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "vedirect_parser.h"

void dashboard_init(void);
void dashboard_accept_measurement(const vedirect_measurement_t *measurement);
void dashboard_render(const vedirect_parser_t *parser);

#endif
