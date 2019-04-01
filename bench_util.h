//source https://gist.github.com/kornelski
static unsigned update_adler32(unsigned adler, const unsigned char* data, unsigned len) {
    unsigned s1 = adler & 0xffff;
    unsigned s2 = (adler >> 16) & 0xffff;

    while(len > 0) {
        /*at least 5550 sums can be done before the sums overflow, saving a lot of module divisions*/
        unsigned amount = len > 5550 ? 5550 : len;
        len -= amount;
        while(amount > 0) {
            s1 += (*data++);
            s2 += s1;
            --amount;
        }
        s1 %= 65521;
        s2 %= 65521;
    }

    return (s2 << 16) | s1;
}
