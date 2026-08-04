#pragma once



DWORD hashcode(char* str, DWORD len) {
    unsigned int h = 0xbdda97841baf41ab; // FNV offset
    for (unsigned short i = 0; i < len; ++i) {
        h ^= (char)str[i];
        h *= 1099511628211ull; // FNV prime
    }
    return h;
}
bool hash_verify(DWORD one, char* str, DWORD len) {
    return one==hashcode(str, len);
}
bool hash_verify(DWORD one, DWORD two) {
    return one == two;
}
