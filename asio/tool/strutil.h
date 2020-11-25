#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ningto
{
    namespace strutil {
        // Supports std::string("hello") + 10.
        std::string operator+(const std::string& s, int n);  // NOLINT(runtime/std::string)

        // Converts integer to std::string.
        std::string SimpleItoa(uint64_t n);
        std::string SimpleItoa(int64_t n);
        std::string SimpleItoa(int n);

        // Returns whether the provided std::string starts with the supplied prefix.
        bool HasPrefixString(const std::string& s, const std::string& prefix);

        // Returns the index of the nth occurence of c in s or std::string::npos if less than
        // n occurrences are present.
        size_t FindNth(const std::string& s, char c, int n);

        // Splits a std::string using a character delimiter. Appends the components to the
        // provided std::vector. Note that empty tokens are ignored.
        void SplitStringUsing(const std::string& s, const std::string& delimiter,
            std::vector<std::string>* result);

        std::string FromVector(const std::vector<std::string> &v, const std::string &delimiter);

        // Replaces any occurrence of the character 'remove' (or the characters
        // in 'remove') with the character 'replacewith'.
        void StripString(std::string* s, const char* remove, char replacewith);

        // Returns true if 'in' starts with 'prefix' and writes 'in' minus 'prefix' into
        // 'out'.
        bool TryStripPrefixString(const std::string& in, const std::string& prefix, std::string* out);

        // Returns true if 's' ends with 'suffix'.
        bool HasSuffixString(const std::string& s, const std::string& suffix);

        // Converts std::string to int32_t.
        void safe_strto32(const std::string& s, int32_t *n);

        // Converts std::string to uint64_t.
        void safe_strtou64(const std::string& s, uint64_t *n);

        // Converts std::string to int64_t.
        void safe_strto64(const std::string& s, int64_t* n);

        // Remove all occurrences of a given set of characters from a std::string.
        void strrmm(std::string* s, const std::string& chars);

        // Replaces all instances of 'substring' in 's' with 'replacement'. Returns the
        // number of instances replaced. Replacements are not subject to re-matching.
        int GlobalReplaceSubstring(const std::string& substring, const std::string& replacement,
            std::string* s);

        std::string random_string(int length);

        // Holds a reference to a std::std::string or C std::string. It can also be constructed
        // from an integer which is converted to a std::string.
        class StringHolder {
        public:
            // Don't make the constructors explicit to make the StrCat usage convenient.
            StringHolder(const std::string& s);  // NOLINT(runtime/explicit)
            StringHolder(const char* s);    // NOLINT(runtime/explicit)
            StringHolder(uint64_t n);         // NOLINT(runtime/explicit)
            ~StringHolder();

            const std::string* GetString() const {
                return string_;
            }

            const char* GetCString() const {
                return cstring_;
            }

            size_t Length() const {
                return len_;
            }

        private:
            const std::string converted_string_;  // Stores the std::string converted from integer.
            const std::string* const string_;
            const char* const cstring_;
            const size_t len_;
        };

        std::string& operator+=(std::string& lhs, const StringHolder& rhs);

        // Efficient std::string concatenation.

        std::string StrCat(const StringHolder& s1, const StringHolder& s2);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11, const StringHolder& s12);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11, const StringHolder& s12,
            const StringHolder& s13);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11, const StringHolder& s12,
            const StringHolder& s13, const StringHolder& s14);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11, const StringHolder& s12,
            const StringHolder& s13, const StringHolder& s14,
            const StringHolder& s15);

        std::string StrCat(const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5, const StringHolder& s6,
            const StringHolder& s7, const StringHolder& s8,
            const StringHolder& s9, const StringHolder& s10,
            const StringHolder& s11, const StringHolder& s12,
            const StringHolder& s13, const StringHolder& s14,
            const StringHolder& s15, const StringHolder& s16);

        void StrAppend(std::string* dest, const StringHolder& s1);

        void StrAppend(std::string* dest, const StringHolder& s1, const StringHolder& s2);

        void StrAppend(std::string* dest, const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3);

        void StrAppend(std::string* dest, const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4);

        void StrAppend(std::string* dest, const StringHolder& s1, const StringHolder& s2,
            const StringHolder& s3, const StringHolder& s4,
            const StringHolder& s5);
    }
}