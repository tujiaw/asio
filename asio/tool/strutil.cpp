#include <algorithm>
#include <cassert>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <random>
#include "strutil.h"

namespace ningto
{
    namespace strutil {
        std::string operator+(const std::string& s, int n) {  // NOLINT(runtime/std::string)
            std::stringstream stream;

            stream << s << n;
            std::string result;
            stream >> result;

            return result;
        }

        template <typename T>
        std::string GenericSimpleItoa(const T& n) {
            std::stringstream stream;

            stream << n;
            std::string result;
            stream >> result;

            return result;
        }

        std::string SimpleItoa(int n) {
            return GenericSimpleItoa(n);
        }

        std::string SimpleItoa(uint64_t n) {
            return GenericSimpleItoa(n);
        }

        std::string SimpleItoa(int64_t n) {
            return GenericSimpleItoa(n);
        }

        bool HasPrefixString(const std::string& s, const std::string& prefix) {
            return s.size() >= prefix.size() &&
                std::equal(s.begin(), s.begin() + prefix.size(), prefix.begin());
        }

        size_t FindNth(const std::string& s, char c, int n) {
            size_t pos = std::string::npos;

            for (int i = 0; i < n; ++i) {
                pos = s.find_first_of(c, pos + 1);
                if (pos == std::string::npos) {
                    break;
                }
            }
            return pos;
        }

        void SplitStringUsing(const std::string& s, const std::string& delimiter,
            std::vector<std::string>* result) {
            assert(result);
            size_t start_pos = 0;
            size_t find_pos = std::string::npos;
            if (delimiter.empty()) {
                return;
            }
            while ((find_pos = s.find(delimiter, start_pos)) != std::string::npos) {
                const std::string substring = s.substr(start_pos, find_pos - start_pos);
                if (!substring.empty()) {
                    result->push_back(substring);
                }
                start_pos = find_pos + delimiter.length();
            }
            if (start_pos != s.length()) {
                result->push_back(s.substr(start_pos));
            }
        }

        std::string FromVector(const std::vector<std::string> &v, const std::string &delimiter)
        {
            std::string result;
            std::size_t c = v.size();
            for (std::size_t i = 0; i < c; i++) {
                result += v[i];
                if (i != c - 1) {
                    result += delimiter;
                }
            }
            return result;
        }

        void StripString(std::string* s, const char* remove, char replacewith) {
            const char* str_start = s->c_str();
            const char* str = str_start;
            for (str = strpbrk(str, remove);
                str != NULL;
                str = strpbrk(str + 1, remove)) {
                (*s)[str - str_start] = replacewith;
            }
        }

        bool TryStripPrefixString(const std::string& in, const std::string& prefix, std::string* out) {
            assert(out);
            const bool has_prefix = in.compare(0, prefix.length(), prefix) == 0;
            out->assign(has_prefix ? in.substr(prefix.length()) : in);

            return has_prefix;
        }

        bool HasSuffixString(const std::string& s, const std::string& suffix) {
            if (s.length() < suffix.length()) {
                return false;
            }
            return s.compare(s.length() - suffix.length(), suffix.length(), suffix) == 0;
        }

        template <typename T>
        void GenericAtoi(const std::string& s, T* out) {
            std::stringstream stream;
            stream << s;
            stream >> *out;
        }

        void safe_strto32(const std::string& s, int32_t *n) {
            GenericAtoi(s, n);
        }

        void safe_strtou64(const std::string& s, uint64_t *n) {
            GenericAtoi(s, n);
        }

        void safe_strto64(const std::string& s, int64_t* n) {
            GenericAtoi(s, n);
        }

        void strrmm(std::string* s, const std::string& chars) {
            for (std::string::iterator it = s->begin(); it != s->end(); ) {
                const char current_char = *it;
                if (chars.find(current_char) != std::string::npos) {
                    it = s->erase(it);
                } else {
                    ++it;
                }
            }
        }

        int GlobalReplaceSubstring(const std::string& substring,
            const std::string& replacement,
            std::string* s) {
            assert(s != NULL);
            if (s->empty() || substring.empty())
                return 0;
            std::string tmp;
            int num_replacements = 0;
            int pos = 0;
            for (size_t match_pos = s->find(substring.data(), pos, substring.length());
                match_pos != std::string::npos;
                pos = match_pos + substring.length(),
                match_pos = s->find(substring.data(), pos, substring.length())) {
                ++num_replacements;
                // Append the original content before the match.
                tmp.append(*s, pos, match_pos - pos);
                // Append the replacement for the match.
                tmp.append(replacement.begin(), replacement.end());
            }
            // Append the content after the last match.
            tmp.append(*s, pos, s->length() - pos);
            s->swap(tmp);
            return num_replacements;
        }

        std::string random_string(int length)
        {
            const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
            const size_t max_index = (sizeof(charset) - 1);
            std::string str(length, 0);
            std::generate_n(str.begin(), length, [&]() {
                return charset[rand() % max_index];
            });
            return str;
        }

        // StringHolder class

        StringHolder::StringHolder(const std::string& s)
            : string_(&s),
            cstring_(NULL),
            len_(s.size())
        {}

        StringHolder::StringHolder(const char* s)
            : string_(NULL),
            cstring_(s),
            len_(std::strlen(s))
        {}

