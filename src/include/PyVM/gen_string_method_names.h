// Do not directly edit this file. this file is auto generated by a custom build step

#include <string>

enum EStringMethods
{
    STRM_BEGINSWITH = 1,
    STRM_CONTAINS,
    STRM_C_PTR,
    STRM_ENDSWITH,
    STRM_EQUALS,
    STRM_GLOB_PTR,
    STRM_IBEGINSWITH,
    STRM_ICONTAINS,
    STRM_IENDSWITH,
    STRM_IEQUALS,
    STRM_JOIN,
    STRM_LOWER,
    STRM_PATHBEGINSWITH,
    STRM_PATHCONTAINS,
    STRM_PATHENDSWITH,
    STRM_PATHEQUALS,
    STRM_SPLIT,
    STRM_STARTSWITH,
    STRM_STRIP,
};

EStringMethods stringMethod_enumFromStr(const std::string& s)
{
    const char *k = nullptr;
    EStringMethods v = (EStringMethods)0;
    int sz = (int)s.size();
    switch(sz)
    {
    case 5: { // 4 options
        if (s[0] == 's') { // 2 options
            if (s[1] == 'p') {
                k = "split";
                v = STRM_SPLIT;
            }
            else if (s[1] == 't') {
                k = "strip";
                v = STRM_STRIP;
            }
        }
        else if (s[0] == 'c') {
            k = "c_ptr";
            v = STRM_C_PTR;
        }
        else if (s[0] == 'l') {
            k = "lower";
            v = STRM_LOWER;
        }
        break;
    } // case 5
    case 8: { // 4 options
        if (s[0] == 'e') { // 2 options
            if (s[4] == 'W') {
                k = "endsWith";
                v = STRM_ENDSWITH;
            }
            else if (s[4] == 'w') {
                k = "endswith";
                v = STRM_ENDSWITH;
            }
        }
        else if (s[0] == 'c') {
            k = "contains";
            v = STRM_CONTAINS;
        }
        else if (s[0] == 'g') {
            k = "glob_ptr";
            v = STRM_GLOB_PTR;
        }
        break;
    } // case 8
    case 10: { // 3 options
        if (s[0] == 'p') {
            k = "pathEquals";
            v = STRM_PATHEQUALS;
        }
        else if (s[0] == 's') {
            k = "startswith";
            v = STRM_STARTSWITH;
        }
        else if (s[0] == 'b') {
            k = "beginsWith";
            v = STRM_BEGINSWITH;
        }
        break;
    } // case 10
    case 9: { // 2 options
        if (s[1] == 'C') {
            k = "iContains";
            v = STRM_ICONTAINS;
        }
        else if (s[1] == 'E') {
            k = "iEndsWith";
            v = STRM_IENDSWITH;
        }
        break;
    } // case 9
    case 12: { // 2 options
        if (s[4] == 'C') {
            k = "pathContains";
            v = STRM_PATHCONTAINS;
        }
        else if (s[4] == 'E') {
            k = "pathEndsWith";
            v = STRM_PATHENDSWITH;
        }
        break;
    } // case 12
    case 4: {
        k = "join";
        v = STRM_JOIN;
        break;
    } // case 4
    case 6: {
        k = "equals";
        v = STRM_EQUALS;
        break;
    } // case 6
    case 7: {
        k = "iEquals";
        v = STRM_IEQUALS;
        break;
    } // case 7
    case 11: {
        k = "iBeginsWith";
        v = STRM_IBEGINSWITH;
        break;
    } // case 11
    case 14: {
        k = "pathBeginsWith";
        v = STRM_PATHBEGINSWITH;
        break;
    } // case 14
    } // switch(sz)

    if (k == nullptr || s != k)
        return (EStringMethods)0;
    return v;
}
