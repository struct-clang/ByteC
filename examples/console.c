#include <byteio> as ByteIO
#include <bytefs> as ByteFS

int main() {
    ByteIO.print("Input a path: ");
    char path[1024] = ByteIO.input();
    char data[1024];
    ByteIO.print("Reading file: ");
    ByteIO.print(path);
    ByteFS.readf(path, data);
    ByteIO.print(data);
    return 0;
}