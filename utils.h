#pragma once

#include <vector>
#include <string>

void
bail_out(const char *msg);

void
check_res(VkResult res, const char *msg);

std::vector<char>
read_file(const std::string& filename);

const char *
yes_no(VkBool32 b);
