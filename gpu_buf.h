#pragma once

#include "main.h"

void
create_vertex_buffer(handles_t *handles, std::vector<Vertex> vertices);

void
create_index_buffer(handles_t *handles, std::vector<uint16_t> indices);

void
create_uniform_buffer(handles_t *handles);

