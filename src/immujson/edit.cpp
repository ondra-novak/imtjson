#include "edit.h"
#include "compress.tcc"


namespace json {

template class Compress<void (*)(unsigned char b)>;

}
