# kernel 中的__maybe_unused

#define __always_unused attribute((unused))
#define __maybe_unused attribute((unused))

这个定义主要是当用__maybe_unused定义的变量时，这个变量没有任何code用到，编译器也不会报错．
