#include <iostream>
#include <stdio.h>
#include <stdlib.h>  //for atoi

#define BUF_SIZE   4096

using std::cout;
using std::endl;

void print_usage()
{
    cout << "Usage: cut_bin <src file> <dst file> <lines>" << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        print_usage();
        return -1;
    }

    const char *src_file = argv[1];
    const char *dst_file = argv[2];
    int lines = atoi(argv[3]);

    FILE *fin = fopen(src_file, "rb");
    if (!fin) {
        cout << "open src file failed" << endl;
        return -1;
    }
    FILE *fout = fopen(dst_file, "wb");

    char buf[BUF_SIZE];

    while (!feof(fin)) {
        int cut = 0;
        int ret = fread(buf, 1, BUF_SIZE, fin);
        if (!ret)
            break;

        if (lines > 0) {
            cut = std::min(lines, ret);
            lines -= cut;
            ret -= cut;
        }

        if (ret > 0) {
            fwrite(buf + cut, 1, ret, fout);
        }
    }

    fclose(fout);
    fclose(fin);
    return 0;
}