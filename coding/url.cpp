#include "coding/url.hpp"
#include "coding/hex.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <sstream>
#include <vector>

namespace url
{
using namespace std;

std::string DebugPrint(Param const & param)
{
  return "UrlParam [" + param.m_name + "=" + param.m_value + "]";
}

Url::Url(std::string const & url)
{
  if (!Parse(url))
  {
    ASSERT(m_scheme.empty() && m_host.empty() && m_path.empty() && !IsValid(), ());
  }
}

Url Url::FromString(std::string const & url)
{
  bool const hasProtocol = strings::StartsWith(url, "http://") || strings::StartsWith(url, "https://");
  return Url(hasProtocol ? url : "https://" + url);
}

bool Url::Parse(std::string const & url)
{
  // Get url scheme.
  size_t start = url.find(':');
  if (start == string::npos || start == 0)
    return false;
  m_scheme = url.substr(0, start);

  // Skip slashes.
  start = url.find_first_not_of('/', start + 1);
  if (start == std::string::npos)
    return true;

  // Get host.
  size_t end = url.find_first_of("?/", start);
  if (end == string::npos)
  {
    m_host = url.substr(start);
    return true;
  }
  else
    m_host = url.substr(start, end - start);

  // Get path.
  if (url[end] == '/')
  {
    // Skip slashes.
    start = url.find_first_not_of('/', end);
    if (start == std::string::npos)
      return true;

    end = url.find('?', start);
    if (end == string::npos)
    {
      m_path = url.substr(start);
      return true;
    }
    else
      m_path = url.substr(start, end - start);
  }

  // Parse query for keys and values.
  for (start = end + 1; start < url.size();)
  {
    end = url.find('&', start);
    if (end == string::npos)
      end = url.size();

    // Skip empty keys.
    if (end != start)
    {
      size_t const eq = url.find('=', start);

      string key;
      string value;
      if (eq != string::npos && eq < end)
      {
        key = UrlDecode(url.substr(start, eq - start));
        value = UrlDecode(url.substr(eq + 1, end - eq - 1));
      }
      else
      {
        key = UrlDecode(url.substr(start, end - start));
      }

      m_params.emplace_back(key, value);
    }

    start = end + 1;
  }

  return true;
}

string Make(string const & baseUrl, Params const & params)
{
  ostringstream os;
  os << baseUrl;

  bool firstParam = baseUrl.find('?') == string::npos;
  for (auto const & param : params)
  {
    if (firstParam)
    {
      firstParam = false;
      os << "?";
    }
    else
    {
      os << "&";
    }

    os << param.m_name << "=" << param.m_value;
  }

  return os.str();
}

string Join(string const & lhs, string const & rhs)
{
  if (lhs.empty())
    return rhs;
  if (rhs.empty())
    return lhs;

  if (lhs.back() == '/' && rhs.front() == '/')
    return lhs + rhs.substr(1);

  if (lhs.back() != '/' && rhs.front() != '/')
    return lhs + '/' + rhs;

  return lhs + rhs;
}

string UrlEncode(string const & rawUrl)
{
  size_t const count = rawUrl.size();
  string result;
  result.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    char const c = rawUrl[i];
    if (c < '-' || c == '/' || (c > '9' && c < 'A') || (c > 'Z' && c < '_') ||
        c == '`' || (c > 'z' && c < '~') || c > '~')
    {
      result += '%';
      result += NumToHex(c);
    }
    else
      result += rawUrl[i];
  }

  return result;
}

string UrlDecode(string const & encodedUrl)
{
  size_t const count = encodedUrl.size();
  string result;
  result.reserve(count);

  for (size_t i = 0; i < count; ++i)
  {
    if (encodedUrl[i] == '%')
    {
      result += FromHex(encodedUrl.substr(i + 1, 2));
      i += 2;
    }
    else
    {
      result += encodedUrl[i];
    }
  }

  return result;
}

}  // namespace url
