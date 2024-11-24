#ifndef UTILS_H
#define UTILS_H

#define CLAMP(val, min_val, max_val) ((val) < (min_val) ? (min_val) : ((val) > (max_val) ? (max_val) : (val)))

#define CLEAR_BUFFER(buf) memset((buf), 0, sizeof(buf))

#endif // UTILS_H