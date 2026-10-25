#include "qso_flags.h"
#include <string.h>

const qso_flag_t *qso_flag_lookup_key(const char *key);
const qso_flag_t *qso_flag_find(const char *country)
{
    if(!country || strchr(country,'/'))return NULL;
    char key[96];size_t used=0;
    for(const unsigned char *p=(const unsigned char *)country;*p;++p) {
        unsigned char c=*p;
        if(c>='A' && c<='Z')c=(unsigned char)(c-'A'+'a');
        if((c>='a' && c<='z') || (c>='0' && c<='9')) {
            if(used>=sizeof(key)-1)return NULL;
            key[used++]=(char)c;
        }
    }
    key[used]=0;
    return used?qso_flag_lookup_key(key):NULL;
}

const char *qso_flag_license(void)
{
    return "The MIT License (MIT)\n\nCopyright (c) 2013 Panayiotis Lipiridis\n\nPermission is hereby granted, free of charge, to any person obtaining a copy of\nthis software and associated documentation files (the \"Software\"), to deal in\nthe Software without restriction, including without limitation the rights to\nuse, copy, modify, merge, publish, distribute, sublicense, and/or sell copies\nof the Software, and to permit persons to whom the Software is furnished to do\nso, subject to the following conditions:\n\nThe above copyright notice and this permission notice shall be included in all\ncopies or substantial portions of the Software.\n\nTHE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\nIMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\nFITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\nAUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\nLIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\nOUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\nSOFTWARE.\n";
}
