//
// Created by kizim on 3/3/16.
//

#ifndef FINAL_MIME_TYPES_HPP
#define FINAL_MIME_TYPES_HPP

#include <string>

struct mapping
{
  const char* extension;
  const char* mime_type;
} mappings[] =
    {
        { "gif", "image/gif" },
        { "htm", "text/html" },
        { "html", "text/html" },
        { "jpg", "image/jpeg" },
        { "png", "image/png" }
    };

std::string extension_to_type(const std::string& extension)
{
  for (mapping m: mappings)
  {
    if (m.extension == extension)
    {
      return m.mime_type;
    }
  }

  return "text/plain";
}

#endif //FINAL_MIME_TYPES_HPP