        StringHolder::StringHolder(uint64_t n)
            : converted_string_(SimpleItoa(n)),
            string_(&converted_string_),
            cstring_(NULL),
            len_(converted_string_.length())
        {}

        StringHolder::~StringHolder() {}

        // StrCat

        // Implements s += sh; (s: std::string, sh: StringHolder)
        std::string& operator+=(std::string& lhs, const StringHolder& rhs) {
            const std::string* const s = rhs.GetString();
            if (s) {
                lhs += *s;
            } else {
                const char* const cs = rhs.GetCString();
                if (cs)
                    lhs.append(cs, rhs.Length());
            }
            return lhs;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + 1);

            result += s1;
            result += s2;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + 1);

            result += s1;
            result += s2;
            result += s3;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() + 1);

            result += s1;
            result += s2;
            result += s3;
            result += s4;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + 1);
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + s6.Length() + 1);
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;
            result += s6;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + s6.Length() + s7.Length() + 1);
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;
            result += s6;
            result += s7;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + s6.Length() + s7.Length() + s8.Length() + 1);
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;
            result += s6;
            result += s7;
            result += s8;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + s6.Length() + s7.Length() + s8.Length() +
                s9.Length() + 1);
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;
            result += s6;
            result += s7;
            result += s8;
            result += s9;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + s6.Length() + s7.Length() + s8.Length() +
                s9.Length() + s10.Length() + s11.Length());
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;
            result += s6;
            result += s7;
            result += s8;
            result += s9;
            result += s10;
            result += s11;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11, const StringHolder& s12) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + s6.Length() + s7.Length() + s8.Length() +
                s9.Length() + s10.Length() + s11.Length() + s12.Length());
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;
            result += s6;
            result += s7;
            result += s8;
            result += s9;
            result += s10;
            result += s11;
            result += s12;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11, const StringHolder& s12,
            const StringHolder& s13) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + s6.Length() + s7.Length() + s8.Length() +
                s9.Length() + s10.Length() + s11.Length() + s12.Length() +
                s13.Length());
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;
            result += s6;
            result += s7;
            result += s8;
            result += s9;
            result += s10;
            result += s11;
            result += s12;
            result += s13;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11, const StringHolder& s12,
            const StringHolder& s13, const StringHolder& s14) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + s6.Length() + s7.Length() + s8.Length() +
                s9.Length() + s10.Length() + s11.Length() + s12.Length() +
                s13.Length() + s14.Length());
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;
            result += s6;
            result += s7;
            result += s8;
            result += s9;
            result += s10;
            result += s11;
            result += s12;
            result += s13;
            result += s14;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11, const StringHolder& s12,
            const StringHolder& s13, const StringHolder& s14,
            const StringHolder& s15) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + s6.Length() + s7.Length() + s8.Length() +
                s9.Length() + s10.Length() + s11.Length() + s12.Length() +
                s13.Length() + s14.Length() + s15.Length());
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;
            result += s6;
            result += s7;
            result += s8;
            result += s9;
            result += s10;
            result += s11;
            result += s12;
            result += s13;
            result += s14;
            result += s15;

            return result;
        }

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11, const StringHolder& s12,
            const StringHolder& s13, const StringHolder& s14,
            const StringHolder& s15, const StringHolder& s16) {
            std::string result;
            result.reserve(s1.Length() + s2.Length() + s3.Length() + s4.Length() +
                s5.Length() + s6.Length() + s7.Length() + s8.Length() +
                s9.Length() + s10.Length() + s11.Length() + s12.Length() +
                s13.Length() + s14.Length() + s15.Length() + s16.Length());
            result += s1;
            result += s2;
            result += s3;
            result += s4;
            result += s5;
            result += s6;
            result += s7;
            result += s8;
            result += s9;
            result += s10;
            result += s11;
            result += s12;
            result += s13;
            result += s14;
            result += s15;
            result += s16;

            return result;
        }

        // StrAppend

        void StrAppend(std::string* dest, const StringHolder& s1) {
            assert(dest);

            dest->reserve(dest->length() + s1.Length() + 1);
            *dest += s1;
        }

        void StrAppend(std::string* dest, const StringHolder& s1, const StringHolder& s2) {
            assert(dest);

            dest->reserve(dest->length() + s1.Length() + s2.Length() + 1);
            *dest += s1;
            *dest += s2;
        }

        void StrAppend(std::string* dest, const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3) {
            assert(dest);

            dest->reserve(dest->length() + s1.Length() + s2.Length() + s3.Length() + 1);
            *dest += s1;
            *dest += s2;
            *dest += s3;
        }

        void StrAppend(std::string* dest, const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4) {
            assert(dest);

            dest->reserve(dest->length() + s1.Length() + s2.Length() + s3.Length() +
                s4.Length() + 1);
            *dest += s1;
            *dest += s2;
            *dest += s3;
            *dest += s4;
        }

        void StrAppend(std::string* dest, const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5) {
            assert(dest);

            dest->reserve(dest->length() + s1.Length() + s2.Length() + s3.Length() +
                s4.Length() + s5.Length() + 1);
            *dest += s1;
            *dest += s2;
            *dest += s3;
            *dest += s4;
            *dest += s5;
        }
    }
}